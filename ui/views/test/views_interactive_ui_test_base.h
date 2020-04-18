// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_TEST_VIEWS_INTERACTIVE_UI_TEST_BASE_H_
#define UI_VIEWS_TEST_VIEWS_INTERACTIVE_UI_TEST_BASE_H_

#include "ui/views/test/views_test_base.h"

namespace views {

// A ViewsTestBase setups for interactive uitests.
class ViewsInteractiveUITestBase : public ViewsTestBase {
 public:
  ViewsInteractiveUITestBase();
  ~ViewsInteractiveUITestBase() override;

  static void InteractiveSetUp();

  // ViewsTestBase:
  void SetUp() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ViewsInteractiveUITestBase);
};

}  // namespace views

#endif  // UI_VIEWS_TEST_VIEWS_INTERACTIVE_UI_TEST_BASE_H_
