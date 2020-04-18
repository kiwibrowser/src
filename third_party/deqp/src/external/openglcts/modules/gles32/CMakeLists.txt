# GL-CTS.es32

include_directories(
	../gles31/functional
	../common
	)

set(GLCTS_ES32_SRCS
	es32cTestPackage.cpp
	es32cTestPackage.hpp
	es32cCopyImageTests.cpp
	es32cCopyImageTests.hpp
	)

set(GLCTS_ES32_LIBS
	glcts-common
	tcutil
	glutil
	${DEQP_GLES32_LIBRARIES}
	)

add_library(glcts-es32 STATIC ${GLCTS_ES32_SRCS})
target_link_libraries(glcts-es32 ${GLCTS_ES32_LIBS})
