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

#include "third_party/blink/renderer/modules/serviceworkers/service_worker_global_scope_proxy.h"

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_client.mojom-blink.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_event_status.mojom-blink.h"
#include "third_party/blink/public/platform/modules/notifications/web_notification_data.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_request.h"
#include "third_party/blink/public/web/modules/serviceworker/web_service_worker_context_client.h"
#include "third_party/blink/public/web/web_serialized_script_value.h"
#include "third_party/blink/renderer/bindings/core/v8/source_location.h"
#include "third_party/blink/renderer/bindings/core/v8/worker_or_worklet_script_controller.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/fetch/headers.h"
#include "third_party/blink/renderer/core/frame/csp/content_security_policy.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/messaging/blink_transferable_message.h"
#include "third_party/blink/renderer/core/messaging/message_port.h"
#include "third_party/blink/renderer/core/origin_trials/origin_trials.h"
#include "third_party/blink/renderer/core/workers/parent_execution_context_task_runners.h"
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"
#include "third_party/blink/renderer/core/workers/worker_thread.h"
#include "third_party/blink/renderer/modules/background_fetch/background_fetch_click_event.h"
#include "third_party/blink/renderer/modules/background_fetch/background_fetch_click_event_init.h"
#include "third_party/blink/renderer/modules/background_fetch/background_fetch_event.h"
#include "third_party/blink/renderer/modules/background_fetch/background_fetch_event_init.h"
#include "third_party/blink/renderer/modules/background_fetch/background_fetch_fail_event.h"
#include "third_party/blink/renderer/modules/background_fetch/background_fetch_fail_event_init.h"
#include "third_party/blink/renderer/modules/background_fetch/background_fetch_settled_event_init.h"
#include "third_party/blink/renderer/modules/background_fetch/background_fetch_update_event.h"
#include "third_party/blink/renderer/modules/background_sync/sync_event.h"
#include "third_party/blink/renderer/modules/cookie_store/extendable_cookie_change_event.h"
#include "third_party/blink/renderer/modules/exported/web_embedded_worker_impl.h"
#include "third_party/blink/renderer/modules/notifications/notification.h"
#include "third_party/blink/renderer/modules/notifications/notification_event.h"
#include "third_party/blink/renderer/modules/notifications/notification_event_init.h"
#include "third_party/blink/renderer/modules/payments/abort_payment_event.h"
#include "third_party/blink/renderer/modules/payments/abort_payment_respond_with_observer.h"
#include "third_party/blink/renderer/modules/payments/can_make_payment_event.h"
#include "third_party/blink/renderer/modules/payments/can_make_payment_respond_with_observer.h"
#include "third_party/blink/renderer/modules/payments/payment_event_data_conversion.h"
#include "third_party/blink/renderer/modules/payments/payment_request_event.h"
#include "third_party/blink/renderer/modules/payments/payment_request_event_init.h"
#include "third_party/blink/renderer/modules/payments/payment_request_respond_with_observer.h"
#include "third_party/blink/renderer/modules/push_messaging/push_event.h"
#include "third_party/blink/renderer/modules/push_messaging/push_message_data.h"
#include "third_party/blink/renderer/modules/serviceworkers/extendable_event.h"
#include "third_party/blink/renderer/modules/serviceworkers/extendable_message_event.h"
#include "third_party/blink/renderer/modules/serviceworkers/fetch_event.h"
#include "third_party/blink/renderer/modules/serviceworkers/fetch_respond_with_observer.h"
#include "third_party/blink/renderer/modules/serviceworkers/install_event.h"
#include "third_party/blink/renderer/modules/serviceworkers/service_worker_client.h"
#include "third_party/blink/renderer/modules/serviceworkers/service_worker_global_scope.h"
#include "third_party/blink/renderer/modules/serviceworkers/service_worker_window_client.h"
#include "third_party/blink/renderer/modules/serviceworkers/wait_until_observer.h"
#include "third_party/blink/renderer/platform/cross_thread_functional.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_response.h"
#include "third_party/blink/renderer/platform/network/content_security_policy_response_headers.h"
#include "third_party/blink/renderer/platform/waitable_event.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"

namespace blink {

namespace {

void SetContentSecurityPolicyAndReferrerPolicyOnMainThread(
    WebEmbeddedWorkerImpl* embedded_worker,
    ContentSecurityPolicyResponseHeaders csp_headers,
    String referrer_policy,
    WaitableEvent* waitable_event) {
  DCHECK(IsMainThread());
  ContentSecurityPolicy* content_security_policy =
      ContentSecurityPolicy::Create();
  content_security_policy->DidReceiveHeaders(csp_headers);
  embedded_worker->SetContentSecurityPolicyAndReferrerPolicy(
      content_security_policy, std::move(referrer_policy));
  waitable_event->Signal();
}

}  // namespace

ServiceWorkerGlobalScopeProxy* ServiceWorkerGlobalScopeProxy::Create(
    WebEmbeddedWorkerImpl& embedded_worker,
    WebServiceWorkerContextClient& client) {
  return new ServiceWorkerGlobalScopeProxy(embedded_worker, client);
}

ServiceWorkerGlobalScopeProxy::~ServiceWorkerGlobalScopeProxy() {
  DCHECK(IsMainThread());
  // Verify that the proxy has been detached.
  DCHECK(!embedded_worker_);
}

void ServiceWorkerGlobalScopeProxy::Trace(blink::Visitor* visitor) {
  visitor->Trace(parent_execution_context_task_runners_);
}

void ServiceWorkerGlobalScopeProxy::ReadyToEvaluateScript() {
  WorkerGlobalScope()->ReadyToEvaluateScript();
}

void ServiceWorkerGlobalScopeProxy::SetRegistration(
    std::unique_ptr<WebServiceWorkerRegistration::Handle> handle) {
  DCHECK(WorkerGlobalScope()->IsContextThread());
  WorkerGlobalScope()->SetRegistration(std::move(handle));
}

void ServiceWorkerGlobalScopeProxy::DispatchBackgroundFetchAbortEvent(
    int event_id,
    const WebString& developer_id,
    const WebString& unique_id,
    const WebVector<WebBackgroundFetchSettledFetch>& fetches) {
  DCHECK(WorkerGlobalScope()->IsContextThread());
  WaitUntilObserver* observer = WaitUntilObserver::Create(
      WorkerGlobalScope(), WaitUntilObserver::kBackgroundFetchAbort, event_id);

  ScriptState* script_state =
      WorkerGlobalScope()->ScriptController()->GetScriptState();

  BackgroundFetchSettledEventInit init;
  init.setId(developer_id);
  init.setFetches(BackgroundFetchSettledFetches::Create(script_state, fetches));

  BackgroundFetchSettledEvent* event = BackgroundFetchSettledEvent::Create(
      EventTypeNames::backgroundfetchabort, init, unique_id, observer);

  WorkerGlobalScope()->DispatchExtendableEvent(event, observer);
}

void ServiceWorkerGlobalScopeProxy::DispatchBackgroundFetchClickEvent(
    int event_id,
    const WebString& developer_id,
    BackgroundFetchState status) {
  DCHECK(WorkerGlobalScope()->IsContextThread());
  WaitUntilObserver* observer = WaitUntilObserver::Create(
      WorkerGlobalScope(), WaitUntilObserver::kBackgroundFetchClick, event_id);

  BackgroundFetchClickEventInit init;
  init.setId(developer_id);

  switch (status) {
    case BackgroundFetchState::kPending:
      init.setState("pending");
      break;
    case BackgroundFetchState::kSucceeded:
      init.setState("succeeded");
      break;
    case BackgroundFetchState::kFailed:
      init.setState("failed");
      break;
  }

  BackgroundFetchClickEvent* event = BackgroundFetchClickEvent::Create(
      EventTypeNames::backgroundfetchclick, init, observer);

  WorkerGlobalScope()->DispatchExtendableEvent(event, observer);
}

void ServiceWorkerGlobalScopeProxy::DispatchBackgroundFetchFailEvent(
    int event_id,
    const WebString& developer_id,
    const WebString& unique_id,
    const WebVector<WebBackgroundFetchSettledFetch>& fetches) {
  DCHECK(WorkerGlobalScope()->IsContextThread());
  WaitUntilObserver* observer = WaitUntilObserver::Create(
      WorkerGlobalScope(), WaitUntilObserver::kBackgroundFetchFail, event_id);

  ScriptState* script_state =
      WorkerGlobalScope()->ScriptController()->GetScriptState();

  BackgroundFetchSettledEventInit init;
  init.setId(developer_id);
  init.setFetches(BackgroundFetchSettledFetches::Create(script_state, fetches));

  BackgroundFetchUpdateEvent* event = BackgroundFetchUpdateEvent::Create(
      EventTypeNames::backgroundfetched, init, unique_id, script_state,
      observer, worker_global_scope_->registration());

  WorkerGlobalScope()->DispatchExtendableEvent(event, observer);
}

void ServiceWorkerGlobalScopeProxy::DispatchBackgroundFetchedEvent(
    int event_id,
    const WebString& developer_id,
    const WebString& unique_id,
    const WebVector<WebBackgroundFetchSettledFetch>& fetches) {
  DCHECK(WorkerGlobalScope()->IsContextThread());
  WaitUntilObserver* observer = WaitUntilObserver::Create(
      WorkerGlobalScope(), WaitUntilObserver::kBackgroundFetched, event_id);

  ScriptState* script_state =
      WorkerGlobalScope()->ScriptController()->GetScriptState();

  // Do not remove this, it modifies V8 state.
  ScriptState::Scope scope(script_state);

  BackgroundFetchSettledEventInit init;
  init.setId(developer_id);
  init.setFetches(BackgroundFetchSettledFetches::Create(script_state, fetches));

  BackgroundFetchUpdateEvent* event = BackgroundFetchUpdateEvent::Create(
      EventTypeNames::backgroundfetched, init, unique_id, script_state,
      observer, worker_global_scope_->registration());

  WorkerGlobalScope()->DispatchExtendableEvent(event, observer);
}

void ServiceWorkerGlobalScopeProxy::DispatchActivateEvent(int event_id) {
  DCHECK(WorkerGlobalScope()->IsContextThread());
  WaitUntilObserver* observer = WaitUntilObserver::Create(
      WorkerGlobalScope(), WaitUntilObserver::kActivate, event_id);
  Event* event = ExtendableEvent::Create(EventTypeNames::activate,
                                         ExtendableEventInit(), observer);
  WorkerGlobalScope()->DispatchExtendableEvent(event, observer);
}

void ServiceWorkerGlobalScopeProxy::DispatchCookieChangeEvent(
    int event_id,
    const WebString& cookie_name,
    const WebString& cookie_value,
    bool is_cookie_delete) {
  DCHECK(WorkerGlobalScope()->IsContextThread());
  WaitUntilObserver* observer = WaitUntilObserver::Create(
      WorkerGlobalScope(), WaitUntilObserver::kCookieChange, event_id);

  HeapVector<CookieListItem> changed;
  HeapVector<CookieListItem> deleted;
  ExtendableCookieChangeEvent::ToCookieChangeListItem(
      cookie_name, cookie_value, is_cookie_delete, changed, deleted);
  Event* event = ExtendableCookieChangeEvent::Create(
      EventTypeNames::cookiechange, std::move(changed), std::move(deleted),
      observer);

  // TODO(pwnall): When switching to blink::CanonicalCookie, handle the case
  //               when (changed.IsEmpty() && deleted.IsEmpty()).

  // TODO(pwnall): Investigate dispatching this on cookieStore.
  WorkerGlobalScope()->DispatchExtendableEvent(event, observer);
}

void ServiceWorkerGlobalScopeProxy::DispatchExtendableMessageEvent(
    int event_id,
    TransferableMessage message,
    const WebSecurityOrigin& source_origin,
    const WebServiceWorkerClientInfo& client) {
  DCHECK(WorkerGlobalScope()->IsContextThread());
  auto msg = ToBlinkTransferableMessage(std::move(message));
  MessagePortArray* ports =
      MessagePort::EntanglePorts(*worker_global_scope_, std::move(msg.ports));
  String origin;
  if (!source_origin.IsUnique())
    origin = source_origin.ToString();
  ServiceWorkerClient* source = nullptr;
  if (client.client_type == mojom::ServiceWorkerClientType::kWindow)
    source = ServiceWorkerWindowClient::Create(client);
  else
    source = ServiceWorkerClient::Create(client);
  WaitUntilObserver* observer = WaitUntilObserver::Create(
      WorkerGlobalScope(), WaitUntilObserver::kMessage, event_id);

  Event* event = ExtendableMessageEvent::Create(std::move(msg.message), origin,
                                                ports, source, observer);
  WorkerGlobalScope()->DispatchExtendableEvent(event, observer);
}

void ServiceWorkerGlobalScopeProxy::DispatchExtendableMessageEvent(
    int event_id,
    TransferableMessage message,
    const WebSecurityOrigin& source_origin,
    std::unique_ptr<WebServiceWorker::Handle> handle) {
  DCHECK(WorkerGlobalScope()->IsContextThread());
  auto msg = ToBlinkTransferableMessage(std::move(message));
  MessagePortArray* ports =
      MessagePort::EntanglePorts(*worker_global_scope_, std::move(msg.ports));
  String origin;
  if (!source_origin.IsUnique())
    origin = source_origin.ToString();
  ServiceWorker* source =
      ServiceWorker::From(worker_global_scope_->GetExecutionContext(),
                          base::WrapUnique(handle.release()));
  WaitUntilObserver* observer = WaitUntilObserver::Create(
      WorkerGlobalScope(), WaitUntilObserver::kMessage, event_id);

  Event* event = ExtendableMessageEvent::Create(std::move(msg.message), origin,
                                                ports, source, observer);
  WorkerGlobalScope()->DispatchExtendableEvent(event, observer);
}

void ServiceWorkerGlobalScopeProxy::DispatchFetchEvent(
    int fetch_event_id,
    const WebServiceWorkerRequest& web_request,
    bool navigation_preload_sent) {
  DCHECK(WorkerGlobalScope()->IsContextThread());
  ScriptState::Scope scope(
      WorkerGlobalScope()->ScriptController()->GetScriptState());
  WaitUntilObserver* wait_until_observer = WaitUntilObserver::Create(
      WorkerGlobalScope(), WaitUntilObserver::kFetch, fetch_event_id);
  FetchRespondWithObserver* respond_with_observer =
      FetchRespondWithObserver::Create(
          WorkerGlobalScope(), fetch_event_id, web_request.Url(),
          web_request.Mode(), web_request.RedirectMode(),
          web_request.GetFrameType(), web_request.GetRequestContext(),
          wait_until_observer);
  Request* request = Request::Create(
      WorkerGlobalScope()->ScriptController()->GetScriptState(), web_request);
  request->getHeaders()->SetGuard(Headers::kImmutableGuard);
  FetchEventInit event_init;
  event_init.setCancelable(true);
  event_init.setRequest(request);
  event_init.setClientId(
      web_request.IsMainResourceLoad() ? WebString() : web_request.ClientId());
  event_init.setIsReload(web_request.IsReload());
  ScriptState* script_state =
      WorkerGlobalScope()->ScriptController()->GetScriptState();
  FetchEvent* fetch_event = FetchEvent::Create(
      script_state, EventTypeNames::fetch, event_init, respond_with_observer,
      wait_until_observer, navigation_preload_sent);
  if (navigation_preload_sent) {
    // Keep |fetchEvent| until OnNavigationPreloadComplete() or
    // onNavigationPreloadError() will be called.
    pending_preload_fetch_events_.insert(fetch_event_id, fetch_event);
  }

  WorkerGlobalScope()->DispatchExtendableEventWithRespondWith(
      fetch_event, wait_until_observer, respond_with_observer);
}

void ServiceWorkerGlobalScopeProxy::OnNavigationPreloadResponse(
    int fetch_event_id,
    std::unique_ptr<WebURLResponse> response,
    std::unique_ptr<WebDataConsumerHandle> data_consume_handle) {
  DCHECK(WorkerGlobalScope()->IsContextThread());
  auto it = pending_preload_fetch_events_.find(fetch_event_id);
  DCHECK(it != pending_preload_fetch_events_.end());
  FetchEvent* fetch_event = it->value.Get();
  DCHECK(fetch_event);
  fetch_event->OnNavigationPreloadResponse(
      WorkerGlobalScope()->ScriptController()->GetScriptState(),
      std::move(response), std::move(data_consume_handle));
}

void ServiceWorkerGlobalScopeProxy::OnNavigationPreloadError(
    int fetch_event_id,
    std::unique_ptr<WebServiceWorkerError> error) {
  DCHECK(WorkerGlobalScope()->IsContextThread());
  FetchEvent* fetch_event = pending_preload_fetch_events_.Take(fetch_event_id);
  DCHECK(fetch_event);
  // Display an unsanitized console message.
  if (!error->unsanitized_message.IsEmpty()) {
    WorkerGlobalScope()->AddConsoleMessage(ConsoleMessage::Create(
        kWorkerMessageSource, blink::MessageLevel::kErrorMessageLevel,
        error->unsanitized_message));
  }
  // Reject the preloadResponse promise.
  fetch_event->OnNavigationPreloadError(
      WorkerGlobalScope()->ScriptController()->GetScriptState(),
      std::move(error));
}

void ServiceWorkerGlobalScopeProxy::OnNavigationPreloadComplete(
    int fetch_event_id,
    TimeTicks completion_time,
    int64_t encoded_data_length,
    int64_t encoded_body_length,
    int64_t decoded_body_length) {
  DCHECK(WorkerGlobalScope()->IsContextThread());
  FetchEvent* fetch_event = pending_preload_fetch_events_.Take(fetch_event_id);
  DCHECK(fetch_event);
  fetch_event->OnNavigationPreloadComplete(
      WorkerGlobalScope(), completion_time, encoded_data_length,
      encoded_body_length, decoded_body_length);
}

void ServiceWorkerGlobalScopeProxy::DispatchInstallEvent(int event_id) {
  DCHECK(WorkerGlobalScope()->IsContextThread());
  WaitUntilObserver* observer = WaitUntilObserver::Create(
      WorkerGlobalScope(), WaitUntilObserver::kInstall, event_id);
  Event* event = InstallEvent::Create(
      EventTypeNames::install, ExtendableEventInit(), event_id, observer);
  WorkerGlobalScope()->SetIsInstalling(true);
  WorkerGlobalScope()->DispatchExtendableEvent(event, observer);
}

void ServiceWorkerGlobalScopeProxy::DispatchNotificationClickEvent(
    int event_id,
    const WebString& notification_id,
    const WebNotificationData& data,
    int action_index,
    const WebString& reply) {
  DCHECK(WorkerGlobalScope()->IsContextThread());
  WaitUntilObserver* observer = WaitUntilObserver::Create(
      WorkerGlobalScope(), WaitUntilObserver::kNotificationClick, event_id);
  NotificationEventInit event_init;
  event_init.setNotification(Notification::Create(
      WorkerGlobalScope(), notification_id, data, true /* showing */));
  if (0 <= action_index && action_index < static_cast<int>(data.actions.size()))
    event_init.setAction(data.actions[action_index].action);
  event_init.setReply(reply);
  Event* event = NotificationEvent::Create(EventTypeNames::notificationclick,
                                           event_init, observer);
  WorkerGlobalScope()->DispatchExtendableEvent(event, observer);
}

void ServiceWorkerGlobalScopeProxy::DispatchNotificationCloseEvent(
    int event_id,
    const WebString& notification_id,
    const WebNotificationData& data) {
  DCHECK(WorkerGlobalScope()->IsContextThread());
  WaitUntilObserver* observer = WaitUntilObserver::Create(
      WorkerGlobalScope(), WaitUntilObserver::kNotificationClose, event_id);
  NotificationEventInit event_init;
  event_init.setAction(WTF::String());  // initialize as null.
  event_init.setNotification(Notification::Create(
      WorkerGlobalScope(), notification_id, data, false /* showing */));
  Event* event = NotificationEvent::Create(EventTypeNames::notificationclose,
                                           event_init, observer);
  WorkerGlobalScope()->DispatchExtendableEvent(event, observer);
}

void ServiceWorkerGlobalScopeProxy::DispatchPushEvent(int event_id,
                                                      const WebString& data) {
  DCHECK(WorkerGlobalScope()->IsContextThread());
  WaitUntilObserver* observer = WaitUntilObserver::Create(
      WorkerGlobalScope(), WaitUntilObserver::kPush, event_id);
  Event* event = PushEvent::Create(EventTypeNames::push,
                                   PushMessageData::Create(data), observer);
  WorkerGlobalScope()->DispatchExtendableEvent(event, observer);
}

void ServiceWorkerGlobalScopeProxy::DispatchSyncEvent(int event_id,
                                                      const WebString& id,
                                                      bool last_chance) {
  DCHECK(WorkerGlobalScope()->IsContextThread());
  WaitUntilObserver* observer = WaitUntilObserver::Create(
      WorkerGlobalScope(), WaitUntilObserver::kSync, event_id);
  Event* event =
      SyncEvent::Create(EventTypeNames::sync, id, last_chance, observer);
  WorkerGlobalScope()->DispatchExtendableEvent(event, observer);
}

void ServiceWorkerGlobalScopeProxy::DispatchAbortPaymentEvent(int event_id) {
  DCHECK(WorkerGlobalScope()->IsContextThread());
  WaitUntilObserver* wait_until_observer = WaitUntilObserver::Create(
      WorkerGlobalScope(), WaitUntilObserver::kAbortPayment, event_id);
  AbortPaymentRespondWithObserver* respond_with_observer =
      new AbortPaymentRespondWithObserver(WorkerGlobalScope(), event_id,
                                          wait_until_observer);

  Event* event = AbortPaymentEvent::Create(
      EventTypeNames::abortpayment, ExtendableEventInit(),
      respond_with_observer, wait_until_observer);

  WorkerGlobalScope()->DispatchExtendableEventWithRespondWith(
      event, wait_until_observer, respond_with_observer);
}

void ServiceWorkerGlobalScopeProxy::DispatchCanMakePaymentEvent(
    int event_id,
    const WebCanMakePaymentEventData& web_event_data) {
  DCHECK(WorkerGlobalScope()->IsContextThread());
  WaitUntilObserver* wait_until_observer = WaitUntilObserver::Create(
      WorkerGlobalScope(), WaitUntilObserver::kCanMakePayment, event_id);
  CanMakePaymentRespondWithObserver* respond_with_observer =
      new CanMakePaymentRespondWithObserver(WorkerGlobalScope(), event_id,
                                            wait_until_observer);

  Event* event = CanMakePaymentEvent::Create(
      EventTypeNames::canmakepayment,
      PaymentEventDataConversion::ToCanMakePaymentEventInit(
          WorkerGlobalScope()->ScriptController()->GetScriptState(),
          web_event_data),
      respond_with_observer, wait_until_observer);

  WorkerGlobalScope()->DispatchExtendableEventWithRespondWith(
      event, wait_until_observer, respond_with_observer);
}

void ServiceWorkerGlobalScopeProxy::DispatchPaymentRequestEvent(
    int event_id,
    const WebPaymentRequestEventData& web_app_request) {
  DCHECK(WorkerGlobalScope()->IsContextThread());
  WaitUntilObserver* wait_until_observer = WaitUntilObserver::Create(
      WorkerGlobalScope(), WaitUntilObserver::kPaymentRequest, event_id);
  PaymentRequestRespondWithObserver* respond_with_observer =
      PaymentRequestRespondWithObserver::Create(WorkerGlobalScope(), event_id,
                                                wait_until_observer);

  Event* event = PaymentRequestEvent::Create(
      EventTypeNames::paymentrequest,
      PaymentEventDataConversion::ToPaymentRequestEventInit(
          WorkerGlobalScope()->ScriptController()->GetScriptState(),
          web_app_request),
      respond_with_observer, wait_until_observer);

  WorkerGlobalScope()->DispatchExtendableEventWithRespondWith(
      event, wait_until_observer, respond_with_observer);
}

bool ServiceWorkerGlobalScopeProxy::HasFetchEventHandler() {
  DCHECK(WorkerGlobalScope()->IsContextThread());
  return WorkerGlobalScope()->HasEventListeners(EventTypeNames::fetch);
}

void ServiceWorkerGlobalScopeProxy::CountFeature(WebFeature feature) {
  Client().CountFeature(feature);
}

void ServiceWorkerGlobalScopeProxy::CountDeprecation(WebFeature feature) {
  // Go through the same code path with countFeature() because a deprecation
  // message is already shown on the worker console and a remaining work is just
  // to record an API use.
  CountFeature(feature);
}

void ServiceWorkerGlobalScopeProxy::ReportException(
    const String& error_message,
    std::unique_ptr<SourceLocation> location,
    int exception_id) {
  Client().ReportException(error_message, location->LineNumber(),
                           location->ColumnNumber(), location->Url());
}

void ServiceWorkerGlobalScopeProxy::ReportConsoleMessage(
    MessageSource source,
    MessageLevel level,
    const String& message,
    SourceLocation* location) {
  Client().ReportConsoleMessage(source, level, message, location->LineNumber(),
                                location->Url());
}

void ServiceWorkerGlobalScopeProxy::PostMessageToPageInspector(
    int session_id,
    const String& message) {
  DCHECK(embedded_worker_);
  PostCrossThreadTask(
      *parent_execution_context_task_runners_->Get(
          TaskType::kInternalInspector),
      FROM_HERE,
      CrossThreadBind(&WebEmbeddedWorkerImpl::PostMessageToPageInspector,
                      CrossThreadUnretained(embedded_worker_), session_id,
                      message));
}

void ServiceWorkerGlobalScopeProxy::DidCreateWorkerGlobalScope(
    WorkerOrWorkletGlobalScope* worker_global_scope) {
  DCHECK(!worker_global_scope_);
  worker_global_scope_ =
      static_cast<ServiceWorkerGlobalScope*>(worker_global_scope);
  Client().WorkerContextStarted(this);
}

void ServiceWorkerGlobalScopeProxy::DidInitializeWorkerContext() {
  DCHECK(WorkerGlobalScope()->IsContextThread());
  ScriptState::Scope scope(
      WorkerGlobalScope()->ScriptController()->GetScriptState());
  Client().DidInitializeWorkerContext(
      WorkerGlobalScope()->ScriptController()->GetContext());
}

void ServiceWorkerGlobalScopeProxy::DidLoadInstalledScript(
    const ContentSecurityPolicyResponseHeaders& csp_headers_on_worker_thread,
    const String& referrer_policy_on_worker_thread) {
  // Post a task to the main thread to set CSP and ReferrerPolicy on the shadow
  // page.
  DCHECK(embedded_worker_);
  WaitableEvent waitable_event;
  PostCrossThreadTask(
      *parent_execution_context_task_runners_->Get(TaskType::kInternalWorker),
      FROM_HERE,
      CrossThreadBind(&SetContentSecurityPolicyAndReferrerPolicyOnMainThread,
                      CrossThreadUnretained(embedded_worker_),
                      csp_headers_on_worker_thread,
                      referrer_policy_on_worker_thread,
                      CrossThreadUnretained(&waitable_event)));
  Client().WorkerScriptLoaded();

  // Wait for the task to complete before returning. This ensures that worker
  // script evaluation can't start and issue any fetches until CSP and
  // ReferrerPolicy are set.
  waitable_event.Wait();
}

void ServiceWorkerGlobalScopeProxy::WillEvaluateClassicScript(
    size_t script_size,
    size_t cached_metadata_size) {
  DCHECK(WorkerGlobalScope()->IsContextThread());
  worker_global_scope_->CountWorkerScript(script_size, cached_metadata_size);
}

void ServiceWorkerGlobalScopeProxy::WillEvaluateImportedClassicScript(
    size_t script_size,
    size_t cached_metadata_size) {
  DCHECK(WorkerGlobalScope()->IsContextThread());
  worker_global_scope_->CountImportedScript(script_size, cached_metadata_size);
}

void ServiceWorkerGlobalScopeProxy::DidEvaluateClassicScript(bool success) {
  DCHECK(WorkerGlobalScope()->IsContextThread());
  WorkerGlobalScope()->DidEvaluateClassicScript();
  Client().DidEvaluateClassicScript(success);
}

void ServiceWorkerGlobalScopeProxy::DidCloseWorkerGlobalScope() {
  // This should never be called because close() is not defined in
  // ServiceWorkerGlobalScope.
  NOTREACHED();
}

void ServiceWorkerGlobalScopeProxy::WillDestroyWorkerGlobalScope() {
  DCHECK(WorkerGlobalScope()->IsContextThread());
  v8::HandleScope handle_scope(WorkerGlobalScope()->GetThread()->GetIsolate());
  Client().WillDestroyWorkerContext(
      WorkerGlobalScope()->ScriptController()->GetContext());
  worker_global_scope_ = nullptr;
}

void ServiceWorkerGlobalScopeProxy::DidTerminateWorkerThread() {
  // This should be called after WillDestroyWorkerGlobalScope().
  DCHECK(!worker_global_scope_);
  Client().WorkerContextDestroyed();
}

ServiceWorkerGlobalScopeProxy::ServiceWorkerGlobalScopeProxy(
    WebEmbeddedWorkerImpl& embedded_worker,
    WebServiceWorkerContextClient& client)
    : embedded_worker_(&embedded_worker),
      client_(&client),
      worker_global_scope_(nullptr) {
  DCHECK(IsMainThread());
  // ServiceWorker can sometimes run tasks that are initiated by/associated with
  // a document's frame but these documents can be from a different process. So
  // we intentionally populate the task runners with default task runners of the
  // main thread.
  parent_execution_context_task_runners_ =
      ParentExecutionContextTaskRunners::Create();
}

void ServiceWorkerGlobalScopeProxy::Detach() {
  DCHECK(IsMainThread());
  embedded_worker_ = nullptr;
  client_ = nullptr;
}

void ServiceWorkerGlobalScopeProxy::TerminateWorkerContext() {
  embedded_worker_->TerminateWorkerContext();
}

WebServiceWorkerContextClient& ServiceWorkerGlobalScopeProxy::Client() const {
  DCHECK(client_);
  return *client_;
}

ServiceWorkerGlobalScope* ServiceWorkerGlobalScopeProxy::WorkerGlobalScope()
    const {
  DCHECK(worker_global_scope_);
  DCHECK(worker_global_scope_->IsContextThread());
  return worker_global_scope_;
}

}  // namespace blink
