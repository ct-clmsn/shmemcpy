/*  Copyright (c) 2025 Christopher Taylor
 *
 *  SPDX-License-Identifier: BSL-1.0
 *  Distributed under the Boost Software License, Version 1.0. *(See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "shmemcpy.h"

#include <shmem.h>
#include <stdio.h>
#include <unistd.h>

#define ARRAY_SIZE 32
#define DST_PE 1
#define SRC_PE 0

int main(int argc, char ** argv) {

   const long page_size = sysconf(_SC_PAGESIZE);

   int a[ARRAY_SIZE];

   shmem_init();

   shmemcpy_ctx c;
   shmemcpy_ini(&c, page_size); 

   if(c.self == 0) {
      for(int i = 0; i < ARRAY_SIZE; ++i) {
         a[i] = i;
      }
   }
   else {
      for(int i = 0; i < ARRAY_SIZE; ++i) {
         a[i] = 0;
      }
   }

   shmemcpy(&c, (uint8_t*)&a[0], ARRAY_SIZE*sizeof(int), DST_PE, SRC_PE);

   shmemcpy_fin(&c);

   shmem_finalize();

   if(c.self == DST_PE) {
      for(int i = 0; i < ARRAY_SIZE; ++i) {
         printf("%d\n", a[i]);
      }
   }

   return 0;
}
