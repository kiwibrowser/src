# GL-CTS.es2

set(GLCTS_GLES2_SRCS
	es2cTestPackage.cpp
	es2cTestPackage.hpp
	es2cTexture3DTests.cpp
	es2cTexture3DTests.hpp
	)

set(GLCTS_GLES2_LIBS
	glcts-common
	tcutil
	glutil
	${DEQP_GLES2_LIBRARIES}
	)

add_library(glcts-es2 STATIC ${GLCTS_GLES2_SRCS})
target_link_libraries(glcts-es2 ${GLCTS_GLES2_LIBS})
