// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/push_messaging/push_messaging_bridge.h"

#include "third_party/blink/public/platform/modules/push_messaging/web_push_error.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/modules/permissions/permission_utils.h"
#include "third_party/blink/renderer/modules/push_messaging/push_error.h"
#include "third_party/blink/renderer/modules/push_messaging/push_subscription_options_init.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"

namespace blink {
namespace {

// Error message to explain that the userVisibleOnly flag must be set.
const char kUserVisibleOnlyRequired[] =
    "Push subscriptions that don't enable userVisibleOnly are not supported.";

String PermissionStatusToString(mojom::blink::PermissionStatus status) {
  switch (status) {
    case mojom::blink::PermissionStatus::GRANTED:
      return "granted";
    case mojom::blink::PermissionStatus::DENIED:
      return "denied";
    case mojom::blink::PermissionStatus::ASK:
      return "prompt";
  }

  NOTREACHED();
  return "denied";
}

}  // namespace

// static
PushMessagingBridge* PushMessagingBridge::From(
    ServiceWorkerRegistration* service_worker_registration) {
  DCHECK(service_worker_registration);

  PushMessagingBridge* bridge =
      Supplement<ServiceWorkerRegistration>::From<PushMessagingBridge>(
          service_worker_registration);

  if (!bridge) {
    bridge = new PushMessagingBridge(*service_worker_registration);
    Supplement<ServiceWorkerRegistration>::ProvideTo(
        *service_worker_registration, bridge);
  }

  return bridge;
}

PushMessagingBridge::PushMessagingBridge(
    ServiceWorkerRegistration& registration)
    : Supplement<ServiceWorkerRegistration>(registration) {}

PushMessagingBridge::~PushMessagingBridge() = default;

const char PushMessagingBridge::kSupplementName[] = "PushMessagingBridge";

ScriptPromise PushMessagingBridge::GetPermissionState(
    ScriptState* script_state,
    const PushSubscriptionOptionsInit& options) {
  ExecutionContext* context = ExecutionContext::From(script_state);
  if (!permission_service_) {
    ConnectToPermissionService(context,
                               mojo::MakeRequest(&permission_service_));
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  // The `userVisibleOnly` flag on |options| must be set, as it's intended to be
  // a contract with the developer that they will show a notification upon
  // receiving a push message. Permission is denied without this setting.
  //
  // TODO(peter): Would it be better to resolve DENIED rather than rejecting?
  if (!options.hasUserVisibleOnly() || !options.userVisibleOnly()) {
    resolver->Reject(
        DOMException::Create(kNotSupportedError, kUserVisibleOnlyRequired));
    return promise;
  }

  permission_service_->HasPermission(
      CreatePermissionDescriptor(mojom::blink::PermissionName::NOTIFICATIONS),
      WTF::Bind(&PushMessagingBridge::DidGetPermissionState,
                WrapPersistent(this), WrapPersistent(resolver)));

  return promise;
}

void PushMessagingBridge::DidGetPermissionState(
    ScriptPromiseResolver* resolver,
    mojom::blink::PermissionStatus status) {
  resolver->Resolve(PermissionStatusToString(status));
}

}  // namespace blink
