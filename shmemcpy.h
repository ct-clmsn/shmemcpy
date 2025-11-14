/*  Copyright (c) 2025 Christopher Taylor
 *
 *  SPDX-License-Identifier: BSL-1.0
 *  Distributed under the Boost Software License, Version 1.0. *(See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef __SYMMETRICMEMCPY_H__
#define __SYMMETRICMEMCPY_H__

#include <stdint.h>
#include <shmem.h>

struct __shmemcpy_ctx_t {
   int npes;
   int self;
   long amount;
   long buf_size;
   uint8_t * buf;
   unsigned int * notify;
};

typedef struct __shmemcpy_ctx_t shmemcpy_ctx;

void shmemcpy_ini(shmemcpy_ctx * c, const long num_bytes);

void shmemcpy_fin(shmemcpy_ctx * c);

void shmemcpy(shmemcpy_ctx * c, uint8_t * a, const int nelements, const int dst, const int src);

void shmemcpy_bcast(shmemcpy_ctx * c, uint8_t * a, const int nelements, shmem_team_t team, const int src);

#endif
