#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "fltk::fltk" for configuration ""
set_property(TARGET fltk::fltk APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(fltk::fltk PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "C;CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib64/libfltk.a"
  )

list(APPEND _cmake_import_check_targets fltk::fltk )
list(APPEND _cmake_import_check_files_for_fltk::fltk "${_IMPORT_PREFIX}/lib64/libfltk.a" )

# Import target "fltk::forms" for configuration ""
set_property(TARGET fltk::forms APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(fltk::forms PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib64/libfltk_forms.a"
  )

list(APPEND _cmake_import_check_targets fltk::forms )
list(APPEND _cmake_import_check_files_for_fltk::forms "${_IMPORT_PREFIX}/lib64/libfltk_forms.a" )

# Import target "fltk::images" for configuration ""
set_property(TARGET fltk::images APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(fltk::images PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib64/libfltk_images.a"
  )

list(APPEND _cmake_import_check_targets fltk::images )
list(APPEND _cmake_import_check_files_for_fltk::images "${_IMPORT_PREFIX}/lib64/libfltk_images.a" )

# Import target "fltk::gl" for configuration ""
set_property(TARGET fltk::gl APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(fltk::gl PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib64/libfltk_gl.a"
  )

list(APPEND _cmake_import_check_targets fltk::gl )
list(APPEND _cmake_import_check_files_for_fltk::gl "${_IMPORT_PREFIX}/lib64/libfltk_gl.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
