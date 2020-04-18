/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/modules/notifications/notification.h"

#include "base/unguessable_token.h"
#include "third_party/blink/public/platform/modules/notifications/web_notification_action.h"
#include "third_party/blink/public/platform/modules/notifications/web_notification_constants.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/bindings/core/v8/serialization/serialized_script_value_factory.h"
#include "third_party/blink/renderer/bindings/core/v8/source_location.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_notification_action.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/dom/scoped_window_focus_allowed_indicator.h"
#include "third_party/blink/renderer/core/dom/user_gesture_indicator.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/deprecation.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/performance_monitor.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/modules/notifications/notification_action.h"
#include "third_party/blink/renderer/modules/notifications/notification_data.h"
#include "third_party/blink/renderer/modules/notifications/notification_manager.h"
#include "third_party/blink/renderer/modules/notifications/notification_options.h"
#include "third_party/blink/renderer/modules/notifications/notification_resources_loader.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/instrumentation/resource_coordinator/frame_resource_coordinator.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"

namespace blink {

Notification* Notification::Create(ExecutionContext* context,
                                   const String& title,
                                   const NotificationOptions& options,
                                   ExceptionState& exception_state) {
  // The Notification constructor may be disabled through a runtime feature when
  // the platform does not support non-persistent notifications.
  if (!RuntimeEnabledFeatures::NotificationConstructorEnabled()) {
    exception_state.ThrowTypeError(
        "Illegal constructor. Use ServiceWorkerRegistration.showNotification() "
        "instead.");
    return nullptr;
  }

  // The Notification constructor may not be used in Service Worker contexts.
  if (context->IsServiceWorkerGlobalScope()) {
    exception_state.ThrowTypeError("Illegal constructor.");
    return nullptr;
  }

  if (!options.actions().IsEmpty()) {
    exception_state.ThrowTypeError(
        "Actions are only supported for persistent notifications shown using "
        "ServiceWorkerRegistration.showNotification().");
    return nullptr;
  }

  if (context->IsSecureContext()) {
    UseCounter::Count(context, WebFeature::kNotificationSecureOrigin);
    if (context->IsDocument()) {
      UseCounter::CountCrossOriginIframe(
          *ToDocument(context), WebFeature::kNotificationAPISecureOriginIframe);
    }
  } else {
    Deprecation::CountDeprecation(context,
                                  WebFeature::kNotificationInsecureOrigin);
    if (context->IsDocument()) {
      Deprecation::CountDeprecationCrossOriginIframe(
          *ToDocument(context),
          WebFeature::kNotificationAPIInsecureOriginIframe);
    }
  }

  WebNotificationData data =
      CreateWebNotificationData(context, title, options, exception_state);
  if (exception_state.HadException())
    return nullptr;

  if (context->IsContextDestroyed()) {
    exception_state.ThrowTypeError("Illegal invocation.");
    return nullptr;
  }

  Notification* notification =
      new Notification(context, Type::kNonPersistent, data);

  // TODO(https://crbug.com/595685): Make |token| a constructor parameter
  // once persistent notifications have been mojofied too.
  if (data.tag.IsNull() || data.tag.IsEmpty()) {
    auto unguessable_token = base::UnguessableToken::Create();
    notification->SetToken(unguessable_token.ToString().c_str());
  } else {
    notification->SetToken(data.tag);
  }

  notification->SchedulePrepareShow();

  Document* document = context->IsDocument() ? ToDocument(context) : nullptr;
  if (document && document->GetFrame()) {
    if (auto* frame_resource_coordinator =
            document->GetFrame()->GetFrameResourceCoordinator()) {
      frame_resource_coordinator->OnNonPersistentNotificationCreated();
    }
  }

  return notification;
}

Notification* Notification::Create(ExecutionContext* context,
                                   const String& notification_id,
                                   const WebNotificationData& data,
                                   bool showing) {
  Notification* notification =
      new Notification(context, Type::kPersistent, data);
  notification->SetState(showing ? State::kShowing : State::kClosed);
  notification->SetNotificationId(notification_id);
  return notification;
}

Notification::Notification(ExecutionContext* context,
                           Type type,
                           const WebNotificationData& data)
    : ContextLifecycleObserver(context),
      type_(type),
      state_(State::kLoading),
      data_(data),
      listener_binding_(this) {
}

Notification::~Notification() = default;

void Notification::SchedulePrepareShow() {
  DCHECK_EQ(state_, State::kLoading);
  DCHECK(!prepare_show_method_runner_);

  prepare_show_method_runner_ = AsyncMethodRunner<Notification>::Create(
      this, &Notification::PrepareShow,
      GetExecutionContext()->GetTaskRunner(TaskType::kMiscPlatformAPI));
  prepare_show_method_runner_->RunAsync();
}

void Notification::PrepareShow() {
  DCHECK_EQ(state_, State::kLoading);
  if (!GetExecutionContext()->IsSecureContext()) {
    DispatchErrorEvent();
    return;
  }

  if (NotificationManager::From(GetExecutionContext())->GetPermissionStatus() !=
      mojom::blink::PermissionStatus::GRANTED) {
    DispatchErrorEvent();
    return;
  }

  loader_ = new NotificationResourcesLoader(
      WTF::Bind(&Notification::DidLoadResources, WrapWeakPersistent(this)));
  loader_->Start(GetExecutionContext(), data_);
}

void Notification::DidLoadResources(NotificationResourcesLoader* loader) {
  DCHECK_EQ(loader, loader_.Get());

  mojom::blink::NonPersistentNotificationListenerPtr event_listener;

  listener_binding_.Bind(mojo::MakeRequest(&event_listener));

  NotificationManager::From(GetExecutionContext())
      ->DisplayNonPersistentNotification(token_, data_, loader->GetResources(),
                                         std::move(event_listener));

  loader_.Clear();

  state_ = State::kShowing;
}

void Notification::close() {
  if (state_ != State::kShowing)
    return;

  // Schedule the "close" event to be fired for non-persistent notifications.
  // Persistent notifications won't get such events for programmatic closes.
  if (type_ == Type::kNonPersistent) {
    state_ = State::kClosing;
    NotificationManager::From(GetExecutionContext())
        ->CloseNonPersistentNotification(token_);
    return;
  }

  state_ = State::kClosed;

  NotificationManager::From(GetExecutionContext())
      ->ClosePersistentNotification(notification_id_);
}

void Notification::OnShow() {
  DispatchEvent(Event::Create(EventTypeNames::show));
}

void Notification::OnClick() {
  ExecutionContext* context = GetExecutionContext();
  Document* document = context->IsDocument() ? ToDocument(context) : nullptr;
  std::unique_ptr<UserGestureIndicator> gesture_indicator =
      Frame::NotifyUserActivation(document ? document->GetFrame() : nullptr,
                                  UserGestureToken::kNewGesture);
  ScopedWindowFocusAllowedIndicator window_focus_allowed(GetExecutionContext());
  DispatchEvent(Event::Create(EventTypeNames::click));
}

void Notification::OnClose(OnCloseCallback completed_closure) {
  // The notification should be Showing if the user initiated the close, or it
  // should be Closing if the developer initiated the close.
  if (state_ == State::kShowing || state_ == State::kClosing) {
    state_ = State::kClosed;
    DispatchEvent(Event::Create(EventTypeNames::close));
  }
  std::move(completed_closure).Run();
}

void Notification::DispatchErrorEvent() {
  DispatchEvent(Event::Create(EventTypeNames::error));
}

String Notification::title() const {
  return data_.title;
}

String Notification::dir() const {
  switch (data_.direction) {
    case WebNotificationData::kDirectionLeftToRight:
      return "ltr";
    case WebNotificationData::kDirectionRightToLeft:
      return "rtl";
    case WebNotificationData::kDirectionAuto:
      return "auto";
  }

  NOTREACHED();
  return String();
}

String Notification::lang() const {
  return data_.lang;
}

String Notification::body() const {
  return data_.body;
}

String Notification::tag() const {
  return data_.tag;
}

String Notification::image() const {
  return data_.image.GetString();
}

String Notification::icon() const {
  return data_.icon.GetString();
}

String Notification::badge() const {
  return data_.badge.GetString();
}

NavigatorVibration::VibrationPattern Notification::vibrate() const {
  NavigatorVibration::VibrationPattern pattern;
  pattern.AppendRange(data_.vibrate.begin(), data_.vibrate.end());

  return pattern;
}

DOMTimeStamp Notification::timestamp() const {
  return data_.timestamp;
}

bool Notification::renotify() const {
  return data_.renotify;
}

bool Notification::silent() const {
  return data_.silent;
}

bool Notification::requireInteraction() const {
  return data_.require_interaction;
}

ScriptValue Notification::data(ScriptState* script_state) {
  const WebVector<char>& serialized_data = data_.data;
  scoped_refptr<SerializedScriptValue> serialized_value =
      SerializedScriptValue::Create(serialized_data.Data(),
                                    serialized_data.size());

  return ScriptValue(script_state,
                     serialized_value->Deserialize(script_state->GetIsolate()));
}

Vector<v8::Local<v8::Value>> Notification::actions(
    ScriptState* script_state) const {
  Vector<v8::Local<v8::Value>> actions;
  actions.Grow(data_.actions.size());

  for (size_t i = 0; i < data_.actions.size(); ++i) {
    NotificationAction action;

    switch (data_.actions[i].type) {
      case WebNotificationAction::kButton:
        action.setType("button");
        break;
      case WebNotificationAction::kText:
        action.setType("text");
        break;
      default:
        NOTREACHED() << "Unknown action type: " << data_.actions[i].type;
    }

    action.setAction(data_.actions[i].action);
    action.setTitle(data_.actions[i].title);
    action.setIcon(data_.actions[i].icon.GetString());
    action.setPlaceholder(data_.actions[i].placeholder);

    // Both the Action dictionaries themselves and the sequence they'll be
    // returned in are expected to the frozen. This cannot be done with WebIDL.
    actions[i] =
        FreezeV8Object(ToV8(action, script_state), script_state->GetIsolate());
  }

  return actions;
}

String Notification::PermissionString(
    mojom::blink::PermissionStatus permission) {
  switch (permission) {
    case mojom::blink::PermissionStatus::GRANTED:
      return "granted";
    case mojom::blink::PermissionStatus::DENIED:
      return "denied";
    case mojom::blink::PermissionStatus::ASK:
      return "default";
  }

  NOTREACHED();
  return "denied";
}

String Notification::permission(ExecutionContext* context) {
  // Permission is always denied for insecure contexts. Skip the sync IPC call.
  if (!context->IsSecureContext())
    return PermissionString(mojom::blink::PermissionStatus::DENIED);

  mojom::blink::PermissionStatus status =
      NotificationManager::From(context)->GetPermissionStatus();

  // Permission can only be requested from top-level frames and same-origin
  // iframes. This should be reflected in calls getting permission status.
  //
  // TODO(crbug.com/758603): Move this check to the browser process when the
  // NotificationService connection becomes frame-bound.
  if (status == mojom::blink::PermissionStatus::ASK && context->IsDocument()) {
    LocalFrame* frame = ToDocument(context)->GetFrame();
    if (!frame || frame->IsCrossOriginSubframe())
      status = mojom::blink::PermissionStatus::DENIED;
  }

  return PermissionString(status);
}

ScriptPromise Notification::requestPermission(
    ScriptState* script_state,
    V8NotificationPermissionCallback* deprecated_callback) {
  ExecutionContext* context = ExecutionContext::From(script_state);
  Document* doc = ToDocumentOrNull(context);

  probe::breakableLocation(context, "Notification.requestPermission");
  if (!Frame::HasTransientUserActivation(doc ? doc->GetFrame() : nullptr)) {
    PerformanceMonitor::ReportGenericViolation(
        context, PerformanceMonitor::kDiscouragedAPIUse,
        "Only request notification permission in response to a user gesture.",
        0, nullptr);
  }

  // Sites cannot request notification permission from insecure contexts.
  if (!context->IsSecureContext()) {
    Deprecation::CountDeprecation(
        context, WebFeature::kNotificationPermissionRequestedInsecureOrigin);
  }

  // Sites cannot request notification permission from cross-origin iframes,
  // but they can use notifications if permission had already been granted.
  if (context->IsDocument()) {
    LocalFrame* frame = ToDocument(context)->GetFrame();
    if (!frame || frame->IsCrossOriginSubframe()) {
      Deprecation::CountDeprecation(
          context, WebFeature::kNotificationPermissionRequestedIframe);
    }
  }

  return NotificationManager::From(context)->RequestPermission(
      script_state, deprecated_callback);
}

size_t Notification::maxActions() {
  return kWebNotificationMaxActions;
}

DispatchEventResult Notification::DispatchEventInternal(Event* event) {
  DCHECK(GetExecutionContext()->IsContextThread());
  return EventTarget::DispatchEventInternal(event);
}

const AtomicString& Notification::InterfaceName() const {
  return EventTargetNames::Notification;
}

void Notification::ContextDestroyed(ExecutionContext* context) {
  listener_binding_.Close();

  state_ = State::kClosed;

  if (prepare_show_method_runner_)
    prepare_show_method_runner_->Stop();

  if (loader_)
    loader_->Stop();
}

bool Notification::HasPendingActivity() const {
  // Non-persistent notification can receive events until they've been closed.
  // Persistent notifications should be subject to regular garbage collection.
  if (type_ == Type::kNonPersistent)
    return state_ != State::kClosed;

  return false;
}

void Notification::Trace(blink::Visitor* visitor) {
  visitor->Trace(prepare_show_method_runner_);
  visitor->Trace(loader_);
  EventTargetWithInlineData::Trace(visitor);
  ContextLifecycleObserver::Trace(visitor);
}

}  // namespace blink
