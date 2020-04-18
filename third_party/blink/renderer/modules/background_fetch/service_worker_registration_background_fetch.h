// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_SERVICE_WORKER_REGISTRATION_BACKGROUND_FETCH_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_SERVICE_WORKER_REGISTRATION_BACKGROUND_FETCH_H_

#include "third_party/blink/renderer/modules/serviceworkers/service_worker_registration.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/supplementable.h"

namespace blink {

class BackgroundFetchManager;

class ServiceWorkerRegistrationBackgroundFetch final
    : public GarbageCollectedFinalized<
          ServiceWorkerRegistrationBackgroundFetch>,
      public Supplement<ServiceWorkerRegistration> {
  USING_GARBAGE_COLLECTED_MIXIN(ServiceWorkerRegistrationBackgroundFetch);
  WTF_MAKE_NONCOPYABLE(ServiceWorkerRegistrationBackgroundFetch);

 public:
  static const char kSupplementName[];

  virtual ~ServiceWorkerRegistrationBackgroundFetch();

  static ServiceWorkerRegistrationBackgroundFetch& From(
      ServiceWorkerRegistration& registration);

  static BackgroundFetchManager* backgroundFetch(
      ServiceWorkerRegistration& registration);
  BackgroundFetchManager* backgroundFetch();

  void Trace(blink::Visitor* visitor) override;

 private:
  explicit ServiceWorkerRegistrationBackgroundFetch(
      ServiceWorkerRegistration* registration);

  Member<ServiceWorkerRegistration> registration_;
  Member<BackgroundFetchManager> background_fetch_manager_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_SERVICE_WORKER_REGISTRATION_BACKGROUND_FETCH_H_
