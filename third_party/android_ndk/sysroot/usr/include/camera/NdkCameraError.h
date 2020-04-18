/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @addtogroup Camera
 * @{
 */

/**
 * @file NdkCameraError.h
 */

/*
 * This file defines an NDK API.
 * Do not remove methods.
 * Do not change method signatures.
 * Do not change the value of constants.
 * Do not change the size of any of the classes defined in here.
 * Do not reference types that are not part of the NDK.
 * Do not #include files that aren't part of the NDK.
 */

#ifndef _NDK_CAMERA_ERROR_H
#define _NDK_CAMERA_ERROR_H

#include <sys/cdefs.h>

__BEGIN_DECLS

#if __ANDROID_API__ >= 24

typedef enum {
    ACAMERA_OK = 0,

    ACAMERA_ERROR_BASE                  = -10000,

    /**
     * Camera operation has failed due to an unspecified cause.
     */
    ACAMERA_ERROR_UNKNOWN               = ACAMERA_ERROR_BASE,

    /**
     * Camera operation has failed due to an invalid parameter being passed to the method.
     */
    ACAMERA_ERROR_INVALID_PARAMETER     = ACAMERA_ERROR_BASE - 1,

    /**
     * Camera operation has failed because the camera device has been closed, possibly because a
     * higher-priority client has taken ownership of the camera device.
     */
    ACAMERA_ERROR_CAMERA_DISCONNECTED   = ACAMERA_ERROR_BASE - 2,

    /**
     * Camera operation has failed due to insufficient memory.
     */
    ACAMERA_ERROR_NOT_ENOUGH_MEMORY     = ACAMERA_ERROR_BASE - 3,

    /**
     * Camera operation has failed due to the requested metadata tag cannot be found in input
     * {@link ACameraMetadata} or {@link ACaptureRequest}.
     */
    ACAMERA_ERROR_METADATA_NOT_FOUND    = ACAMERA_ERROR_BASE - 4,

    /**
     * Camera operation has failed and the camera device has encountered a fatal error and needs to
     * be re-opened before it can be used again.
     */
    ACAMERA_ERROR_CAMERA_DEVICE         = ACAMERA_ERROR_BASE - 5,

    /**
     * Camera operation has failed and the camera service has encountered a fatal error.
     *
     * <p>The Android device may need to be shut down and restarted to restore
     * camera function, or there may be a persistent hardware problem.</p>
     *
     * <p>An attempt at recovery may be possible by closing the
     * ACameraDevice and the ACameraManager, and trying to acquire all resources
     * again from scratch.</p>
     */
    ACAMERA_ERROR_CAMERA_SERVICE        = ACAMERA_ERROR_BASE - 6,

    /**
     * The {@link ACameraCaptureSession} has been closed and cannnot perform any operation other
     * than {@link ACameraCaptureSession_close}.
     */
    ACAMERA_ERROR_SESSION_CLOSED        = ACAMERA_ERROR_BASE - 7,

    /**
     * Camera operation has failed due to an invalid internal operation. Usually this is due to a
     * low-level problem that may resolve itself on retry
     */
    ACAMERA_ERROR_INVALID_OPERATION     = ACAMERA_ERROR_BASE - 8,

    /**
     * Camera device does not support the stream configuration provided by application in
     * {@link ACameraDevice_createCaptureSession}.
     */
    ACAMERA_ERROR_STREAM_CONFIGURE_FAIL = ACAMERA_ERROR_BASE - 9,

    /**
     * Camera device is being used by another higher priority camera API client.
     */
    ACAMERA_ERROR_CAMERA_IN_USE         = ACAMERA_ERROR_BASE - 10,

    /**
     * The system-wide limit for number of open cameras or camera resources has been reached, and
     * more camera devices cannot be opened until previous instances are closed.
     */
    ACAMERA_ERROR_MAX_CAMERA_IN_USE     = ACAMERA_ERROR_BASE - 11,

    /**
     * The camera is disabled due to a device policy, and cannot be opened.
     */
    ACAMERA_ERROR_CAMERA_DISABLED       = ACAMERA_ERROR_BASE - 12,

    /**
     * The application does not have permission to open camera.
     */
    ACAMERA_ERROR_PERMISSION_DENIED     = ACAMERA_ERROR_BASE - 13,
} camera_status_t;

#endif /* __ANDROID_API__ >= 24 */

__END_DECLS

#endif /* _NDK_CAMERA_ERROR_H */

/** @} */
