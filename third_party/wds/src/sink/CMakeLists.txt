#FIXME in the future: with cmake 2.8.12 and up it's better
#to use target_compile_options or add_compile_options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pthread -std=c++11")

include_directories ("${PROJECT_SOURCE_DIR}")
include_directories ("${PROJECT_SOURCE_DIR}/include")
include_directories ("${PROJECT_SOURCE_DIR}/p2p")
include_directories ("${PROJECT_SOURCE_DIR}/mirac_network")

find_package(PkgConfig REQUIRED)

pkg_check_modules (GLIB2 REQUIRED glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})

pkg_check_modules (GIO REQUIRED gio-2.0)
include_directories(${GIO_INCLUDE_DIRS})

pkg_check_modules (GST REQUIRED gstreamer-1.0)
include_directories(${GST_INCLUDE_DIRS})

add_executable(sink-test main.cpp sink-app.cpp sink.cpp gst_sink_media_manager.cpp)
target_link_libraries (sink-test mirac wds p2p ${GIO_LIBRARIES} ${GST_LIBRARIES})
