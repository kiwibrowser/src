// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_GOOGLE_GOOGLE_URL_TRACKER_CLIENT_H_
#define COMPONENTS_GOOGLE_GOOGLE_URL_TRACKER_CLIENT_H_

#include "base/macros.h"
#include "url/gurl.h"

class GoogleURLTracker;
class PrefService;

namespace network {
namespace mojom {
class URLLoaderFactory;
}
}

// Interface by which GoogleURLTracker communicates with its embedder.
class GoogleURLTrackerClient {
 public:
  GoogleURLTrackerClient();
  virtual ~GoogleURLTrackerClient();

  // Sets the GoogleURLTracker that is associated with this client.
  void set_google_url_tracker(GoogleURLTracker* google_url_tracker) {
    google_url_tracker_ = google_url_tracker;
  }

  // Returns whether background networking is enabled.
  virtual bool IsBackgroundNetworkingEnabled() = 0;

  // Returns the PrefService that the GoogleURLTracker should use.
  virtual PrefService* GetPrefs() = 0;

  // Returns the URL loader factory that the GoogleURLTracker should use.
  virtual network::mojom::URLLoaderFactory* GetURLLoaderFactory() = 0;

 protected:
  GoogleURLTracker* google_url_tracker() { return google_url_tracker_; }

 private:
  GoogleURLTracker* google_url_tracker_;

  DISALLOW_COPY_AND_ASSIGN(GoogleURLTrackerClient);
};

#endif  // COMPONENTS_GOOGLE_GOOGLE_URL_TRACKER_CLIENT_H_
