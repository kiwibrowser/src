// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/crash/content/browser/child_process_crash_observer_android.h"

#include <utility>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/task_scheduler/post_task.h"
#include "components/crash/content/app/breakpad_linux.h"
#include "components/crash/content/browser/crash_dump_manager_android.h"

namespace breakpad {

ChildProcessCrashObserver::ChildProcessCrashObserver(
    const base::FilePath crash_dump_dir,
    int descriptor_id)
    : crash_dump_dir_(crash_dump_dir), descriptor_id_(descriptor_id) {}

ChildProcessCrashObserver::~ChildProcessCrashObserver() {}

void ChildProcessCrashObserver::OnChildStart(
    int process_host_id,
    content::PosixFileDescriptorInfo* mappings) {
  if (!breakpad::IsCrashReporterEnabled())
    return;

  base::ScopedFD file(
      CrashDumpManager::GetInstance()->CreateMinidumpFileForChild(
          process_host_id));
  if (file.is_valid())
    mappings->Transfer(descriptor_id_, std::move(file));
}

void ChildProcessCrashObserver::OnChildExit(
    const CrashDumpObserver::TerminationInfo& info) {
  // This might be called twice for a given child process, with a
  // NOTIFICATION_RENDERER_PROCESS_TERMINATED and then with
  // NOTIFICATION_RENDERER_PROCESS_CLOSED.

  base::PostTaskWithTraits(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::Bind(&CrashDumpManager::ProcessMinidumpFileFromChild,
                 base::Unretained(CrashDumpManager::GetInstance()),
                 crash_dump_dir_, info));
}

}  // namespace breakpad
