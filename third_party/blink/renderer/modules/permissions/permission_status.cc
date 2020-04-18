// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/permissions/permission_status.h"

#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/modules/event_target_modules_names.h"
#include "third_party/blink/renderer/modules/permissions/permission_utils.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"

namespace blink {

// static
PermissionStatus* PermissionStatus::Take(ScriptPromiseResolver* resolver,
                                         MojoPermissionStatus status,
                                         MojoPermissionDescriptor descriptor) {
  return PermissionStatus::CreateAndListen(resolver->GetExecutionContext(),
                                           status, std::move(descriptor));
}

PermissionStatus* PermissionStatus::CreateAndListen(
    ExecutionContext* execution_context,
    MojoPermissionStatus status,
    MojoPermissionDescriptor descriptor) {
  PermissionStatus* permission_status =
      new PermissionStatus(execution_context, status, std::move(descriptor));
  permission_status->PauseIfNeeded();
  permission_status->StartListening();
  return permission_status;
}

PermissionStatus::PermissionStatus(ExecutionContext* execution_context,
                                   MojoPermissionStatus status,
                                   MojoPermissionDescriptor descriptor)
    : PausableObject(execution_context),
      status_(status),
      descriptor_(std::move(descriptor)),
      binding_(this) {}

PermissionStatus::~PermissionStatus() = default;

void PermissionStatus::Dispose() {
  StopListening();
}

const AtomicString& PermissionStatus::InterfaceName() const {
  return EventTargetNames::PermissionStatus;
}

ExecutionContext* PermissionStatus::GetExecutionContext() const {
  return PausableObject::GetExecutionContext();
}

bool PermissionStatus::HasPendingActivity() const {
  return binding_.is_bound();
}

void PermissionStatus::Unpause() {
  StartListening();
}

void PermissionStatus::Pause() {
  StopListening();
}

void PermissionStatus::ContextDestroyed(ExecutionContext*) {
  StopListening();
}

String PermissionStatus::state() const {
  switch (status_) {
    case MojoPermissionStatus::GRANTED:
      return "granted";
    case MojoPermissionStatus::DENIED:
      return "denied";
    case MojoPermissionStatus::ASK:
      return "prompt";
  }

  NOTREACHED();
  return "denied";
}

void PermissionStatus::StartListening() {
  DCHECK(!binding_.is_bound());
  mojom::blink::PermissionObserverPtr observer;
  binding_.Bind(mojo::MakeRequest(&observer));

  mojom::blink::PermissionServicePtr service;
  ConnectToPermissionService(GetExecutionContext(),
                             mojo::MakeRequest(&service));
  service->AddPermissionObserver(descriptor_->Clone(), status_,
                                 std::move(observer));
}

void PermissionStatus::StopListening() {
  binding_.Close();
}

void PermissionStatus::OnPermissionStatusChange(MojoPermissionStatus status) {
  if (status_ == status)
    return;

  status_ = status;
  DispatchEvent(Event::Create(EventTypeNames::change));
}

void PermissionStatus::Trace(blink::Visitor* visitor) {
  EventTargetWithInlineData::Trace(visitor);
  PausableObject::Trace(visitor);
}

}  // namespace blink
