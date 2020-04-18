message("*** Using Surfaceless target")

set(DEQP_TARGET_NAME "Surfaceless")

include(FindPkgConfig)

set(DEQP_USE_SURFACELESS ON)

set(DEQP_SUPPORT_GLES2   ON)
set(DEQP_SUPPORT_GLES3   ON)
set(DEQP_SUPPORT_EGL     ON)

find_library(GLES2_LIBRARIES GLESv2)
find_library(GLES3_LIBRARIES GLESv3)
find_path(GLES2_INCLUDE_PATH GLES2/gl2.h)
find_path(GLES3_INCLUDE_PATH GLES3/gl3.h)

if (GLES2_INCLUDE_PATH AND GLES2_LIBRARIES)
        set(DEQP_GLES2_LIBRARIES ${GLES2_LIBRARIES})
else ()
        message (SEND_ERROR "GLESv2 support not found")
endif ()

if (GLES3_INCLUDE_PATH AND GLES3_LIBRARIES)
        set(DEQP_GLES3_LIBRARIES ${GLES3_LIBRARIES})
elseif (GLES3_INCLUDE_PATH AND GLES2_LIBRARIES)
        # Assume GLESv2 provides ES3 symbols if gl3.h was found
        # and the GLESv3 library was not.
        set(DEQP_GLES3_LIBRARIES ${GLES2_LIBRARIES})
else ()
        message (FATAL_ERROR "GLESv3 support not found")
endif ()

pkg_check_modules(EGL REQUIRED egl)
set(DEQP_EGL_LIBRARIES ${EGL_LIBRARIES})

pkg_check_modules(GBM REQUIRED gbm)
pkg_check_modules(KMS REQUIRED libkms)
pkg_check_modules(DRM REQUIRED libdrm)

include_directories(${GLES2_INCLUDE_PATH} ${GLES3_INCLUDE_PATH}
                    ${EGL_INCLUDE_DIRS} ${GBM_INCLUDE_DIRS}
                    ${KMS_INCLUDE_DIRS} ${DRM_INCLUDE_DIRS})

set(DEQP_PLATFORM_LIBRARIES ${DEQP_GLES2_LIBRARIES} ${DEQP_GLES3_LIBRARIES}
                            ${DEQP_EGL_LIBRARIES} ${GBM_LIBRARIES}
                            ${KMS_LIBRARIES} ${DRM_LIBRARIES})
