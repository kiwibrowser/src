cmake_minimum_required(VERSION 2.8.12)
project(shaderc)
enable_testing()

message(STATUS "Shaderc: build type is \"${CMAKE_BUILD_TYPE}\".")

option(SHADERC_SKIP_TESTS
       "Skip building unit tests based on googlemock and googletest"
       ${SHADERC_SKIP_TESTS})
if(NOT ${SHADERC_SKIP_TESTS})
  set(SHADERC_ENABLE_TESTS ON)
endif()
if(${SHADERC_ENABLE_TESTS})
  message(STATUS "Configuring Shaderc to build unit tests.")
else()
  message(STATUS "Configuring Shaderc to avoid building unit tests.")
endif()

include(cmake/setup_build.cmake)
include(cmake/utils.cmake)

add_custom_target(check-copyright ALL
  ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/utils/add_copyright.py
  --check
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Check copyright")

add_custom_target(add-copyright
  ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/utils/add_copyright.py
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Add copyright")

if(MSVC)
  option(SHADERC_ENABLE_SHARED_CRT
          "Use the shared CRT instead of the static CRT"
          ${SHADERC_ENABLE_SHARED_CRT})
  if (NOT SHADERC_ENABLE_SHARED_CRT)
    # Link executables statically by replacing /MD with /MT everywhere.
    foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
      if(${flag_var} MATCHES "/MD")
        string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
      endif(${flag_var} MATCHES "/MD")
    endforeach(flag_var)
  endif(NOT SHADERC_ENABLE_SHARED_CRT)
endif(MSVC)

# Configure subdirectories.
# We depend on these for later projects, so they should come first.
add_subdirectory(third_party)

add_subdirectory(libshaderc_util)
add_subdirectory(libshaderc)
add_subdirectory(glslc)
add_subdirectory(examples)

add_custom_target(build-version
  ${PYTHON_EXECUTABLE}
  ${CMAKE_CURRENT_SOURCE_DIR}/utils/update_build_version.py
  ${shaderc_SOURCE_DIR} ${spirv-tools_SOURCE_DIR} ${glslang_SOURCE_DIR}
  COMMENT "Update build-version.inc in the Shaderc build directory (if necessary).")
