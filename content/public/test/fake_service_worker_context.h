// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_TEST_FAKE_SERVICE_WORKER_CONTEXT_H_
#define CONTENT_PUBLIC_TEST_FAKE_SERVICE_WORKER_CONTEXT_H_

#include <string>

#include "base/callback_forward.h"
#include "content/public/browser/service_worker_context.h"

class GURL;

namespace content {

class ServiceWorkerContextObserver;

// Fake implementation of ServiceWorkerContext.
//
// Currently it only implements StartServiceWorkerForNavigationHint. Add
// what you need.
class FakeServiceWorkerContext : public ServiceWorkerContext {
 public:
  FakeServiceWorkerContext();
  ~FakeServiceWorkerContext() override;

  void AddObserver(ServiceWorkerContextObserver* observer) override;
  void RemoveObserver(ServiceWorkerContextObserver* observer) override;
  void RegisterServiceWorker(
      const GURL& script_url,
      const blink::mojom::ServiceWorkerRegistrationOptions& options,
      ResultCallback callback) override;
  void UnregisterServiceWorker(const GURL& pattern,
                               ResultCallback callback) override;
  bool StartingExternalRequest(int64_t service_worker_version_id,
                               const std::string& request_uuid) override;
  bool FinishedExternalRequest(int64_t service_worker_version_id,
                               const std::string& request_uuid) override;
  void CountExternalRequestsForTest(
      const GURL& url,
      CountExternalRequestsCallback callback) override;
  void GetAllOriginsInfo(GetUsageInfoCallback callback) override;
  void DeleteForOrigin(const GURL& origin, ResultCallback callback) override;
  void CheckHasServiceWorker(const GURL& url,
                             const GURL& other_url,
                             CheckHasServiceWorkerCallback callback) override;
  void ClearAllServiceWorkersForTest(base::OnceClosure) override;
  void StartActiveWorkerForPattern(
      const GURL& pattern,
      ServiceWorkerContext::StartActiveWorkerCallback info_callback,
      base::OnceClosure failure_callback) override;
  void StartServiceWorkerForNavigationHint(
      const GURL& document_url,
      StartServiceWorkerForNavigationHintCallback callback) override;
  void StopAllServiceWorkersForOrigin(const GURL& origin) override;
  void StopAllServiceWorkers(base::OnceClosure callback) override;

  bool start_service_worker_for_navigation_hint_called() {
    return start_service_worker_for_navigation_hint_called_;
  }

 private:
  bool start_service_worker_for_navigation_hint_called_ = false;

  DISALLOW_COPY_AND_ASSIGN(FakeServiceWorkerContext);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_TEST_FAKE_SERVICE_WORKER_CONTEXT_H_
