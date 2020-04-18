// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/public/web_state/web_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

WebState::CreateParams::CreateParams(web::BrowserState* browser_state)
    : browser_state(browser_state), created_with_opener(false) {}

WebState::CreateParams::~CreateParams() {}

WebState::OpenURLParams::OpenURLParams(const GURL& url,
                                       const Referrer& referrer,
                                       WindowOpenDisposition disposition,
                                       ui::PageTransition transition,
                                       bool is_renderer_initiated)
    : url(url),
      referrer(referrer),
      disposition(disposition),
      transition(transition),
      is_renderer_initiated(is_renderer_initiated) {}

WebState::OpenURLParams::~OpenURLParams() {}

}  // namespace web
