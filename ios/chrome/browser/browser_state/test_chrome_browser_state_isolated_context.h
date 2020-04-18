// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_BROWSER_STATE_TEST_CHROME_BROWSER_STATE_ISOLATED_CONTEXT_H_
#define IOS_CHROME_BROWSER_BROWSER_STATE_TEST_CHROME_BROWSER_STATE_ISOLATED_CONTEXT_H_

#include <memory>

#include "base/macros.h"
#include "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#import "ios/public/provider/chrome/browser/ui/default_ios_web_view_factory.h"

// A TestChromeBrowserState with an isolated URLRequestContext.
class TestChromeBrowserStateWithIsolatedContext
    : public TestChromeBrowserState {
 public:
  TestChromeBrowserStateWithIsolatedContext();
  ~TestChromeBrowserStateWithIsolatedContext() override;

  bool MainContextCalled() const;
  IOSWebViewFactoryExternalService SharingService();

  static std::unique_ptr<TestChromeBrowserStateWithIsolatedContext> Build();

  // ChromeBrowserState:
  net::URLRequestContextGetter* GetRequestContext() override;
  net::URLRequestContextGetter* CreateIsolatedRequestContext(
      const base::FilePath& partition_path) override;

 private:
  bool main_context_called_;
  scoped_refptr<net::URLRequestContextGetter> request_context_;

  DISALLOW_COPY_AND_ASSIGN(TestChromeBrowserStateWithIsolatedContext);
};

#endif  // IOS_CHROME_BROWSER_BROWSER_STATE_TEST_CHROME_BROWSER_STATE_ISOLATED_CONTEXT_H_
