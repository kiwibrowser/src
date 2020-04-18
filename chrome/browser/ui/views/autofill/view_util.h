// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_AUTOFILL_VIEW_UTIL_H_
#define CHROME_BROWSER_UI_VIEWS_AUTOFILL_VIEW_UTIL_H_

#include "ui/views/controls/textfield/textfield.h"

namespace autofill {

// Creates and returns a small Textfield intended to be used for CVC entry.
views::Textfield* CreateCvcTextfield();

}  // namespace autofill

#endif  // CHROME_BROWSER_UI_VIEWS_AUTOFILL_VIEW_UTIL_H_
