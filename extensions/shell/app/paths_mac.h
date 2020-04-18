// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_SHELL_APP_PATHS_MAC_H_
#define EXTENSIONS_SHELL_APP_PATHS_MAC_H_

namespace extensions {

// Override the framework bundle path.
void OverrideFrameworkBundlePath();

// Override the helper (child process) path.
void OverrideChildProcessFilePath();

}  // namespace extensions

#endif  // EXTENSIONS_SHELL_APP_PATHS_MAC_H_
