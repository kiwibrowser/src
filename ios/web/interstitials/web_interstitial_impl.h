// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_INTERSTITIALS_WEB_INTERSTITIAL_IMPL_H_
#define IOS_WEB_INTERSTITIALS_WEB_INTERSTITIAL_IMPL_H_

#import <UIKit/UIKit.h>

#include "ios/web/public/interstitials/web_interstitial.h"
#import "ios/web/public/web_state/ui/crw_content_view.h"
#include "ios/web/public/web_state/web_state_observer.h"
#import "ios/web/web_state/ui/web_view_js_utils.h"
#include "url/gurl.h"

namespace web {

class NavigationManagerImpl;
class WebInterstitialDelegate;
class WebInterstitialImpl;
class WebStateImpl;

// May be implemented in tests to run JavaScript on interstitials. This function
// has access to private ExecuteJavaScript method to be used for testing.
void ExecuteScriptForTesting(WebInterstitialImpl*,
                             NSString*,
                             JavaScriptResultBlock);

// An abstract subclass of WebInterstitial that exposes the views necessary to
// embed the interstitial into a WebState.
class WebInterstitialImpl : public WebInterstitial, public WebStateObserver {
 public:
  WebInterstitialImpl(WebStateImpl* web_state,
                      bool new_navigation,
                      const GURL& url);
  ~WebInterstitialImpl() override;

  // Returns the transient content view used to display interstitial content.
  virtual CRWContentView* GetContentView() const = 0;

  // Returns the url corresponding to this interstitial.
  const GURL& GetUrl() const;

  // WebInterstitial implementation:
  void Show() override;
  void Hide() override;
  void DontProceed() override;
  void Proceed() override;

  // WebStateObserver implementation:
  void WebStateDestroyed(WebState* web_state) override;

 protected:
  // Called before the WebInterstitialImpl is shown, giving subclasses a chance
  // to instantiate its view.
  virtual void PrepareForDisplay() {}

  // Returns the WebInterstitialDelegate that will handle Proceed/DontProceed
  // user actions.
  virtual WebInterstitialDelegate* GetDelegate() const = 0;

  // Convenience method for getting the WebStateImpl.
  WebStateImpl* GetWebStateImpl() const;

  // Executes the given |script| on interstitial's web view if there is one.
  // Calls |completionHandler| with results of the evaluation.
  // The |completionHandler| can be nil. Must be used only for testing.
  virtual void ExecuteJavaScript(NSString* script,
                                 JavaScriptResultBlock completion_handler) = 0;

 private:
  // The WebState this instance is observing. Will be null after
  // WebStateDestroyed has been called.
  WebStateImpl* web_state_ = nullptr;

  // The navigation manager corresponding to the WebState the interstiatial was
  // created for.
  NavigationManagerImpl* navigation_manager_;
  // The URL corresponding to the page that resulted in this interstitial.
  GURL url_;
  // Whether or not to create a new transient entry on display.
  bool new_navigation_;
  // Whether or not either Proceed() or DontProceed() has been called.
  bool action_taken_;

  // Must be implemented only for testing purposes.
  friend void web::ExecuteScriptForTesting(WebInterstitialImpl*,
                                           NSString*,
                                           JavaScriptResultBlock);
};

}  // namespace web

#endif  // IOS_WEB_INTERSTITIALS_WEB_INTERSTITIAL_IMPL_H_
