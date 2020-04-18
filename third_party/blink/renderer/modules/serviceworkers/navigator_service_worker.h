// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_NAVIGATOR_SERVICE_WORKER_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_NAVIGATOR_SERVICE_WORKER_H_

#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/supplementable.h"

namespace blink {

class Document;
class ExceptionState;
class Navigator;
class ScriptState;
class ServiceWorkerContainer;

class MODULES_EXPORT NavigatorServiceWorker final
    : public GarbageCollected<NavigatorServiceWorker>,
      public Supplement<Navigator> {
  USING_GARBAGE_COLLECTED_MIXIN(NavigatorServiceWorker);

 public:
  static const char kSupplementName[];

  static NavigatorServiceWorker* From(Document&);
  static NavigatorServiceWorker& From(Navigator&);
  static NavigatorServiceWorker* ToNavigatorServiceWorker(Navigator&);
  static ServiceWorkerContainer* serviceWorker(ScriptState*,
                                               Navigator&,
                                               ExceptionState&);
  static ServiceWorkerContainer* serviceWorker(ScriptState*,
                                               Navigator&,
                                               String& error_message);
  void ClearServiceWorker();

  void Trace(blink::Visitor*) override;

 private:
  explicit NavigatorServiceWorker(Navigator&);
  ServiceWorkerContainer* serviceWorker(LocalFrame*, ExceptionState&);
  ServiceWorkerContainer* serviceWorker(LocalFrame*, String& error_message);

  Member<ServiceWorkerContainer> service_worker_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_NAVIGATOR_SERVICE_WORKER_H_
