# Attach the AC792 port without modifying the downloaded upstream source tree.
function(opus_add_pi32v2_port target upstream_source)
  set(port_dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../pi32v2")
  get_filename_component(port_dir "${port_dir}" ABSOLUTE)
  target_sources(${target} PRIVATE "${port_dir}/opus_pi32v2.c")
  target_include_directories(${target} PRIVATE "${port_dir}")
  target_compile_definitions(${target} PRIVATE
    OPUS_JIELI_HW_FFT=$<BOOL:${OPUS_JIELI_HW_FFT}>
    OPUS_JIELI_PI32V2_ASM=$<BOOL:${OPUS_JIELI_PI32V2_ASM}>)

  if(OPUS_JIELI_PI32V2_ASM)
    # pitch.h has explicit OVERRIDE_* hooks. Load their declarations before any
    # upstream header, independently of the order each translation unit uses.
    target_compile_options(${target} PRIVATE
      -include "${port_dir}/opus_pi32v2_overrides.h")
    target_compile_definitions(${target} PRIVATE CUSTOM_SUPPORT)
  endif()

  if(OPUS_JIELI_HW_FFT)
    # Only rename the definition: calls within kiss_fft.c must also dispatch to
    # the new wrapper. MDCT's direct opus_fft_impl calls are covered as well.
    set(original "${upstream_source}/celt/kiss_fft.c")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${original}")
    file(READ "${original}" contents)
    set(anchor "void opus_fft_impl(const kiss_fft_state *st,kiss_fft_cpx *fout ARG_FIXED(int downshift))")
    string(REGEX MATCHALL "void opus_fft_impl\\(" definitions "${contents}")
    list(LENGTH definitions count)
    string(FIND "${contents}" "${anchor}" position)
    if(NOT count EQUAL 1 OR position EQUAL -1)
      message(FATAL_ERROR "Pinned Opus FFT definition changed; review the PI32V2 dispatch patch.")
    endif()
    string(REPLACE "${anchor}"
      "void opus_fft_impl_c(const kiss_fft_state *st,kiss_fft_cpx *fout ARG_FIXED(int downshift))"
      contents "${contents}")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/pi32v2-generated")
    # configure_file avoids changing timestamps when the generated file matches.
    set(generated "${CMAKE_CURRENT_BINARY_DIR}/pi32v2-generated/kiss_fft_pi32v2.c")
    file(WRITE "${generated}.in" "${contents}")
    configure_file("${generated}.in" "${generated}" COPYONLY)
    set_source_files_properties("${original}" TARGET_DIRECTORY ${target}
      PROPERTIES HEADER_FILE_ONLY TRUE)
    target_sources(${target} PRIVATE "${generated}")
  endif()
endfunction()
