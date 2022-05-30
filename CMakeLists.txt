cmake_minimum_required (VERSION 3.7.2 FATAL_ERROR)


project (msk144gensim
  VERSION 2.4.0.0
  LANGUAGES C CXX Fortran
  )

# set CMAKE_BUILD_TYPE to Release if not defined
if (NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE RELEASE CACHE STRING
    "Choose the type of build, options are: None Debug Release."
    FORCE)
endif (NOT CMAKE_BUILD_TYPE)


# project definitions
add_definitions (-DBIGSYM=1 )

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=gnu++11 -pthread")


set (General_FFLAGS "-Wall -Wno-conversion -fno-second-underscore")
# gfortran specific settings
set (CMAKE_Fortran_FLAGS_RELEASE "${CMAKE_Fortran_FLAGS_RELEASE} -fbounds-check -funroll-all-loops -fno-f2c ${General_FFLAGS}")
set (CMAKE_Fortran_FLAGS_DEBUG   "${CMAKE_Fortran_FLAGS_DEBUG} -fbounds-check -fno-f2c ${General_FFLAGS}")


set(WSJT_BASE_DIR ${CMAKE_SOURCE_DIR}/deps/wsjtx  CACHE STRING "Provide path to WSJT sorurces")


set (wsjt_msk144_CSRCS
  ${WSJT_BASE_DIR}/lib/wsprd/nhash.c
  )

set (wsjt_CXXSRCS
  ${WSJT_BASE_DIR}/lib/crc13.cpp
  )


# deal with a GCC v6 UB error message
set_source_files_properties (
  ${WSJT_BASE_DIR}/lib/crc13.cpp
  PROPERTIES COMPILE_FLAGS -fpermissive)

set (wsjt_common_FSRCS
  ${WSJT_BASE_DIR}/lib/packjt.f90
  ${WSJT_BASE_DIR}/lib/deg2grid.f90
  ${WSJT_BASE_DIR}/lib/grid2deg.f90
  ${WSJT_BASE_DIR}/lib/fmtmsg.f90
  )


set (wsjt_msk144_FSRCS
  ${WSJT_BASE_DIR}/lib/genmsk_128_90.f90
  ${WSJT_BASE_DIR}/lib/crc.f90
  ${WSJT_BASE_DIR}/lib/hashing.f90
  ${WSJT_BASE_DIR}/lib/77bit/packjt77.f90
  ${WSJT_BASE_DIR}/lib/chkcall.f90
  ${WSJT_BASE_DIR}/lib/encode_msk40.f90
  ${WSJT_BASE_DIR}/lib/encode_128_90.f90
  ${WSJT_BASE_DIR}/lib/genmsk40.f90
  ${WSJT_BASE_DIR}/lib/hash.f90
  )



set (project_common_SRCS 
  )

add_executable (msk144gensim
  src/msk144gensim_main.cpp 
  src/msg_item.cpp
  src/msg_item.h
  src/f_interop.h
  src/f_interop.cpp
  ${wsjt_common_FSRCS}
  ${project_common_SRCS}
  ${wsjt_msk144_FSRCS} 
  ${wsjt_CXXSRCS} 
  ${wsjt_msk144_CSRCS})

target_link_libraries(msk144gensim stdc++)

