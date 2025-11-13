<!-- Copyright (c) 2025 Christopher Taylor                                          -->
<!--                                                                                -->
<!--   Distributed under the Boost Software License, Version 1.0. (See accompanying -->
<!--   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)        -->
# [shmemcpy - symmetric memcpy](https://github.com/ct-clmsn/shmemcpy)

[shmemcpy](https://github.com/ct-clmsn/shmemcpy) implements a point-to-point and broadcast
symmetric memcpy capability using OpenSHMEM.

### Installation Requirements

Dependencies:

* OpenMPI's OpenSHMEM implementation
* pkg-config
* cmake

### Compilation

Build the library `libshmemcpy.a` without the test driver

* `mkdir build`
* cd build
* cmake ..
* make

Build the library `libshmemcpy.a` with test drivers `shmemcpy_driver` and `shmemcpy_bcast_driver`

* `mkdir build`
* cd build
* cmake -DTEST_DRIVER=ON ..
* make

### Running Programs

Copy `shmemcpy_driver` on to a distributed file system. Change
directory to the NFS directory that hosts `shmemcpy_driver`.

```
oshrun -n 2 ./shmemcpy_driver
```

This library is designed to be run on an HPC system that manages jobs using
bulk synchronous workload managers: [Slurm](https://slurm.schedmd.com), PBS,
etc.

### Licenses

* Boost Version 1.0 (2022-)

### Date

12 November 2025

### Author

Christopher Taylor

### Special Thanks

* The OpenMPI community's support for OpenSHMEM
* The OpenSHMEM community

### Dependencies

* [OpenMPI](https://open-mpi.org) this link directs to the OpenMPI the OpenSHMEM implementation used for this library
* [OpenSHMEM](https://github.com/openshmem-org/osss-ucx) the OpenSHMEM github project
