// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_INSTALLABLE_INSTALLABLE_DATA_H_
#define CHROME_BROWSER_INSTALLABLE_INSTALLABLE_DATA_H_

#include "chrome/browser/installable/installable_logging.h"
#include "third_party/blink/public/common/manifest/manifest.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "url/gurl.h"

// This struct is passed to an InstallableCallback when the InstallableManager
// has finished working. Each reference is owned by InstallableManager, and
// callers should copy any objects which they wish to use later. Non-requested
// fields will be set to null, empty, or false.
struct InstallableData {
  InstallableData(InstallableStatusCode error_code,
                  GURL manifest_url,
                  const blink::Manifest* manifest,
                  GURL primary_icon_url,
                  const SkBitmap* primary_icon,
                  GURL badge_icon_url,
                  const SkBitmap* badge_icon,
                  bool valid_manifest,
                  bool has_worker);
  ~InstallableData();

  // NO_ERROR_DETECTED if there were no issues.
  const InstallableStatusCode error_code = NO_ERROR_DETECTED;

  // Empty if the site has no <link rel="manifest"> tag.
  const GURL manifest_url;

  // Empty if the site has an unparseable manifest.
  const blink::Manifest* manifest;

  // Empty if no primary_icon was requested.
  const GURL primary_icon_url;

  // nullptr if the most appropriate primary icon couldn't be determined or
  // downloaded. The underlying primary icon is owned by the InstallableManager;
  // clients must copy the bitmap if they want to to use it. If
  // valid_primary_icon was true and a primary icon could not be retrieved, the
  // reason will be in error_code.
  const SkBitmap* primary_icon;

  // Empty if no badge_icon was requested.
  const GURL badge_icon_url;

  // nullptr if the most appropriate badge icon couldn't be determined or
  // downloaded. The underlying badge icon is owned by the InstallableManager;
  // clients must copy the bitmap if they want to to use it. Since the badge
  // icon is optional, no error code is set if it cannot be fetched, and clients
  // specifying valid_badge_icon must check that the bitmap exists before using
  // it.
  const SkBitmap* badge_icon;

  // true if the site has a viable web app manifest. If valid_manifest or
  // has_worker was true and the site isn't installable, the reason will be in
  // error_code.
  const bool valid_manifest = false;

  // true if the site has a service worker with a fetch handler. If has_worker
  // was true and the site isn't installable, the reason will be in error_code.
  const bool has_worker = false;
};

using InstallableCallback = base::Callback<void(const InstallableData&)>;

#endif  // CHROME_BROWSER_INSTALLABLE_INSTALLABLE_DATA_H_
