set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -fvisibility=hidden")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -std=c99 -Wall")


add_library(wds
    SHARED $<TARGET_OBJECTS:wdsrtsp>
           $<TARGET_OBJECTS:wdscommon>
           $<TARGET_OBJECTS:wdssink>
           $<TARGET_OBJECTS:wdssource>)
set_target_properties(wds PROPERTIES
    VERSION ${WDS_VERSION_MAJOR}.${WDS_VERSION_MINOR}.${WDS_VERSION_PATCH}
    SOVERSION ${WDS_VERSION_MAJOR})

install(TARGETS wds LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

set(PUBLIC_HEADERS
    public/connector_type.h
    public/peer.h
    public/video_format.h
    public/sink.h
    public/wds_export.h
    public/audio_codec.h
    public/media_manager.h
    public/source.h
    public/logging.h)

install(FILES ${PUBLIC_HEADERS} DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/wds)
