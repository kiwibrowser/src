// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/presentation/presentation_request.h"

#include <memory>

#include "third_party/blink/renderer/bindings/core/v8/callback_promise_adapter.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/deprecation.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/loader/mixed_content_checker.h"
#include "third_party/blink/renderer/modules/event_target_modules.h"
#include "third_party/blink/renderer/modules/presentation/presentation_availability.h"
#include "third_party/blink/renderer/modules/presentation/presentation_availability_callbacks.h"
#include "third_party/blink/renderer/modules/presentation/presentation_availability_state.h"
#include "third_party/blink/renderer/modules/presentation/presentation_connection.h"
#include "third_party/blink/renderer/modules/presentation/presentation_connection_callbacks.h"
#include "third_party/blink/renderer/modules/presentation/presentation_controller.h"
#include "third_party/blink/renderer/modules/presentation/presentation_error.h"

namespace blink {

namespace {

Settings* GetSettings(ExecutionContext* execution_context) {
  DCHECK(execution_context);

  Document* document = ToDocument(execution_context);
  return document->GetSettings();
}

}  // anonymous namespace

// static
PresentationRequest* PresentationRequest::Create(
    ExecutionContext* execution_context,
    const String& url,
    ExceptionState& exception_state) {
  Vector<String> urls(1);
  urls[0] = url;
  return Create(execution_context, urls, exception_state);
}

PresentationRequest* PresentationRequest::Create(
    ExecutionContext* execution_context,
    const Vector<String>& urls,
    ExceptionState& exception_state) {
  if (ToDocument(execution_context)
          ->IsSandboxed(kSandboxPresentationController)) {
    exception_state.ThrowSecurityError(
        "The document is sandboxed and lacks the 'allow-presentation' flag.");
    return nullptr;
  }

  if (urls.IsEmpty()) {
    exception_state.ThrowDOMException(kNotSupportedError,
                                      "Do not support empty sequence of URLs.");
    return nullptr;
  }

  Vector<KURL> parsed_urls(urls.size());
  for (size_t i = 0; i < urls.size(); ++i) {
    const KURL& parsed_url = KURL(execution_context->Url(), urls[i]);

    if (!parsed_url.IsValid()) {
      exception_state.ThrowDOMException(
          kSyntaxError, "'" + urls[i] + "' can't be resolved to a valid URL.");
      return nullptr;
    }

    if (parsed_url.ProtocolIsInHTTPFamily() &&
        MixedContentChecker::IsMixedContent(
            execution_context->GetSecurityOrigin(), parsed_url)) {
      exception_state.ThrowSecurityError(
          "Presentation of an insecure document [" + urls[i] +
          "] is prohibited from a secure context.");
      return nullptr;
    }

    parsed_urls[i] = parsed_url;
  }
  return new PresentationRequest(execution_context, parsed_urls);
}

const AtomicString& PresentationRequest::InterfaceName() const {
  return EventTargetNames::PresentationRequest;
}

ExecutionContext* PresentationRequest::GetExecutionContext() const {
  return ContextClient::GetExecutionContext();
}

void PresentationRequest::AddedEventListener(
    const AtomicString& event_type,
    RegisteredEventListener& registered_listener) {
  EventTargetWithInlineData::AddedEventListener(event_type,
                                                registered_listener);
  if (event_type == EventTypeNames::connectionavailable) {
    UseCounter::Count(
        GetExecutionContext(),
        WebFeature::kPresentationRequestConnectionAvailableEventListener);
  }
}

bool PresentationRequest::HasPendingActivity() const {
  // Prevents garbage collecting of this object when not hold by another
  // object but still has listeners registered.
  if (!GetExecutionContext())
    return false;

  if (HasEventListeners())
    return true;

  return availability_property_ && availability_property_->GetState() ==
                                       ScriptPromisePropertyBase::kPending;
}

// static
void PresentationRequest::RecordStartOriginTypeAccess(
    ExecutionContext& execution_context) {
  if (execution_context.IsSecureContext()) {
    UseCounter::Count(&execution_context,
                      WebFeature::kPresentationRequestStartSecureOrigin);
  } else {
    Deprecation::CountDeprecation(
        &execution_context,
        WebFeature::kPresentationRequestStartInsecureOrigin);
  }
}

ScriptPromise PresentationRequest::start(ScriptState* script_state) {
  ExecutionContext* execution_context = GetExecutionContext();
  Settings* context_settings = GetSettings(execution_context);
  Document* doc = ToDocumentOrNull(execution_context);

  bool is_user_gesture_required =
      !context_settings ||
      context_settings->GetPresentationRequiresUserGesture();

  if (is_user_gesture_required &&
      !Frame::HasTransientUserActivation(doc ? doc->GetFrame() : nullptr))
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(
            kInvalidAccessError,
            "PresentationRequest::start() requires user gesture."));

  PresentationController* controller =
      PresentationController::FromContext(GetExecutionContext());
  if (!controller)
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(
            kInvalidStateError,
            "The PresentationRequest is no longer associated to a frame."));

  RecordStartOriginTypeAccess(*execution_context);
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);

  controller->GetPresentationService()->StartPresentation(
      urls_,
      WTF::Bind(
          &PresentationConnectionCallbacks::HandlePresentationResponse,
          std::make_unique<PresentationConnectionCallbacks>(resolver, this)));
  return resolver->Promise();
}

ScriptPromise PresentationRequest::reconnect(ScriptState* script_state,
                                             const String& id) {
  PresentationController* controller =
      PresentationController::FromContext(GetExecutionContext());
  if (!controller)
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(
            kInvalidStateError,
            "The PresentationRequest is no longer associated to a frame."));

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);

  ControllerPresentationConnection* existing_connection =
      controller->FindExistingConnection(urls_, id);
  if (existing_connection) {
    controller->GetPresentationService()->ReconnectPresentation(
        urls_, id,
        WTF::Bind(&PresentationConnectionCallbacks::HandlePresentationResponse,
                  std::make_unique<PresentationConnectionCallbacks>(
                      resolver, existing_connection)));
  } else {
    controller->GetPresentationService()->ReconnectPresentation(
        urls_, id,
        WTF::Bind(
            &PresentationConnectionCallbacks::HandlePresentationResponse,
            std::make_unique<PresentationConnectionCallbacks>(resolver, this)));
  }
  return resolver->Promise();
}

ScriptPromise PresentationRequest::getAvailability(ScriptState* script_state) {
  PresentationController* controller =
      PresentationController::FromContext(GetExecutionContext());
  if (!controller)
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(
            kInvalidStateError,
            "The PresentationRequest is no longer associated to a frame."));

  if (!availability_property_) {
    availability_property_ = new PresentationAvailabilityProperty(
        ExecutionContext::From(script_state), this,
        PresentationAvailabilityProperty::kReady);

    controller->GetAvailabilityState()->RequestAvailability(
        urls_, std::make_unique<PresentationAvailabilityCallbacks>(
                   availability_property_, urls_));
  }
  return availability_property_->Promise(script_state->World());
}

const Vector<KURL>& PresentationRequest::Urls() const {
  return urls_;
}

void PresentationRequest::Trace(blink::Visitor* visitor) {
  visitor->Trace(availability_property_);
  EventTargetWithInlineData::Trace(visitor);
  ContextClient::Trace(visitor);
}

PresentationRequest::PresentationRequest(ExecutionContext* execution_context,
                                         const Vector<KURL>& urls)
    : ContextClient(execution_context), urls_(urls) {
  RecordConstructorOriginTypeAccess(*execution_context);
}

// static
void PresentationRequest::RecordConstructorOriginTypeAccess(
    ExecutionContext& execution_context) {
  if (execution_context.IsSecureContext()) {
    UseCounter::Count(&execution_context,
                      WebFeature::kPresentationRequestSecureOrigin);
  } else {
    UseCounter::Count(&execution_context,
                      WebFeature::kPresentationRequestInsecureOrigin);
  }
}

}  // namespace blink
