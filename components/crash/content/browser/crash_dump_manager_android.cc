// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/crash/content/browser/crash_dump_manager_android.h"

#include <stdint.h>

#include <string>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file_util.h"
#include "base/format_macros.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/process/process.h"
#include "base/rand_util.h"
#include "base/stl_util.h"
#include "base/strings/stringprintf.h"
#include "components/crash/content/app/breakpad_linux.h"
#include "jni/CrashDumpManager_jni.h"

namespace breakpad {

namespace {
base::LazyInstance<CrashDumpManager>::Leaky g_instance =
    LAZY_INSTANCE_INITIALIZER;

// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum class ProcessedCrashCounts {
  kGpuForegroundOom = 0,
  kRendererForegroundVisibleOom = 1,
  kRendererForegroundIntentionalKill = 2,
  kRendererForegroundVisibleSubframeOom = 3,
  kRendererForegroundVisibleSubframeIntentionalKill = 4,
  kRendererForegroundVisibleCrash = 5,
  kRendererForegroundVisibleSubframeCrash = 6,
  kGpuCrashAll = 7,
  kRendererCrashAll = 8,
  kMaxValue = kRendererCrashAll
};

void LogCount(ProcessedCrashCounts type) {
  UMA_HISTOGRAM_ENUMERATION("Stability.Android.ProcessedCrashCounts", type);
}

void LogProcessedMetrics(const CrashDumpObserver::TerminationInfo& info,
                         bool has_crash_dump) {
  const bool app_foreground =
      info.app_state ==
          base::android::APPLICATION_STATE_HAS_RUNNING_ACTIVITIES ||
      info.app_state == base::android::APPLICATION_STATE_HAS_PAUSED_ACTIVITIES;
  const bool intentional_kill = info.was_killed_intentionally_by_browser;
  const bool android_oom_kill = !info.was_killed_intentionally_by_browser &&
                                !has_crash_dump && !info.normal_termination;
  const bool renderer_visible = info.renderer_has_visible_clients;
  const bool renderer_sub_frame = info.renderer_was_subframe;

  if (info.process_type == content::PROCESS_TYPE_GPU) {
    if (app_foreground && android_oom_kill) {
      LogCount(ProcessedCrashCounts::kGpuForegroundOom);
    }
    if (has_crash_dump) {
      LogCount(ProcessedCrashCounts::kGpuCrashAll);
    }
  }

  if (info.process_type == content::PROCESS_TYPE_RENDERER) {
    if (app_foreground && renderer_visible) {
      if (android_oom_kill) {
        LogCount(
            renderer_sub_frame
                ? ProcessedCrashCounts::kRendererForegroundVisibleSubframeOom
                : ProcessedCrashCounts::kRendererForegroundVisibleOom);
      }
      if (has_crash_dump) {
        LogCount(
            renderer_sub_frame
                ? ProcessedCrashCounts::kRendererForegroundVisibleSubframeCrash
                : ProcessedCrashCounts::kRendererForegroundVisibleCrash);
      }
    }
    if (app_foreground && intentional_kill) {
      LogCount(ProcessedCrashCounts::kRendererForegroundIntentionalKill);
      if (renderer_visible && renderer_sub_frame) {
        LogCount(ProcessedCrashCounts::
                     kRendererForegroundVisibleSubframeIntentionalKill);
      }
    }
    if (has_crash_dump) {
      LogCount(ProcessedCrashCounts::kRendererCrashAll);
    }
  }
}

}  // namespace

CrashDumpManager::CrashDumpDetails::CrashDumpDetails(
    int process_host_id,
    content::ProcessType process_type,
    bool was_oom_protected_status,
    base::android::ApplicationState app_state)
    : process_host_id(process_host_id),
      process_type(process_type),
      was_oom_protected_status(was_oom_protected_status),
      app_state(app_state) {}

CrashDumpManager::CrashDumpDetails::CrashDumpDetails() {}
CrashDumpManager::CrashDumpDetails::~CrashDumpDetails() {}

CrashDumpManager::CrashDumpDetails::CrashDumpDetails(
    const CrashDumpManager::CrashDumpDetails& other)
    : CrashDumpDetails(other.process_host_id,
                       other.process_type,
                       other.was_oom_protected_status,
                       other.app_state) {
  file_size = other.file_size;
  status = other.status;
}

// static
CrashDumpManager* CrashDumpManager::GetInstance() {
  return g_instance.Pointer();
}

// static
bool CrashDumpManager::IsForegroundOom(const CrashDumpDetails& details) {
  // If the crash is size 0, it is an OOM.
  return details.process_type == content::PROCESS_TYPE_RENDERER &&
         details.was_oom_protected_status && details.file_size == 0 &&
         details.app_state ==
             base::android::APPLICATION_STATE_HAS_RUNNING_ACTIVITIES;
}

void CrashDumpManager::AddObserver(Observer* observer) {
  async_observers_->AddObserver(observer);
}

void CrashDumpManager::RemoveObserver(Observer* observer) {
  async_observers_->RemoveObserver(observer);
}

CrashDumpManager::CrashDumpManager()
    : async_observers_(
          base::MakeRefCounted<
              base::ObserverListThreadSafe<CrashDumpManager::Observer>>()) {}

CrashDumpManager::~CrashDumpManager() {}

base::ScopedFD CrashDumpManager::CreateMinidumpFileForChild(
    int process_host_id) {
  base::AssertBlockingAllowed();
  base::FilePath minidump_path;
  if (!base::CreateTemporaryFile(&minidump_path)) {
    LOG(ERROR) << "Failed to create temporary file, crash won't be reported.";
    return base::ScopedFD();
  }

  // We need read permission as the minidump is generated in several phases
  // and needs to be read at some point.
  int flags = base::File::FLAG_OPEN | base::File::FLAG_READ |
              base::File::FLAG_WRITE;
  base::File minidump_file(minidump_path, flags);
  if (!minidump_file.IsValid()) {
    LOG(ERROR) << "Failed to open temporary file, crash won't be reported.";
    return base::ScopedFD();
  }

  SetMinidumpPath(process_host_id, minidump_path);
  return base::ScopedFD(minidump_file.TakePlatformFile());
}

void CrashDumpManager::ProcessMinidumpFileFromChild(
    base::FilePath crash_dump_dir,
    const CrashDumpObserver::TerminationInfo& info) {
  base::AssertBlockingAllowed();
  CrashDumpDetails details(info.process_host_id, info.process_type,
                           info.was_oom_protected_status, info.app_state);
  base::FilePath minidump_path;
  // If the minidump for a given child process has already been
  // processed, then there is no more work to do.
  if (!GetMinidumpPath(info.process_host_id, &minidump_path)) {
    NotifyObservers(details);
    return;
  }

  int64_t file_size = 0;

  if (!base::PathExists(minidump_path)) {
    LOG(ERROR) << "minidump does not exist " << minidump_path.value();
    return;
  }

  int r = base::GetFileSize(minidump_path, &file_size);
  DCHECK(r) << "Failed to retrieve size for minidump " << minidump_path.value();

  // TODO(wnwen): If these numbers match up to TabWebContentsObserver's
  //     TabRendererCrashStatus histogram, then remove that one as this is more
  //     accurate with more detail.
  if ((info.process_type == content::PROCESS_TYPE_RENDERER ||
       info.process_type == content::PROCESS_TYPE_GPU) &&
      info.app_state != base::android::APPLICATION_STATE_UNKNOWN) {
    ExitStatus exit_status;
    bool is_running = (info.app_state ==
                       base::android::APPLICATION_STATE_HAS_RUNNING_ACTIVITIES);
    bool is_paused = (info.app_state ==
                      base::android::APPLICATION_STATE_HAS_PAUSED_ACTIVITIES);
    if (file_size == 0) {
      if (is_running) {
        exit_status = EMPTY_MINIDUMP_WHILE_RUNNING;
      } else if (is_paused) {
        exit_status = EMPTY_MINIDUMP_WHILE_PAUSED;
      } else {
        exit_status = EMPTY_MINIDUMP_WHILE_BACKGROUND;
      }
    } else {
      if (is_running) {
        exit_status = VALID_MINIDUMP_WHILE_RUNNING;
      } else if (is_paused) {
        exit_status = VALID_MINIDUMP_WHILE_PAUSED;
      } else {
        exit_status = VALID_MINIDUMP_WHILE_BACKGROUND;
      }
    }
    if (info.process_type == content::PROCESS_TYPE_RENDERER) {
      if (info.was_oom_protected_status) {
        UMA_HISTOGRAM_ENUMERATION("Tab.RendererDetailedExitStatus", exit_status,
                                  ExitStatus::MINIDUMP_STATUS_COUNT);
      } else {
        UMA_HISTOGRAM_ENUMERATION("Tab.RendererDetailedExitStatusUnbound",
                                  exit_status,
                                  ExitStatus::MINIDUMP_STATUS_COUNT);
      }
    } else if (info.process_type == content::PROCESS_TYPE_GPU) {
      UMA_HISTOGRAM_ENUMERATION("GPU.GPUProcessDetailedExitStatus", exit_status,
                                ExitStatus::MINIDUMP_STATUS_COUNT);
    }
  }

  LogProcessedMetrics(info, file_size != 0);

  if (file_size == 0) {
    // Empty minidump, this process did not crash. Just remove the file.
    r = base::DeleteFile(minidump_path, false);
    DCHECK(r) << "Failed to delete temporary minidump file "
              << minidump_path.value();
    details.status = CrashDumpStatus::kEmptyDump;
    NotifyObservers(details);
    return;
  }

  // We are dealing with a valid minidump. Copy it to the crash report
  // directory from where Java code will upload it later on.
  if (crash_dump_dir.empty()) {
    NOTREACHED() << "Failed to retrieve the crash dump directory.";
    return;
  }
  const uint64_t rand = base::RandUint64();
  const std::string filename =
      base::StringPrintf("chromium-renderer-minidump-%016" PRIx64 ".dmp%d",
                         rand, info.process_host_id);
  base::FilePath dest_path = crash_dump_dir.Append(filename);
  r = base::Move(minidump_path, dest_path);
  if (!r) {
    LOG(ERROR) << "Failed to move crash dump from " << minidump_path.value()
               << " to " << dest_path.value();
    base::DeleteFile(minidump_path, false);
    return;
  }
  VLOG(1) << "Crash minidump successfully generated: " << dest_path.value();

  // Hop over to Java to attempt to attach the logcat to the crash. This may
  // fail, which is ok -- if it does, the crash will still be uploaded on the
  // next browser start.
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jstring> j_dest_path =
      base::android::ConvertUTF8ToJavaString(env, dest_path.value());
  Java_CrashDumpManager_tryToUploadMinidump(env, j_dest_path);
  details.status = CrashDumpStatus::kValidDump;
  NotifyObservers(details);
}

void CrashDumpManager::NotifyObservers(const CrashDumpDetails& details) {
  async_observers_->Notify(
      FROM_HERE, &CrashDumpManager::Observer::OnCrashDumpProcessed, details);
}

void CrashDumpManager::SetMinidumpPath(int process_host_id,
                                       const base::FilePath& minidump_path) {
  base::AutoLock auto_lock(process_host_id_to_minidump_path_lock_);
  DCHECK(
      !base::ContainsKey(process_host_id_to_minidump_path_, process_host_id));
  process_host_id_to_minidump_path_[process_host_id] = minidump_path;
}

bool CrashDumpManager::GetMinidumpPath(int process_host_id,
                                       base::FilePath* minidump_path) {
  base::AutoLock auto_lock(process_host_id_to_minidump_path_lock_);
  ChildProcessIDToMinidumpPath::iterator iter =
      process_host_id_to_minidump_path_.find(process_host_id);
  if (iter == process_host_id_to_minidump_path_.end()) {
    return false;
  }
  *minidump_path = iter->second;
  process_host_id_to_minidump_path_.erase(iter);
  return true;
}

}  // namespace breakpad
