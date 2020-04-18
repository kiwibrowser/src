// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/chrome_watcher/chrome_watcher_main_api.h"
#include "base/strings/string_number_conversions.h"

const base::FilePath::CharType kChromeWatcherDll[] =
    FILE_PATH_LITERAL("chrome_watcher.dll");
const char kChromeWatcherDLLEntrypoint[] = "WatcherMain";
const base::FilePath::CharType kPermanentlyFailedReportsSubdir[] =
    L"Crash Reports Fallback";
