// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ANDROID_CDM_MEDIA_DRM_STORAGE_FACTORY_H_
#define CHROME_BROWSER_MEDIA_ANDROID_CDM_MEDIA_DRM_STORAGE_FACTORY_H_

#include "media/mojo/interfaces/media_drm_storage.mojom.h"

namespace content {
class RenderFrameHost;
}

void CreateMediaDrmStorage(content::RenderFrameHost* render_frame_host,
                           media::mojom::MediaDrmStorageRequest request);

#endif  // CHROME_BROWSER_MEDIA_ANDROID_CDM_MEDIA_DRM_STORAGE_FACTORY_H_
