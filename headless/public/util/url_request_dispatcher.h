// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_PUBLIC_UTIL_URL_REQUEST_DISPATCHER_H_
#define HEADLESS_PUBLIC_UTIL_URL_REQUEST_DISPATCHER_H_

#include "base/macros.h"
#include "headless/public/headless_export.h"
#include "net/base/net_errors.h"

namespace headless {
class ManagedDispatchURLRequestJob;
class NavigationRequest;

// Interface to abstract and potentially reorder (for determinism) calls to
// ManagedDispatchUrlRequestJob::OnHeadersComplete and
// ManagedDispatchUrlRequestJob::NotifyStartError.
class HEADLESS_EXPORT URLRequestDispatcher {
 public:
  URLRequestDispatcher() {}
  virtual ~URLRequestDispatcher() {}

  // Tells us a URLRequestJob was created. Can be called from any thread.
  virtual void JobCreated(ManagedDispatchURLRequestJob* job) = 0;

  // Tells us a URLRequestJob got killed. Can be called from any thread.
  virtual void JobKilled(ManagedDispatchURLRequestJob* job) = 0;

  // Tells us a URLRequestJob failed. Can be called from any thread.
  virtual void JobFailed(ManagedDispatchURLRequestJob* job,
                         net::Error error) = 0;

  // Tells us a URLRequestJob has fetched the data and is ready for the net
  // stack to consume it. Can be called from any thread.
  virtual void DataReady(ManagedDispatchURLRequestJob* job) = 0;

  // Tells us the job has finished. Can be called from any thread.
  virtual void JobDeleted(ManagedDispatchURLRequestJob* job) = 0;

  // Tells us a navigation has been requested. Can be called from any thread.
  virtual void NavigationRequested(
      std::unique_ptr<NavigationRequest> navigation_request) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(URLRequestDispatcher);
};

}  // namespace headless

#endif  // HEADLESS_PUBLIC_UTIL_URL_REQUEST_DISPATCHER_H_
