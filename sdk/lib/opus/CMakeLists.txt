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
set(CMAKE_C_COMPILER_TARGET pi32v2 CACHE STRING "" FORCE)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS
    "-integrated-as -mcpu=r3 -mfprev1 -isystem ${JIELI_TOOLCHAIN_ROOT}/include"
    CACHE STRING "Jieli pi32v2 C flags" FORCE)

project(opus_pi32v2 LANGUAGES C)

include(FetchContent)

# Pin the exact upstream revision so builds remain reproducible across machines.
set(OPUS_GIT_REPOSITORY "https://github.com/xiph/opus.git"
    CACHE STRING "Opus upstream Git repository")
set(OPUS_GIT_TAG "03647f524a40b05a1898522e92033810b58103c7"
    CACHE STRING "Opus upstream Git revision")

set(OPUS_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(OPUS_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(OPUS_BUILD_SHARED_LIBRARY OFF CACHE BOOL "" FORCE)
set(OPUS_INSTALL_PKG_CONFIG_MODULE OFF CACHE BOOL "" FORCE)
set(OPUS_INSTALL_CMAKE_CONFIG_MODULE OFF CACHE BOOL "" FORCE)

# Put the final archive alongside the other WL83 SDK libraries.
get_filename_component(
  OPUS_ARCHIVE_OUTPUT_DIRECTORY
  "${CMAKE_CURRENT_LIST_DIR}/../../cpu/wl83/liba"
  ABSOLUTE
)
file(MAKE_DIRECTORY "${OPUS_ARCHIVE_OUTPUT_DIRECTORY}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OPUS_ARCHIVE_OUTPUT_DIRECTORY}")

FetchContent_Declare(
  opus_upstream
  GIT_REPOSITORY "${OPUS_GIT_REPOSITORY}"
  GIT_TAG "${OPUS_GIT_TAG}"
  GIT_SHALLOW FALSE
  GIT_PROGRESS TRUE
)

# FetchContent_MakeAvailable() 用于把一个外部依赖“下载并直接加入当前 CMake 构建”。
FetchContent_MakeAvailable(opus_upstream)

