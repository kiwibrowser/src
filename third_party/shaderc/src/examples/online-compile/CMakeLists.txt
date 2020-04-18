add_executable(shaderc-online-compile main.cc)
shaderc_default_compile_options(shaderc-online-compile)
target_link_libraries(shaderc-online-compile PRIVATE shaderc)
