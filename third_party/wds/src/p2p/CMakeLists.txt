#FIXME in the future: with cmake 2.8.12 and up it's better
#to use target_compile_options or add_compile_options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -std=c++11 -ggdb3 -Wall")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -O0 -std=c99 -ggdb3 -Wall")

find_package(PkgConfig REQUIRED)
pkg_check_modules (GIO REQUIRED gio-2.0)
include_directories(${GIO_INCLUDE_DIRS})

add_library(p2p STATIC
    connman-peer.cpp connman-client.cpp information-element.cpp
)

add_executable(register-peer-service main.cpp)
target_link_libraries (register-peer-service p2p ${GIO_LIBRARIES})

add_executable(test-ie test-ie.cpp)
target_link_libraries (test-ie p2p ${GIO_LIBRARIES})

add_test(InformationElementTest test-ie)
