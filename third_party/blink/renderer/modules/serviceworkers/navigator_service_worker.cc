// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/serviceworkers/navigator_service_worker.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/modules/serviceworkers/service_worker_container.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"

namespace blink {

NavigatorServiceWorker::NavigatorServiceWorker(Navigator& navigator) {}

NavigatorServiceWorker* NavigatorServiceWorker::From(Document& document) {
  if (!document.GetFrame() || !document.GetFrame()->DomWindow())
    return nullptr;
  Navigator& navigator = *document.GetFrame()->DomWindow()->navigator();
  return &From(navigator);
}

NavigatorServiceWorker& NavigatorServiceWorker::From(Navigator& navigator) {
  NavigatorServiceWorker* supplement = ToNavigatorServiceWorker(navigator);
  if (!supplement) {
    supplement = new NavigatorServiceWorker(navigator);
    ProvideTo(navigator, supplement);
  }
  if (navigator.GetFrame() && navigator.GetFrame()
                                  ->GetSecurityContext()
                                  ->GetSecurityOrigin()
                                  ->CanAccessServiceWorkers()) {
    // Ensure ServiceWorkerContainer. It can be cleared regardless of
    // |supplement|. See comments in NavigatorServiceWorker::serviceWorker() for
    // details.
    supplement->serviceWorker(navigator.GetFrame(), ASSERT_NO_EXCEPTION);
  }
  return *supplement;
}

NavigatorServiceWorker* NavigatorServiceWorker::ToNavigatorServiceWorker(
    Navigator& navigator) {
  return Supplement<Navigator>::From<NavigatorServiceWorker>(navigator);
}

const char NavigatorServiceWorker::kSupplementName[] = "NavigatorServiceWorker";

ServiceWorkerContainer* NavigatorServiceWorker::serviceWorker(
    ScriptState* script_state,
    Navigator& navigator,
    ExceptionState& exception_state) {
  ExecutionContext* execution_context = ExecutionContext::From(script_state);
  DCHECK(!navigator.GetFrame() ||
         execution_context->GetSecurityOrigin()->CanAccess(
             navigator.GetFrame()->GetSecurityContext()->GetSecurityOrigin()));
  return NavigatorServiceWorker::From(navigator).serviceWorker(
      navigator.GetFrame(), exception_state);
}

ServiceWorkerContainer* NavigatorServiceWorker::serviceWorker(
    ScriptState* script_state,
    Navigator& navigator,
    String& error_message) {
  ExecutionContext* execution_context = ExecutionContext::From(script_state);
  DCHECK(!navigator.GetFrame() ||
         execution_context->GetSecurityOrigin()->CanAccess(
             navigator.GetFrame()->GetSecurityContext()->GetSecurityOrigin()));
  return NavigatorServiceWorker::From(navigator).serviceWorker(
      navigator.GetFrame(), error_message);
}

ServiceWorkerContainer* NavigatorServiceWorker::serviceWorker(
    LocalFrame* frame,
    ExceptionState& exception_state) {
  String error_message;
  ServiceWorkerContainer* result = serviceWorker(frame, error_message);
  if (!error_message.IsEmpty()) {
    DCHECK(!result);
    exception_state.ThrowSecurityError(error_message);
  }
  return result;
}

ServiceWorkerContainer* NavigatorServiceWorker::serviceWorker(
    LocalFrame* frame,
    String& error_message) {
  if (frame && !frame->GetSecurityContext()
                    ->GetSecurityOrigin()
                    ->CanAccessServiceWorkers()) {
    if (frame->GetSecurityContext()->IsSandboxed(kSandboxOrigin)) {
      error_message =
          "Service worker is disabled because the context is sandboxed and "
          "lacks the 'allow-same-origin' flag.";
    } else {
      error_message =
          "Access to service workers is denied in this document origin.";
    }
    return nullptr;
  } else if (frame &&
             frame->GetSecurityContext()->GetSecurityOrigin()->IsLocal()) {
    UseCounter::Count(frame, WebFeature::kFileAccessedServiceWorker);
  }
  if (!service_worker_ && frame) {
    // We need to create a new ServiceWorkerContainer when the frame
    // navigates to a new document. In practice, this happens only when the
    // frame navigates from the initial empty page to a new same-origin page.
    DCHECK(frame->DomWindow());
    service_worker_ = ServiceWorkerContainer::Create(
        frame->DomWindow()->GetExecutionContext(), this);
  }
  return service_worker_.Get();
}

void NavigatorServiceWorker::ClearServiceWorker() {
  service_worker_ = nullptr;
}

void NavigatorServiceWorker::Trace(blink::Visitor* visitor) {
  visitor->Trace(service_worker_);
  Supplement<Navigator>::Trace(visitor);
}

}  // namespace blink
