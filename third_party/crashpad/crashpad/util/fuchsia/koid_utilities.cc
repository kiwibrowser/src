// Copyright 2018 The Crashpad Authors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "util/fuchsia/koid_utilities.h"

#include <zircon/device/sysinfo.h>

#include <vector>

#include "base/files/file_path.h"
#include "base/fuchsia/fuchsia_logging.h"
#include "util/file/file_io.h"

namespace crashpad {

namespace {

base::ScopedZxHandle GetRootJob() {
  ScopedFileHandle sysinfo(
      LoggingOpenFileForRead(base::FilePath("/dev/misc/sysinfo")));
  if (!sysinfo.is_valid())
    return base::ScopedZxHandle();

  zx_handle_t root_job;
  size_t n = ioctl_sysinfo_get_root_job(sysinfo.get(), &root_job);
  if (n != sizeof(root_job)) {
    LOG(ERROR) << "unexpected root job size";
    return base::ScopedZxHandle();
  }
  return base::ScopedZxHandle(root_job);
}

bool FindProcess(const base::ScopedZxHandle& job,
                 zx_koid_t koid,
                 base::ScopedZxHandle* out) {
  for (auto& proc : GetChildHandles(job.get(), ZX_INFO_JOB_PROCESSES)) {
    if (GetKoidForHandle(proc.get()) == koid) {
      *out = std::move(proc);
      return true;
    }
  }

  // TODO(scottmg): As this is recursing down the job tree all the handles are
  // kept open, so this could be very expensive in terms of number of open
  // handles. This function should be replaced by a syscall in the
  // not-too-distant future, so hopefully OK for now.
  for (const auto& child_job :
       GetChildHandles(job.get(), ZX_INFO_JOB_CHILDREN)) {
    if (FindProcess(child_job, koid, out))
      return true;
  }

  return false;
}

}  // namespace

std::vector<zx_koid_t> GetChildKoids(zx_handle_t parent,
                                     zx_object_info_topic_t child_kind) {
  size_t actual = 0;
  size_t available = 0;
  std::vector<zx_koid_t> result(100);

  // This is inherently racy. Better if the process is suspended, but there's
  // still no guarantee that a thread isn't externally created. As a result,
  // must be in a retry loop.
  for (;;) {
    zx_status_t status = zx_object_get_info(parent,
                                            child_kind,
                                            result.data(),
                                            result.size() * sizeof(zx_koid_t),
                                            &actual,
                                            &available);
    // If the buffer is too small (even zero), the result is still ZX_OK, not
    // ZX_ERR_BUFFER_TOO_SMALL.
    if (status != ZX_OK) {
      ZX_LOG(ERROR, status) << "zx_object_get_info";
      break;
    }

    if (actual == available) {
      break;
    }

    // Resize to the expected number next time, with a bit of slop to handle the
    // race between here and the next request.
    result.resize(available + 10);
  }

  result.resize(actual);
  return result;
}

std::vector<base::ScopedZxHandle> GetChildHandles(zx_handle_t parent,
                                                  zx_object_info_topic_t type) {
  auto koids = GetChildKoids(parent, type);
  return GetHandlesForChildKoids(parent, koids);
}

std::vector<base::ScopedZxHandle> GetHandlesForChildKoids(
    zx_handle_t parent,
    const std::vector<zx_koid_t>& koids) {
  std::vector<base::ScopedZxHandle> result;
  result.reserve(koids.size());
  for (zx_koid_t koid : koids) {
    result.emplace_back(GetChildHandleByKoid(parent, koid));
  }
  return result;
}

base::ScopedZxHandle GetChildHandleByKoid(zx_handle_t parent,
                                          zx_koid_t child_koid) {
  zx_handle_t handle;
  zx_status_t status =
      zx_object_get_child(parent, child_koid, ZX_RIGHT_SAME_RIGHTS, &handle);
  if (status != ZX_OK) {
    ZX_LOG(ERROR, status) << "zx_object_get_child";
    return base::ScopedZxHandle();
  }
  return base::ScopedZxHandle(handle);
}

zx_koid_t GetKoidForHandle(zx_handle_t object) {
  zx_info_handle_basic_t info;
  zx_status_t status = zx_object_get_info(
      object, ZX_INFO_HANDLE_BASIC, &info, sizeof(info), nullptr, nullptr);
  if (status != ZX_OK) {
    ZX_LOG(ERROR, status) << "zx_object_get_info";
    return ZX_HANDLE_INVALID;
  }
  return info.koid;
}

// TODO(scottmg): This implementation uses some debug/temporary/hacky APIs and
// ioctls that are currently the only way to go from pid to handle. This should
// hopefully eventually be replaced by more or less a single
// zx_debug_something() syscall.
base::ScopedZxHandle GetProcessFromKoid(zx_koid_t koid) {
  base::ScopedZxHandle result;
  if (!FindProcess(GetRootJob(), koid, &result)) {
    LOG(ERROR) << "process " << koid << " not found";
  }
  return result;
}

}  // namespace crashpad
