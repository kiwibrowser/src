// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_BASE_INTERACTIVE_TEST_UTILS_COCOA_H_
#define CHROME_TEST_BASE_INTERACTIVE_TEST_UTILS_COCOA_H_

#include "chrome/browser/ui/view_ids.h"

class Browser;

namespace ui_test_utils {
namespace internal {

// Returns true of |vid| in |browser| is focused.
bool IsViewFocusedCocoa(const Browser* browser, ViewID vid);

// Simulates a mouse click on |vid| in |browser|.
void ClickOnViewCocoa(const Browser* browser, ViewID vid);

// Moves focus to |vid| in |browser|
void FocusViewCocoa(const Browser* browser, ViewID vid);

}  // namespace internal
}  // namespace ui_test_utils

#endif  // CHROME_TEST_BASE_INTERACTIVE_TEST_UTILS_COCOA_H_
