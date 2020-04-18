// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_SYNC_SYNC_MANAGER_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_SYNC_SYNC_MANAGER_H_

#include "third_party/blink/public/platform/modules/background_sync/background_sync.mojom-blink.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class ScriptPromise;
class ScriptPromiseResolver;
class ScriptState;
class ServiceWorkerRegistration;

class SyncManager final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static SyncManager* Create(ServiceWorkerRegistration* registration) {
    return new SyncManager(registration);
  }

  ScriptPromise registerFunction(ScriptState*, const String& tag);
  ScriptPromise getTags(ScriptState*);

  void Trace(blink::Visitor*) override;

  enum { kUnregisteredSyncID = -1 };

 private:
  explicit SyncManager(ServiceWorkerRegistration*);

  // Returns an initialized BackgroundSyncServicePtr. A connection with the
  // the browser's BackgroundSyncService is created the first time this method
  // is called.
  const mojom::blink::BackgroundSyncServicePtr& GetBackgroundSyncServicePtr();

  // Callbacks
  static void RegisterCallback(ScriptPromiseResolver*,
                               mojom::blink::BackgroundSyncError,
                               mojom::blink::SyncRegistrationPtr options);
  static void GetRegistrationsCallback(
      ScriptPromiseResolver*,
      mojom::blink::BackgroundSyncError,
      WTF::Vector<mojom::blink::SyncRegistrationPtr> registrations);

  Member<ServiceWorkerRegistration> registration_;
  mojom::blink::BackgroundSyncServicePtr background_sync_service_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_SYNC_SYNC_MANAGER_H_
