// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/display_source/display_source_event_router.h"

#include <utility>

#include "content/public/browser/browser_context.h"
#include "extensions/browser/api/display_source/display_source_api.h"
#include "extensions/browser/api/display_source/display_source_connection_delegate_factory.h"
#include "extensions/common/api/display_source.h"

namespace extensions {

DisplaySourceEventRouter::DisplaySourceEventRouter(
    content::BrowserContext* browser_context)
    : browser_context_(browser_context), listening_(false) {
  EventRouter* event_router = EventRouter::Get(browser_context_);
  if (!event_router)
    return;
  event_router->RegisterObserver(
      this, api::display_source::OnSinksUpdated::kEventName);
}

DisplaySourceEventRouter::~DisplaySourceEventRouter() {
  DCHECK(!listening_);
}

void DisplaySourceEventRouter::Shutdown() {
  EventRouter* event_router = EventRouter::Get(browser_context_);
  if (event_router)
    event_router->UnregisterObserver(this);

  if (!listening_)
    return;
  listening_ = false;
  DisplaySourceConnectionDelegate* delegate =
      DisplaySourceConnectionDelegateFactory::GetForBrowserContext(
          browser_context_);
  if (delegate)
    delegate->RemoveObserver(this);
}

void DisplaySourceEventRouter::OnListenerAdded(
    const EventListenerInfo& details) {
  StartOrStopListeningForSinksChanges();
}

void DisplaySourceEventRouter::OnListenerRemoved(
    const EventListenerInfo& details) {
  StartOrStopListeningForSinksChanges();
}

void DisplaySourceEventRouter::StartOrStopListeningForSinksChanges() {
  EventRouter* event_router = EventRouter::Get(browser_context_);
  if (!event_router)
    return;

  bool should_listen = event_router->HasEventListener(
      api::display_source::OnSinksUpdated::kEventName);
  if (should_listen && !listening_) {
    DisplaySourceConnectionDelegate* delegate =
        DisplaySourceConnectionDelegateFactory::GetForBrowserContext(
            browser_context_);
    if (delegate) {
      delegate->AddObserver(this);
      delegate->StartWatchingAvailableSinks();
    }
  }
  if (!should_listen && listening_) {
    DisplaySourceConnectionDelegate* delegate =
        DisplaySourceConnectionDelegateFactory::GetForBrowserContext(
            browser_context_);
    if (delegate) {
      delegate->RemoveObserver(this);
      delegate->StopWatchingAvailableSinks();
    }
  }

  listening_ = should_listen;
}

void DisplaySourceEventRouter::OnSinksUpdated(
    const DisplaySourceSinkInfoList& sinks) {
  EventRouter* event_router = EventRouter::Get(browser_context_);
  if (!event_router)
    return;
  std::unique_ptr<base::ListValue> args(
      api::display_source::OnSinksUpdated::Create(sinks));
  std::unique_ptr<Event> sinks_updated_event(new Event(
      events::DISPLAY_SOURCE_ON_SINKS_UPDATED,
      api::display_source::OnSinksUpdated::kEventName, std::move(args)));
  event_router->BroadcastEvent(std::move(sinks_updated_event));
}

DisplaySourceEventRouter* DisplaySourceEventRouter::Create(
    content::BrowserContext* browser_context) {
  return new DisplaySourceEventRouter(browser_context);
}

}  // namespace extensions
