// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ANDROID_CDM_MEDIA_DRM_LICENSE_MANAGER_H_
#define CHROME_BROWSER_MEDIA_ANDROID_CDM_MEDIA_DRM_LICENSE_MANAGER_H_

#include "base/callback.h"
#include "base/time/time.h"

class GURL;
class PrefService;

// Clear media licenses and related data if:
// 1. Creation time falls in [delete_begin, delete_end], and
// 2. |filter| returns true for the origin. |filter| is passed in to allow
// licenses under specific origins to be cleared. Empty |filter| means remove
// licenses for all origins.
//
// Media license session data will be removed from persist storage. Removing the
// actual license file needs ack response from license server, so it's hard for
// Chromium to do that. Since it's difficult to get the real id for the license
// without the session data, we can treat the licenses as cleared.
//
// If all the licenses under the origin are cleared, the origin will be
// unprovisioned, a.k.a the cert will be removed.
void ClearMediaDrmLicenses(
    PrefService* prefs,
    base::Time delete_begin,
    base::Time delete_end,
    const base::RepeatingCallback<bool(const GURL& url)>& filter,
    base::OnceClosure complete_cb);

#endif  // CHROME_BROWSER_MEDIA_ANDROID_CDM_MEDIA_DRM_LICENSE_MANAGER_H_
