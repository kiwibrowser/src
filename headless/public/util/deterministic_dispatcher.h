// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_PUBLIC_UTIL_DETERMINISTIC_DISPATCHER_H_
#define HEADLESS_PUBLIC_UTIL_DETERMINISTIC_DISPATCHER_H_

#include <map>

#include "base/containers/circular_deque.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/lock.h"
#include "headless/public/headless_export.h"
#include "headless/public/util/url_request_dispatcher.h"
#include "net/base/net_errors.h"

namespace headless {

class ManagedDispatchURLRequestJob;

// The purpose of this class is to queue up navigations and calls to
// OnHeadersComplete / OnStartError and dispatch them in order of creation. This
// helps make renders deterministic at the cost of slower page loads.
class HEADLESS_EXPORT DeterministicDispatcher : public URLRequestDispatcher {
 public:
  explicit DeterministicDispatcher(
      scoped_refptr<base::SingleThreadTaskRunner> io_thread_task_runner);

  ~DeterministicDispatcher() override;

  // UrlRequestDispatcher implementation:
  void JobCreated(ManagedDispatchURLRequestJob* job) override;
  void JobKilled(ManagedDispatchURLRequestJob* job) override;
  void JobFailed(ManagedDispatchURLRequestJob* job, net::Error error) override;
  void DataReady(ManagedDispatchURLRequestJob* job) override;
  void JobDeleted(ManagedDispatchURLRequestJob* job) override;
  void NavigationRequested(
      std::unique_ptr<NavigationRequest> navigation_request) override;

 private:
  void MaybeDispatchNavigationJobLocked();
  void MaybeDispatchJobLocked();
  void MaybeDispatchJobOnIOThreadTask();
  void NavigationDoneTask();

  scoped_refptr<base::SingleThreadTaskRunner> io_thread_task_runner_;

  // Protects all members below.
  base::Lock lock_;

  // TODO(alexclarke): Use std::variant when c++17 is allowed in chromium.
  struct Request {
    Request();
    explicit Request(ManagedDispatchURLRequestJob* url_request);
    explicit Request(std::unique_ptr<NavigationRequest> navigation_request);
    Request(Request&&);
    ~Request();

    Request& operator=(Request&& other);

    ManagedDispatchURLRequestJob* url_request = nullptr;  // NOT OWNED
    std::unique_ptr<NavigationRequest> navigation_request;
  };

  base::circular_deque<Request> pending_requests_;

  using StatusMap = std::map<ManagedDispatchURLRequestJob*, net::Error>;
  StatusMap ready_status_map_;

  // Whether or not a MaybeDispatchJobOnIoThreadTask has been posted on the
  // |io_thread_task_runner_|
  bool dispatch_pending_;
  bool navigation_in_progress_;

  base::WeakPtrFactory<DeterministicDispatcher> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(DeterministicDispatcher);
};

}  // namespace headless

#endif  // HEADLESS_PUBLIC_UTIL_DETERMINISTIC_DISPATCHER_H_
