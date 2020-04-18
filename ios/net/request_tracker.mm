// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/net/request_tracker.h"

#include "base/logging.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace net {

namespace {

// Reference to the single instance of the RequestTrackerFactory.
RequestTracker::RequestTrackerFactory* g_request_tracker_factory = nullptr;

}  // namespace

RequestTracker::RequestTrackerFactory::~RequestTrackerFactory() {
}

// static
void RequestTracker::SetRequestTrackerFactory(RequestTrackerFactory* factory) {
  g_request_tracker_factory = factory;
}

// static
bool RequestTracker::GetRequestTracker(NSURLRequest* request,
                                       base::WeakPtr<RequestTracker>* tracker) {
  DCHECK(request);
  DCHECK(tracker);
  DCHECK(!tracker->get());
  if (!g_request_tracker_factory) {
    return true;
  }
  return g_request_tracker_factory->GetRequestTracker(request, tracker);
}

RequestTracker::RequestTracker()
    : initialized_(false),
      weak_ptr_factory_(this) {
  // RequestTracker can be created from the main thread and used from another
  // thread.
  thread_checker_.DetachFromThread();
}

RequestTracker::~RequestTracker() {
}

base::WeakPtr<RequestTracker> RequestTracker::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void RequestTracker::InvalidateWeakPtrs() {
  weak_ptr_factory_.InvalidateWeakPtrs();
}

void RequestTracker::Init() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!initialized_);
  initialized_ = true;
}

}  // namespace net
