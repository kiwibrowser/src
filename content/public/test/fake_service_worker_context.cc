// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/fake_service_worker_context.h"

#include "base/callback.h"
#include "base/logging.h"

namespace content {

FakeServiceWorkerContext::FakeServiceWorkerContext() {}
FakeServiceWorkerContext::~FakeServiceWorkerContext() {}

void FakeServiceWorkerContext::AddObserver(
    ServiceWorkerContextObserver* observer) {
  NOTREACHED();
}
void FakeServiceWorkerContext::RemoveObserver(
    ServiceWorkerContextObserver* observer) {
  NOTREACHED();
}
void FakeServiceWorkerContext::RegisterServiceWorker(
    const GURL& script_url,
    const blink::mojom::ServiceWorkerRegistrationOptions& options,
    ResultCallback callback) {
  NOTREACHED();
}
void FakeServiceWorkerContext::UnregisterServiceWorker(
    const GURL& pattern,
    ResultCallback callback) {
  NOTREACHED();
}
bool FakeServiceWorkerContext::StartingExternalRequest(
    int64_t service_worker_version_id,
    const std::string& request_uuid) {
  NOTREACHED();
  return false;
}
bool FakeServiceWorkerContext::FinishedExternalRequest(
    int64_t service_worker_version_id,
    const std::string& request_uuid) {
  NOTREACHED();
  return false;
}
void FakeServiceWorkerContext::CountExternalRequestsForTest(
    const GURL& url,
    CountExternalRequestsCallback callback) {
  NOTREACHED();
}
void FakeServiceWorkerContext::GetAllOriginsInfo(
    GetUsageInfoCallback callback) {
  NOTREACHED();
}
void FakeServiceWorkerContext::DeleteForOrigin(const GURL& origin,
                                               ResultCallback callback) {
  NOTREACHED();
}
void FakeServiceWorkerContext::CheckHasServiceWorker(
    const GURL& url,
    const GURL& other_url,
    CheckHasServiceWorkerCallback callback) {
  NOTREACHED();
}
void FakeServiceWorkerContext::ClearAllServiceWorkersForTest(
    base::OnceClosure) {
  NOTREACHED();
}
void FakeServiceWorkerContext::StartActiveWorkerForPattern(
    const GURL& pattern,
    ServiceWorkerContext::StartActiveWorkerCallback info_callback,
    base::OnceClosure failure_callback) {
  NOTREACHED();
}
void FakeServiceWorkerContext::StartServiceWorkerForNavigationHint(
    const GURL& document_url,
    StartServiceWorkerForNavigationHintCallback callback) {
  start_service_worker_for_navigation_hint_called_ = true;
}
void FakeServiceWorkerContext::StopAllServiceWorkersForOrigin(
    const GURL& origin) {
  NOTREACHED();
}
void FakeServiceWorkerContext::StopAllServiceWorkers(base::OnceClosure) {
  NOTREACHED();
}

}  // namespace content
