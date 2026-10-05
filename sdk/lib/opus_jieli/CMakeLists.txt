cmake_minimum_required(VERSION 3.24)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../jieli/JieliCompilePi32v2.cmake"
    CACHE FILEPATH "AC792 toolchain")
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type")
project(opus_jieli LANGUAGES C)

# AC792 r3/fprev1: keep the existing floating-point codec and float API.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(OPUS_BUILD_SHARED_LIBRARY OFF CACHE BOOL "" FORCE)
set(OPUS_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(OPUS_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(OPUS_INSTALL_PKG_CONFIG_MODULE OFF CACHE BOOL "" FORCE)
set(OPUS_INSTALL_CMAKE_CONFIG_MODULE OFF CACHE BOOL "" FORCE)
set(OPUS_FIXED_POINT OFF CACHE BOOL "" FORCE)
set(OPUS_ENABLE_FLOAT_API ON CACHE BOOL "" FORCE)
set(OPUS_DISABLE_INTRINSICS ON CACHE BOOL "" FORCE)
# Compiler flag support alone cannot establish the firmware runtime's support.
set(OPUS_STACK_PROTECTOR OFF CACHE BOOL "" FORCE)
set(OPUS_FORTIFY_SOURCE OFF CACHE BOOL "" FORCE)
set(OPUS_FAST_MATH OFF CACHE BOOL "" FORCE)
set(OPUS_FLOAT_APPROX ON CACHE BOOL "" FORCE)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_INSTALL_LIBDIR lib CACHE PATH "Embedded archive install directory")
# GitHub source archives have no Git tags/package_version. Identify this exact
# snapshot without claiming that it corresponds to an upstream release number.
set(OPUS_PACKAGE_VERSION "0.0.0-g03647f524a40")

include(FetchContent)
# Same revision as ../opus, without cloning the entire upstream Git history.
FetchContent_Declare(opus_upstream
  URL https://codeload.github.com/xiph/opus/tar.gz/03647f524a40b05a1898522e92033810b58103c7
  URL_HASH SHA256=ed046937dfd79a8c5583092aa87ee50f1beadb63cddd6208f57ee4b3ef9ed7fc
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(opus_upstream)

add_custom_target(install-sdk
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
          "$<TARGET_FILE:opus>" "${JIELI_SDK_ROOT}/cpu/wl83/liba/libopus.a"
  DEPENDS opus
  COMMENT "Installing libopus.a into the AC792 SDK"
  VERBATIM)
