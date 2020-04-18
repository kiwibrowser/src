// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/background_fetch/service_worker_registration_background_fetch.h"

#include "third_party/blink/renderer/modules/background_fetch/background_fetch_manager.h"

namespace blink {

ServiceWorkerRegistrationBackgroundFetch::
    ServiceWorkerRegistrationBackgroundFetch(
        ServiceWorkerRegistration* registration)
    : registration_(registration) {}

ServiceWorkerRegistrationBackgroundFetch::
    ~ServiceWorkerRegistrationBackgroundFetch() = default;

const char ServiceWorkerRegistrationBackgroundFetch::kSupplementName[] =
    "ServiceWorkerRegistrationBackgroundFetch";

ServiceWorkerRegistrationBackgroundFetch&
ServiceWorkerRegistrationBackgroundFetch::From(
    ServiceWorkerRegistration& registration) {
  ServiceWorkerRegistrationBackgroundFetch* supplement =
      Supplement<ServiceWorkerRegistration>::From<
          ServiceWorkerRegistrationBackgroundFetch>(registration);

  if (!supplement) {
    supplement = new ServiceWorkerRegistrationBackgroundFetch(&registration);
    ProvideTo(registration, supplement);
  }

  return *supplement;
}

BackgroundFetchManager*
ServiceWorkerRegistrationBackgroundFetch::backgroundFetch(
    ServiceWorkerRegistration& registration) {
  return ServiceWorkerRegistrationBackgroundFetch::From(registration)
      .backgroundFetch();
}

BackgroundFetchManager*
ServiceWorkerRegistrationBackgroundFetch::backgroundFetch() {
  if (!background_fetch_manager_)
    background_fetch_manager_ = BackgroundFetchManager::Create(registration_);

  return background_fetch_manager_.Get();
}

void ServiceWorkerRegistrationBackgroundFetch::Trace(blink::Visitor* visitor) {
  visitor->Trace(registration_);
  visitor->Trace(background_fetch_manager_);
  Supplement<ServiceWorkerRegistration>::Trace(visitor);
}

}  // namespace blink
