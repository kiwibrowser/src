// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/installedapp/navigator_installed_app.h"

#include <memory>

#include "third_party/blink/public/platform/modules/installedapp/web_related_application.h"
#include "third_party/blink/renderer/bindings/core/v8/callback_promise_adapter.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/modules/installedapp/installed_app_controller.h"
#include "third_party/blink/renderer/modules/installedapp/related_application.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"

namespace blink {

NavigatorInstalledApp::NavigatorInstalledApp(Navigator& navigator)
    : Supplement<Navigator>(navigator) {}

NavigatorInstalledApp* NavigatorInstalledApp::From(Document& document) {
  if (!document.GetFrame() || !document.GetFrame()->DomWindow())
    return nullptr;
  Navigator& navigator = *document.GetFrame()->DomWindow()->navigator();
  return &From(navigator);
}

NavigatorInstalledApp& NavigatorInstalledApp::From(Navigator& navigator) {
  NavigatorInstalledApp* supplement =
      Supplement<Navigator>::From<NavigatorInstalledApp>(navigator);
  if (!supplement) {
    supplement = new NavigatorInstalledApp(navigator);
    ProvideTo(navigator, supplement);
  }
  return *supplement;
}

ScriptPromise NavigatorInstalledApp::getInstalledRelatedApps(
    ScriptState* script_state,
    Navigator& navigator) {
  // [SecureContext] from the IDL ensures this.
  DCHECK(ExecutionContext::From(script_state)->IsSecureContext());
  return NavigatorInstalledApp::From(navigator).getInstalledRelatedApps(
      script_state);
}

class RelatedAppArray {
  STATIC_ONLY(RelatedAppArray);

 public:
  using WebType = const WebVector<WebRelatedApplication>&;

  static HeapVector<Member<RelatedApplication>> Take(
      ScriptPromiseResolver*,
      const WebVector<WebRelatedApplication>& web_info) {
    HeapVector<Member<RelatedApplication>> applications;
    for (const auto& web_application : web_info)
      applications.push_back(RelatedApplication::Create(
          web_application.platform, web_application.url, web_application.id));
    return applications;
  }
};

ScriptPromise NavigatorInstalledApp::getInstalledRelatedApps(
    ScriptState* script_state) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  InstalledAppController* app_controller = Controller();
  if (!app_controller) {  // If the associated frame is detached
    DOMException* exception = DOMException::Create(
        kInvalidStateError,
        "The object is no longer associated to a document.");
    resolver->Reject(exception);
    return promise;
  }

  if (!app_controller->GetSupplementable()->IsMainFrame()) {
    DOMException* exception =
        DOMException::Create(kInvalidStateError,
                             "getInstalledRelatedApps() is only supported in "
                             "top-level browsing contexts.");
    resolver->Reject(exception);
    return promise;
  }

  app_controller->GetInstalledRelatedApps(
      std::make_unique<CallbackPromiseAdapter<RelatedAppArray, void>>(
          resolver));
  return promise;
}

InstalledAppController* NavigatorInstalledApp::Controller() {
  if (!GetSupplementable()->GetFrame())
    return nullptr;

  return InstalledAppController::From(*GetSupplementable()->GetFrame());
}

const char NavigatorInstalledApp::kSupplementName[] = "NavigatorInstalledApp";

void NavigatorInstalledApp::Trace(blink::Visitor* visitor) {
  Supplement<Navigator>::Trace(visitor);
}

}  // namespace blink
