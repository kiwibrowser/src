// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Defines all the "content_shell" command-line switches.

#ifndef CONTENT_SHELL_COMMON_SHELL_SWITCHES_H_
#define CONTENT_SHELL_COMMON_SHELL_SWITCHES_H_

#include <string>
#include <vector>

namespace switches {

extern const char kContentBrowserTest[];
extern const char kContentShellDataPath[];
extern const char kCrashDumpsDir[];
extern const char kExposeInternalsForTesting[];
extern const char kIsolateSitesForTesting[];
extern const char kRegisterFontFiles[];
extern const char kContentShellHostWindowSize[];
extern const char kContentShellHideToolbar[];
extern const char kContentShellAlwaysFork[];

// Returns list of extra font files to be made accessible to the renderer.
std::vector<std::string> GetSideloadFontFiles();

// Tells if content shell is running web_tests.
// TODO(lukasza): The function below somewhat violates the layering (by
// enabling shell -> layout_tests dependency) but at least narrows the extent of
// the dependency to a single switch...
bool IsRunWebTestsSwitchPresent();

}  // namespace switches

#endif  // CONTENT_SHELL_COMMON_SHELL_SWITCHES_H_
