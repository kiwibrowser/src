// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/history/ios/browser/web_state_top_sites_observer.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "components/history/core/browser/top_sites.h"
#include "ios/web/public/load_committed_details.h"
#include "ios/web/public/navigation_item.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

DEFINE_WEB_STATE_USER_DATA_KEY(history::WebStateTopSitesObserver);

namespace history {

// static
void WebStateTopSitesObserver::CreateForWebState(web::WebState* web_state,
                                                 TopSites* top_sites) {
  DCHECK(web_state);
  if (!FromWebState(web_state)) {
    web_state->SetUserData(
        UserDataKey(),
        base::WrapUnique(new WebStateTopSitesObserver(web_state, top_sites)));
  }
}

WebStateTopSitesObserver::WebStateTopSitesObserver(web::WebState* web_state,
                                                   TopSites* top_sites)
    : top_sites_(top_sites) {
  web_state->AddObserver(this);
}

WebStateTopSitesObserver::~WebStateTopSitesObserver() {
}

void WebStateTopSitesObserver::NavigationItemCommitted(
    web::WebState* web_state,
    const web::LoadCommittedDetails& load_details) {
  DCHECK(load_details.item);
  if (top_sites_)
    top_sites_->OnNavigationCommitted(load_details.item->GetURL());
}

void WebStateTopSitesObserver::WebStateDestroyed(web::WebState* web_state) {
  web_state->RemoveObserver(this);
}

}  // namespace history
