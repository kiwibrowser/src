# dEQP-GLES31.stress

set(DEQP_GLES31_STRESS_SRCS
	es31sDrawTests.cpp
	es31sDrawTests.hpp
	es31sStressTests.cpp
	es31sStressTests.hpp
	es31sVertexAttributeBindingTests.cpp
	es31sVertexAttributeBindingTests.hpp
	es31sTessellationGeometryInteractionTests.cpp
	es31sTessellationGeometryInteractionTests.hpp
	)

add_library(deqp-gles31-stress STATIC ${DEQP_GLES31_STRESS_SRCS})
target_link_libraries(deqp-gles31-stress deqp-gl-shared glutil tcutil ${DEQP_GLES31_LIBRARIES})
