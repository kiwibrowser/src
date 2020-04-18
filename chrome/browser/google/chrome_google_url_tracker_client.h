// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_GOOGLE_CHROME_GOOGLE_URL_TRACKER_CLIENT_H_
#define CHROME_BROWSER_GOOGLE_CHROME_GOOGLE_URL_TRACKER_CLIENT_H_

#include "base/macros.h"
#include "components/google/core/browser/google_url_tracker_client.h"

class Profile;

class ChromeGoogleURLTrackerClient : public GoogleURLTrackerClient {
 public:
  explicit ChromeGoogleURLTrackerClient(Profile* profile);
  ~ChromeGoogleURLTrackerClient() override;

  // GoogleURLTrackerClient:
  bool IsBackgroundNetworkingEnabled() override;
  PrefService* GetPrefs() override;
  network::mojom::URLLoaderFactory* GetURLLoaderFactory() override;

 private:
  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(ChromeGoogleURLTrackerClient);
};

#endif  // CHROME_BROWSER_GOOGLE_CHROME_GOOGLE_URL_TRACKER_CLIENT_H_
