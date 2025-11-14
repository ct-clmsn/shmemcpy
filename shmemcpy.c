/*  Copyright (c) 2025 Christopher Taylor
 *
 *  SPDX-License-Identifier: BSL-1.0
 *  Distributed under the Boost Software License, Version 1.0. *(See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "shmemcpy.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <regex.h>
#include <sys/resource.h>

static void xmt(unsigned int * notify, uint8_t * buf, uint8_t * a, int nelements, int dst, int src) {
   //static const int value = 1;
   volatile unsigned int * notify_src_ptr = &notify[src];
   shmem_uint8_put( &buf[0], &a[0], nelements, dst );
   //shmem_quiet();
   //shmem_uint8_t_put( &notify[dst], &value, 1, dst );
   shmem_uint_atomic_set( &notify[dst], 1, dst );
   int rc = shmem_uint_test(notify_src_ptr, SHMEM_CMP_GT, 0);
   while( !rc ) {
      rc = shmem_uint_test(notify_src_ptr, SHMEM_CMP_GT, 0);
   }
   //notify[src] = 0;
   shmem_uint_atomic_set( &notify[src], 0, src );
}

static void rcv(unsigned int * notify, uint8_t * buf, uint8_t * a, int nelements, int dst, int src) {
   //static const int value = 1;
   volatile unsigned int * notify_dst_ptr = &notify[dst];
   int rc = shmem_uint_test(notify_dst_ptr, SHMEM_CMP_GT, 0);
   while( !rc ) {
      rc = shmem_uint_test(notify_dst_ptr, SHMEM_CMP_GT, 0);
   }
   memcpy( &a[0], buf, nelements*sizeof(uint8_t) );
   //notify[dst] = 0;
   shmem_uint_atomic_set( &notify[dst], 0, dst );
   //shmem_int_put( &notify[src], &value, 1, src );
   shmem_uint_atomic_set( &notify[src], 1, src );
}

static long validate_page_size(long const user_page_size) {
   long const pages = sysconf(_SC_AVPHYS_PAGES);
   long const page_size = sysconf(_SC_PAGESIZE);
   long const total_available_ram = pages * page_size;

   struct rlimit data_limit;
   int const rlimit_data_amt = getrlimit(RLIMIT_DATA, &data_limit);

   if(total_available_ram <= user_page_size) {
      fprintf(stderr, "shmemcpy error, `SHMEMCPY_SEGMENT_SIZE` is greater than or equal to the total available memory (_SC_AVPHYS_PAGES * _SC_PAGESIZE), falling back to `sysconf(_SC_PAGESIZE)`\n");
      return sysconf(_SC_PAGESIZE);
   }
   else if(data_limit.rlim_cur <= user_page_size) {
      fprintf(stderr, "shmemcpy error, `SHMEMCPY_SEGMENT_SIZE` is greater than or equal to RLIMIT_DATA's 'soft limit', falling back to `sysconf(_SC_PAGESIZE)`\n");
      return sysconf(_SC_PAGESIZE);
   }
   else if(data_limit.rlim_max <= user_page_size) {
      fprintf(stderr, "shmemcpy error, `SHMEMCPY_SEGMENT_SIZE` is greater than or equal to RLIMIT_DATA's 'hard limit', falling back to `sysconf(_SC_PAGESIZE)`\n");
      return sysconf(_SC_PAGESIZE);
   }

   return user_page_size;
}

void shmemcpy_ini(shmemcpy_ctx* c, const long num_bytes) {

   if(num_bytes < 1) {
      char * pagesize_cstr = getenv("SHMEMCPY_SEGMENT_SIZE");
      long page_size = 0L;

      if(pagesize_cstr != NULL) {
         page_size = sysconf(_SC_PAGESIZE);
      }
      else {
         regex_t r;
         regmatch_t m[1];

         regcomp(&r, "^([0-9]+)$", REG_EXTENDED);
         int const regex_rc = regexec(&r, "12345", 1, &m[0], 0);

         if(regex_rc == REG_NOMATCH) {
            fprintf(stderr, "shmemcpy error, invalid `SHMEMCPY_SEGMENT_SIZE`, falling back to `sysconf(_SC_PAGESIZE)`\n");
            page_size = sysconf(_SC_PAGESIZE);
         }
         else {
            page_size = atol(pagesize_cstr);
            page_size = validate_page_size(page_size);
         }
      }
   }

   c->npes = shmem_n_pes();
   c->self = shmem_my_pe();
   c->amount = num_bytes;
   c->buf_size = num_bytes * c->npes;
   c->buf = (uint8_t*)shmem_calloc(1U, (c->buf_size * sizeof(uint8_t))+(c->npes * sizeof(unsigned int)));
   c->notify = (unsigned int *)(&(c->buf[c->buf_size]));
}

void shmemcpy_fin(shmemcpy_ctx* c) {
   c->amount = 0;
   c->buf_size = 0;
   shmem_free(c->buf);
   c->buf = NULL;
   c->notify = NULL;
}

static void shmemcpy_ctx_xmt_lt(shmemcpy_ctx* c, uint8_t * a, const int nelements, const int dst, const int src) {
   int const num_segments = nelements / c->amount;
   uint8_t * buf_ptr = c->buf+(c->amount*dst);
   uint8_t * xmt_buf = &a[0];
   for(int segment = 0; segment < num_segments; ++segment) {
      xmt(c->notify, buf_ptr, xmt_buf, c->amount, dst, src);
      xmt_buf = xmt_buf + c->amount;
   }

   int const remainder = nelements - (c->amount * num_segments);
   if(0 < remainder) {
      xmt(c->notify, buf_ptr, xmt_buf, remainder, dst, src);
   }
}

static void shmemcpy_ctx_xmt_(shmemcpy_ctx* c, uint8_t * a, const int nelements, const int dst, const int src) {
   uint8_t * buf_ptr = c->buf+(c->amount*dst);
   xmt(c->notify, buf_ptr, &a[0], nelements, dst, src);
}

static void shmemcpy_ctx_xmt(shmemcpy_ctx* c, uint8_t * a, const int nelements, const int dst, const int src) {
   static const void ( *xmt_op [2] )(shmemcpy_ctx*, uint8_t *, const int, const int, const int) = { shmemcpy_ctx_xmt_, shmemcpy_ctx_xmt_lt };
   int const cond = c->amount < nelements;
   xmt_op[cond](c, a, nelements, dst, src);
}

static void shmemcpy_ctx_rcv_lt(shmemcpy_ctx* c, uint8_t * a, const int nelements, const int dst, const int src) {
   int const num_segments = nelements / c->amount;
   uint8_t * buf_ptr = c->buf+(c->amount*dst);
   uint8_t * rcv_buf = &a[0];
   for(int segment = 0; segment < num_segments; ++segment) {
      rcv(c->notify, buf_ptr, rcv_buf, c->amount, dst, src);
      rcv_buf = rcv_buf + c->amount;
   }

   int const remainder = nelements - (c->amount * num_segments);
   if(0 < remainder) {
      rcv(c->notify, buf_ptr, rcv_buf, remainder, dst, src);
   }
}

static void shmemcpy_ctx_rcv_(shmemcpy_ctx* c, uint8_t * a, const int nelements, const int dst, const int src) {
   uint8_t * buf_ptr = c->buf+(c->amount*dst);
   rcv(c->notify, buf_ptr, &a[0], nelements, dst, src);
}

static void shmemcpy_ctx_rcv(shmemcpy_ctx* c, uint8_t * a, const int nelements, const int dst, const int src) {
   static const void ( *rcv_op [2] )(shmemcpy_ctx*, uint8_t *, const int, const int, const int) = { shmemcpy_ctx_rcv_, shmemcpy_ctx_rcv_lt };
   int const cond = c->amount < nelements;
   rcv_op[cond](c, a, nelements, dst, src);
}

void shmemcpy(shmemcpy_ctx* c, uint8_t * a, const int nelements, const int dst, const int src) {
   static const void ( *op [2] )(shmemcpy_ctx*, uint8_t *, const int, const int, const int) = { shmemcpy_ctx_xmt, shmemcpy_ctx_rcv };
   if(c->self != dst && c->self != src) { return; }
   int const cond = c->self == dst;
   op[cond](c, a, nelements, dst, src);
}

//
// bcast
//
static void xmt_bcast(shmemcpy_ctx* c, int const* indices, uint8_t * buf, uint8_t * a, int nelements, shmem_team_t team, int src) {
   unsigned int * notify_self_ptr = &(c->notify[c->self]);
   unsigned int * notify_src_ptr = &(c->notify[src]);

   //not implemented yet...
   //int rc = shmem_int_bcast( team, &buf[0], &a[0], nelements, src );
   //shmem_quiet();
   //rc = shmem_int_sum_reduce(team, notify_src_ptr, notify_self_ptr, 1);
   //shmem_quiet();

   int const npes = c->npes;
   int const self = c->self;
   int const rel_rank = (c->self + src) % npes;
   int const logp = (int)ceil(log2((double)npes));
   int k = npes / 2;

   for(int i = 0; i < logp; ++i) {
      int twok = 2 * k;
      int cond = self % twok; 
      if( cond == 0 ) {
         shmemcpy_ctx_xmt(c, a, nelements, indices[(rel_rank+k)], indices[rel_rank]);
      }
      else if(cond == k) {
         shmemcpy_ctx_rcv(c, a, nelements, indices[rel_rank], indices[abs(rel_rank-k)]);
      }

      k >>= 1;
   }
}

static void shmemcpy_ctx_bcast_lt(shmemcpy_ctx* c, int const* indices, uint8_t * a, const int nelements, shmem_team_t team, const int src) {
   int const num_segments = nelements / c->amount;
   uint8_t * buf_ptr = (uint8_t*)(c->buf+(c->buf_size*c->self));
   uint8_t * xmt_buf = &a[0];
   for(int segment = 0; segment < num_segments; ++segment) {
      xmt_bcast(c, indices, buf_ptr, xmt_buf, c->amount, team, src);
      xmt_buf = xmt_buf + c->amount;
   }

   int const remainder = nelements - (c->amount * num_segments);
   if(0 < remainder) {
      xmt_bcast(c, indices, buf_ptr, xmt_buf, remainder, team, src);
   }
}

static void shmemcpy_ctx_bcast_(shmemcpy_ctx* c, int const* indices, uint8_t * a, const int nelements, shmem_team_t team, const int src) {
   uint8_t * buf_ptr = &(c->buf[c->self]);
   xmt_bcast(c, indices, buf_ptr, &a[0], nelements, team, src);
}

void shmemcpy_bcast(shmemcpy_ctx * c, uint8_t * a, const int nelements, shmem_team_t team, const int src) {
   static const void ( *rcv_op [2] )(shmemcpy_ctx*, int const*, uint8_t *, const int, shmem_team_t, const int) = { shmemcpy_ctx_bcast_, shmemcpy_ctx_bcast_lt };

   int const npes = c->npes;
   int indices[npes];
   int const src_end = src + npes;
   for(int i = src; i < src_end; ++i) {
      indices[i] = i % npes;
   }

   int const cond = c->amount < nelements;
   rcv_op[cond](c, indices, a, nelements, team, 0);
}
