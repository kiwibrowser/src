# Suppress all warnings from third-party projects.
set_property(DIRECTORY APPEND PROPERTY COMPILE_OPTIONS -w)

set(SHADERC_THIRD_PARTY_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE STRING
  "Root location of all third_party projects")
set(SHADERC_GOOGLE_TEST_DIR "${SHADERC_THIRD_PARTY_ROOT_DIR}/googletest" CACHE STRING
  "Location of googletest source")
set(SHADERC_SPIRV_TOOLS_DIR "${SHADERC_THIRD_PARTY_ROOT_DIR}/spirv-tools" CACHE STRING
  "Location of spirv-tools source")
set(SHADERC_GLSLANG_DIR "${SHADERC_THIRD_PARTY_ROOT_DIR}/glslang" CACHE STRING
  "Location of glslang source")

# Configure third party projects.
if(${SHADERC_ENABLE_TESTS})
  if (IS_DIRECTORY ${SHADERC_GOOGLE_TEST_DIR})
    add_subdirectory(${SHADERC_GOOGLE_TEST_DIR} googletest)
  endif()
  if (NOT TARGET gmock)
    message(FATAL_ERROR "gmock was not found - required for tests")
  endif()
endif()

set(OLD_PLATFORM_TOOLSET ${CMAKE_GENERATOR_TOOLSET})

if (IS_DIRECTORY ${SHADERC_GLSLANG_DIR})
  add_subdirectory(${SHADERC_GLSLANG_DIR} glslang)
endif()
if (NOT TARGET glslang)
  message(FATAL_ERROR "glslang was not found - required for compilation")
endif()
if(WIN32)
    # This is unfortunate but glslang forces our
    # platform toolset to be v110, which we may not even have
    # installed, undo anything glslang has done to it.
    set(CMAKE_GENERATOR_TOOLSET "${OLD_PLATFORM_TOOLSET}" CACHE STRING
      "Platform Toolset" FORCE)
endif()

if (IS_DIRECTORY ${SHADERC_SPIRV_TOOLS_DIR})
  add_subdirectory(${SHADERC_SPIRV_TOOLS_DIR} spirv-tools)
endif()
if (NOT TARGET SPIRV-Tools)
  message(FATAL_ERROR "SPIRV-Tools was not found - required for compilation")
endif()

# Configure out-of-source-directory tests for glslang.
# The glslang project uses a bash script called "runtests" to run tests.
# The runtests script assumes the glslangValidator executable exists in
# a location inside the source tree, but we build it elsewhere.
# We need to copy the test files, fix the path references, and then run tests.
set(GLSLANG_TEST_SRC_DIR ${SHADERC_GLSLANG_DIR}/Test)
set(GLSLANG_TEST_BIN_DIR
      ${CMAKE_CURRENT_BINARY_DIR}/test-glslang/${CMAKE_CFG_INTDIR})

# If we are building in a multi-configuration setting we have
# to put the glslang tests into their respective subdirectories.
if (CMAKE_CONFIGURATION_TYPES)
  set(GLSLANG_CONFIGURATION_DIR ${CMAKE_CFG_INTDIR})
endif()

add_custom_target(copy-tests-if-necessary ALL
  COMMAND ${PYTHON_EXECUTABLE}
    ${shaderc_SOURCE_DIR}/utils/copy-tests-if-necessary.py
    ${GLSLANG_TEST_SRC_DIR} ${GLSLANG_TEST_BIN_DIR} ${GLSLANG_CONFIGURATION_DIR}
  COMMENT "Copying and patching glslang tests if needed")

if (CMAKE_CONFIGURATION_TYPES)
  # If we are running a multi-configuration project,
  # the tests will be in test-glslang/${Configuration}
  add_test(NAME glslang-testsuite
    COMMAND bash runtests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/test-glslang/$<CONFIGURATION>
  )
else()
  add_test(NAME glslang-testsuite
    COMMAND bash runtests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/test-glslang/
  )
endif()
