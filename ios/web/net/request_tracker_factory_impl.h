// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_NET_REQUEST_TRACKER_FACTORY_IMPL_H_
#define IOS_WEB_NET_REQUEST_TRACKER_FACTORY_IMPL_H_

#include <string>

#import "ios/net/request_tracker.h"

namespace web {

class RequestTrackerFactoryImpl
    : public net::RequestTracker::RequestTrackerFactory {
 public:
  explicit RequestTrackerFactoryImpl(const std::string& application_scheme);
  ~RequestTrackerFactoryImpl() override;

 private:
  // RequestTracker::RequestTrackerFactory implementation
  bool GetRequestTracker(NSURLRequest* request,
                         base::WeakPtr<net::RequestTracker>* tracker) override;

  NSString* application_scheme_;
};

}  // namespace web

#endif  // IOS_WEB_NET_REQUEST_TRACKER_FACTORY_IMPL_H_
