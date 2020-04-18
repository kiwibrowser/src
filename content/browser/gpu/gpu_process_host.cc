// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/gpu/gpu_process_host.h"

#include <stddef.h>

#include <algorithm>
#include <list>
#include <utility>

#include "base/base64.h"
#include "base/base_switches.h"
#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "components/discardable_memory/service/discardable_shared_memory_manager.h"
#include "components/tracing/common/tracing_switches.h"
#include "components/viz/common/features.h"
#include "components/viz/common/switches.h"
#include "content/browser/browser_child_process_host_impl.h"
#include "content/browser/browser_main_loop.h"
#include "content/browser/field_trial_recorder.h"
#include "content/browser/gpu/compositor_util.h"
#include "content/browser/gpu/gpu_data_manager_impl.h"
#include "content/browser/gpu/gpu_main_thread_factory.h"
#include "content/browser/gpu/shader_cache_factory.h"
#include "content/browser/service_manager/service_manager_context.h"
#include "content/common/child_process_host_impl.h"
#include "content/common/in_process_child_thread_params.h"
#include "content/common/service_manager/child_connection.h"
#include "content/common/view_messages.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/gpu_utils.h"
#include "content/public/common/bind_interface_helpers.h"
#include "content/public/common/connection_filter.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/result_codes.h"
#include "content/public/common/sandboxed_process_launcher_delegate.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/common/service_names.mojom.h"
#include "gpu/command_buffer/service/gpu_preferences.h"
#include "gpu/command_buffer/service/gpu_switches.h"
#include "gpu/config/gpu_driver_bug_list.h"
#include "gpu/config/gpu_driver_bug_workaround_type.h"
#include "gpu/ipc/host/shader_disk_cache.h"
#include "media/base/media_switches.h"
#include "media/media_buildflags.h"
#include "mojo/edk/embedder/embedder.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "services/service_manager/runner/common/client_util.h"
#include "services/service_manager/sandbox/sandbox_type.h"
#include "services/service_manager/sandbox/switches.h"
#include "services/ui/public/interfaces/constants.mojom.h"
#include "ui/base/ui_base_features.h"
#include "ui/base/ui_base_switches.h"
#include "ui/display/display_switches.h"
#include "ui/gfx/switches.h"
#include "ui/gl/gl_switches.h"
#include "ui/latency/latency_info.h"

#if defined(OS_ANDROID)
#include "base/android/build_info.h"
#include "content/public/browser/android/java_interfaces.h"
#include "media/mojo/interfaces/android_overlay.mojom.h"
#endif

#if defined(OS_WIN)
#include "sandbox/win/src/sandbox_policy.h"
#include "services/service_manager/sandbox/win/sandbox_win.h"
#include "ui/gfx/switches.h"
#include "ui/gfx/win/rendering_window_manager.h"
#endif

#if defined(USE_OZONE)
#include "ui/ozone/public/gpu_platform_support_host.h"
#include "ui/ozone/public/ozone_platform.h"
#include "ui/ozone/public/ozone_switches.h"
#endif

#if defined(USE_X11)
#include "ui/gfx/x/x11_switches.h"  // nogncheck
#endif

#if defined(OS_MACOSX) || defined(OS_ANDROID)
#include "gpu/ipc/common/gpu_surface_tracker.h"
#endif

namespace content {

base::subtle::Atomic32 GpuProcessHost::gpu_crash_count_ = 0;
int GpuProcessHost::gpu_recent_crash_count_ = 0;
bool GpuProcessHost::crashed_before_ = false;
int GpuProcessHost::swiftshader_crash_count_ = 0;
int GpuProcessHost::swiftshader_recent_crash_count_ = 0;
int GpuProcessHost::display_compositor_crash_count_ = 0;
int GpuProcessHost::display_compositor_recent_crash_count_ = 0;

namespace {

// This matches base::TerminationStatus.
// These values are persisted to logs. Entries (except MAX_ENUM) should not be
// renumbered and numeric values should never be reused. Should also avoid
// OS-defines in this enum to keep the values consistent on all platforms.
enum class GpuTerminationStatus {
  NORMAL_TERMINATION = 0,
  ABNORMAL_TERMINATION = 1,
  PROCESS_WAS_KILLED = 2,
  PROCESS_CRASHED = 3,
  STILL_RUNNING = 4,
  PROCESS_WAS_KILLED_BY_OOM = 5,
  OOM_PROTECTED = 6,
  LAUNCH_FAILED = 7,
  OOM = 8,
  MAX_ENUM = 9,
};

GpuTerminationStatus ConvertToGpuTerminationStatus(
    base::TerminationStatus status) {
  switch (status) {
    case base::TERMINATION_STATUS_NORMAL_TERMINATION:
      return GpuTerminationStatus::NORMAL_TERMINATION;
    case base::TERMINATION_STATUS_ABNORMAL_TERMINATION:
      return GpuTerminationStatus::ABNORMAL_TERMINATION;
    case base::TERMINATION_STATUS_PROCESS_WAS_KILLED:
      return GpuTerminationStatus::PROCESS_WAS_KILLED;
    case base::TERMINATION_STATUS_PROCESS_CRASHED:
      return GpuTerminationStatus::PROCESS_CRASHED;
    case base::TERMINATION_STATUS_STILL_RUNNING:
      return GpuTerminationStatus::STILL_RUNNING;
#if defined(OS_CHROMEOS)
    case base::TERMINATION_STATUS_PROCESS_WAS_KILLED_BY_OOM:
      return GpuTerminationStatus::PROCESS_WAS_KILLED_BY_OOM;
#endif
#if defined(OS_ANDROID)
    case base::TERMINATION_STATUS_OOM_PROTECTED:
      return GpuTerminationStatus::OOM_PROTECTED;
#endif
    case base::TERMINATION_STATUS_LAUNCH_FAILED:
      return GpuTerminationStatus::LAUNCH_FAILED;
    case base::TERMINATION_STATUS_OOM:
      return GpuTerminationStatus::OOM;
    case base::TERMINATION_STATUS_MAX_ENUM:
      NOTREACHED();
      return GpuTerminationStatus::MAX_ENUM;
      // Do not add default.
  }
  NOTREACHED();
  return GpuTerminationStatus::ABNORMAL_TERMINATION;
}

// Command-line switches to propagate to the GPU process.
static const char* const kSwitchNames[] = {
    service_manager::switches::kDisableSeccompFilterSandbox,
    service_manager::switches::kGpuSandboxAllowSysVShm,
    service_manager::switches::kGpuSandboxFailuresFatal,
    service_manager::switches::kDisableGpuSandbox,
    service_manager::switches::kNoSandbox,
#if defined(OS_WIN)
    service_manager::switches::kAddGpuAppContainerCaps,
    service_manager::switches::kDisableGpuAppContainer,
    service_manager::switches::kDisableGpuLpac,
    service_manager::switches::kEnableGpuAppContainer,
#endif  // defined(OS_WIN)
    switches::kDisableBreakpad,
    switches::kDisableGpuRasterization,
    switches::kDisableGLExtensions,
    switches::kDisableLogging,
    switches::kDisableShaderNameHashing,
    switches::kDisableWebRtcHWEncoding,
#if defined(OS_WIN)
    switches::kEnableAcceleratedVpxDecode,
#endif
    switches::kEnableGpuRasterization,
    switches::kEnableLogging,
    switches::kEnableOOPRasterization,
    switches::kEnableVizDevTools,
    switches::kHeadless,
    switches::kLoggingLevel,
    switches::kEnableLowEndDeviceMode,
    switches::kDisableLowEndDeviceMode,
    switches::kRunAllCompositorStagesBeforeDraw,
    switches::kTestGLLib,
    switches::kTraceToConsole,
    switches::kUseFakeJpegDecodeAccelerator,
    switches::kUseGpuInTests,
    switches::kV,
    switches::kVModule,
#if defined(OS_MACOSX)
    service_manager::switches::kEnableSandboxLogging,
    switches::kDisableAVFoundationOverlays,
    switches::kDisableMacOverlays,
    switches::kDisableRemoteCoreAnimation,
    switches::kShowMacOverlayBorders,
#endif
#if defined(USE_OZONE)
    switches::kEnableDrmMojo,
    switches::kOzonePlatform,
    switches::kOzoneDumpFile,
#endif
#if defined(USE_X11)
    switches::kX11Display,
#endif
    switches::kGpuBlacklistTestGroup,
    switches::kGpuDriverBugListTestGroup,
    switches::kUseCmdDecoder,
    switches::kForceVideoOverlays,
#if defined(OS_ANDROID)
    switches::kOrderfileMemoryOptimization,
#endif
};

enum GPUProcessLifetimeEvent {
  LAUNCHED,
  DIED_FIRST_TIME,
  DIED_SECOND_TIME,
  DIED_THIRD_TIME,
  DIED_FOURTH_TIME,
  GPU_PROCESS_LIFETIME_EVENT_MAX = 100
};

// Indexed by GpuProcessKind. There is one of each kind maximum. This array may
// only be accessed from the IO thread.
GpuProcessHost* g_gpu_process_hosts[GpuProcessHost::GPU_PROCESS_KIND_COUNT];

void RunCallbackOnIO(GpuProcessHost::GpuProcessKind kind,
                     bool force_create,
                     const base::Callback<void(GpuProcessHost*)>& callback) {
  GpuProcessHost* host = GpuProcessHost::Get(kind, force_create);
  callback.Run(host);
}

#if defined(USE_OZONE)
// The ozone platform use this callback to send IPC messages to the gpu process.
void SendGpuProcessMessage(base::WeakPtr<GpuProcessHost> host,
                           IPC::Message* message) {
  if (host)
    host->Send(message);
  else
    delete message;
}

// Helper to register Mus/conventional thread bouncers for ozone startup.
void OzoneRegisterStartupCallbackHelper(
    base::OnceCallback<void(ui::OzonePlatform*)> io_callback) {
  // The callback registered in ozone can be called in any thread. So use an
  // intermediary callback that bounces to the IO thread if needed, before
  // running the callback.
  auto bounce_callback = base::BindOnce(
      [](base::TaskRunner* task_runner,
         base::OnceCallback<void(ui::OzonePlatform*)> callback,
         ui::OzonePlatform* platform) {
        if (task_runner->RunsTasksInCurrentSequence()) {
          std::move(callback).Run(platform);
        } else {
          task_runner->PostTask(FROM_HERE,
                                base::BindOnce(std::move(callback), platform));
        }
      },
      base::RetainedRef(base::ThreadTaskRunnerHandle::Get()),
      base::Passed(&io_callback));
  ui::OzonePlatform::RegisterStartupCallback(std::move(bounce_callback));
}
#endif  // defined(USE_OZONE)

void OnGpuProcessHostDestroyedOnUI(int host_id, const std::string& message) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  GpuDataManagerImpl::GetInstance()->AddLogMessage(
      logging::LOG_ERROR, "GpuProcessHostUIShim", message);
#if defined(USE_OZONE)
  ui::OzonePlatform::GetInstance()
      ->GetGpuPlatformSupportHost()
      ->OnChannelDestroyed(host_id);
#endif
}

// NOTE: changes to this class need to be reviewed by the security team.
class GpuSandboxedProcessLauncherDelegate
    : public SandboxedProcessLauncherDelegate {
 public:
  explicit GpuSandboxedProcessLauncherDelegate(
      const base::CommandLine& cmd_line)
#if defined(OS_WIN)
      : cmd_line_(cmd_line),
        enable_appcontainer_(true)
#endif
  {
  }

  ~GpuSandboxedProcessLauncherDelegate() override {}

#if defined(OS_WIN)
  bool DisableDefaultPolicy() override {
    return true;
  }

  enum GPUAppContainerEnableState{
      AC_ENABLED = 0, AC_DISABLED_GL = 1, AC_DISABLED_FORCE = 2,
      MAX_ENABLE_STATE = 3,
  };

  bool GetAppContainerId(std::string* appcontainer_id) override {
    if (UseOpenGLRenderer()) {
      base::UmaHistogramEnumeration("GPU.AppContainer.EnableState",
                                    AC_DISABLED_GL, MAX_ENABLE_STATE);
      return false;
    }

    if (!enable_appcontainer_) {
      base::UmaHistogramEnumeration("GPU.AppContainer.EnableState",
                                    AC_DISABLED_FORCE, MAX_ENABLE_STATE);
      return false;
    }

    *appcontainer_id = base::WideToUTF8(cmd_line_.GetProgram().value());
    base::UmaHistogramEnumeration("GPU.AppContainer.EnableState", AC_ENABLED,
                                  MAX_ENABLE_STATE);
    return true;
  }

  // For the GPU process we gotten as far as USER_LIMITED. The next level
  // which is USER_RESTRICTED breaks both the DirectX backend and the OpenGL
  // backend. Note that the GPU process is connected to the interactive
  // desktop.
  bool PreSpawnTarget(sandbox::TargetPolicy* policy) override {
    if (UseOpenGLRenderer()) {
      // Open GL path.
      policy->SetTokenLevel(sandbox::USER_RESTRICTED_SAME_ACCESS,
                            sandbox::USER_LIMITED);
      service_manager::SandboxWin::SetJobLevel(
          cmd_line_, sandbox::JOB_UNPROTECTED, 0, policy);
      policy->SetDelayedIntegrityLevel(sandbox::INTEGRITY_LEVEL_LOW);
    } else {
      policy->SetTokenLevel(sandbox::USER_RESTRICTED_SAME_ACCESS,
                            sandbox::USER_LIMITED);

      // UI restrictions break when we access Windows from outside our job.
      // However, we don't want a proxy window in this process because it can
      // introduce deadlocks where the renderer blocks on the gpu, which in
      // turn blocks on the browser UI thread. So, instead we forgo a window
      // message pump entirely and just add job restrictions to prevent child
      // processes.
      service_manager::SandboxWin::SetJobLevel(
          cmd_line_, sandbox::JOB_LIMITED_USER,
          JOB_OBJECT_UILIMIT_SYSTEMPARAMETERS | JOB_OBJECT_UILIMIT_DESKTOP |
              JOB_OBJECT_UILIMIT_EXITWINDOWS |
              JOB_OBJECT_UILIMIT_DISPLAYSETTINGS,
          policy);
      policy->SetIntegrityLevel(sandbox::INTEGRITY_LEVEL_LOW);
    }

    // Block this DLL even if it is not loaded by the browser process.
    policy->AddDllToUnload(L"cmsetac.dll");

    if (cmd_line_.HasSwitch(switches::kEnableLogging)) {
      base::string16 log_file_path = logging::GetLogFileFullPath();
      if (!log_file_path.empty()) {
        sandbox::ResultCode result = policy->AddRule(
            sandbox::TargetPolicy::SUBSYS_FILES,
            sandbox::TargetPolicy::FILES_ALLOW_ANY, log_file_path.c_str());
        if (result != sandbox::SBOX_ALL_OK)
          return false;
      }
    }

    return true;
  }

  // TODO: Remove this once AppContainer sandbox is enabled by default.
  void DisableAppContainer() { enable_appcontainer_ = false; }
#endif  // OS_WIN

  service_manager::SandboxType GetSandboxType() override {
#if defined(OS_WIN)
    if (cmd_line_.HasSwitch(service_manager::switches::kDisableGpuSandbox)) {
      DVLOG(1) << "GPU sandbox is disabled";
      return service_manager::SANDBOX_TYPE_NO_SANDBOX;
    }
#endif
    return service_manager::SANDBOX_TYPE_GPU;
  }

 private:
#if defined(OS_WIN)
  bool UseOpenGLRenderer() {
    return cmd_line_.GetSwitchValueASCII(switches::kUseGL) ==
           gl::kGLImplementationDesktopName;
  }

  base::CommandLine cmd_line_;
  bool enable_appcontainer_;
#endif  // OS_WIN
};

#if defined(OS_ANDROID)
template <typename Interface>
void BindJavaInterface(mojo::InterfaceRequest<Interface> request) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  content::GetGlobalJavaInterfaces()->GetInterface(std::move(request));
}
#endif  // defined(OS_ANDROID)

#if defined(OS_WIN)
void RecordAppContainerStatus(int error_code, bool crashed_before) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (!crashed_before &&
      service_manager::SandboxWin::IsAppContainerEnabledForSandbox(
          *command_line, service_manager::SANDBOX_TYPE_GPU)) {
    base::UmaHistogramSparse("GPU.AppContainer.Status", error_code);
  }
}
#endif  // defined(OS_WIN)

void BindDiscardableMemoryRequestOnIO(
    discardable_memory::mojom::DiscardableSharedMemoryManagerRequest request,
    discardable_memory::DiscardableSharedMemoryManager* manager) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  service_manager::BindSourceInfo source_info;
  manager->Bind(std::move(request), source_info);
}

void BindDiscardableMemoryRequestOnUI(
    discardable_memory::mojom::DiscardableSharedMemoryManagerRequest request) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

#if defined(USE_AURA)
  if (features::IsMashEnabled()) {
    ServiceManagerConnection::GetForProcess()->GetConnector()->BindInterface(
        ui::mojom::kServiceName, std::move(request));
    return;
  }
#endif
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(
          &BindDiscardableMemoryRequestOnIO, std::move(request),
          BrowserMainLoop::GetInstance()->discardable_shared_memory_manager()));
}

}  // anonymous namespace

class GpuProcessHost::ConnectionFilterImpl : public ConnectionFilter {
 public:
  ConnectionFilterImpl() {
    auto task_runner = BrowserThread::GetTaskRunnerForThread(BrowserThread::UI);
    registry_.AddInterface(base::Bind(&FieldTrialRecorder::Create),
                           task_runner);
#if defined(OS_ANDROID)
    registry_.AddInterface(
        base::Bind(&BindJavaInterface<media::mojom::AndroidOverlayProvider>),
        task_runner);
#endif
  }

 private:
  // ConnectionFilter:
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle* interface_pipe,
                       service_manager::Connector* connector) override {
    if (!registry_.TryBindInterface(interface_name, interface_pipe)) {
      GetContentClient()->browser()->BindInterfaceRequest(
          source_info, interface_name, interface_pipe);
    }
  }

  service_manager::BinderRegistry registry_;

  DISALLOW_COPY_AND_ASSIGN(ConnectionFilterImpl);
};

// static
bool GpuProcessHost::ValidateHost(GpuProcessHost* host) {
  // The Gpu process is invalid if it's not using SwiftShader, the card is
  // blacklisted, and we can kill it and start over.
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kSingleProcess) ||
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kInProcessGPU) ||
      host->valid_) {
    return true;
  }

  host->ForceShutdown();
  return false;
}

// static
GpuProcessHost* GpuProcessHost::Get(GpuProcessKind kind, bool force_create) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  // Don't grant further access to GPU if it is not allowed.
  GpuDataManagerImpl* gpu_data_manager = GpuDataManagerImpl::GetInstance();
  DCHECK(gpu_data_manager);
  if (!gpu_data_manager->GpuProcessStartAllowed()) {
    DLOG(ERROR) << "!GpuDataManagerImpl::GpuProcessStartAllowed()";
    return nullptr;
  }

  if (g_gpu_process_hosts[kind] && ValidateHost(g_gpu_process_hosts[kind]))
    return g_gpu_process_hosts[kind];

  if (!force_create)
    return nullptr;

  // Do not create a new process if browser is shutting down.
  if (BrowserMainRunner::ExitedMainMessageLoop()) {
    DLOG(ERROR) << "BrowserMainRunner::ExitedMainMessageLoop()";
    return nullptr;
  }

  static int last_host_id = 0;
  int host_id;
  host_id = ++last_host_id;

  GpuProcessHost* host = new GpuProcessHost(host_id, kind);
  if (host->Init())
    return host;

  // TODO(sievers): Revisit this behavior. It's not really a crash, but we also
  // want the fallback-to-sw behavior if we cannot initialize the GPU.
  host->RecordProcessCrash();

  delete host;
  DLOG(ERROR) << "GpuProcessHost::Init() failed";
  return nullptr;
}

// static
void GpuProcessHost::GetHasGpuProcess(base::OnceCallback<void(bool)> callback) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&GpuProcessHost::GetHasGpuProcess, std::move(callback)));
    return;
  }
  bool has_gpu = false;
  for (size_t i = 0; i < arraysize(g_gpu_process_hosts); ++i) {
    GpuProcessHost* host = g_gpu_process_hosts[i];
    if (host && ValidateHost(host)) {
      has_gpu = true;
      break;
    }
  }
  std::move(callback).Run(has_gpu);
}

// static
void GpuProcessHost::CallOnIO(
    GpuProcessKind kind,
    bool force_create,
    const base::Callback<void(GpuProcessHost*)>& callback) {
#if !defined(OS_WIN)
  DCHECK_NE(kind, GpuProcessHost::GPU_PROCESS_KIND_UNSANDBOXED);
#endif
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&RunCallbackOnIO, kind, force_create, callback));
}

void GpuProcessHost::BindInterface(
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  if (interface_name ==
      discardable_memory::mojom::DiscardableSharedMemoryManager::Name_) {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(
            &BindDiscardableMemoryRequestOnUI,
            discardable_memory::mojom::DiscardableSharedMemoryManagerRequest(
                std::move(interface_pipe))));
    return;
  }
  process_->child_connection()->BindInterface(interface_name,
                                              std::move(interface_pipe));
}

// static
GpuProcessHost* GpuProcessHost::FromID(int host_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  for (int i = 0; i < GPU_PROCESS_KIND_COUNT; ++i) {
    GpuProcessHost* host = g_gpu_process_hosts[i];
    if (host && host->host_id_ == host_id && ValidateHost(host))
      return host;
  }

  return nullptr;
}

// static
int GpuProcessHost::GetGpuCrashCount() {
  return static_cast<int>(base::subtle::NoBarrier_Load(&gpu_crash_count_));
}

// static
void GpuProcessHost::IncrementCrashCount(int* crash_count) {
  // Last time the process crashed.
  static base::Time last_crash_time;

  // Allow about 1 crash per hour to be removed from the crash count, so very
  // occasional crashes won't eventually add up and prevent the process from
  // launching.
  base::Time current_time = base::Time::Now();
  if (crashed_before_) {
    int hours_different = (current_time - last_crash_time).InHours();
    *crash_count = std::max(0, *crash_count - hours_different);
  }
  ++(*crash_count);

  crashed_before_ = true;
  last_crash_time = current_time;
}

GpuProcessHost::GpuProcessHost(int host_id, GpuProcessKind kind)
    : host_id_(host_id),
      valid_(true),
      in_process_(false),
      kind_(kind),
      process_launched_(false),
      status_(UNKNOWN),
      gpu_host_binding_(this),
      weak_ptr_factory_(this) {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kSingleProcess) ||
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kInProcessGPU)) {
    in_process_ = true;
  }

  // If the 'single GPU process' policy ever changes, we still want to maintain
  // it for 'gpu thread' mode and only create one instance of host and thread.
  DCHECK(!in_process_ || g_gpu_process_hosts[kind] == nullptr);

  g_gpu_process_hosts[kind] = this;

  process_.reset(new BrowserChildProcessHostImpl(
      PROCESS_TYPE_GPU, this, mojom::kGpuServiceName));
}

GpuProcessHost::~GpuProcessHost() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (in_process_gpu_thread_)
    DCHECK(process_);

  SendOutstandingReplies(EstablishChannelStatus::GPU_HOST_INVALID);

  if (status_ == UNKNOWN) {
    RunRequestGPUInfoCallbacks(gpu::GPUInfo());
  } else {
    DCHECK(request_gpu_info_callbacks_.empty());
  }

  // In case we never started, clean up.
  while (!queued_messages_.empty()) {
    delete queued_messages_.front();
    queued_messages_.pop();
  }

  // This is only called on the IO thread so no race against the constructor
  // for another GpuProcessHost.
  if (g_gpu_process_hosts[kind_] == this)
    g_gpu_process_hosts[kind_] = nullptr;

#if defined(OS_ANDROID)
  UMA_HISTOGRAM_COUNTS_100("GPU.AtExitSurfaceCount",
                           gpu::GpuSurfaceTracker::Get()->GetSurfaceCount());
#endif

  std::string message;
  bool block_offscreen_contexts = true;
  if (!in_process_ && process_launched_) {
    ChildProcessTerminationInfo info =
        process_->GetTerminationInfo(false /* known_dead */);
    UMA_HISTOGRAM_ENUMERATION("GPU.GPUProcessTerminationStatus2",
                              ConvertToGpuTerminationStatus(info.status),
                              GpuTerminationStatus::MAX_ENUM);

    if (info.status == base::TERMINATION_STATUS_NORMAL_TERMINATION ||
        info.status == base::TERMINATION_STATUS_ABNORMAL_TERMINATION ||
        info.status == base::TERMINATION_STATUS_PROCESS_CRASHED) {
      // Windows always returns PROCESS_CRASHED on abnormal termination, as it
      // doesn't have a way to distinguish the two.
      base::UmaHistogramSparse("GPU.GPUProcessExitCode",
                               std::max(0, std::min(100, info.exit_code)));
    }

    switch (info.status) {
      case base::TERMINATION_STATUS_NORMAL_TERMINATION:
        // Don't block offscreen contexts (and force page reload for webgl)
        // if this was an intentional shutdown or the OOM killer on Android
        // killed us while Chrome was in the background.
// TODO(crbug.com/598400): Restrict this to Android for now, since other
// platforms might fall through here for the 'exit_on_context_lost' workaround.
#if defined(OS_ANDROID)
        block_offscreen_contexts = false;
#endif
        message = "The GPU process exited normally. Everything is okay.";
        break;
      case base::TERMINATION_STATUS_ABNORMAL_TERMINATION:
        message = base::StringPrintf("The GPU process exited with code %d.",
                                     info.exit_code);
        break;
      case base::TERMINATION_STATUS_PROCESS_WAS_KILLED:
        message = "You killed the GPU process! Why?";
        break;
#if defined(OS_CHROMEOS)
      case base::TERMINATION_STATUS_PROCESS_WAS_KILLED_BY_OOM:
        message = "The GUP process was killed due to out of memory.";
        break;
#endif
      case base::TERMINATION_STATUS_PROCESS_CRASHED:
        message = "The GPU process crashed!";
        break;
      case base::TERMINATION_STATUS_LAUNCH_FAILED:
        message = "The GPU process failed to start!";
        break;
      default:
        break;
    }
  }

  // If there are any remaining offscreen contexts at the point the
  // GPU process exits, assume something went wrong, and block their
  // URLs from accessing client 3D APIs without prompting.
  if (block_offscreen_contexts)
    BlockLiveOffscreenContexts();

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&OnGpuProcessHostDestroyedOnUI, host_id_, message));
}

#if defined(USE_OZONE)
void GpuProcessHost::InitOzone() {
  // Ozone needs to send the primary DRM device to GPU process as early as
  // possible to ensure the latter always has a valid device. crbug.com/608839
  // When running with mus, the OzonePlatform may not have been created yet. So
  // defer the callback until OzonePlatform instance is created.
  base::CommandLine* browser_command_line =
      base::CommandLine::ForCurrentProcess();
  if (browser_command_line->HasSwitch(switches::kEnableDrmMojo)) {
    // TODO(rjkroege): Remove the legacy IPC code paths when no longer
    // necessary. https://crbug.com/806092
    auto interface_binder = base::BindRepeating(&GpuProcessHost::BindInterface,
                                                weak_ptr_factory_.GetWeakPtr());

    auto io_callback = base::BindOnce(
        [](const base::RepeatingCallback<void(const std::string&,
                                              mojo::ScopedMessagePipeHandle)>&
               interface_binder,
           ui::OzonePlatform* platform) {
          DCHECK_CURRENTLY_ON(BrowserThread::IO);
          platform->GetGpuPlatformSupportHost()->OnGpuServiceLaunched(
              BrowserThread::GetTaskRunnerForThread(BrowserThread::UI),
              BrowserThread::GetTaskRunnerForThread(BrowserThread::IO),
              interface_binder);
        },
        interface_binder);

    OzoneRegisterStartupCallbackHelper(std::move(io_callback));
  } else {
    auto send_callback = base::BindRepeating(&SendGpuProcessMessage,
                                             weak_ptr_factory_.GetWeakPtr());
    // Create the callback that should run on the current thread (i.e. IO
    // thread).
    auto io_callback = base::BindOnce(
        [](int host_id,
           const base::RepeatingCallback<void(IPC::Message*)>& send_callback,
           ui::OzonePlatform* platform) {
          DCHECK_CURRENTLY_ON(BrowserThread::IO);
          platform->GetGpuPlatformSupportHost()->OnGpuProcessLaunched(
              host_id, BrowserThread::GetTaskRunnerForThread(BrowserThread::UI),
              BrowserThread::GetTaskRunnerForThread(BrowserThread::IO),
              send_callback);
        },
        host_id_, send_callback);
    OzoneRegisterStartupCallbackHelper(std::move(io_callback));
  }
}
#endif  // defined(USE_OZONE)

bool GpuProcessHost::Init() {
  init_start_time_ = base::TimeTicks::Now();

  TRACE_EVENT_INSTANT0("gpu", "LaunchGpuProcess", TRACE_EVENT_SCOPE_THREAD);

  // May be null during test execution.
  if (ServiceManagerConnection::GetForProcess()) {
    ServiceManagerConnection::GetForProcess()->AddConnectionFilter(
        std::make_unique<ConnectionFilterImpl>());
  }

  process_->GetHost()->CreateChannelMojo();

  if (in_process_) {
    DCHECK_CURRENTLY_ON(BrowserThread::IO);
    DCHECK(GetGpuMainThreadFactory());
    gpu::GpuPreferences gpu_preferences = GetGpuPreferencesFromCommandLine();
    GpuDataManagerImpl::GetInstance()->UpdateGpuPreferences(&gpu_preferences);
    in_process_gpu_thread_.reset(GetGpuMainThreadFactory()(
        InProcessChildThreadParams(
            base::ThreadTaskRunnerHandle::Get(),
            process_->GetInProcessBrokerClientInvitation(),
            process_->child_connection()->service_token()),
        gpu_preferences));
    base::Thread::Options options;
#if defined(OS_WIN) || defined(OS_MACOSX)
    // WGL needs to create its own window and pump messages on it.
    options.message_loop_type = base::MessageLoop::TYPE_UI;
#endif
#if defined(OS_ANDROID) || defined(OS_CHROMEOS)
    options.priority = base::ThreadPriority::DISPLAY;
#endif
    in_process_gpu_thread_->StartWithOptions(options);

    OnProcessLaunched();  // Fake a callback that the process is ready.
  } else if (!LaunchGpuProcess()) {
    return false;
  }

  process_->child_channel()
      ->GetAssociatedInterfaceSupport()
      ->GetRemoteAssociatedInterface(&gpu_main_ptr_);
  viz::mojom::GpuHostPtr host_proxy;
  gpu_host_binding_.Bind(mojo::MakeRequest(&host_proxy));

  discardable_memory::mojom::DiscardableSharedMemoryManagerPtr
      discardable_manager_ptr;
  auto discardable_request = mojo::MakeRequest(&discardable_manager_ptr);
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::BindOnce(&BindDiscardableMemoryRequestOnUI,
                                         std::move(discardable_request)));

  gpu_main_ptr_->CreateGpuService(
      mojo::MakeRequest(&gpu_service_ptr_), std::move(host_proxy),
      std::move(discardable_manager_ptr), activity_flags_.CloneHandle());

#if defined(USE_OZONE)
  InitOzone();
#endif  // defined(USE_OZONE)

  return true;
}

bool GpuProcessHost::Send(IPC::Message* msg) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (process_->GetHost()->IsChannelOpening()) {
    queued_messages_.push(msg);
    return true;
  }

  bool result = process_->Send(msg);
  if (!result) {
    // Channel is hosed, but we may not get destroyed for a while. Send
    // outstanding channel creation failures now so that the caller can restart
    // with a new process/channel without waiting.
    SendOutstandingReplies(EstablishChannelStatus::GPU_HOST_INVALID);
  }
  return result;
}

bool GpuProcessHost::OnMessageReceived(const IPC::Message& message) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
#if defined(USE_OZONE)
  ui::OzonePlatform::GetInstance()
      ->GetGpuPlatformSupportHost()
      ->OnMessageReceived(message);
#endif
  return true;
}

void GpuProcessHost::OnChannelConnected(int32_t peer_pid) {
  TRACE_EVENT0("gpu", "GpuProcessHost::OnChannelConnected");

  while (!queued_messages_.empty()) {
    Send(queued_messages_.front());
    queued_messages_.pop();
  }
}

void GpuProcessHost::EstablishGpuChannel(
    int client_id,
    uint64_t client_tracing_id,
    bool preempts,
    bool allow_view_command_buffers,
    bool allow_real_time_streams,
    const EstablishChannelCallback& callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  TRACE_EVENT0("gpu", "GpuProcessHost::EstablishGpuChannel");

  // If GPU features are already blacklisted, no need to establish the channel.
  if (!GpuDataManagerImpl::GetInstance()->GpuAccessAllowed(nullptr)) {
    DVLOG(1) << "GPU blacklisted, refusing to open a GPU channel.";
    callback.Run(mojo::ScopedMessagePipeHandle(), gpu::GPUInfo(),
                 gpu::GpuFeatureInfo(),
                 EstablishChannelStatus::GPU_ACCESS_DENIED);
    return;
  }

  DCHECK_EQ(preempts, allow_view_command_buffers);
  DCHECK_EQ(preempts, allow_real_time_streams);
  bool is_gpu_host = preempts;

  channel_requests_.push(callback);
  gpu_service_ptr_->EstablishGpuChannel(
      client_id, client_tracing_id, is_gpu_host,
      base::BindOnce(&GpuProcessHost::OnChannelEstablished,
                     weak_ptr_factory_.GetWeakPtr(), client_id, callback));

  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kDisableGpuShaderDiskCache)) {
    CreateChannelCache(client_id);
  }
}

void GpuProcessHost::CreateGpuMemoryBuffer(
    gfx::GpuMemoryBufferId id,
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    int client_id,
    gpu::SurfaceHandle surface_handle,
    CreateGpuMemoryBufferCallback callback) {
  TRACE_EVENT0("gpu", "GpuProcessHost::CreateGpuMemoryBuffer");

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  create_gpu_memory_buffer_requests_.push(std::move(callback));
  gpu_service_ptr_->CreateGpuMemoryBuffer(
      id, size, format, usage, client_id, surface_handle,
      base::BindOnce(&GpuProcessHost::OnGpuMemoryBufferCreated,
                     weak_ptr_factory_.GetWeakPtr()));
}

void GpuProcessHost::DestroyGpuMemoryBuffer(gfx::GpuMemoryBufferId id,
                                            int client_id,
                                            const gpu::SyncToken& sync_token) {
  TRACE_EVENT0("gpu", "GpuProcessHost::DestroyGpuMemoryBuffer");
  gpu_service_ptr_->DestroyGpuMemoryBuffer(id, client_id, sync_token);
}

void GpuProcessHost::ConnectFrameSinkManager(
    viz::mojom::FrameSinkManagerRequest request,
    viz::mojom::FrameSinkManagerClientPtrInfo client) {
  TRACE_EVENT0("gpu", "GpuProcessHost::ConnectFrameSinkManager");
  viz::mojom::FrameSinkManagerParamsPtr params =
      viz::mojom::FrameSinkManagerParams::New();
  params->restart_id = host_id_;
  base::Optional<uint32_t> activation_deadline_in_frames =
      switches::GetDeadlineToSynchronizeSurfaces();
  params->use_activation_deadline = activation_deadline_in_frames.has_value();
  params->activation_deadline_in_frames =
      activation_deadline_in_frames.value_or(0u);
  params->frame_sink_manager = std::move(request);
  params->frame_sink_manager_client = std::move(client);
  gpu_main_ptr_->CreateFrameSinkManager(std::move(params));
}

void GpuProcessHost::RequestGPUInfo(RequestGPUInfoCallback request_cb) {
  if (status_ == SUCCESS || status_ == FAILURE) {
    std::move(request_cb).Run(GpuDataManagerImpl::GetInstance()->GetGPUInfo());
    return;
  }

  request_gpu_info_callbacks_.push_back(std::move(request_cb));
}

void GpuProcessHost::RequestHDRStatus(RequestHDRStatusCallback request_cb) {
  gpu_service_ptr_->RequestHDRStatus(std::move(request_cb));
}

#if defined(OS_ANDROID)
void GpuProcessHost::SendDestroyingVideoSurface(int surface_id,
                                                const base::Closure& done_cb) {
  TRACE_EVENT0("gpu", "GpuProcessHost::SendDestroyingVideoSurface");
  DCHECK(send_destroying_video_surface_done_cb_.is_null());
  DCHECK(!done_cb.is_null());
  send_destroying_video_surface_done_cb_ = done_cb;
  gpu_service_ptr_->DestroyingVideoSurface(
      surface_id, base::Bind(&GpuProcessHost::OnDestroyingVideoSurfaceAck,
                             weak_ptr_factory_.GetWeakPtr()));
}
#endif

void GpuProcessHost::OnChannelEstablished(
    int client_id,
    const EstablishChannelCallback& callback,
    mojo::ScopedMessagePipeHandle channel_handle) {
  TRACE_EVENT0("gpu", "GpuProcessHost::OnChannelEstablished");
  DCHECK(!channel_requests_.empty());
  DCHECK(channel_requests_.front().Equals(callback));
  channel_requests_.pop();

  auto* gpu_data_manager = GpuDataManagerImpl::GetInstance();
  // Currently if any of the GPU features are blacklisted, we don't establish a
  // GPU channel.
  if (channel_handle.is_valid() &&
      !gpu_data_manager->GpuAccessAllowed(nullptr)) {
    gpu_service_ptr_->CloseChannel(client_id);
    callback.Run(mojo::ScopedMessagePipeHandle(), gpu::GPUInfo(),
                 gpu::GpuFeatureInfo(),
                 EstablishChannelStatus::GPU_ACCESS_DENIED);
    RecordLogMessage(logging::LOG_WARNING, "WARNING",
                     "Hardware acceleration is unavailable.");
    return;
  }

  callback.Run(std::move(channel_handle), gpu_data_manager->GetGPUInfo(),
               gpu_data_manager->GetGpuFeatureInfo(),
               EstablishChannelStatus::SUCCESS);
}

void GpuProcessHost::OnGpuMemoryBufferCreated(
    const gfx::GpuMemoryBufferHandle& handle) {
  TRACE_EVENT0("gpu", "GpuProcessHost::OnGpuMemoryBufferCreated");

  DCHECK(!create_gpu_memory_buffer_requests_.empty());
  auto callback = std::move(create_gpu_memory_buffer_requests_.front());
  create_gpu_memory_buffer_requests_.pop();
  std::move(callback).Run(handle, BufferCreationStatus::SUCCESS);
}

#if defined(OS_ANDROID)
void GpuProcessHost::OnDestroyingVideoSurfaceAck() {
  TRACE_EVENT0("gpu", "GpuProcessHost::OnDestroyingVideoSurfaceAck");
  if (!send_destroying_video_surface_done_cb_.is_null())
    base::ResetAndReturn(&send_destroying_video_surface_done_cb_).Run();
}
#endif

void GpuProcessHost::OnProcessLaunched() {
  UMA_HISTOGRAM_TIMES("GPU.GPUProcessLaunchTime",
                      base::TimeTicks::Now() - init_start_time_);
#if defined(OS_WIN)
  if (kind_ == GPU_PROCESS_KIND_SANDBOXED)
    RecordAppContainerStatus(sandbox::SBOX_ALL_OK, crashed_before_);
#endif  // defined(OS_WIN)
}

void GpuProcessHost::OnProcessLaunchFailed(int error_code) {
#if defined(OS_WIN)
  if (kind_ == GPU_PROCESS_KIND_SANDBOXED)
    RecordAppContainerStatus(error_code, crashed_before_);
#endif  // defined(OS_WIN)
  RecordProcessCrash();
}

void GpuProcessHost::OnProcessCrashed(int exit_code) {
  // If the GPU process crashed while compiling a shader, we may have invalid
  // cached binaries. Completely clear the shader cache to force shader binaries
  // to be re-created.
  if (activity_flags_.IsFlagSet(
          gpu::ActivityFlagsBase::FLAG_LOADING_PROGRAM_BINARY)) {
    for (auto cache_key : client_id_to_shader_cache_) {
      // This call will temporarily extend the lifetime of the cache (kept
      // alive in the factory), and may drop loads of cached shader binaries if
      // it takes a while to complete. As we are intentionally dropping all
      // binaries, this behavior is fine.
      GetShaderCacheFactorySingleton()->ClearByClientId(
          cache_key.first, base::Time(), base::Time::Max(), base::Bind([] {}));
    }
  }
  SendOutstandingReplies(EstablishChannelStatus::GPU_HOST_INVALID);
  RecordProcessCrash();

  ChildProcessTerminationInfo info =
      process_->GetTerminationInfo(true /* known_dead */);
  GpuDataManagerImpl::GetInstance()->ProcessCrashed(info.status);
}

void GpuProcessHost::DidInitialize(
    const gpu::GPUInfo& gpu_info,
    const gpu::GpuFeatureInfo& gpu_feature_info,
    const base::Optional<gpu::GPUInfo>& gpu_info_for_hardware_gpu,
    const base::Optional<gpu::GpuFeatureInfo>&
        gpu_feature_info_for_hardware_gpu) {
  UMA_HISTOGRAM_BOOLEAN("GPU.GPUProcessInitialized", true);
  status_ = SUCCESS;

  // Set GPU driver bug workaround flags that are checked on the browser side.
  if (gpu_feature_info.IsWorkaroundEnabled(gpu::WAKE_UP_GPU_BEFORE_DRAWING)) {
    wake_up_gpu_before_drawing_ = true;
  }
  if (gpu_feature_info.IsWorkaroundEnabled(
          gpu::DONT_DISABLE_WEBGL_WHEN_COMPOSITOR_CONTEXT_LOST)) {
    dont_disable_webgl_when_compositor_context_lost_ = true;
  }

  GpuDataManagerImpl* gpu_data_manager = GpuDataManagerImpl::GetInstance();
  // Update GpuFeatureInfo first, because UpdateGpuInfo() will notify all
  // listeners.
  gpu_data_manager->UpdateGpuFeatureInfo(gpu_feature_info,
                                         gpu_feature_info_for_hardware_gpu);
  gpu_data_manager->UpdateGpuInfo(gpu_info, gpu_info_for_hardware_gpu);
  RunRequestGPUInfoCallbacks(gpu_data_manager->GetGPUInfo());
}

void GpuProcessHost::DidFailInitialize() {
  UMA_HISTOGRAM_BOOLEAN("GPU.GPUProcessInitialized", false);
  status_ = FAILURE;
  GpuDataManagerImpl* gpu_data_manager = GpuDataManagerImpl::GetInstance();
  gpu_data_manager->OnGpuProcessInitFailure();
  RunRequestGPUInfoCallbacks(gpu_data_manager->GetGPUInfo());
}

void GpuProcessHost::DidCreateContextSuccessfully() {
#if defined(OS_ANDROID)
  // Android may kill the GPU process to free memory, especially when the app
  // is the background, so Android cannot have a hard limit on GPU starts.
  // Reset crash count on Android when context creation succeeds.
  gpu_recent_crash_count_ = 0;
#endif
}

void GpuProcessHost::DidCreateOffscreenContext(const GURL& url) {
  urls_with_live_offscreen_contexts_.insert(url);
}

void GpuProcessHost::DidDestroyOffscreenContext(const GURL& url) {
  // We only want to remove *one* of the entries in the multiset for
  // this particular URL, so can't use the erase method taking a key.
  auto candidate = urls_with_live_offscreen_contexts_.find(url);
  if (candidate != urls_with_live_offscreen_contexts_.end()) {
    urls_with_live_offscreen_contexts_.erase(candidate);
  }
}

void GpuProcessHost::DidDestroyChannel(int32_t client_id) {
  TRACE_EVENT0("gpu", "GpuProcessHost::DidDestroyChannel");
  client_id_to_shader_cache_.erase(client_id);
}

void GpuProcessHost::DidLoseContext(bool offscreen,
                                    gpu::error::ContextLostReason reason,
                                    const GURL& active_url) {
  // TODO(kbr): would be nice to see the "offscreen" flag too.
  TRACE_EVENT2("gpu", "GpuProcessHost::DidLoseContext", "reason", reason, "url",
               active_url.possibly_invalid_spec());

  if (!offscreen || active_url.is_empty()) {
    // Assume that the loss of the compositor's or accelerated canvas'
    // context is a serious event and blame the loss on all live
    // offscreen contexts. This more robustly handles situations where
    // the GPU process may not actually detect the context loss in the
    // offscreen context. However, situations have been seen where the
    // compositor's context can be lost due to driver bugs (as of this
    // writing, on Android), so allow that possibility.
    if (!dont_disable_webgl_when_compositor_context_lost_) {
      BlockLiveOffscreenContexts();
    }
    return;
  }

  GpuDataManagerImpl::DomainGuilt guilt =
      GpuDataManagerImpl::DOMAIN_GUILT_UNKNOWN;
  switch (reason) {
    case gpu::error::kGuilty:
      guilt = GpuDataManagerImpl::DOMAIN_GUILT_KNOWN;
      break;
    // Treat most other error codes as though they had unknown provenance.
    // In practice this doesn't affect the user experience. A lost context
    // of either known or unknown guilt still causes user-level 3D APIs
    // (e.g. WebGL) to be blocked on that domain until the user manually
    // reenables them.
    case gpu::error::kUnknown:
    case gpu::error::kOutOfMemory:
    case gpu::error::kMakeCurrentFailed:
    case gpu::error::kGpuChannelLost:
    case gpu::error::kInvalidGpuMessage:
      break;
    case gpu::error::kInnocent:
      return;
  }

  GpuDataManagerImpl::GetInstance()->BlockDomainFrom3DAPIs(active_url, guilt);
}

void GpuProcessHost::SetChildSurface(gpu::SurfaceHandle parent_handle,
                                     gpu::SurfaceHandle window_handle) {
#if defined(OS_WIN)
  constexpr char kBadMessageError[] = "Bad parenting request from gpu process.";
  if (!in_process_) {
    DCHECK(process_);

    DWORD parent_process_id = 0;
    DWORD parent_thread_id =
        GetWindowThreadProcessId(parent_handle, &parent_process_id);
    if (!parent_thread_id || parent_process_id != ::GetCurrentProcessId()) {
      LOG(ERROR) << kBadMessageError;
      return;
    }

    DWORD child_process_id = 0;
    DWORD child_thread_id =
        GetWindowThreadProcessId(window_handle, &child_process_id);
    if (!child_thread_id || child_process_id != process_->GetProcess().Pid()) {
      LOG(ERROR) << kBadMessageError;
      return;
    }
  }

  if (!gfx::RenderingWindowManager::GetInstance()->RegisterChild(
          parent_handle, window_handle)) {
    LOG(ERROR) << kBadMessageError;
  }
#endif
}

void GpuProcessHost::StoreShaderToDisk(int32_t client_id,
                                       const std::string& key,
                                       const std::string& shader) {
  TRACE_EVENT0("gpu", "GpuProcessHost::StoreShaderToDisk");
  ClientIdToShaderCacheMap::iterator iter =
      client_id_to_shader_cache_.find(client_id);
  // If the cache doesn't exist then this is an off the record profile.
  if (iter == client_id_to_shader_cache_.end())
    return;
  iter->second->Cache(GetShaderPrefixKey() + ":" + key, shader);
}

void GpuProcessHost::RecordLogMessage(int32_t severity,
                                      const std::string& header,
                                      const std::string& message) {
  GpuDataManagerImpl::GetInstance()->AddLogMessage(severity, header, message);
}

GpuProcessHost::GpuProcessKind GpuProcessHost::kind() {
  return kind_;
}

void GpuProcessHost::ForceShutdown() {
  // This is only called on the IO thread so no race against the constructor
  // for another GpuProcessHost.
  if (g_gpu_process_hosts[kind_] == this)
    g_gpu_process_hosts[kind_] = nullptr;

  process_->ForceShutdown();
}

bool GpuProcessHost::LaunchGpuProcess() {
  const base::CommandLine& browser_command_line =
      *base::CommandLine::ForCurrentProcess();

  base::CommandLine::StringType gpu_launcher =
      browser_command_line.GetSwitchValueNative(switches::kGpuLauncher);

#if defined(OS_ANDROID)
  // crbug.com/447735. readlink("self/proc/exe") sometimes fails on Android
  // at startup with EACCES. As a workaround ignore this here, since the
  // executable name is actually not used or useful anyways.
  std::unique_ptr<base::CommandLine> cmd_line =
      std::make_unique<base::CommandLine>(base::CommandLine::NO_PROGRAM);
#else
#if defined(OS_LINUX)
  int child_flags = gpu_launcher.empty() ? ChildProcessHost::CHILD_ALLOW_SELF :
                                           ChildProcessHost::CHILD_NORMAL;
#else
  int child_flags = ChildProcessHost::CHILD_NORMAL;
#endif

  base::FilePath exe_path = ChildProcessHost::GetChildPath(child_flags);
  if (exe_path.empty())
    return false;

  std::unique_ptr<base::CommandLine> cmd_line =
      std::make_unique<base::CommandLine>(exe_path);
#endif

  cmd_line->AppendSwitchASCII(switches::kProcessType, switches::kGpuProcess);

  BrowserChildProcessHostImpl::CopyFeatureAndFieldTrialFlags(cmd_line.get());
  BrowserChildProcessHostImpl::CopyTraceStartupFlags(cmd_line.get());

#if defined(OS_WIN)
  cmd_line->AppendArg(switches::kPrefetchArgumentGpu);
#endif  // defined(OS_WIN)

  if (kind_ == GPU_PROCESS_KIND_UNSANDBOXED)
    cmd_line->AppendSwitch(service_manager::switches::kDisableGpuSandbox);

  // TODO(penghuang): Replace all GPU related switches with GpuPreferences.
  // https://crbug.com/590825
  // If you want a browser command-line switch passed to the GPU process
  // you need to add it to |kSwitchNames| at the beginning of this file.
  cmd_line->CopySwitchesFrom(browser_command_line, kSwitchNames,
                             arraysize(kSwitchNames));
  cmd_line->CopySwitchesFrom(
      browser_command_line, switches::kGLSwitchesCopiedFromGpuProcessHost,
      switches::kGLSwitchesCopiedFromGpuProcessHostNumSwitches);

  if (browser_command_line.HasSwitch(switches::kDisableFrameRateLimit))
    cmd_line->AppendSwitch(switches::kDisableGpuVsync);

  std::vector<const char*> gpu_workarounds;
  gpu::GpuDriverBugList::AppendAllWorkarounds(&gpu_workarounds);
  cmd_line->CopySwitchesFrom(browser_command_line, gpu_workarounds.data(),
                             gpu_workarounds.size());

  GetContentClient()->browser()->AppendExtraCommandLineSwitches(
      cmd_line.get(), process_->GetData().id);

  GpuDataManagerImpl::GetInstance()->AppendGpuCommandLine(cmd_line.get());
  bool swiftshader_rendering =
      (cmd_line->GetSwitchValueASCII(switches::kUseGL) ==
       gl::kGLImplementationSwiftShaderForWebGLName);

  UMA_HISTOGRAM_BOOLEAN("GPU.GPUProcessSoftwareRendering",
                        swiftshader_rendering);

  // If specified, prepend a launcher program to the command line.
  if (!gpu_launcher.empty())
    cmd_line->PrependWrapper(gpu_launcher);

  std::unique_ptr<GpuSandboxedProcessLauncherDelegate> delegate =
      std::make_unique<GpuSandboxedProcessLauncherDelegate>(*cmd_line);
#if defined(OS_WIN)
  if (crashed_before_)
    delegate->DisableAppContainer();
#endif  // defined(OS_WIN)
  process_->Launch(std::move(delegate), std::move(cmd_line), true);
  process_launched_ = true;

  UMA_HISTOGRAM_ENUMERATION("GPU.GPUProcessLifetimeEvents",
                            LAUNCHED, GPU_PROCESS_LIFETIME_EVENT_MAX);
  return true;
}

void GpuProcessHost::SendOutstandingReplies(
    EstablishChannelStatus failure_status) {
  DCHECK_NE(failure_status, EstablishChannelStatus::SUCCESS);
  valid_ = false;

  // First send empty channel handles for all EstablishChannel requests.
  while (!channel_requests_.empty()) {
    auto callback = channel_requests_.front();
    channel_requests_.pop();
    std::move(callback).Run(mojo::ScopedMessagePipeHandle(), gpu::GPUInfo(),
                            gpu::GpuFeatureInfo(), failure_status);
  }

  while (!create_gpu_memory_buffer_requests_.empty()) {
    auto callback = std::move(create_gpu_memory_buffer_requests_.front());
    create_gpu_memory_buffer_requests_.pop();
    std::move(callback).Run(gfx::GpuMemoryBufferHandle(),
                            BufferCreationStatus::GPU_HOST_INVALID);
  }

  if (!send_destroying_video_surface_done_cb_.is_null())
    base::ResetAndReturn(&send_destroying_video_surface_done_cb_).Run();
}

void GpuProcessHost::RunRequestGPUInfoCallbacks(const gpu::GPUInfo& gpu_info) {
  for (auto& callback : request_gpu_info_callbacks_)
    std::move(callback).Run(gpu_info);

  request_gpu_info_callbacks_.clear();
}

void GpuProcessHost::BlockLiveOffscreenContexts() {
  for (std::multiset<GURL>::iterator iter =
           urls_with_live_offscreen_contexts_.begin();
       iter != urls_with_live_offscreen_contexts_.end(); ++iter) {
    GpuDataManagerImpl::GetInstance()->BlockDomainFrom3DAPIs(
        *iter, GpuDataManagerImpl::DOMAIN_GUILT_UNKNOWN);
  }
}

void GpuProcessHost::RecordProcessCrash() {
#if !defined(OS_ANDROID)
  // Maximum number of times the GPU process is allowed to crash in a session.
  // Once this limit is reached, any request to launch the GPU process will
  // fail.
  const int kGpuMaxCrashCount = 3;
#else
  // On android there is no way to recover without gpu, and the OS can kill the
  // gpu process arbitrarily, so use a higher count to allow for that.
  const int kGpuMaxCrashCount = 6;
#endif

  bool disable_crash_limit = base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kDisableGpuProcessCrashLimit);

  // Ending only acts as a failure if the GPU process was actually started and
  // was intended for actual rendering (and not just checking caps or other
  // options).
  if (process_launched_ && kind_ == GPU_PROCESS_KIND_SANDBOXED) {
    if (GpuDataManagerImpl::GetInstance()->HardwareAccelerationEnabled()) {
      int count = static_cast<int>(
          base::subtle::NoBarrier_AtomicIncrement(&gpu_crash_count_, 1));
      UMA_HISTOGRAM_EXACT_LINEAR(
          "GPU.GPUProcessLifetimeEvents",
          std::min(DIED_FIRST_TIME + count, GPU_PROCESS_LIFETIME_EVENT_MAX - 1),
          static_cast<int>(GPU_PROCESS_LIFETIME_EVENT_MAX));

      IncrementCrashCount(&gpu_recent_crash_count_);
      if ((gpu_recent_crash_count_ >= kGpuMaxCrashCount ||
           status_ == FAILURE) &&
          !disable_crash_limit) {
#if defined(OS_ANDROID)
        // Android can not fall back to software. If things are too unstable
        // then we just crash chrome to reset everything. Sorry.
        LOG(FATAL) << "Unable to start gpu process, giving up.";
#elif defined(OS_CHROMEOS)
        // ChromeOS also can not fall back to software. There we will just
        // keep retrying to make the gpu process forever. Good luck.
        DLOG(ERROR) << "Gpu process is unstable and crashing repeatedly, if "
                       "you didn't notice already.";
#else
        // The GPU process is too unstable to use. Disable it for current
        // session.
        GpuDataManagerImpl::GetInstance()->DisableHardwareAcceleration();
#endif
      }
    } else if (GpuDataManagerImpl::GetInstance()->SwiftShaderAllowed()) {
      UMA_HISTOGRAM_EXACT_LINEAR(
          "GPU.SwiftShaderLifetimeEvents",
          DIED_FIRST_TIME + swiftshader_crash_count_,
          static_cast<int>(GPU_PROCESS_LIFETIME_EVENT_MAX));
      ++swiftshader_crash_count_;

      IncrementCrashCount(&swiftshader_recent_crash_count_);
      if (swiftshader_recent_crash_count_ >= kGpuMaxCrashCount &&
          !disable_crash_limit) {
        // SwiftShader is too unstable to use. Disable it for current session.
        GpuDataManagerImpl::GetInstance()->BlockSwiftShader();
      }
    } else {
      UMA_HISTOGRAM_EXACT_LINEAR(
          "GPU.DisplayCompositorLifetimeEvents",
          DIED_FIRST_TIME + display_compositor_crash_count_,
          static_cast<int>(GPU_PROCESS_LIFETIME_EVENT_MAX));
      ++display_compositor_crash_count_;

      IncrementCrashCount(&display_compositor_recent_crash_count_);
      if (display_compositor_recent_crash_count_ >= kGpuMaxCrashCount &&
          !disable_crash_limit) {
        // Viz display compositor is too unstable. Crash chrome to reset
        // everything.
        LOG(FATAL) << "Unable to start viz process, giving up.";
      }
    }
  }
}

std::string GpuProcessHost::GetShaderPrefixKey() {
  if (shader_prefix_key_.empty()) {
    gpu::GPUInfo info = GpuDataManagerImpl::GetInstance()->GetGPUInfo();

    shader_prefix_key_ = GetContentClient()->GetProduct() + "-" +
                         info.gl_vendor + "-" + info.gl_renderer + "-" +
                         info.driver_version + "-" + info.driver_vendor;

#if defined(OS_ANDROID)
    std::string build_fp =
        base::android::BuildInfo::GetInstance()->android_build_fp();
    // TODO(ericrk): Remove this after it's up for a few days. crbug.com/699122
    CHECK(!build_fp.empty());
    shader_prefix_key_ += "-" + build_fp;
#endif
  }

  return shader_prefix_key_;
}

void GpuProcessHost::LoadedShader(const std::string& key,
                                  const std::string& data) {
  std::string prefix = GetShaderPrefixKey();
  bool prefix_ok = !key.compare(0, prefix.length(), prefix);
  UMA_HISTOGRAM_BOOLEAN("GPU.ShaderLoadPrefixOK", prefix_ok);
  if (prefix_ok) {
    // Remove the prefix from the key before load.
    std::string key_no_prefix = key.substr(prefix.length() + 1);
    gpu_service_ptr_->LoadedShader(key_no_prefix, data);
  }
}

viz::mojom::GpuService* GpuProcessHost::gpu_service() {
  DCHECK(gpu_service_ptr_.is_bound());
  return gpu_service_ptr_.get();
}

void GpuProcessHost::CreateChannelCache(int32_t client_id) {
  TRACE_EVENT0("gpu", "GpuProcessHost::CreateChannelCache");

  scoped_refptr<gpu::ShaderDiskCache> cache =
      GetShaderCacheFactorySingleton()->Get(client_id);
  if (!cache.get())
    return;

  cache->set_shader_loaded_callback(base::Bind(&GpuProcessHost::LoadedShader,
                                               weak_ptr_factory_.GetWeakPtr()));

  client_id_to_shader_cache_[client_id] = cache;
}

}  // namespace content
