# Based on the official pi32v2-r3 large-program lib_demo (2024-04-02).
# Match apps/ds5dongle/board/wl83/Makefile, including its float/bitfield ABI.
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
  set(_jieli_default_root "C:/JL/pi32")
  set(_jieli_tool_suffix ".exe")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
  set(_jieli_default_root "/opt/jieli/pi32v2")
  set(_jieli_tool_suffix "")
else()
  message(FATAL_ERROR "Build AC792 libraries on Windows or Linux x64 (OrbStack debian-x64 on macOS).")
endif()

set(JIELI_TOOLCHAIN_ROOT "${_jieli_default_root}" CACHE PATH "Jieli pi32v2 toolchain")
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES JIELI_TOOLCHAIN_ROOT)
foreach(tool clang lto-ar)
  if(NOT EXISTS "${JIELI_TOOLCHAIN_ROOT}/bin/${tool}${_jieli_tool_suffix}")
    message(FATAL_ERROR "Missing Jieli tool: ${JIELI_TOOLCHAIN_ROOT}/bin/${tool}${_jieli_tool_suffix}")
  endif()
endforeach()

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR pi32v2)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_C_COMPILER "${JIELI_TOOLCHAIN_ROOT}/bin/clang${_jieli_tool_suffix}")
set(CMAKE_CXX_COMPILER "${JIELI_TOOLCHAIN_ROOT}/bin/clang${_jieli_tool_suffix}")
set(CMAKE_C_COMPILER_TARGET pi32v2)
set(CMAKE_CXX_COMPILER_TARGET pi32v2)
set(CMAKE_AR "${JIELI_TOOLCHAIN_ROOT}/bin/lto-ar${_jieli_tool_suffix}" CACHE FILEPATH "LTO archiver")

# Never use the host ar/ranlib for Jieli LLVM bitcode, including try_compile().
foreach(lang C CXX)
  set(CMAKE_${lang}_COMPILER_AR "${CMAKE_AR}")
  set(CMAKE_${lang}_ARCHIVE_CREATE "<CMAKE_AR> qc <TARGET> <LINK_FLAGS> <OBJECTS>")
  set(CMAKE_${lang}_ARCHIVE_APPEND "<CMAKE_AR> q <TARGET> <LINK_FLAGS> <OBJECTS>")
  set(CMAKE_${lang}_ARCHIVE_FINISH "<CMAKE_AR> s <TARGET>")
  #-Oz
  set(CMAKE_${lang}_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "AC792 release flags")
  set(CMAKE_${lang}_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG" CACHE STRING "AC792 release/debug flags")
  set(CMAKE_${lang}_FLAGS_MINSIZEREL "-O2 -DNDEBUG" CACHE STRING "AC792 size flags")
endforeach()

get_filename_component(JIELI_SDK_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(_jieli_flags
  -mcpu=r3 -mfprev1 -integrated-as -flto -g
  -fno-common -fallow-pointer-null -fprefer-gnu-section -femulated-tls
  -mllvm -pi32v2-large-program=true -fms-extensions
  -fno-unwind-tables -ffunction-sections -fdata-sections -fmessage-length=0
  -Wuninitialized -Wno-invalid-noreturn -Wno-shift-negative-value
  -DSUPPORT_MS_EXTENSIONS -DCONFIG_CPU_WL83 -D__GCC_PI32V2__
)
list(JOIN _jieli_flags " " _jieli_flags)
# The firmware's newlib headers take precedence over toolchain/host headers.
string(APPEND _jieli_flags
  " -isystem \"${JIELI_SDK_ROOT}/include_lib/newlib/include\""
  " -isystem \"${JIELI_TOOLCHAIN_ROOT}/include\"")
set(CMAKE_C_FLAGS_INIT "${_jieli_flags}")
set(CMAKE_CXX_FLAGS_INIT "${_jieli_flags} -std=gnu++14 -fno-exceptions -fno-rtti")
# Deliberately omit the AC710N demo's -fdiscrete-bitfield-abi.
