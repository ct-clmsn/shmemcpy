# Copyright (c) 2025 Christopher Taylor
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. *(See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required(VERSION 3.25.1)

option(TEST_DRIVER "TEST_DRIVER" OFF)

find_package(PkgConfig REQUIRED QUIET COMPONENTS)
set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH TRUE)

pkg_search_module(OSHMEM REQUIRED IMPORTED_TARGET GLOBAL oshmem-c) 

include(GNUInstallDirs)

set(CMAKE_C_COMPILER oshcc)

project("shmemcpy"
	VERSION 1.0.0
	DESCRIPTION "A symmetric memcpy for SPMD applications implemented using OpenSHMEM"
	LANGUAGES C
)

add_library(shmemcpy STATIC shmemcpy.h shmemcpy.c)
set_target_properties(shmemcpy PROPERTIES PUBLIC_HEADER shmemcpy.h)
target_link_libraries(shmemcpy PRIVATE m PkgConfig::OSHMEM)

if(TEST_DRIVER)
  add_executable(shmemcpy_driver shmemcpy_driver.c) 
  target_link_libraries(shmemcpy_driver PRIVATE shmemcpy)
  add_dependencies(shmemcpy_driver shmemcpy)

  add_executable(shmemcpy_bcast_driver shmemcpy_bcast_driver.c) 
  target_link_libraries(shmemcpy_bcast_driver PRIVATE shmemcpy)
  add_dependencies(shmemcpy_bcast_driver shmemcpy)
endif()

configure_file(
   "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.pc.in"
   "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
   @ONLY
)

configure_file(
   "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}Config.cmake.in"
   "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
   @ONLY
)

install(TARGETS shmemcpy
	DESTINATION lib
)

install(FILES shmemcpy.h
	DESTINATION include
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
	DESTINATION include/pkgconfig
)
