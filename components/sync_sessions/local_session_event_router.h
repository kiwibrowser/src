// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_SESSIONS_LOCAL_SESSION_EVENT_ROUTER_H_
#define COMPONENTS_SYNC_SESSIONS_LOCAL_SESSION_EVENT_ROUTER_H_

#include <set>

#include "base/macros.h"
#include "url/gurl.h"

namespace sync_sessions {

class SyncedTabDelegate;

// An interface defining the ways in which local open tab events can interact
// with session sync.  All local tab events flow to sync via this interface.
// In that way it is analogous to sync changes flowing to the local model
// via ProcessSyncChanges, just with a more granular breakdown.
class LocalSessionEventHandler {
 public:
  virtual ~LocalSessionEventHandler() {}

  // A local navigation event took place that affects the synced session
  // for this instance of Chrome.
  virtual void OnLocalTabModified(SyncedTabDelegate* modified_tab) = 0;

  // A local navigation occurred that triggered updates to favicon data for
  // each page URL in |page_urls| (e.g. http://www.google.com) and the icon URL
  // |icon_url| (e.g. http://www.google.com/favicon.ico). This is routed through
  // Sessions Sync so that we can filter (exclude) favicon updates for pages
  // that aren't currently part of the set of local open tabs, and pass relevant
  // updates on to FaviconCache for out-of-band favicon syncing.
  virtual void OnFaviconsChanged(const std::set<GURL>& page_urls,
                                 const GURL& icon_url) = 0;

 protected:
  LocalSessionEventHandler() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(LocalSessionEventHandler);
};

// The LocalSessionEventRouter is responsible for hooking itself up to various
// notification sources in the browser process and forwarding relevant
// events to a handler as defined in the LocalSessionEventHandler contract.
class LocalSessionEventRouter {
 public:
  virtual ~LocalSessionEventRouter() {}
  virtual void StartRoutingTo(LocalSessionEventHandler* handler) = 0;
  virtual void Stop() = 0;

 protected:
  LocalSessionEventRouter() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(LocalSessionEventRouter);
};

}  // namespace sync_sessions

#endif  // COMPONENTS_SYNC_SESSIONS_LOCAL_SESSION_EVENT_ROUTER_H_
