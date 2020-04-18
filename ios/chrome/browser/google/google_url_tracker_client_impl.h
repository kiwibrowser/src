// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_GOOGLE_GOOGLE_URL_TRACKER_CLIENT_IMPL_H_
#define IOS_CHROME_BROWSER_GOOGLE_GOOGLE_URL_TRACKER_CLIENT_IMPL_H_

#include "base/macros.h"
#include "components/google/core/browser/google_url_tracker_client.h"

class PrefService;

namespace ios {
class ChromeBrowserState;
}

class GoogleURLTrackerClientImpl : public GoogleURLTrackerClient {
 public:
  explicit GoogleURLTrackerClientImpl(ios::ChromeBrowserState* browser_state);
  ~GoogleURLTrackerClientImpl() override;

 private:
  // GoogleURLTrackerClient implementation.
  bool IsBackgroundNetworkingEnabled() override;
  PrefService* GetPrefs() override;
  network::mojom::URLLoaderFactory* GetURLLoaderFactory() override;

  ios::ChromeBrowserState* browser_state_;

  DISALLOW_COPY_AND_ASSIGN(GoogleURLTrackerClientImpl);
};

#endif  // IOS_CHROME_BROWSER_GOOGLE_GOOGLE_URL_TRACKER_CLIENT_IMPL_H_
