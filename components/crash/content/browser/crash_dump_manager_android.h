// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRASH_CONTENT_BROWSER_CRASH_DUMP_MANAGER_ANDROID_H_
#define COMPONENTS_CRASH_CONTENT_BROWSER_CRASH_DUMP_MANAGER_ANDROID_H_

#include <map>

#include "base/android/application_status_listener.h"
#include "base/files/file_path.h"
#include "base/files/platform_file.h"
#include "base/lazy_instance.h"
#include "base/memory/ref_counted.h"
#include "base/observer_list_threadsafe.h"
#include "base/synchronization/lock.h"
#include "components/crash/content/browser/crash_dump_observer_android.h"
#include "content/public/common/child_process_host.h"
#include "content/public/common/process_type.h"

namespace breakpad {

// This class manages the crash minidumps.
// On Android, because of process isolation, each renderer process runs with a
// different UID. As a result, we cannot generate the minidumps in the browser
// (as the browser process does not have access to some system files for the
// crashed process). So the minidump is generated in the renderer process.
// Since the isolated process cannot open files, we provide it on creation with
// a file descriptor into which to write the minidump in the event of a crash.
// This class creates these file descriptors and associates them with render
// processes and takes the appropriate action when the render process
// terminates.
class CrashDumpManager {
 public:
  // This enum is used to back a UMA histogram, and must be treated as
  // append-only.
  enum ExitStatus {
    EMPTY_MINIDUMP_WHILE_RUNNING,
    EMPTY_MINIDUMP_WHILE_PAUSED,
    EMPTY_MINIDUMP_WHILE_BACKGROUND,
    VALID_MINIDUMP_WHILE_RUNNING,
    VALID_MINIDUMP_WHILE_PAUSED,
    VALID_MINIDUMP_WHILE_BACKGROUND,
    MINIDUMP_STATUS_COUNT
  };

  enum class CrashDumpStatus {
    // The dump for this process did not have a path set. This can happen if the
    // dump was already processed or if crash dump generation is not turned on.
    kNoDump,

    // The crash dump was empty.
    kEmptyDump,

    // The crash dump is valid.
    kValidDump,
  };
  struct CrashDumpDetails {
    CrashDumpDetails(int process_host_id,
                     content::ProcessType process_type,
                     bool was_oom_protected_status,
                     base::android::ApplicationState app_state);
    CrashDumpDetails();
    ~CrashDumpDetails();
    CrashDumpDetails(const CrashDumpDetails& other);

    int process_host_id = content::ChildProcessHost::kInvalidUniqueID;

    content::ProcessType process_type = content::PROCESS_TYPE_UNKNOWN;
    bool was_oom_protected_status = false;
    base::android::ApplicationState app_state;
    int64_t file_size = 0;
    CrashDumpStatus status = CrashDumpStatus::kNoDump;
  };

  // Careful note: the CrashDumpManager observers are asynchronous, and are
  // notified via PostTask. This could be problematic with a large number of
  // observers. Consider using a middle-layer observer to fan out synchronously
  // to leaf observers if you need many objects listening to these messages.
  class Observer {
   public:
    virtual void OnCrashDumpProcessed(const CrashDumpDetails& details) {}
  };

  static CrashDumpManager* GetInstance();

  // True when |details| is a foreground out of memory crash.
  static bool IsForegroundOom(const CrashDumpDetails& details);

  // Can be called on any thread.
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void ProcessMinidumpFileFromChild(
      base::FilePath crash_dump_dir,
      const CrashDumpObserver::TerminationInfo& info);

  base::ScopedFD CreateMinidumpFileForChild(int process_host_id);

 private:
  friend struct base::LazyInstanceTraitsBase<CrashDumpManager>;

  CrashDumpManager();
  ~CrashDumpManager();

  void NotifyObservers(const CrashDumpDetails& details);

  typedef std::map<int, base::FilePath> ChildProcessIDToMinidumpPath;

  void SetMinidumpPath(int process_host_id,
                       const base::FilePath& minidump_path);
  bool GetMinidumpPath(int process_host_id, base::FilePath* minidump_path);

  scoped_refptr<base::ObserverListThreadSafe<CrashDumpManager::Observer>>
      async_observers_;

  // This map should only be accessed with its lock aquired as it is accessed
  // from the PROCESS_LAUNCHER and UI threads.
  base::Lock process_host_id_to_minidump_path_lock_;
  ChildProcessIDToMinidumpPath process_host_id_to_minidump_path_;

  DISALLOW_COPY_AND_ASSIGN(CrashDumpManager);
};

}  // namespace breakpad

#endif  // COMPONENTS_CRASH_CONTENT_BROWSER_CRASH_DUMP_MANAGER_ANDROID_H_
