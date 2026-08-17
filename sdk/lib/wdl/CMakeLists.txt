cmake_minimum_required(VERSION 3.24)

set(JIELI_TOOLCHAIN_ROOT "/opt/jieli/pi32v2"
    CACHE PATH "Jieli pi32v2 toolchain root")

if(NOT EXISTS "${JIELI_TOOLCHAIN_ROOT}/bin/clang")
  message(FATAL_ERROR
          "Jieli compiler not found: ${JIELI_TOOLCHAIN_ROOT}/bin/clang")
endif()

# The compiler must be selected before project() enables the C language.
set(CMAKE_SYSTEM_NAME Generic CACHE STRING "" FORCE)
set(CMAKE_SYSTEM_PROCESSOR pi32v2 CACHE STRING "" FORCE)
set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
set(CMAKE_C_COMPILER "${JIELI_TOOLCHAIN_ROOT}/bin/clang"
    CACHE FILEPATH "Jieli C compiler" FORCE)
set(CMAKE_CXX_COMPILER "${JIELI_TOOLCHAIN_ROOT}/bin/clang++"
    CACHE FILEPATH "Jieli C++ compiler" FORCE)
set(CMAKE_C_COMPILER_TARGET pi32v2 CACHE STRING "" FORCE)
set(CMAKE_CXX_COMPILER_TARGET pi32v2 CACHE STRING "" FORCE)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS
    "-integrated-as -mcpu=r3 -mfprev1 -isystem ${JIELI_TOOLCHAIN_ROOT}/include"
    CACHE STRING "Jieli pi32v2 C flags" FORCE)
set(CMAKE_CXX_FLAGS
    "-integrated-as -mcpu=r3 -mfprev1 -std=gnu++14 -fno-exceptions -fno-rtti -isystem ${JIELI_TOOLCHAIN_ROOT}/include"
    CACHE STRING "Jieli pi32v2 C++ flags" FORCE)

project(wdlresampler_pi32v2 LANGUAGES C CXX)

include(FetchContent)

# Pin the exact upstream revision so builds remain reproducible across machines.
set(WDL_GIT_REPOSITORY "https://github.com/justinfrankel/WDL.git"
    CACHE STRING "WDL upstream Git repository")
set(WDL_GIT_TAG "8338e8193e4c1523c9eb0ec99d1815e14d062375"
    CACHE STRING "WDL upstream Git revision")


# Put the final archive alongside the other WL83 SDK libraries.
get_filename_component(
  WDL_ARCHIVE_OUTPUT_DIRECTORY
  "${CMAKE_CURRENT_LIST_DIR}/../../cpu/wl83/liba"
  ABSOLUTE
)
file(MAKE_DIRECTORY "${WDL_ARCHIVE_OUTPUT_DIRECTORY}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${WDL_ARCHIVE_OUTPUT_DIRECTORY}")

FetchContent_Declare(
  wdl_upstream
  GIT_REPOSITORY "${WDL_GIT_REPOSITORY}"
  GIT_TAG "${WDL_GIT_TAG}"
  GIT_SHALLOW FALSE
  GIT_PROGRESS TRUE
)

# FetchContent_Populate 只负责获取源码
FetchContent_Populate(wdl_upstream)

add_library(wdl_resampler STATIC
  "${wdl_upstream_SOURCE_DIR}/WDL/resample.cpp"
)
target_include_directories(wdl_resampler PUBLIC
  "${wdl_upstream_SOURCE_DIR}/WDL"
  "${CMAKE_CURRENT_LIST_DIR}/../../include_lib/c++"
  "${CMAKE_CURRENT_LIST_DIR}/../../include_lib/c++/include"
  "${CMAKE_CURRENT_LIST_DIR}/../../include_lib/c++/simple_pthread"
  "${CMAKE_CURRENT_LIST_DIR}/../../include_lib/newlib/include"
  "${CMAKE_CURRENT_LIST_DIR}/../../include_lib"
)
target_compile_options(wdl_resampler PRIVATE
  -fsigned-char
  -ffunction-sections
)
target_compile_definitions(wdl_resampler PRIVATE
  WDL_RESAMPLE_TYPE=float
)
