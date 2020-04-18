// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_MIXIN_BASED_BROWSER_TEST_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_MIXIN_BASED_BROWSER_TEST_H_

#include <memory>
#include <vector>

#include "chrome/test/base/in_process_browser_test.h"

namespace chromeos {

class MixinBasedBrowserTest : public InProcessBrowserTest {
 public:
  // A class that can be used to add some features not related directly to the
  // testing process in order not to make the test class too complicated and to
  // set up them in a proper time (at the same time when the corresponding set
  // ups for the main test are run).
  //
  // To used this you need to derive a class from from
  // MixinBasedBrowserTest::Mixin, e.g. MixinYouWantToUse, and declare all
  // the methods you'd like in this new class. You also can reload setups and
  // teardowns if you need so. Test which wants to use some mixin should call
  // AddMixin(mixin_) from its constructor, where mixin_ should be an instance
  // of MixinYouWantToUse.
  //
  // All methods in Mixin are complete analogs of those in InProcessBrowserTest,
  // so if some usecases are unclear, take a look at in_process_browser_test.h
  class Mixin {
   public:
    Mixin() {}
    virtual ~Mixin() {}

    // Is called before creating the browser and running
    // SetUpInProcessBrowserTestFixture.
    // Should be used for setting up the command line.
    virtual void SetUpCommandLine(base::CommandLine* command_line) {}

    // Is called before creating the browser.
    // Should be used to set up the environment for running the browser.
    virtual void SetUpInProcessBrowserTestFixture() {}

    // Is called after creating the browser and before executing test code.
    // Should be used for setting up things related to the browser object.
    virtual void SetUpOnMainThread() {}

    // Is called after executing the test code and before the browser is torn
    // down.
    // Should be used to do the necessary cleanup on the working browser.
    virtual void TearDownOnMainThread() {}

    // Is called after the browser is torn down.
    // Should be used to do the remaining cleanup.
    virtual void TearDownInProcessBrowserTestFixture() {}
  };

  MixinBasedBrowserTest();
  ~MixinBasedBrowserTest() override;

  // Override from InProcessBrowserTest.
  void SetUpCommandLine(base::CommandLine* command_line) override;
  void SetUpInProcessBrowserTestFixture() override;
  void SetUpOnMainThread() override;
  void TearDownOnMainThread() override;
  void TearDownInProcessBrowserTestFixture() override;

 protected:
  // Adds |mixin| as an mixin for this test, passing ownership
  // for it to MixinBasedBrowserTest.
  // Should be called in constructor of the test (should be already completed
  // before running set ups).
  void AddMixin(std::unique_ptr<Mixin> mixin);

 private:
  // Keeps all the mixins for this test,
  std::vector<std::unique_ptr<Mixin>> mixins_;

  // Is false initially, becomes true when any of SetUp* methods is called.
  // Required to check that AddMixin is always called before setting up.
  bool setup_was_launched_;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_MIXIN_BASED_BROWSER_TEST_H_
