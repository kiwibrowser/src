// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/infobars/infobar_manager_impl.h"

#include <utility>

#include "base/logging.h"
#include "components/infobars/core/confirm_infobar_delegate.h"
#include "components/infobars/core/infobar.h"
#include "components/infobars/core/infobar_delegate.h"
#include "ios/chrome/browser/infobars/infobar_utils.h"
#include "ios/web/public/load_committed_details.h"
#include "ios/web/public/navigation_item.h"
#include "ios/web/public/navigation_manager.h"
#include "ios/web/public/web_state/web_state.h"
#include "ui/base/page_transition_types.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

DEFINE_WEB_STATE_USER_DATA_KEY(InfoBarManagerImpl);

namespace {

infobars::InfoBarDelegate::NavigationDetails
NavigationDetailsFromLoadCommittedDetails(
    const web::LoadCommittedDetails& load_details) {
  infobars::InfoBarDelegate::NavigationDetails navigation_details;
  navigation_details.entry_id = load_details.item->GetUniqueID();
  const ui::PageTransition transition = load_details.item->GetTransitionType();
  navigation_details.is_navigation_to_different_page =
      ui::PageTransitionIsMainFrame(transition) && !load_details.is_in_page;
  // web::LoadCommittedDetails doesn't store this information, default to false.
  navigation_details.did_replace_entry = false;
  navigation_details.is_reload =
      ui::PageTransitionCoreTypeIs(transition, ui::PAGE_TRANSITION_RELOAD);
  navigation_details.is_redirect = ui::PageTransitionIsRedirect(transition);

  return navigation_details;
}

}  // namespace

InfoBarManagerImpl::InfoBarManagerImpl(web::WebState* web_state)
    : web_state_(web_state) {
  web_state_->AddObserver(this);
}

InfoBarManagerImpl::~InfoBarManagerImpl() {
  ShutDown();

  // As the object can commit suicide, it is possible that its destructor
  // is called before WebStateDestroyed. In that case stop observing the
  // WebState.
  if (web_state_) {
    web_state_->RemoveObserver(this);
    web_state_ = nullptr;
  }
}

int InfoBarManagerImpl::GetActiveEntryID() {
  web::NavigationItem* visible_item =
      web_state_->GetNavigationManager()->GetVisibleItem();
  return visible_item ? visible_item->GetUniqueID() : 0;
}

std::unique_ptr<infobars::InfoBar> InfoBarManagerImpl::CreateConfirmInfoBar(
    std::unique_ptr<ConfirmInfoBarDelegate> delegate) {
  return ::CreateConfirmInfoBar(std::move(delegate));
}

void InfoBarManagerImpl::NavigationItemCommitted(
    web::WebState* web_state,
    const web::LoadCommittedDetails& load_details) {
  DCHECK_EQ(web_state_, web_state);
  OnNavigation(NavigationDetailsFromLoadCommittedDetails(load_details));
}

void InfoBarManagerImpl::WebStateDestroyed(web::WebState* web_state) {
  DCHECK_EQ(web_state_, web_state);
  // The WebState is going away; be aggressively paranoid and delete this
  // InfoBarManagerImpl lest other parts of the system attempt to add infobars
  // or use it otherwise during the destruction. As this is the equivalent of
  // "delete this", returning from this function is the only safe thing to do.
  web_state_->RemoveUserData(UserDataKey());
}

void InfoBarManagerImpl::OpenURL(const GURL& url,
                                 WindowOpenDisposition disposition) {
  web::WebState::OpenURLParams params(url, web::Referrer(), disposition,
                                      ui::PAGE_TRANSITION_LINK,
                                      /*is_renderer_initiated=*/false);
  web_state_->OpenURL(params);
}
