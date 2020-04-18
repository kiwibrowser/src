# GL-CTS

include_directories(
	common
	gl
	gles2
	gles3
	gles31
	gles32
	glesext
	.
	)

if (DEQP_GTF_AVAILABLE)
	include_directories(${GTF_HOME}/glcts-gtf)
endif ()

add_subdirectory(common)
add_subdirectory(gl)
add_subdirectory(gles2)
add_subdirectory(gles3)
add_subdirectory(gles31)
add_subdirectory(gles32)
add_subdirectory(glesext)

if (DEQP_GTF_AVAILABLE)
	set(GTF_DIR ${GTF_HOME})
	add_subdirectory(${GTF_HOME}/glcts-gtf gtf)
endif ()

# glcTestPackageRegistry.cpp includes dEQP modules directly
include_directories(
	${PROJECT_SOURCE_DIR}/modules/egl
	${PROJECT_SOURCE_DIR}/modules/gles2
	${PROJECT_SOURCE_DIR}/modules/gles3
	${PROJECT_SOURCE_DIR}/modules/gles31
	)

set(GLCTS_SRCS
	glcTestPackageRegistry.cpp
	)

set(GLCTS_LIBS
	glcts-common-nocontext-package
	glcts-gl
	glcts-es2
	glcts-es3
	glcts-es31
	glcts-es32
	glcts-esext
	deqp-egl${MODULE_LIB_TARGET_POSTFIX}
	deqp-gles2${MODULE_LIB_TARGET_POSTFIX}
	deqp-gles3${MODULE_LIB_TARGET_POSTFIX}
	deqp-gles31${MODULE_LIB_TARGET_POSTFIX}
	)

# \note This module contains multiple packages (ES2-CTS, ES3-CTS, ..)
add_deqp_module(glcts "${GLCTS_SRCS}" "${GLCTS_LIBS}" glcTestPackageEntry.cpp)

# Data directories
add_data_dir(glcts ../data gl_cts/data)
add_data_dir(glcts ../../../data/gles2 gles2)
add_data_dir(glcts ../../../data/gles3 gles3)
add_data_dir(glcts ../../../external/graphicsfuzz/data/gles3 gles3)
add_data_dir(glcts ../../../data/gles31 gles31)

# CTS Test Runner
add_subdirectory(runner)
include_directories(runner)

if (DE_OS_IS_WIN32 OR DE_OS_IS_OSX OR DE_OS_IS_UNIX OR DE_OS_IS_IOS OR DE_OS_IS_QNX)
	# Stand-alone cts-runner executable
	add_executable(cts-runner runner/glcTestRunnerMain.cpp glcTestPackageEntry.cpp)
	target_link_libraries(cts-runner tcutil-platform glcts-runner glcts${MODULE_LIB_TARGET_POSTFIX})

elseif (DE_OS_IS_ANDROID)

	# glcAndroidMain.cpp contains additional entry points for
	# different CTS versions. We need to add it to DEQP_MODULE_ENTRY_POINTS
	# in order to include them in final libdeqp.so.
	set(DEQP_MODULE_ENTRY_POINTS	${DEQP_MODULE_ENTRY_POINTS} ${CMAKE_CURRENT_SOURCE_DIR}/runner/glcAndroidMain.cpp PARENT_SCOPE)
	set(DEQP_MODULE_LIBRARIES		${DEQP_MODULE_LIBRARIES} glcts-runner PARENT_SCOPE)
endif ()
