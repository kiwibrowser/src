set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -fvisibility=hidden -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -std=c99 -Wall")

include_directories ("${PROJECT_SOURCE_DIR}" "../gen")

add_executable(test-wds tests.cpp $<TARGET_OBJECTS:wdsrtsp> $<TARGET_OBJECTS:wdscommon>)
set(LINK_FLAGS ${LINK_FLAGS} "-Wl,-whole-archive")
target_link_libraries (test-wds)

add_test(WfdTest test-wds)

OPTION(WDS_FUZZER "Binary that is used for fuzzer tests." OFF)
IF(WDS_FUZZER)
add_executable(wdsfuzzer wdsfuzzer.cpp $<TARGET_OBJECTS:wdsrtsp> $<TARGET_OBJECTS:wdscommon>)
set(LINK_FLAGS ${LINK_FLAGS} "-Wl,-whole-archive")
target_link_libraries (wdsfuzzer)
ENDIF(WDS_FUZZER)