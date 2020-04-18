include_directories(..)

set(DEQP_VK_GEOMETRY_SRCS
	vktGeometryBasicClass.cpp
	vktGeometryBasicClass.hpp
	vktGeometryBasicGeometryShaderTests.cpp
	vktGeometryBasicGeometryShaderTests.hpp
	vktGeometryEmitGeometryShaderTests.cpp
	vktGeometryEmitGeometryShaderTests.hpp
	vktGeometryVaryingGeometryShaderTests.cpp
	vktGeometryVaryingGeometryShaderTests.hpp
	vktGeometryInputGeometryShaderTests.cpp
	vktGeometryInputGeometryShaderTests.hpp
	vktGeometryInstancedRenderingTests.cpp
	vktGeometryInstancedRenderingTests.hpp
	vktGeometryLayeredRenderingTests.cpp
	vktGeometryLayeredRenderingTests.hpp
	vktGeometryTests.cpp
	vktGeometryTests.hpp
	vktGeometryTestsUtil.cpp
	vktGeometryTestsUtil.hpp
	)

set(DEQP_VK_GEOMETRY_LIBS
	tcutil
	vkutil
	)

add_library(deqp-vk-geometry STATIC ${DEQP_VK_GEOMETRY_SRCS})
target_link_libraries(deqp-vk-geometry ${DEQP_VK_GEOMETRY_LIBS})
