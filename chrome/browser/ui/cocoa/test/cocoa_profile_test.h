// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_TEST_COCOA_PROFILE_TEST_H_
#define CHROME_BROWSER_UI_COCOA_TEST_COCOA_PROFILE_TEST_H_

#include <memory>

#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "chrome/test/base/testing_profile_manager.h"

namespace content {
class TestBrowserThreadBundle;
}

class Browser;
class TestingProfile;

// Base class which contains a valid Browser*.  Lots of boilerplate to
// recycle between unit test classes.
//
// This class creates fake UI, file, and IO threads because many objects that
// are attached to the TestingProfile (and other objects) have traits that limit
// their destruction to certain threads. For example, the net::URLRequestContext
// can only be deleted on the IO thread; without this fake IO thread, the object
// would never be deleted and would report as a leak under Valgrind. Note that
// these are fake threads and they all share the same MessageLoop.
//
// TODO(jrg): move up a level (chrome/browser/ui/cocoa -->
// chrome/browser), and use in non-Mac unit tests such as
// back_forward_menu_model_unittest.cc,
// navigation_controller_unittest.cc, ..
class CocoaProfileTest : public CocoaTest {
 public:
  CocoaProfileTest();
  ~CocoaProfileTest() override;

  // This constructs a a Browser and a TestingProfile. It is guaranteed to
  // succeed, else it will ASSERT and cause the test to fail. Subclasses that
  // do work in SetUp should ASSERT that either browser() or profile() are
  // non-NULL before proceeding after the call to super (this).
  void SetUp() override;

  void TearDown() override;

  TestingProfileManager* testing_profile_manager() { return &profile_manager_; }
  TestingProfile* profile() { return profile_; }
  Browser* browser() { return browser_.get(); }

  // Closes the window for this browser. This will automatically be called as
  // part of TearDown() if it's not been done already.
  void CloseBrowserWindow();

 protected:
  // Overridden by test subclasses to create their own browser, e.g. with a
  // test window.
  virtual Browser* CreateBrowser();

  // Define the TestingFactories to be used when SetUp() builds a Profile. To be
  // called in the subclass' constructor.
  void AddTestingFactories(
      const TestingProfile::TestingFactories& testing_factories);

  const TestingProfile::TestingFactories& testing_factories() {
    return testing_factories_;
  }

 private:
  TestingProfileManager profile_manager_;
  TestingProfile* profile_;  // Weak; owned by profile_manager_.
  TestingProfile::TestingFactories testing_factories_;
  std::unique_ptr<Browser> browser_;

  std::unique_ptr<content::TestBrowserThreadBundle> thread_bundle_;
};

#endif  // CHROME_BROWSER_UI_COCOA_TEST_COCOA_PROFILE_TEST_H_
