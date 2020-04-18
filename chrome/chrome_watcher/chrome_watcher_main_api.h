// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_CHROME_WATCHER_CHROME_WATCHER_MAIN_API_H_
#define CHROME_CHROME_WATCHER_CHROME_WATCHER_MAIN_API_H_

#include <Windows.h>
#include "base/files/file_path.h"
#include "base/process/process_handle.h"
#include "base/strings/string16.h"

// The name of the watcher DLL.
extern const base::FilePath::CharType kChromeWatcherDll[];
// The name of the watcher DLLs entrypoint function.
extern const char kChromeWatcherDLLEntrypoint[];
// The subdirectory of the browser data directory where permanently failed crash
// reports will be stored.
extern const base::FilePath::CharType kPermanentlyFailedReportsSubdir[];

// The type of the watcher DLL's main entry point.
// Watches |parent_process|, whose main thread ID is |main_thread_id|, and
// records its exit code under |registry_path| in HKCU. The Chrome message
// window, owned by |parent_process|, will be monitored for responsiveness.
// |on_initialized_event| will be signaled once the watcher process is fully
// initialized. Takes ownership of |parent_process| and |on_initialized_event|.
typedef int (*ChromeWatcherMainFunction)(
    const base::char16* registry_path,
    HANDLE parent_process,
    DWORD main_thread_id,
    HANDLE on_initialized_event,
    const base::char16* browser_data_directory);

#endif  // CHROME_CHROME_WATCHER_CHROME_WATCHER_MAIN_API_H_
