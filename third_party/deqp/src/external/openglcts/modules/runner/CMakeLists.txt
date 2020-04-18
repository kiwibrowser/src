set(GLCTS_RUNNER_SRCS
	glcTestRunner.cpp
	glcTestRunner.hpp
	)

if (DE_OS_IS_ANDROID)
	include_directories(${PROJECT_SOURCE_DIR}/framework/platform/android)

	set(GLCTS_RUNNER_SRCS
		${GLCTS_RUNNER_SRCS}
		glcAndroidTestActivity.cpp
		glcAndroidTestActivity.hpp
		)
endif ()

add_library(glcts-runner STATIC ${GLCTS_RUNNER_SRCS})
