// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_INSTALLABLE_INSTALLABLE_PARAMS_H_
#define CHROME_BROWSER_INSTALLABLE_INSTALLABLE_PARAMS_H_

// This struct specifies the work to be done by the InstallableManager.
// Data is cached and fetched in the order specified in this struct.
// If the eligibility check fails, processing halts immediately. Otherwise, a
// web app manifest is fetched before the remaining items.
struct InstallableParams {
  // Check whether the current WebContents is eligible to be installed, i.e it:
  //  - is served over HTTPS
  //  - is a top-level frame
  //  - is not in an incognito profile.
  bool check_eligibility = false;

  // Check whether there is a fetchable, non-empty icon in the manifest
  // conforming to the primary icon size parameters.
  bool valid_primary_icon = false;

  // Check whether there is a fetchable, non-empty icon in the manifest
  // conforming to the badge icon size parameters.
  bool valid_badge_icon = false;

  // Check whether the site has a manifest valid for a web app.
  bool valid_manifest = false;

  // Check whether the site has a service worker controlling the manifest start
  // URL and the current URL.
  bool has_worker = false;

  // Whether or not to wait indefinitely for a service worker. If this is set to
  // false, the worker status will not be cached and will be re-checked if
  // GetData() is called again for the current page. Setting this to true means
  // that the callback will not be called for any site that does not install a
  // service worker.
  bool wait_for_worker = false;
};

#endif  // CHROME_BROWSER_INSTALLABLE_INSTALLABLE_PARAMS_H_
