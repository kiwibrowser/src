// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_AW_BROWSER_TERMINATOR_H_
#define ANDROID_WEBVIEW_BROWSER_AW_BROWSER_TERMINATOR_H_

#include <map>

#include "base/synchronization/lock.h"
#include "components/crash/content/browser/crash_dump_manager_android.h"
#include "components/crash/content/browser/crash_dump_observer_android.h"

namespace base {
class SyncSocket;
}

namespace android_webview {

// This class manages behavior of the browser on renderer crashes when
// microdumps are used for capturing the crash stack.  Normally, in this case
// the browser doesn't need to do much, because a microdump is written into
// Android log by the renderer process itself.  However, the browser may need to
// crash itself on a renderer crash.  Since on Android renderers are not child
// processes of the browser, it can't access the exit code. Instead, the browser
// uses a dedicated pipe in order to receive the information about the renderer
// crash status.
class AwBrowserTerminator : public breakpad::CrashDumpObserver::Client {
 public:
  AwBrowserTerminator(base::FilePath crash_dump_dir);
  ~AwBrowserTerminator() override;

  // breakpad::CrashDumpObserver::Client implementation.
  void OnChildStart(int process_host_id,
                    content::PosixFileDescriptorInfo* mappings) override;
  void OnChildExit(
      const breakpad::CrashDumpObserver::TerminationInfo& info) override;

 private:
  static void OnChildExitAsync(
      const breakpad::CrashDumpObserver::TerminationInfo& info,
      base::FilePath crash_dump_dir,
      std::unique_ptr<base::SyncSocket> pipe);

  base::FilePath crash_dump_dir_;

  // This map should only be accessed with its lock aquired as it is accessed
  // from the PROCESS_LAUNCHER, FILE, and UI threads.
  base::Lock process_host_id_to_pipe_lock_;
  std::map<int, std::unique_ptr<base::SyncSocket>> process_host_id_to_pipe_;

  DISALLOW_COPY_AND_ASSIGN(AwBrowserTerminator);
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_AW_BROWSER_TERMINATOR_H_
