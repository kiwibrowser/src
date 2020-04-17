// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_GOOGLE_SEARCH_URL_TRACKER_CLIENT_H_
#define COMPONENTS_GOOGLE_SEARCH_URL_TRACKER_CLIENT_H_

#include "base/macros.h"
#include "url/gurl.h"

class SearchURLTracker;
class PrefService;

namespace network {
namespace mojom {
class URLLoaderFactory;
}
}

// Interface by which SearchURLTracker communicates with its embedder.
class SearchURLTrackerClient {
 public:
  SearchURLTrackerClient();
  virtual ~SearchURLTrackerClient();

  // Sets the SearchURLTracker that is associated with this client.
  void set_search_url_tracker(SearchURLTracker* search_url_tracker) {
    search_url_tracker_ = search_url_tracker;
  }

  // Returns whether background networking is enabled.
  virtual bool IsBackgroundNetworkingEnabled() = 0;

  // Returns the PrefService that the SearchURLTracker should use.
  virtual PrefService* GetPrefs() = 0;

  // Returns the URL loader factory that the SearchURLTracker should use.
  virtual network::mojom::URLLoaderFactory* GetURLLoaderFactory() = 0;

 protected:
  SearchURLTracker* search_url_tracker() { return search_url_tracker_; }

 private:
  SearchURLTracker* search_url_tracker_;

  DISALLOW_COPY_AND_ASSIGN(SearchURLTrackerClient);
};

#endif  // COMPONENTS_GOOGLE_SEARCH_URL_TRACKER_CLIENT_H_
