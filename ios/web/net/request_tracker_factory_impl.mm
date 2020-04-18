// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/net/request_tracker_factory_impl.h"

#include "base/logging.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/sys_string_conversions.h"
#import "ios/web/net/request_group_util.h"
#import "ios/web/net/request_tracker_impl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

RequestTrackerFactoryImpl::RequestTrackerFactoryImpl(
    const std::string& application_scheme) {
  if (!application_scheme.empty()) {
    application_scheme_ = [base::SysUTF8ToNSString(application_scheme) copy];
    DCHECK(application_scheme_);
  }
}

RequestTrackerFactoryImpl::~RequestTrackerFactoryImpl() {
}

bool RequestTrackerFactoryImpl::GetRequestTracker(
    NSURLRequest* request,
    base::WeakPtr<net::RequestTracker>* tracker) {
  DCHECK(tracker);
  DCHECK(!tracker->get());
  NSString* request_group_id =
      web::ExtractRequestGroupIDFromRequest(request, application_scheme_);
  if (!request_group_id) {
    // There was no request_group_id, so the request was from something like a
    // data: or file: URL.
    return true;
  }
  RequestTrackerImpl* tracker_impl =
      RequestTrackerImpl::GetTrackerForRequestGroupID(request_group_id);
  if (tracker_impl)
    *tracker = tracker_impl->GetWeakPtr();
  // If there is a request group ID, but no associated tracker, return false.
  // This usually happens when the tab has been closed, but can maybe also
  // happen in other cases (see http://crbug.com/228397).
  return tracker->get() != nullptr;
}

}  // namespace web
