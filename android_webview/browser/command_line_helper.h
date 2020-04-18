// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_COMMAND_LINE_HELPER_H_
#define ANDROID_WEBVIEW_BROWSER_COMMAND_LINE_HELPER_H_

#include <string>

#include "base/macros.h"

namespace base {
class CommandLine;
}  // namespace base

class CommandLineHelper {
 public:
  // Add a feature to the --enabled-features list, but only if it's not
  // already enabled or disabled.
  static void AddEnabledFeature(base::CommandLine& command_line,
                                const std::string& feature_name);

  // Add a feature to the --disabled-features list, but only if it's not already
  // enabled or disabled.
  static void AddDisabledFeature(base::CommandLine& command_line,
                                 const std::string& feature_name);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(CommandLineHelper);
};

#endif  // ANDROID_WEBVIEW_BROWSER_COMMAND_LINE_HELPER_H_
