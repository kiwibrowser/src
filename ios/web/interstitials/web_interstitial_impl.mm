// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/interstitials/web_interstitial_impl.h"

#include "base/logging.h"
#import "ios/web/navigation/navigation_manager_impl.h"
#import "ios/web/public/interstitials/web_interstitial_delegate.h"
#import "ios/web/public/navigation_manager.h"
#include "ios/web/public/reload_type.h"
#import "ios/web/web_state/web_state_impl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

WebInterstitialImpl::WebInterstitialImpl(WebStateImpl* web_state,
                                         bool new_navigation,
                                         const GURL& url)
    : web_state_(web_state),
      navigation_manager_(&web_state->GetNavigationManagerImpl()),
      url_(url),
      new_navigation_(new_navigation),
      action_taken_(false) {
  DCHECK(web_state_);
  web_state_->AddObserver(this);
}

WebInterstitialImpl::~WebInterstitialImpl() {
  Hide();
  if (web_state_) {
    web_state_->RemoveObserver(this);
    web_state_ = nullptr;
  }
}

const GURL& WebInterstitialImpl::GetUrl() const {
  return url_;
}

void WebInterstitialImpl::Show() {
  PrepareForDisplay();
  GetWebStateImpl()->ShowWebInterstitial(this);

  if (new_navigation_) {
    // TODO(crbug.com/706578): Plumb transient entry handling through
    // NavigationManager, and remove the NavigationManagerImpl usage here.
    navigation_manager_->AddTransientItem(url_);

    // Give delegates a chance to set some states on the navigation item.
    GetDelegate()->OverrideItem(navigation_manager_->GetTransientItem());

    web_state_->DidChangeVisibleSecurityState();
  }
}

void WebInterstitialImpl::Hide() {
  GetWebStateImpl()->ClearTransientContent();
}

void WebInterstitialImpl::DontProceed() {
  // Proceed() and DontProceed() are not re-entrant, as they delete |this|.
  if (action_taken_)
    return;
  action_taken_ = true;

  // Clear the pending entry, since that's the page that's not being
  // proceeded to.
  GetWebStateImpl()->GetNavigationManager()->DiscardNonCommittedItems();

  Hide();

  GetDelegate()->OnDontProceed();

  delete this;
}

void WebInterstitialImpl::Proceed() {
  // Proceed() and DontProceed() are not re-entrant, as they delete |this|.
  if (action_taken_)
    return;
  action_taken_ = true;
  Hide();
  GetDelegate()->OnProceed();
  delete this;
}

void WebInterstitialImpl::WebStateDestroyed(WebState* web_state) {
  DCHECK_EQ(web_state_, web_state);
  // There is no need to remove the current instance from WebState's observer
  // as DontProceed() delete "this" and the removal is done in the destructor.
  // In addition since the current instance has been deleted, "this" should no
  // longer be used after the method call.
  DontProceed();
}

WebStateImpl* WebInterstitialImpl::GetWebStateImpl() const {
  return web_state_;
}

}  // namespace web
