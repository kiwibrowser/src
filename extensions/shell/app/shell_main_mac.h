// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_SHELL_APP_SHELL_MAIN_MAC_H_
#define EXTENSIONS_SHELL_APP_SHELL_MAIN_MAC_H_

#include "build/build_config.h"

extern "C" {
__attribute__((visibility("default"))) int ContentMain(int argc,
                                                       const char** argv);
}  // extern "C"

#endif  // EXTENSIONS_SHELL_APP_SHELL_MAIN_MAC_H_

