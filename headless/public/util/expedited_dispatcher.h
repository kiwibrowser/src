// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_PUBLIC_UTIL_EXPEDITED_DISPATCHER_H_
#define HEADLESS_PUBLIC_UTIL_EXPEDITED_DISPATCHER_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "headless/public/headless_export.h"
#include "headless/public/util/url_request_dispatcher.h"
#include "net/base/net_errors.h"

namespace headless {

class ManagedDispatchURLRequestJob;

// This class dispatches calls to OnHeadersComplete and OnStartError
// immediately sacrificing determinism for performance.
class HEADLESS_EXPORT ExpeditedDispatcher : public URLRequestDispatcher {
 public:
  explicit ExpeditedDispatcher(
      scoped_refptr<base::SingleThreadTaskRunner> io_thread_task_runner);
  ~ExpeditedDispatcher() override;

  // UrlRequestDispatcher implementation:
  void JobCreated(ManagedDispatchURLRequestJob* job) override;
  void JobKilled(ManagedDispatchURLRequestJob* job) override;
  void JobFailed(ManagedDispatchURLRequestJob* job, net::Error error) override;
  void DataReady(ManagedDispatchURLRequestJob* job) override;
  void JobDeleted(ManagedDispatchURLRequestJob* job) override;
  void NavigationRequested(
      std::unique_ptr<NavigationRequest> navigation_request) override;

 protected:
  scoped_refptr<base::SingleThreadTaskRunner> io_thread_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(ExpeditedDispatcher);
};

}  // namespace headless

#endif  // HEADLESS_PUBLIC_UTIL_EXPEDITED_DISPATCHER_H_
