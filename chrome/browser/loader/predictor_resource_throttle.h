// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_LOADER_PREDICTOR_RESOURCE_THROTTLE_H_
#define CHROME_BROWSER_LOADER_PREDICTOR_RESOURCE_THROTTLE_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/resource_throttle.h"

namespace chrome_browser_net {
class Predictor;
}

namespace net {
struct RedirectInfo;
class URLRequest;
}

class GURL;
class ProfileIOData;

// This resource throttle tracks requests in order to help the predictor learn
// resource relationships. It notifies the predictor of redirect and referrer
// relationships, and populates the predictor's timed cache of ongoing
// navigations. It also initiates predictive actions based on navigation
// requests and redirects.
// Note: This class does not issue predictive actions off of the initial main
// frame request (before any redirects). That is done on the UI thread in
// response to navigation callbacks in predictor_tab_helper.cc.
// TODO(csharrison): This class shouldn't depend on chrome_browser_net. The
// predictor should probably be moved here (along with its dependencies).
class PredictorResourceThrottle : public content::ResourceThrottle {
 public:
  PredictorResourceThrottle(net::URLRequest* request,
                            chrome_browser_net::Predictor* predictor);
  ~PredictorResourceThrottle() override;

  static std::unique_ptr<PredictorResourceThrottle> MaybeCreate(
      net::URLRequest* request,
      ProfileIOData* io_data);

  // content::ResourceThrottle:
  void WillStartRequest(bool* defer) override;
  void WillRedirectRequest(const net::RedirectInfo& redirect_info,
                           bool* defer) override;
  const char* GetNameForLogging() const override;

 private:
  void DispatchPredictions(const GURL& url, const GURL& site_for_cookies);
  void DoPredict(const GURL& url, const GURL& site_for_cookies);

  net::URLRequest* request_;
  chrome_browser_net::Predictor* predictor_;

  base::WeakPtrFactory<PredictorResourceThrottle> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PredictorResourceThrottle);
};

#endif  // CHROME_BROWSER_LOADER_PREDICTOR_RESOURCE_THROTTLE_H_
