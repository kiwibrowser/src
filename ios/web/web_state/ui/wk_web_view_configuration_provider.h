// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_WEB_STATE_UI_WK_WEB_VIEW_CONFIGURATION_PROVIDER_H_
#define IOS_WEB_WEB_STATE_UI_WK_WEB_VIEW_CONFIGURATION_PROVIDER_H_

#include "base/macros.h"
#include "base/supports_user_data.h"

@class CRWWKScriptMessageRouter;
@class WKWebViewConfiguration;

namespace web {

class BrowserState;

// A provider class associated with a single web::BrowserState object. Manages
// the lifetime and performs setup of WKWebViewConfiguration and
// CRWWKScriptMessageRouter instances. Not threadsafe. Must be used only on the
// main thread.
class WKWebViewConfigurationProvider : public base::SupportsUserData::Data {
 public:
  ~WKWebViewConfigurationProvider() override;

  // Returns a provider for the given |browser_state|. Lazily attaches one if it
  // does not exist. |browser_state| can not be null.
  static web::WKWebViewConfigurationProvider& FromBrowserState(
      web::BrowserState* browser_state);

  // Returns an autoreleased shallow copy of WKWebViewConfiguration associated
  // with browser state. Lazily creates the config. Configuration's
  // |preferences| will have scriptCanOpenWindowsAutomatically property set to
  // YES.
  // Must be used instead of [[WKWebViewConfiguration alloc] init].
  // Callers must not retain the returned object.
  WKWebViewConfiguration* GetWebViewConfiguration();

  // Returns CRWWKScriptMessafeRouter associated with WKWebViewConfiguration.
  // Lazily creates the router. Callers must not retain the returned object
  // (this will be enforced in debug builds).
  CRWWKScriptMessageRouter* GetScriptMessageRouter();

  // Purges config and router objects if they exist. When this method is called
  // config and config's process pool must not be retained by anyone (this will
  // be enforced in debug builds).
  void Purge();

 private:
  explicit WKWebViewConfigurationProvider(BrowserState* browser_state);
  WKWebViewConfigurationProvider() = delete;

  WKWebViewConfiguration* configuration_;
  CRWWKScriptMessageRouter* router_;
  BrowserState* browser_state_;

  DISALLOW_COPY_AND_ASSIGN(WKWebViewConfigurationProvider);
};

}  // namespace web

#endif  // IOS_WEB_WEB_STATE_UI_WK_WEB_VIEW_CONFIGURATION_PROVIDER_H_
