shaderc_add_nosetests(expect)
shaderc_add_nosetests(glslc_test_framework)

add_test(NAME glslc_tests
  COMMAND ${PYTHON_EXECUTABLE}
  ${CMAKE_CURRENT_SOURCE_DIR}/glslc_test_framework.py
  $<TARGET_FILE:glslc_exe> --test-dir ${CMAKE_CURRENT_SOURCE_DIR})

