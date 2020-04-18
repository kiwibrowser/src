// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/autofill/form_input_accessory_view_tab_helper.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#import "ios/chrome/browser/autofill/form_input_accessory_view_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

DEFINE_WEB_STATE_USER_DATA_KEY(FormInputAccessoryViewTabHelper);

FormInputAccessoryViewTabHelper::~FormInputAccessoryViewTabHelper() = default;

// static
void FormInputAccessoryViewTabHelper::CreateForWebState(
    web::WebState* web_state,
    NSArray<id<FormInputAccessoryViewProvider>>* providers) {
  DCHECK(web_state);
  if (!FromWebState(web_state)) {
    web_state->SetUserData(UserDataKey(),
                           base::WrapUnique(new FormInputAccessoryViewTabHelper(
                               web_state, providers)));
  }
}

void FormInputAccessoryViewTabHelper::CloseKeyboard() {
  [controller_ closeKeyboardWithoutButtonPress];
}

FormInputAccessoryViewTabHelper::FormInputAccessoryViewTabHelper(
    web::WebState* web_state,
    NSArray<id<FormInputAccessoryViewProvider>>* providers)
    : controller_([[FormInputAccessoryViewController alloc]
          initWithWebState:web_state
                 providers:providers]) {
  web_state->AddObserver(this);
}

void FormInputAccessoryViewTabHelper::WasShown(web::WebState* web_state) {
  [controller_ wasShown];
}

void FormInputAccessoryViewTabHelper::WasHidden(web::WebState* web_state) {
  [controller_ wasHidden];
}

void FormInputAccessoryViewTabHelper::WebStateDestroyed(
    web::WebState* web_state) {
  [controller_ detachFromWebState];
  web_state->RemoveObserver(this);
  controller_ = nil;
}
