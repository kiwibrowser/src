// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_SEARCH_INSTANT_TEST_UTILS_H_
#define CHROME_BROWSER_UI_SEARCH_INSTANT_TEST_UTILS_H_

#include <string>

#include "base/macros.h"
#include "content/public/test/browser_test_utils.h"

namespace instant_test_utils {

bool GetBoolFromJS(const content::ToRenderFrameHost& adapter,
                   const std::string& script,
                   bool* result) WARN_UNUSED_RESULT;
bool GetIntFromJS(const content::ToRenderFrameHost& adapter,
                  const std::string& script,
                  int* result) WARN_UNUSED_RESULT;
bool GetDoubleFromJS(const content::ToRenderFrameHost& adapter,
                     const std::string& script,
                     double* result) WARN_UNUSED_RESULT;
bool GetStringFromJS(const content::ToRenderFrameHost& adapter,
                     const std::string& script,
                     std::string* result) WARN_UNUSED_RESULT;

}  // namespace instant_test_utils

#endif  // CHROME_BROWSER_UI_SEARCH_INSTANT_TEST_UTILS_H_
