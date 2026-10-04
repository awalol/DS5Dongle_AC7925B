cmake_minimum_required(VERSION 3.24)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../jieli/JieliCompilePi32v2.cmake"
    CACHE FILEPATH "AC792 toolchain")
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type")
project(wdl_jieli LANGUAGES C CXX)

include(FetchContent)
# Same revision as ../wdl; the archive hash also verifies downloaded content.
FetchContent_Declare(wdl_upstream
  URL https://codeload.github.com/justinfrankel/WDL/tar.gz/8338e8193e4c1523c9eb0ec99d1815e14d062375
  URL_HASH SHA256=1077c299962add8fc5a8a7252398f984eaf5c44a2135597fb816a8fb0fe341cc
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  # WDL is a source collection: do not configure unrelated upstream projects.
  SOURCE_SUBDIR ac792-no-cmake-subdirectory
)
FetchContent_MakeAvailable(wdl_upstream)

add_library(wdl_resampler STATIC "${wdl_upstream_SOURCE_DIR}/WDL/resample.cpp")
set_target_properties(wdl_resampler PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
target_include_directories(wdl_resampler PUBLIC
  "${wdl_upstream_SOURCE_DIR}/WDL"
  "${JIELI_SDK_ROOT}/include_lib/c++"
  "${JIELI_SDK_ROOT}/include_lib/c++/include"
  "${JIELI_SDK_ROOT}/include_lib/c++/simple_pthread"
  "${JIELI_SDK_ROOT}/include_lib"
)
# This changes the public sample type: callers must use the same definition.
target_compile_definitions(wdl_resampler PUBLIC WDL_RESAMPLE_TYPE=float)
target_compile_definitions(wdl_resampler PRIVATE WDL_RESAMPLE_NO_SSE)
target_compile_options(wdl_resampler PUBLIC -fsigned-char)

add_custom_target(install-sdk
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
          "$<TARGET_FILE:wdl_resampler>" "${JIELI_SDK_ROOT}/cpu/wl83/liba/libwdl_resampler.a"
  DEPENDS wdl_resampler
  COMMENT "Installing libwdl_resampler.a into the AC792 SDK"
  VERBATIM)
