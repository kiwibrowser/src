# dEQP-EGL

set(DEQP_EGL_SRCS
	teglAndroidUtil.cpp
	teglAndroidUtil.hpp
	teglApiCase.cpp
	teglApiCase.hpp
	teglBufferAgeTests.hpp
	teglBufferAgeTests.cpp
	teglChooseConfigReference.cpp
	teglChooseConfigReference.hpp
	teglChooseConfigTests.cpp
	teglChooseConfigTests.hpp
	teglQueryConfigTests.cpp
	teglQueryConfigTests.hpp
	teglColorClearCase.cpp
	teglColorClearCase.hpp
	teglColorClearTests.cpp
	teglColorClearTests.hpp
	teglConfigList.cpp
	teglConfigList.hpp
	teglCreateContextTests.cpp
	teglCreateContextTests.hpp
	teglGetFrameTimestampsTests.cpp
	teglGetFrameTimestampsTests.hpp
	teglQueryContextTests.cpp
	teglQueryContextTests.hpp
	teglCreateSurfaceTests.cpp
	teglCreateSurfaceTests.hpp
	teglQuerySurfaceTests.cpp
	teglQuerySurfaceTests.hpp
	teglGetProcAddressTests.cpp
	teglGetProcAddressTests.hpp
	teglGLES1RenderUtil.cpp
	teglGLES1RenderUtil.hpp
	teglGLES2RenderUtil.cpp
	teglGLES2RenderUtil.hpp
	teglImageUtil.cpp
	teglImageUtil.hpp
	teglImageTests.cpp
	teglImageTests.hpp
	teglInfoTests.cpp
	teglInfoTests.hpp
	teglMutableRenderBufferTests.cpp
	teglMutableRenderBufferTests.hpp
	teglNegativeApiTests.cpp
	teglNegativeApiTests.hpp
	teglPartialUpdateTests.cpp
	teglPartialUpdateTests.hpp
	teglNegativePartialUpdateTests.cpp
	teglNegativePartialUpdateTests.hpp
	teglRenderCase.cpp
	teglRenderCase.hpp
	teglRenderTests.cpp
	teglRenderTests.hpp
	teglSimpleConfigCase.cpp
	teglSimpleConfigCase.hpp
	teglTestCase.cpp
	teglTestCase.hpp
	teglTestPackage.cpp
	teglTestPackage.hpp
	teglVGRenderUtil.cpp
	teglVGRenderUtil.hpp
	teglImageFormatTests.hpp
	teglImageFormatTests.cpp
	teglGLES2SharingTests.hpp
	teglGLES2SharingTests.cpp
	teglGLES2SharingThreadedTests.hpp
	teglGLES2SharingThreadedTests.cpp
	teglSyncTests.hpp
	teglSyncTests.cpp
	teglMultiThreadTests.hpp
	teglMultiThreadTests.cpp
	teglMemoryStressTests.hpp
	teglMemoryStressTests.cpp
	teglMakeCurrentPerfTests.hpp
	teglMakeCurrentPerfTests.cpp
	teglGLES2SharedRenderingPerfTests.hpp
	teglGLES2SharedRenderingPerfTests.cpp
	teglPreservingSwapTests.hpp
	teglPreservingSwapTests.cpp
	teglClientExtensionTests.hpp
	teglClientExtensionTests.cpp
	teglCreateContextExtTests.hpp
	teglCreateContextExtTests.cpp
	teglSurfacelessContextTests.hpp
	teglSurfacelessContextTests.cpp
	teglSwapBuffersTests.hpp
	teglSwapBuffersTests.cpp
	teglNativeColorMappingTests.hpp
	teglNativeColorMappingTests.cpp
	teglNativeCoordMappingTests.hpp
	teglNativeCoordMappingTests.cpp
	teglResizeTests.hpp
	teglResizeTests.cpp
	teglSwapBuffersWithDamageTests.hpp
	teglSwapBuffersWithDamageTests.cpp
	teglMultiContextTests.hpp
	teglMultiContextTests.cpp
	teglThreadCleanUpTests.hpp
	teglThreadCleanUpTests.cpp
	teglRobustnessTests.hpp
	teglRobustnessTests.cpp
	teglWideColorTests.hpp
	teglWideColorTests.cpp
	)

set(DEQP_EGL_LIBS
	tcutil
	eglutil
	referencerenderer
	glutil
	glutil-sglr
	${DEQP_EGL_LIBRARIES}
	)

if (DE_OS_IS_ANDROID)
	# EGL tests require headers from Android port directory
	include_directories(${CMAKE_SOURCE_DIR}/framework/platform/android)
	set(DEQP_EGL_LIBS ${DEQP_EGL_LIBS} tcutil-platform)
endif ()

if (DEQP_SUPPORT_GLES1)
	set(DEQP_EGL_LIBS ${DEQP_EGL_LIBS} ${DEQP_GLES1_LIBRARIES})
endif ()

if (DEQP_SUPPORT_VG)
	set(DEQP_EGL_LIBS ${DEQP_EGL_LIBS} ${DEQP_VG_LIBRARIES})
endif ()

add_deqp_module(deqp-egl "${DEQP_EGL_SRCS}" "${DEQP_EGL_LIBS}" teglTestPackageEntry.cpp)
