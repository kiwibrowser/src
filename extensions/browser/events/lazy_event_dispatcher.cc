// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/events/lazy_event_dispatcher.h"

#include "base/bind.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extensions_browser_client.h"
#include "extensions/browser/lazy_background_task_queue.h"
#include "extensions/browser/lazy_context_id.h"
#include "extensions/browser/service_worker_task_queue.h"
#include "extensions/common/manifest_handlers/incognito_info.h"

using content::BrowserContext;

namespace extensions {

LazyEventDispatcher::LazyEventDispatcher(
    BrowserContext* browser_context,
    const linked_ptr<Event>& event,
    const DispatchFunction& dispatch_function)
    : browser_context_(browser_context),
      event_(event),
      dispatch_function_(dispatch_function) {}

LazyEventDispatcher::~LazyEventDispatcher() {}

void LazyEventDispatcher::DispatchToEventPage(
    const ExtensionId& extension_id,
    const base::DictionaryValue* listener_filter) {
  LazyContextId dispatch_context(browser_context_, extension_id);
  DispatchToLazyContext(&dispatch_context, listener_filter);
}

void LazyEventDispatcher::DispatchToServiceWorker(
    const ExtensionId& extension_id,
    const GURL& service_worker_scope,
    const base::DictionaryValue* listener_filter) {
  LazyContextId dispatch_context(browser_context_, extension_id,
                                 service_worker_scope);
  DispatchToLazyContext(&dispatch_context, listener_filter);
}

bool LazyEventDispatcher::HasAlreadyDispatched(
    BrowserContext* context,
    const EventListener* listener) const {
  std::unique_ptr<LazyContextId> dispatch_context;
  if (listener->is_for_service_worker()) {
    dispatch_context = std::make_unique<LazyContextId>(
        context, listener->extension_id(), listener->listener_url());
  } else {
    dispatch_context =
        std::make_unique<LazyContextId>(context, listener->extension_id());
  }

  return HasAlreadyDispatchedImpl(dispatch_context.get());
}

void LazyEventDispatcher::DispatchToLazyContext(
    LazyContextId* dispatch_context,
    const base::DictionaryValue* listener_filter) {
  const Extension* extension = ExtensionRegistry::Get(browser_context_)
                                   ->enabled_extensions()
                                   .GetByID(dispatch_context->extension_id());
  if (!extension)
    return;

  // Check both the original and the incognito browser context to see if we
  // should load a non-peristent context (a lazy background page or an
  // extension service worker) to handle the event. We need to use the incognito
  // context in the case of split-mode extensions.
  if (QueueEventDispatch(dispatch_context, extension, listener_filter))
    RecordAlreadyDispatched(dispatch_context);

  BrowserContext* additional_context = GetIncognitoContext(extension);
  if (!additional_context)
    return;

  dispatch_context->set_browser_context(additional_context);
  if (QueueEventDispatch(dispatch_context, extension, listener_filter))
    RecordAlreadyDispatched(dispatch_context);
}

bool LazyEventDispatcher::QueueEventDispatch(
    LazyContextId* dispatch_context,
    const Extension* extension,
    const base::DictionaryValue* listener_filter) {
  if (!EventRouter::CanDispatchEventToBrowserContext(
          dispatch_context->browser_context(), extension, *event_)) {
    return false;
  }

  if (HasAlreadyDispatchedImpl(dispatch_context))
    return false;

  LazyContextTaskQueue* queue = dispatch_context->GetTaskQueue();
  if (!queue->ShouldEnqueueTask(dispatch_context->browser_context(),
                                extension)) {
    return false;
  }

  linked_ptr<Event> dispatched_event(event_);

  // If there's a dispatch callback, call it now (rather than dispatch time)
  // to avoid lifetime issues. Use a separate copy of the event args, so they
  // last until the event is dispatched.
  if (!event_->will_dispatch_callback.is_null()) {
    dispatched_event.reset(event_->DeepCopy());
    if (!dispatched_event->will_dispatch_callback.Run(
            dispatch_context->browser_context(), extension,
            dispatched_event.get(), listener_filter)) {
      // The event has been canceled.
      return true;
    }
    // Ensure we don't call it again at dispatch time.
    dispatched_event->will_dispatch_callback.Reset();
  }

  queue->AddPendingTaskToDispatchEvent(
      dispatch_context, base::BindOnce(dispatch_function_, dispatched_event));

  return true;
}

bool LazyEventDispatcher::HasAlreadyDispatchedImpl(
    const LazyContextId* dispatch_context) const {
  if (dispatch_context->is_for_service_worker()) {
    ServiceWorkerDispatchIdentifier dispatch_id(
        dispatch_context->browser_context(),
        dispatch_context->service_worker_scope());
    return base::ContainsKey(dispatched_ids_for_service_worker_, dispatch_id);
  }
  DCHECK(dispatch_context->is_for_event_page());
  EventPageDispatchIdentifier dispatch_id(dispatch_context->browser_context(),
                                          dispatch_context->extension_id());
  return base::ContainsKey(dispatched_ids_for_event_page_, dispatch_id);
}

void LazyEventDispatcher::RecordAlreadyDispatched(
    LazyContextId* dispatch_context) {
  if (dispatch_context->is_for_service_worker()) {
    dispatched_ids_for_service_worker_.insert(
        std::make_pair(dispatch_context->browser_context(),
                       dispatch_context->service_worker_scope()));
    return;
  }
  DCHECK(dispatch_context->is_for_event_page());
  dispatched_ids_for_event_page_.insert(std::make_pair(
      dispatch_context->browser_context(), dispatch_context->extension_id()));
}

BrowserContext* LazyEventDispatcher::GetIncognitoContext(
    const Extension* extension) {
  if (!IncognitoInfo::IsSplitMode(extension))
    return nullptr;
  ExtensionsBrowserClient* browser_client = ExtensionsBrowserClient::Get();
  if (!browser_client->HasOffTheRecordContext(browser_context_))
    return nullptr;
  return browser_client->GetOffTheRecordContext(browser_context_);
}

}  // namespace extensions
