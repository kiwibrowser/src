// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/interstitials/native_web_interstitial_impl.h"

#include <utility>

#include "base/logging.h"
#import "ios/web/public/interstitials/web_interstitial_delegate.h"
#import "ios/web/public/web_state/ui/crw_generic_content_view.h"
#import "ios/web/web_state/web_state_impl.h"
#include "ui/gfx/geometry/size.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

// static
WebInterstitial* WebInterstitial::CreateNativeInterstitial(
    WebState* web_state,
    bool new_navigation,
    const GURL& url,
    std::unique_ptr<NativeWebInterstitialDelegate> delegate) {
  WebStateImpl* web_state_impl = static_cast<WebStateImpl*>(web_state);
  return new NativeWebInterstitialImpl(web_state_impl, new_navigation, url,
                                       std::move(delegate));
}

NativeWebInterstitialImpl::NativeWebInterstitialImpl(
    WebStateImpl* web_state,
    bool new_navigation,
    const GURL& url,
    std::unique_ptr<NativeWebInterstitialDelegate> delegate)
    : web::WebInterstitialImpl(web_state, new_navigation, url),
      delegate_(std::move(delegate)) {
  DCHECK(delegate_);
}

NativeWebInterstitialImpl::~NativeWebInterstitialImpl() {
}

CRWContentView* NativeWebInterstitialImpl::GetContentView() const {
  return content_view_;
}

void NativeWebInterstitialImpl::PrepareForDisplay() {
  if (!content_view_) {
    content_view_ = [[CRWGenericContentView alloc]
        initWithView:delegate_->GetContentView()];
  }
}

WebInterstitialDelegate* NativeWebInterstitialImpl::GetDelegate() const {
  return delegate_.get();
}

void NativeWebInterstitialImpl::ExecuteJavaScript(
    NSString* script,
    JavaScriptResultBlock completion_handler) {
  NOTREACHED() << "JavaScript cannot be executed on native interstitials.";
}

}  // namespace web
