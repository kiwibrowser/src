// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRASH_CONTENT_BROWSER_CHILD_PROCESS_CRASH_OBSERVER_ANDROID_H_
#define COMPONENTS_CRASH_CONTENT_BROWSER_CHILD_PROCESS_CRASH_OBSERVER_ANDROID_H_

#include "base/files/file_path.h"
#include "components/crash/content/browser/crash_dump_observer_android.h"

namespace breakpad {

class ChildProcessCrashObserver : public breakpad::CrashDumpObserver::Client {
 public:
  ChildProcessCrashObserver(const base::FilePath crash_dump_dir,
                            int descriptor_id);
  ~ChildProcessCrashObserver() override;

  // breakpad::CrashDumpObserver::Client implementation:
  void OnChildStart(int process_host_id,
                    content::PosixFileDescriptorInfo* mappings) override;
  void OnChildExit(const CrashDumpObserver::TerminationInfo& info) override;

 private:
  base::FilePath crash_dump_dir_;
  // The id used to identify the file descriptor in the set of file
  // descriptor mappings passed to the child process.
  int descriptor_id_;

  DISALLOW_COPY_AND_ASSIGN(ChildProcessCrashObserver);
};

}  // namespace breakpad

#endif  // COMPONENTS_CRASH_CONTENT_BROWSER_CHILD_PROCESS_CRASH_OBSERVER_ANDROID_H_
