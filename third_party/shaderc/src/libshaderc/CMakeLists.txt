project(libshaderc)

# Even though shaderc.hpp is a headers-only library, adding
# a dependency here will force clients of the library to rebuild
# when it changes.
add_library(shaderc STATIC
  include/shaderc/shaderc.h
  include/shaderc/shaderc.hpp
  src/shaderc.cc
  src/shaderc_private.h
)

shaderc_default_compile_options(shaderc)
target_include_directories(shaderc PUBLIC include PRIVATE ${glslang_SOURCE_DIR})
find_package(Threads)
target_link_libraries(shaderc PRIVATE
  glslang OSDependent OGLCompiler glslang ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(shaderc PRIVATE shaderc_util)
target_link_libraries(shaderc PRIVATE SPIRV)  # from glslang
target_link_libraries(shaderc PRIVATE SPIRV-Tools)


shaderc_add_tests(
  TEST_PREFIX shaderc
  LINK_LIBS shaderc
  INCLUDE_DIRS include ${glslang_SOURCE_DIR}
  TEST_NAMES
    shaderc
    shaderc_cpp)


shaderc_combine_static_lib(shaderc_combined shaderc)

shaderc_add_tests(
TEST_PREFIX shaderc_combined
LINK_LIBS shaderc_combined ${CMAKE_THREAD_LIBS_INIT}
INCLUDE_DIRS include ${glslang_SOURCE_DIR}
TEST_NAMES
  shaderc
  shaderc_cpp)

add_executable(shaderc_c_smoke_test ./src/shaderc_c_smoke_test.c)
shaderc_default_c_compile_options(shaderc_c_smoke_test)

target_link_libraries(shaderc_c_smoke_test PRIVATE shaderc)
add_test(NAME shaderc_c_smoke_test COMMAND shaderc_c_smoke_test)

