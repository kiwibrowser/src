/*
 * Copyright (C) 2014 The Android Open Source Project
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


/*
 * This file defines an NDK API.
 * Do not remove methods.
 * Do not change method signatures.
 * Do not change the value of constants.
 * Do not change the size of any of the classes defined in here.
 * Do not reference types that are not part of the NDK.
 * Do not #include files that aren't part of the NDK.
 */

#ifndef _NDK_MEDIA_ERROR_H
#define _NDK_MEDIA_ERROR_H

#include <sys/cdefs.h>

__BEGIN_DECLS

typedef enum {
    AMEDIA_OK = 0,

    AMEDIA_ERROR_BASE                  = -10000,
    AMEDIA_ERROR_UNKNOWN               = AMEDIA_ERROR_BASE,
    AMEDIA_ERROR_MALFORMED             = AMEDIA_ERROR_BASE - 1,
    AMEDIA_ERROR_UNSUPPORTED           = AMEDIA_ERROR_BASE - 2,
    AMEDIA_ERROR_INVALID_OBJECT        = AMEDIA_ERROR_BASE - 3,
    AMEDIA_ERROR_INVALID_PARAMETER     = AMEDIA_ERROR_BASE - 4,
    AMEDIA_ERROR_INVALID_OPERATION     = AMEDIA_ERROR_BASE - 5,

    AMEDIA_DRM_ERROR_BASE              = -20000,
    AMEDIA_DRM_NOT_PROVISIONED         = AMEDIA_DRM_ERROR_BASE - 1,
    AMEDIA_DRM_RESOURCE_BUSY           = AMEDIA_DRM_ERROR_BASE - 2,
    AMEDIA_DRM_DEVICE_REVOKED          = AMEDIA_DRM_ERROR_BASE - 3,
    AMEDIA_DRM_SHORT_BUFFER            = AMEDIA_DRM_ERROR_BASE - 4,
    AMEDIA_DRM_SESSION_NOT_OPENED      = AMEDIA_DRM_ERROR_BASE - 5,
    AMEDIA_DRM_TAMPER_DETECTED         = AMEDIA_DRM_ERROR_BASE - 6,
    AMEDIA_DRM_VERIFY_FAILED           = AMEDIA_DRM_ERROR_BASE - 7,
    AMEDIA_DRM_NEED_KEY                = AMEDIA_DRM_ERROR_BASE - 8,
    AMEDIA_DRM_LICENSE_EXPIRED         = AMEDIA_DRM_ERROR_BASE - 9,

    AMEDIA_IMGREADER_ERROR_BASE          = -30000,
    AMEDIA_IMGREADER_NO_BUFFER_AVAILABLE = AMEDIA_IMGREADER_ERROR_BASE - 1,
    AMEDIA_IMGREADER_MAX_IMAGES_ACQUIRED = AMEDIA_IMGREADER_ERROR_BASE - 2,
    AMEDIA_IMGREADER_CANNOT_LOCK_IMAGE   = AMEDIA_IMGREADER_ERROR_BASE - 3,
    AMEDIA_IMGREADER_CANNOT_UNLOCK_IMAGE = AMEDIA_IMGREADER_ERROR_BASE - 4,
    AMEDIA_IMGREADER_IMAGE_NOT_LOCKED    = AMEDIA_IMGREADER_ERROR_BASE - 5,

} media_status_t;

__END_DECLS

#endif // _NDK_MEDIA_ERROR_H
