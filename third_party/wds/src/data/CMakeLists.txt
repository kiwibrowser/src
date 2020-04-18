configure_file(wds.pc.in wds.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/wds.pc
        DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/pkgconfig)
