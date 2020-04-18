// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_DATA_WEBUI_SIGNIN_BROWSERTEST_H_
#define CHROME_TEST_DATA_WEBUI_SIGNIN_BROWSERTEST_H_

#include "base/macros.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/test/base/web_ui_browser_test.h"
#include "components/signin/core/browser/scoped_account_consistency.h"

class SigninBrowserTest : public WebUIBrowserTest {
 public:
  SigninBrowserTest();
  ~SigninBrowserTest() override;

 protected:
  void EnableDice();
  void EnableUnity();

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
  std::unique_ptr<signin::ScopedAccountConsistency> scoped_account_consistency_;

  DISALLOW_COPY_AND_ASSIGN(SigninBrowserTest);
};

#endif  // CHROME_TEST_DATA_WEBUI_SIGNIN_BROWSERTEST_H_
