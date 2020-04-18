// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/search/instant_test_utils.h"

namespace {

std::string WrapScript(const std::string& script) {
  return "domAutomationController.send(" + script + ")";
}

}  // namespace

namespace instant_test_utils {

bool GetBoolFromJS(const content::ToRenderFrameHost& adapter,
                   const std::string& script,
                   bool* result) {
  return content::ExecuteScriptAndExtractBool(adapter, WrapScript(script),
                                              result);
}

bool GetIntFromJS(const content::ToRenderFrameHost& adapter,
                  const std::string& script,
                  int* result) {
  return content::ExecuteScriptAndExtractInt(adapter, WrapScript(script),
                                             result);
}

bool GetDoubleFromJS(const content::ToRenderFrameHost& adapter,
                     const std::string& script,
                     double* result) {
  return content::ExecuteScriptAndExtractDouble(adapter, WrapScript(script),
                                                result);
}

bool GetStringFromJS(const content::ToRenderFrameHost& adapter,
                     const std::string& script,
                     std::string* result) {
  return content::ExecuteScriptAndExtractString(adapter, WrapScript(script),
                                                result);
}

}  // namespace instant_test_utils
