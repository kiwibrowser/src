// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_EVENTS_LAZY_EVENT_DISPATCHER_H_
#define EXTENSIONS_BROWSER_EVENTS_LAZY_EVENT_DISPATCHER_H_

#include <set>
#include <utility>

#include "base/callback.h"
#include "base/memory/linked_ptr.h"
#include "extensions/browser/lazy_context_task_queue.h"
#include "extensions/common/extension_id.h"

namespace base {
class DictionaryValue;
}

namespace content {
class BrowserContext;
}

namespace extensions {
class EventListener;
class Extension;
class LazyContextId;
struct Event;

// Helper class for EventRouter to dispatch lazy events to lazy contexts.
//
// Manages waking up lazy contexts if they are stopped.
class LazyEventDispatcher {
 public:
  using DispatchFunction =
      base::Callback<void(const linked_ptr<Event>&,
                          std::unique_ptr<LazyContextTaskQueue::ContextInfo>)>;

  LazyEventDispatcher(content::BrowserContext* browser_context,
                      const linked_ptr<Event>& event,
                      const DispatchFunction& dispatch_function);
  ~LazyEventDispatcher();

  // Dispatches the lazy |event_| to |extension_id|.
  //
  // Ensures that all lazy background pages that are interested in the given
  // event are loaded, and queues the event if the page is not ready yet.
  void DispatchToEventPage(const ExtensionId& extension_id,
                           const base::DictionaryValue* listener_filter);
  // Dispatches the lazy |event_| to |extension_id|'s service worker.
  //
  // Service workers are started if they were stopped, before dispatching the
  // event.
  void DispatchToServiceWorker(const ExtensionId& extension_id,
                               const GURL& service_worker_scope,
                               const base::DictionaryValue* listener_filter);

  // Returns whether or not an event listener identical to |listener| is queued
  // for dispatch already.
  bool HasAlreadyDispatched(content::BrowserContext* context,
                            const EventListener* listener) const;

 private:
  using EventPageDispatchIdentifier =
      std::pair<const content::BrowserContext*, std::string>;
  using ServiceWorkerDispatchIdentifier =
      std::pair<const content::BrowserContext*, GURL>;

  void DispatchToLazyContext(LazyContextId* dispatch_context,
                             const base::DictionaryValue* listener_filter);

  // Possibly loads given extension's background page or extension Service
  // Worker in preparation to dispatch an event.  Returns true if the event was
  // queued for subsequent dispatch, false otherwise.
  bool QueueEventDispatch(LazyContextId* dispatch_context,
                          const Extension* extension,
                          const base::DictionaryValue* listener_filter);

  bool HasAlreadyDispatchedImpl(const LazyContextId* dispatch_context) const;

  void RecordAlreadyDispatched(LazyContextId* dispatch_context);

  content::BrowserContext* GetIncognitoContext(const Extension* extension);

  content::BrowserContext* const browser_context_;
  linked_ptr<Event> event_;
  DispatchFunction dispatch_function_;

  // TODO(lazyboy): Instead of keeping these two std::sets, compbine them using
  // LazyContextId key when service worker event listeners are more common.
  std::set<EventPageDispatchIdentifier> dispatched_ids_for_event_page_;
  std::set<ServiceWorkerDispatchIdentifier> dispatched_ids_for_service_worker_;

  DISALLOW_COPY_AND_ASSIGN(LazyEventDispatcher);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_EVENTS_LAZY_EVENT_DISPATCHER_H_
