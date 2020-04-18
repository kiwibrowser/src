// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_TEST_CONTENT_BROWSER_TEST_H_
#define CONTENT_PUBLIC_TEST_CONTENT_BROWSER_TEST_H_

#include <memory>

#include "build/build_config.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_base.h"

#if defined(OS_MACOSX)
#include "base/mac/scoped_nsautorelease_pool.h"
#endif

namespace content {
class Shell;
class ShellMainDelegate;

// Base class for browser tests which use content_shell.
class ContentBrowserTest : public BrowserTestBase {
 protected:
  ContentBrowserTest();
  ~ContentBrowserTest() override;

  // testing::Test:
  void SetUp() override;
  void TearDown() override;

  // BrowserTestBase:
  void PreRunTestOnMainThread() override;
  void PostRunTestOnMainThread() override;

 protected:
  // Creates a new window and loads about:blank.
  Shell* CreateBrowser();

  // Creates an off-the-record window and loads about:blank.
  Shell* CreateOffTheRecordBrowser();

  // Returns the window for the test.
  Shell* shell() const { return shell_; }

 private:
  Shell* shell_;

#if defined(OS_MACOSX)
  // On Mac, without the following autorelease pool, code which is directly
  // executed (as opposed to executed inside a message loop) would autorelease
  // objects into a higher-level pool. This pool is not recycled in-sync with
  // the message loops' pools and causes problems with code relying on
  // deallocation via an autorelease pool (such as browser window closure and
  // browser shutdown). To avoid this, the following pool is recycled after each
  // time code is directly executed.
  base::mac::ScopedNSAutoreleasePool* pool_ = nullptr;
#endif

#if defined(OS_ANDROID)
  // For all other platforms, this is done automatically when calling into
  // ContentMain. For Android we set things up manually.
  std::unique_ptr<ShellMainDelegate> shell_main_delegate_;
#endif
};

}  // namespace content

#endif  // CONTENT_PUBLIC_TEST_CONTENT_BROWSER_TEST_H_
