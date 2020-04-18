// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_PUBLIC_UTIL_MANAGED_DISPATCH_URL_REQUEST_JOB_H_
#define HEADLESS_PUBLIC_UTIL_MANAGED_DISPATCH_URL_REQUEST_JOB_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "headless/public/headless_export.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_job.h"

namespace headless {
class URLRequestDispatcher;

// A ManagedDispatchURLRequestJob exists to allow a URLRequestDispatcher control
// the order in which a set of fetches complete. Typically this is done to make
// fetching deterministic. NOTE URLRequestDispatcher sub classes use
// OnHeadersComplete and OnStartError to drive the URLRequestJob.
class HEADLESS_EXPORT ManagedDispatchURLRequestJob : public net::URLRequestJob {
 public:
  ManagedDispatchURLRequestJob(net::URLRequest* request,
                               net::NetworkDelegate* network_delegate,
                               URLRequestDispatcher* url_request_dispatcher);

  ~ManagedDispatchURLRequestJob() override;

  // Tells the net::URLRequestJob that the data has been fetched and is ready to
  // be consumed.
  // Virtual for FakeManagedDispatchURLRequestJob.
  virtual void OnHeadersComplete();

  // Tells the net::URLRequestJob that the fetch failed.
  // Virtual for FakeManagedDispatchURLRequestJob.
  virtual void OnStartError(net::Error error);

  base::WeakPtr<ManagedDispatchURLRequestJob> GetWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

 protected:
  // net::URLRequestJob implementation:
  void Kill() override;

  // Tell the dispatcher the request failed.
  virtual void DispatchStartError(net::Error error);

  // Tell the dispatcher the data is ready for the net stack to consume.
  virtual void DispatchHeadersComplete();

  URLRequestDispatcher* url_request_dispatcher_;  // Not owned.

 private:
  // Derived classes should not use NotifyHeadersComplete or NotifyStartError
  // directly. Instead they should use DispatchHeadersComplete or
  // DispatchStartError.
  using URLRequestJob::NotifyHeadersComplete;
  using URLRequestJob::NotifyStartError;

  base::WeakPtrFactory<ManagedDispatchURLRequestJob> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ManagedDispatchURLRequestJob);
};

}  // namespace headless

#endif  // HEADLESS_PUBLIC_UTIL_MANAGED_DISPATCH_URL_REQUEST_JOB_H_
