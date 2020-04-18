// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_INTERSTITIALS_NATIVE_WEB_INTERSTITIAL_IMPL_H_
#define IOS_WEB_INTERSTITIALS_NATIVE_WEB_INTERSTITIAL_IMPL_H_

#import "ios/web/interstitials/web_interstitial_impl.h"

#include <memory>


namespace web {

class NativeWebInterstitialDelegate;

// A concrete subclass of WebInterstitialImpl that is used to display
// interstitials created via native views.
class NativeWebInterstitialImpl : public WebInterstitialImpl {
 public:
  NativeWebInterstitialImpl(
      WebStateImpl* web_state,
      bool new_navigation,
      const GURL& url,
      std::unique_ptr<NativeWebInterstitialDelegate> delegate);
  ~NativeWebInterstitialImpl() override;

  // WebInterstitialImpl implementation:
  CRWContentView* GetContentView() const override;

 protected:
  // WebInterstitialImpl implementation:
  void PrepareForDisplay() override;
  WebInterstitialDelegate* GetDelegate() const override;
  void ExecuteJavaScript(NSString* script,
                         JavaScriptResultBlock completion_handler) override;

 private:
  // The native interstitial delegate.
  std::unique_ptr<NativeWebInterstitialDelegate> delegate_;
  // The transient content view containing interstitial content.
  CRWContentView* content_view_;
};

}  // namespace web

#endif  // IOS_WEB_INTERSTITIALS_NATIVE_WEB_INTERSTITIAL_IMPL_H_
