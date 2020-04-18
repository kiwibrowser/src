// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/android/apk_assets.h"
#include "base/android/jni_array.h"
#include "base/i18n/icu_util.h"
#include "base/logging.h"
#include "base/metrics/field_trial.h"
#include "content/browser/child_process_launcher.h"
#include "content/browser/child_process_launcher_helper.h"
#include "content/browser/child_process_launcher_helper_posix.h"
#include "content/browser/posix_file_descriptor_info_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_launcher_utils.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/content_descriptors.h"
#include "content/public/common/content_switches.h"
#include "jni/ChildProcessLauncherHelper_jni.h"
#include "services/service_manager/sandbox/switches.h"

using base::android::AttachCurrentThread;
using base::android::JavaParamRef;
using base::android::ScopedJavaGlobalRef;
using base::android::ScopedJavaLocalRef;
using base::android::ToJavaArrayOfStrings;

namespace content {
namespace internal {
namespace {

// Stops a child process based on the handle returned from StartChildProcess.
void StopChildProcess(base::ProcessHandle handle) {
  DCHECK(CurrentlyOnProcessLauncherTaskRunner());
  JNIEnv* env = AttachCurrentThread();
  DCHECK(env);
  Java_ChildProcessLauncherHelper_stop(env, static_cast<jint>(handle));
}

}  // namespace

void ChildProcessLauncherHelper::BeforeLaunchOnClientThread() {
  // Android only supports renderer, sandboxed utility and gpu.
  std::string process_type =
      command_line()->GetSwitchValueASCII(switches::kProcessType);
  CHECK(process_type == switches::kGpuProcess ||
        process_type == switches::kRendererProcess ||
#if BUILDFLAG(ENABLE_PLUGINS)
        process_type == switches::kPpapiPluginProcess ||
#endif
        process_type == switches::kUtilityProcess)
      << "Unsupported process type: " << process_type;

  // Non-sandboxed utility or renderer process are currently not supported.
  DCHECK(process_type == switches::kGpuProcess ||
         !command_line()->HasSwitch(service_manager::switches::kNoSandbox));
}

mojo::edk::ScopedInternalPlatformHandle
ChildProcessLauncherHelper::PrepareMojoPipeHandlesOnClientThread() {
  return mojo::edk::ScopedInternalPlatformHandle();
}

std::unique_ptr<PosixFileDescriptorInfo>
ChildProcessLauncherHelper::GetFilesToMap() {
  DCHECK(CurrentlyOnProcessLauncherTaskRunner());

  // Android WebView runs in single process, ensure that we never get here when
  // running in single process mode.
  CHECK(!command_line()->HasSwitch(switches::kSingleProcess));

  std::unique_ptr<PosixFileDescriptorInfo> files_to_register =
      CreateDefaultPosixFilesToMap(child_process_id(), mojo_client_handle(),
                                   true /* include_service_required_files */,
                                   GetProcessType(), command_line());

#if ICU_UTIL_DATA_IMPL == ICU_UTIL_DATA_FILE
  base::MemoryMappedFile::Region icu_region;
  int fd = base::i18n::GetIcuDataFileHandle(&icu_region);
  files_to_register->ShareWithRegion(kAndroidICUDataDescriptor, fd, icu_region);
#endif  // ICU_UTIL_DATA_IMPL == ICU_UTIL_DATA_FILE

  return files_to_register;
}

bool ChildProcessLauncherHelper::BeforeLaunchOnLauncherThread(
    const PosixFileDescriptorInfo& files_to_register,
    base::LaunchOptions* options) {
  return true;
}

ChildProcessLauncherHelper::Process
ChildProcessLauncherHelper::LaunchProcessOnLauncherThread(
    const base::LaunchOptions& options,
    std::unique_ptr<PosixFileDescriptorInfo> files_to_register,
    bool* is_synchronous_launch,
    int* launch_result) {
  *is_synchronous_launch = false;

  JNIEnv* env = AttachCurrentThread();
  DCHECK(env);

  // Create the Command line String[]
  ScopedJavaLocalRef<jobjectArray> j_argv =
      ToJavaArrayOfStrings(env, command_line()->argv());

  size_t file_count = files_to_register->GetMappingSize();
  DCHECK(file_count > 0);

  ScopedJavaLocalRef<jclass> j_file_info_class = base::android::GetClass(
      env, "org/chromium/base/process_launcher/FileDescriptorInfo");
  ScopedJavaLocalRef<jobjectArray> j_file_infos(
      env, env->NewObjectArray(file_count, j_file_info_class.obj(), NULL));
  base::android::CheckException(env);

  for (size_t i = 0; i < file_count; ++i) {
    int fd = files_to_register->GetFDAt(i);
    PCHECK(0 <= fd);
    int id = files_to_register->GetIDAt(i);
    const auto& region = files_to_register->GetRegionAt(i);
    bool auto_close = files_to_register->OwnsFD(fd);
    ScopedJavaLocalRef<jobject> j_file_info =
        Java_ChildProcessLauncherHelper_makeFdInfo(env, id, fd, auto_close,
                                                   region.offset, region.size);
    PCHECK(j_file_info.obj());
    env->SetObjectArrayElement(j_file_infos.obj(), i, j_file_info.obj());
    if (auto_close) {
      ignore_result(files_to_register->ReleaseFD(fd).release());
    }
  }

  java_peer_.Reset(Java_ChildProcessLauncherHelper_createAndStart(
      env, reinterpret_cast<intptr_t>(this), j_argv, j_file_infos));
  AddRef();  // Balanced by OnChildProcessStarted.
  BrowserThread::PostTask(
      client_thread_id_, FROM_HERE,
      base::Bind(
          &ChildProcessLauncherHelper::set_java_peer_available_on_client_thread,
          this));

  return Process();
}

void ChildProcessLauncherHelper::AfterLaunchOnLauncherThread(
    const ChildProcessLauncherHelper::Process& process,
    const base::LaunchOptions& options) {
}

ChildProcessTerminationInfo ChildProcessLauncherHelper::GetTerminationInfo(
    const ChildProcessLauncherHelper::Process& process,
    bool known_dead) {
  ChildProcessTerminationInfo info;
  info.has_oom_protection_bindings =
      java_peer_avaiable_on_client_thread_ &&
      Java_ChildProcessLauncherHelper_hasOomProtectionBindings(
          AttachCurrentThread(), java_peer_);
  info.was_killed_intentionally_by_browser =
      java_peer_avaiable_on_client_thread_ &&
      Java_ChildProcessLauncherHelper_isKilledByUs(AttachCurrentThread(),
                                                   java_peer_);
  bool app_foreground =
      java_peer_avaiable_on_client_thread_ &&
      Java_ChildProcessLauncherHelper_isApplicationInForeground(
          AttachCurrentThread(), java_peer_);
  if (app_foreground && info.has_oom_protection_bindings) {
    info.status = base::TERMINATION_STATUS_OOM_PROTECTED;
  } else {
    // Note waitpid does not work on Android since these are not actually child
    // processes. So there is no need for base::GetTerminationInfo.
    info.status = base::TERMINATION_STATUS_NORMAL_TERMINATION;
  }
  return info;
}

// static
bool ChildProcessLauncherHelper::TerminateProcess(const base::Process& process,
                                                  int exit_code) {
  GetProcessLauncherTaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(&StopChildProcess, process.Handle()));
  return true;
}

// static
void ChildProcessLauncherHelper::ForceNormalProcessTerminationSync(
    ChildProcessLauncherHelper::Process process) {
  DCHECK(CurrentlyOnProcessLauncherTaskRunner());
  VLOG(1) << "ChromeProcess: Stopping process with handle "
          << process.process.Handle();
  StopChildProcess(process.process.Handle());
}

void ChildProcessLauncherHelper::SetProcessPriorityOnLauncherThread(
    base::Process process,
    const ChildProcessLauncherPriority& priority) {
  JNIEnv* env = AttachCurrentThread();
  DCHECK(env);
  return Java_ChildProcessLauncherHelper_setPriority(
      env, java_peer_, process.Handle(), !priority.background,
      priority.frame_depth, priority.boost_for_pending_views,
      static_cast<jint>(priority.importance));
}

// static
void ChildProcessLauncherHelper::SetRegisteredFilesForService(
    const std::string& service_name,
    catalog::RequiredFileMap required_files) {
  SetFilesToShareForServicePosix(service_name, std::move(required_files));
}

// static
void ChildProcessLauncherHelper::ResetRegisteredFilesForTesting() {
  ResetFilesToShareForTestingPosix();
}

// static
base::File OpenFileToShare(const base::FilePath& path,
                           base::MemoryMappedFile::Region* region) {
  return base::File(base::android::OpenApkAsset(path.value(), region));
}

// Called from ChildProcessLauncher.java when the ChildProcess was started.
// |handle| is the processID of the child process as originated in Java, 0 if
// the ChildProcess could not be created.
void ChildProcessLauncherHelper::OnChildProcessStarted(
    JNIEnv*,
    const base::android::JavaParamRef<jobject>& obj,
    jint handle) {
  DCHECK(CurrentlyOnProcessLauncherTaskRunner());
  scoped_refptr<ChildProcessLauncherHelper> ref(this);
  Release();  // Balances with LaunchProcessOnLauncherThread.

  int launch_result = (handle == base::kNullProcessHandle)
                          ? LAUNCH_RESULT_FAILURE
                          : LAUNCH_RESULT_SUCCESS;

  ChildProcessLauncherHelper::Process process;
  process.process = base::Process(handle);
  PostLaunchOnLauncherThread(std::move(process), launch_result);
}

}  // namespace internal

}  // namespace content
