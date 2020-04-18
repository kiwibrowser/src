set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -fvisibility=hidden -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -std=c99 -Wall")

include_directories ("${PROJECT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/libwds/rtsp/gen")

add_library(wdscommon OBJECT
    logging.cpp message_handler.cpp rtsp_input_handler.cpp video_format.cpp)
add_dependencies(wdscommon wdsrtsp)
