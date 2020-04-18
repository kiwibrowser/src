// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file defines helper functions for accessing the clipboard.

#ifndef CHROME_BROWSER_UI_OMNIBOX_CLIPBOARD_UTILS_H_
#define CHROME_BROWSER_UI_OMNIBOX_CLIPBOARD_UTILS_H_

#include "base/strings/string16.h"

// Returns the current clipboard contents as a string that can be pasted in.
// In addition to just getting CF_UNICODETEXT out, this can also extract URLs
// from bookmarks on the clipboard.
base::string16 GetClipboardText();

#endif  // CHROME_BROWSER_UI_OMNIBOX_CLIPBOARD_UTILS_H_
