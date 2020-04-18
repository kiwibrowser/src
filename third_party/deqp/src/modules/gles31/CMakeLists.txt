# dEQP-GLES31

include_directories(
	../glshared
	.				# For child modules
	)

add_subdirectory(functional)
add_subdirectory(stress)

include_directories(
	functional
	stress
	)

set(DEQP_GLES31_SRCS
	tes31Context.cpp
	tes31Context.hpp
	tes31InfoTests.cpp
	tes31InfoTests.hpp
	tes31TestCase.cpp
	tes31TestCase.hpp
	tes31TestPackage.cpp
	tes31TestPackage.hpp
	)

set(DEQP_GLES31_LIBS
	deqp-gles31-functional
	deqp-gles31-stress
	tcutil
	glutil
	${DEQP_GLES3_LIBRARIES}
	)

add_deqp_module(deqp-gles31 "${DEQP_GLES31_SRCS}" "${DEQP_GLES31_LIBS}" tes31TestPackageEntry.cpp)

# Data directories
add_data_dir(deqp-gles31 ../../data/gles31/data		gles31/data)
add_data_dir(deqp-gles31 ../../data/gles31/shaders	gles31/shaders)
