// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ssl/insecure_input_tab_helper.h"

#import <Foundation/Foundation.h>

#include <memory>

#include "base/logging.h"
#include "components/security_state/ios/ssl_status_input_event_data.h"
#import "ios/web/public/navigation_item.h"
#import "ios/web/public/navigation_manager.h"
#import "ios/web/public/origin_util.h"
#include "ios/web/public/web_state/form_activity_params.h"
#import "ios/web/public/web_state/web_state.h"
#import "ios/web/public/web_state/web_state_user_data.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Creates or retrieves the |user_data| object in the SSLStatus attached to the
// WebState's NavigationItem.
security_state::SSLStatusInputEventData* GetOrCreateSSLStatusInputEventData(
    web::WebState* web_state) {
  web::NavigationItem* item =
      web_state->GetNavigationManager()->GetLastCommittedItem();

  // We aren't guaranteed to always have a navigation item.
  if (!item)
    return nullptr;

  web::SSLStatus& ssl = item->GetSSL();
  security_state::SSLStatusInputEventData* input_events =
      static_cast<security_state::SSLStatusInputEventData*>(
          ssl.user_data.get());
  if (!input_events) {
    ssl.user_data = std::make_unique<security_state::SSLStatusInputEventData>();
    input_events = static_cast<security_state::SSLStatusInputEventData*>(
        ssl.user_data.get());
  }
  return input_events;
}

}  // namespace

DEFINE_WEB_STATE_USER_DATA_KEY(InsecureInputTabHelper);

InsecureInputTabHelper::~InsecureInputTabHelper() = default;

// static
InsecureInputTabHelper* InsecureInputTabHelper::GetOrCreateForWebState(
    web::WebState* web_state) {
  InsecureInputTabHelper* helper = FromWebState(web_state);
  if (!helper) {
    CreateForWebState(web_state);
    helper = FromWebState(web_state);
    DCHECK(helper);
  }
  return helper;
}

void InsecureInputTabHelper::DidShowPasswordFieldInInsecureContext() {
  DCHECK(!web::IsOriginSecure(web_state_->GetLastCommittedURL()));

  security_state::SSLStatusInputEventData* input_events =
      GetOrCreateSSLStatusInputEventData(web_state_);
  if (!input_events)
    return;

  // If the first password field for the web contents was just
  // shown, update the SSLStatusInputEventData.
  if (!input_events->input_events()->password_field_shown) {
    input_events->input_events()->password_field_shown = true;
    web_state_->DidChangeVisibleSecurityState();
  }
}

void InsecureInputTabHelper::DidInteractWithNonsecureCreditCardInput() {
  DCHECK(!web::IsOriginSecure(web_state_->GetLastCommittedURL()));

  security_state::SSLStatusInputEventData* input_events =
      GetOrCreateSSLStatusInputEventData(web_state_);
  if (!input_events)
    return;

  // If the first credit card field for the web contents was just
  // shown, update the SSLStatusInputEventData.
  if (!input_events->input_events()->credit_card_field_edited) {
    input_events->input_events()->credit_card_field_edited = true;
    web_state_->DidChangeVisibleSecurityState();
  }
}

void InsecureInputTabHelper::DidEditFieldInInsecureContext() {
  DCHECK(!web::IsOriginSecure(web_state_->GetLastCommittedURL()));

  security_state::SSLStatusInputEventData* input_events =
      GetOrCreateSSLStatusInputEventData(web_state_);
  if (!input_events)
    return;

  // If the first field edit in the web contents was just performed,
  // update the SSLStatusInputEventData.
  if (!input_events->input_events()->insecure_field_edited) {
    input_events->input_events()->insecure_field_edited = true;
    web_state_->DidChangeVisibleSecurityState();
  }
}

InsecureInputTabHelper::InsecureInputTabHelper(web::WebState* web_state)
    : web_state_(web_state) {
  web_state_->AddObserver(this);
}

void InsecureInputTabHelper::FormActivityRegistered(
    web::WebState* web_state,
    const web::FormActivityParams& params) {
  DCHECK_EQ(web_state_, web_state);
  if (params.type == "input" &&
      !web::IsOriginSecure(web_state->GetLastCommittedURL())) {
    DidEditFieldInInsecureContext();
  }
}

void InsecureInputTabHelper::WebStateDestroyed(web::WebState* web_state) {
  DCHECK_EQ(web_state_, web_state);
  web_state_->RemoveObserver(this);
  web_state_ = nullptr;
}
