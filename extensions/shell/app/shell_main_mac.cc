// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/shell/app/shell_main_mac.h"

#include "content/public/app/content_main.h"
#include "extensions/shell/app/shell_main_delegate.h"

int ContentMain(int argc, const char** argv) {
  extensions::ShellMainDelegate delegate;
  content::ContentMainParams params(&delegate);
  params.argc = argc;
  params.argv = argv;
  return content::ContentMain(params);
}
