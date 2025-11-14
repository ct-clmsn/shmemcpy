<!-- Copyright (c) 2025 Christopher Taylor                                          -->
<!--                                                                                -->
<!--   Distributed under the Boost Software License, Version 1.0. (See accompanying -->
<!--   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)        -->
# [shmemcpy - symmetric memcpy](https://github.com/ct-clmsn/shmemcpy)

[shmemcpy](https://github.com/ct-clmsn/shmemcpy) implements a point-to-point and broadcast
memcpy capability using OpenSHMEM.

### Data Types 

`shmemcpy_ctx`

Users are required to use `shmemcpy_ctx` in order to perform shmemcpy operations. The `shmemcpy_ctx`
data type contains pointers to symmetric shared memory that are intrinsic to the underlying
communication process.

### Function Calls 

`void shmemcpy_ini(shmemcpy_ctx * c, const long num_bytes);`

Users are required to initialize a `shmemcpy_ctx` data type. This function peforms the required
initialization. The second argument defines the number of bytes that are allocated on the symmetric
heap (for each processing element, or PE) to use for the purposes of performing shmemcpy operations.

`void shmemcpy_ini(shmemcpy_ctx * c);`

Users are required to initialize a `shmemcpy_ctx` data type. This function peforms the required
initialization by checking for the environment variable `SHMEMCPY_SEGMENT_SIZE`, or if not provided,
calling `sysconf(_SC_PAGESIZE)` to determine the amount of each symmetric heap allocation that needs
to be made for each processing element (PE) to participate in shmemcpy operations. 

`void shmemcpy_fin(shmemcpy_ctx * c);`

Users are required to finalize a `shmemcpy_ctx` data type. This function peforms the required
finalization; symmetric memory is freed.

`void shmemcpy(shmemcpy_ctx * c, uint8_t * a, const int nelements, const int dst, const int src);`

This function performs a processing element (PE) to processing element (PE) memory copy using symmetric
memory. The function accepts an initialized `shmemcpy_ctx`, a pointer to bytes at a local address in
memory (not a symmetric address), the number of bytes to communicate, the destination processing element
(PE), and the source processing element (PE).

`void shmemcpy_bcast(shmemcpy_ctx * c, uint8_t * a, const int nelements, shmem_team_t team, const int src);`

This function performs a broadcast memory copy using symmetric memory. The function accepts an initialized
`shmemcpy_ctx`, a pointer to bytes at a local address in memory (not a symmetric address), the number of
bytes to communicate, the `shmem_team_t` to perform the broadcast across, and the source processing element (PE).
