set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -fvisibility=hidden -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -std=c99 -Wall")

include_directories ("${PROJECT_SOURCE_DIR}")

add_library(wdssource OBJECT
    source.cpp cap_negotiation_state.cpp init_state.cpp
    session_state.cpp streaming_state.cpp)
add_dependencies(wdssource wdsrtsp)
