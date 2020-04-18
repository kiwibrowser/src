// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/web/print_tab_helper.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/memory/ptr_util.h"
#include "base/values.h"
#include "ios/chrome/browser/web/web_state_printer.h"
#import "ios/web/public/web_state/web_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Prefix for print JavaScript command.
const char kPrintCommandPrefix[] = "print";
}

DEFINE_WEB_STATE_USER_DATA_KEY(PrintTabHelper);

// static
void PrintTabHelper::CreateForWebState(web::WebState* web_state,
                                       id<WebStatePrinter> printer) {
  DCHECK(web_state);
  if (!FromWebState(web_state)) {
    web_state->SetUserData(UserDataKey(), base::WrapUnique(new PrintTabHelper(
                                              web_state, printer)));
  }
}

PrintTabHelper::~PrintTabHelper() = default;

PrintTabHelper::PrintTabHelper(web::WebState* web_state,
                               id<WebStatePrinter> printer)
    : printer_(printer) {
  DCHECK(printer);
  web_state->AddObserver(this);
  web_state->AddScriptCommandCallback(
      base::Bind(&PrintTabHelper::OnPrintCommand, base::Unretained(this),
                 base::Unretained(web_state)),
      kPrintCommandPrefix);
}

void PrintTabHelper::WebStateDestroyed(web::WebState* web_state) {
  // Stops handling print requests from the web page.
  web_state->RemoveScriptCommandCallback(kPrintCommandPrefix);
  web_state->RemoveObserver(this);
}

bool PrintTabHelper::OnPrintCommand(web::WebState* web_state,
                                    const base::DictionaryValue& command,
                                    const GURL& page_url,
                                    bool user_initiated) {
  DCHECK(web_state);
  [printer_ printWebState:web_state];
  return true;
}
