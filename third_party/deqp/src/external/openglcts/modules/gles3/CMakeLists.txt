# GL-CTS.es3

set(GLCTS_ES3_SRCS
	es3cTestPackage.cpp
	es3cTestPackage.hpp
	)

set(GLCTS_ES3_LIBS
	glcts-common
	tcutil
	glutil
	${DEQP_GLES3_LIBRARIES}
	)

add_library(glcts-es3 STATIC ${GLCTS_ES3_SRCS})
target_link_libraries(glcts-es3 ${GLCTS_ES3_LIBS})
