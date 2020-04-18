// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_VIEWS_CHROME_TEST_VIEWS_DELEGATE_H_
#define CHROME_TEST_VIEWS_CHROME_TEST_VIEWS_DELEGATE_H_

#include "base/macros.h"
#include "ui/views/test/test_views_delegate.h"

// A TestViewsDelegate specific to Chrome tests.
class ChromeTestViewsDelegate : public views::TestViewsDelegate {
 public:
  ChromeTestViewsDelegate();
  ~ChromeTestViewsDelegate() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ChromeTestViewsDelegate);
};

#endif  // CHROME_TEST_VIEWS_CHROME_TEST_VIEWS_DELEGATE_H_
