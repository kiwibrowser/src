// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/utility_process_host.h"

#include <utility>

#include "base/base_switches.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/utf_string_conversions.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/browser/browser_child_process_host_impl.h"
#include "content/browser/renderer_host/render_process_host_impl.h"
#include "content/browser/service_manager/service_manager_context.h"
#include "content/browser/utility_process_host_client.h"
#include "content/common/child_process_host_impl.h"
#include "content/common/in_process_child_thread_params.h"
#include "content/common/service_manager/child_connection.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/process_type.h"
#include "content/public/common/sandboxed_process_launcher_delegate.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/common/service_names.mojom.h"
#include "media/base/media_switches.h"
#include "services/network/public/cpp/network_switches.h"
#include "services/service_manager/embedder/switches.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "services/service_manager/sandbox/sandbox_type.h"
#include "services/service_manager/sandbox/switches.h"
#include "services/service_manager/zygote/common/zygote_buildflags.h"
#include "ui/base/ui_base_switches.h"
#include "ui/gl/gl_switches.h"

#if defined(OS_WIN)
#include "sandbox/win/src/sandbox_policy.h"
#include "sandbox/win/src/sandbox_types.h"
#include "services/network/network_sandbox_win.h"
#endif

#if BUILDFLAG(USE_ZYGOTE_HANDLE)
#include "services/service_manager/zygote/common/zygote_handle.h"  // nogncheck
#endif

namespace content {

// NOTE: changes to this class need to be reviewed by the security team.
class UtilitySandboxedProcessLauncherDelegate
    : public SandboxedProcessLauncherDelegate {
 public:
  UtilitySandboxedProcessLauncherDelegate(
      service_manager::SandboxType sandbox_type,
      const base::EnvironmentMap& env)
      :
#if defined(OS_POSIX)
        env_(env),
#endif
        sandbox_type_(sandbox_type) {
#if DCHECK_IS_ON()
    bool supported_sandbox_type =
        sandbox_type_ == service_manager::SANDBOX_TYPE_NO_SANDBOX ||
#if defined(OS_WIN)
        sandbox_type_ ==
            service_manager::SANDBOX_TYPE_NO_SANDBOX_AND_ELEVATED_PRIVILEGES ||
#endif
        sandbox_type_ == service_manager::SANDBOX_TYPE_UTILITY ||
        sandbox_type_ == service_manager::SANDBOX_TYPE_NETWORK ||
        sandbox_type_ == service_manager::SANDBOX_TYPE_CDM ||
        sandbox_type_ == service_manager::SANDBOX_TYPE_PDF_COMPOSITOR ||
        sandbox_type_ == service_manager::SANDBOX_TYPE_PROFILING ||
        sandbox_type_ == service_manager::SANDBOX_TYPE_PPAPI;
    DCHECK(supported_sandbox_type);
#endif  // DCHECK_IS_ON()
  }

  ~UtilitySandboxedProcessLauncherDelegate() override {}

#if defined(OS_WIN)
  bool ShouldLaunchElevated() override {
    return sandbox_type_ ==
           service_manager::SANDBOX_TYPE_NO_SANDBOX_AND_ELEVATED_PRIVILEGES;
  }

  bool PreSpawnTarget(sandbox::TargetPolicy* policy) override {
    if (sandbox_type_ == service_manager::SANDBOX_TYPE_NETWORK)
      return network::NetworkPreSpawnTarget(policy);

    return true;
  }
#endif  // OS_WIN

#if BUILDFLAG(USE_ZYGOTE_HANDLE)
  service_manager::ZygoteHandle GetZygote() override {
    if (service_manager::IsUnsandboxedSandboxType(sandbox_type_) ||
        sandbox_type_ == service_manager::SANDBOX_TYPE_NETWORK) {
      return nullptr;
    }
    return service_manager::GetGenericZygote();
  }
#endif  // BUILDFLAG(USE_ZYGOTE_HANDLE)

#if defined(OS_POSIX)
  base::EnvironmentMap GetEnvironment() override { return env_; }
#endif  // OS_POSIX

  service_manager::SandboxType GetSandboxType() override {
    return sandbox_type_;
  }

 private:
#if defined(OS_POSIX)
  base::EnvironmentMap env_;
#endif  // OS_WIN
  service_manager::SandboxType sandbox_type_;
};

UtilityMainThreadFactoryFunction g_utility_main_thread_factory = nullptr;

void UtilityProcessHost::RegisterUtilityMainThreadFactory(
    UtilityMainThreadFactoryFunction create) {
  g_utility_main_thread_factory = create;
}

UtilityProcessHost::UtilityProcessHost(
    const scoped_refptr<UtilityProcessHostClient>& client,
    const scoped_refptr<base::SequencedTaskRunner>& client_task_runner)
    : client_(client),
      client_task_runner_(client_task_runner),
      sandbox_type_(service_manager::SANDBOX_TYPE_UTILITY),
#if defined(OS_LINUX)
      child_flags_(ChildProcessHost::CHILD_ALLOW_SELF),
#else
      child_flags_(ChildProcessHost::CHILD_NORMAL),
#endif
      started_(false),
      name_(base::ASCIIToUTF16("utility process")),
      weak_ptr_factory_(this) {
  process_.reset(new BrowserChildProcessHostImpl(PROCESS_TYPE_UTILITY, this,
                                                 mojom::kUtilityServiceName));
}

UtilityProcessHost::~UtilityProcessHost() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
}

base::WeakPtr<UtilityProcessHost> UtilityProcessHost::AsWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

bool UtilityProcessHost::Send(IPC::Message* message) {
  if (!StartProcess())
    return false;

  return process_->Send(message);
}

void UtilityProcessHost::SetSandboxType(
    service_manager::SandboxType sandbox_type) {
  DCHECK(sandbox_type != service_manager::SANDBOX_TYPE_INVALID);
  sandbox_type_ = sandbox_type;
}

const ChildProcessData& UtilityProcessHost::GetData() {
  return process_->GetData();
}

#if defined(OS_POSIX)
void UtilityProcessHost::SetEnv(const base::EnvironmentMap& env) {
  env_ = env;
}
#endif

bool UtilityProcessHost::Start() {
  return StartProcess();
}

void UtilityProcessHost::BindInterface(
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  process_->child_connection()->BindInterface(interface_name,
                                              std::move(interface_pipe));
}

void UtilityProcessHost::SetName(const base::string16& name) {
  name_ = name;
}

void UtilityProcessHost::SetServiceIdentity(
    const service_manager::Identity& identity) {
  service_identity_ = identity;
}

void UtilityProcessHost::SetLaunchCallback(
    base::OnceCallback<void(base::ProcessId)> callback) {
  DCHECK(!launched_);
  launch_callback_ = std::move(callback);
}

bool UtilityProcessHost::StartProcess() {
  if (started_)
    return true;

  started_ = true;
  process_->SetName(name_);
  process_->GetHost()->CreateChannelMojo();

  if (RenderProcessHost::run_renderer_in_process()) {
    DCHECK(g_utility_main_thread_factory);
    // See comment in RenderProcessHostImpl::Init() for the background on why we
    // support single process mode this way.
    in_process_thread_.reset(
        g_utility_main_thread_factory(InProcessChildThreadParams(
            BrowserThread::GetTaskRunnerForThread(BrowserThread::IO),
            process_->GetInProcessBrokerClientInvitation(),
            process_->child_connection()->service_token())));
    in_process_thread_->Start();
  } else {
    const base::CommandLine& browser_command_line =
        *base::CommandLine::ForCurrentProcess();

    bool has_cmd_prefix =
        browser_command_line.HasSwitch(switches::kUtilityCmdPrefix);

#if defined(OS_ANDROID)
    // readlink("/prof/self/exe") sometimes fails on Android at startup.
    // As a workaround skip calling it here, since the executable name is
    // not needed on Android anyway. See crbug.com/500854.
    std::unique_ptr<base::CommandLine> cmd_line =
        std::make_unique<base::CommandLine>(base::CommandLine::NO_PROGRAM);
#else
    int child_flags = child_flags_;

    // When running under gdb, forking /proc/self/exe ends up forking the gdb
    // executable instead of Chromium. It is almost safe to assume that no
    // updates will happen while a developer is running with
    // |switches::kUtilityCmdPrefix|. See ChildProcessHost::GetChildPath() for
    // a similar case with Valgrind.
    if (has_cmd_prefix)
      child_flags = ChildProcessHost::CHILD_NORMAL;

    base::FilePath exe_path = ChildProcessHost::GetChildPath(child_flags);
    if (exe_path.empty()) {
      NOTREACHED() << "Unable to get utility process binary name.";
      return false;
    }

    std::unique_ptr<base::CommandLine> cmd_line =
        std::make_unique<base::CommandLine>(exe_path);
#endif

    cmd_line->AppendSwitchASCII(switches::kProcessType,
                                switches::kUtilityProcess);
    BrowserChildProcessHostImpl::CopyFeatureAndFieldTrialFlags(cmd_line.get());
    BrowserChildProcessHostImpl::CopyTraceStartupFlags(cmd_line.get());
    std::string locale = GetContentClient()->browser()->GetApplicationLocale();
    cmd_line->AppendSwitchASCII(switches::kLang, locale);

#if defined(OS_WIN)
    cmd_line->AppendArg(switches::kPrefetchArgumentOther);
#endif  // defined(OS_WIN)

    service_manager::SetCommandLineFlagsForSandboxType(cmd_line.get(),
                                                       sandbox_type_);

    // Browser command-line switches to propagate to the utility process.
    static const char* const kSwitchNames[] = {
      network::switches::kForceEffectiveConnectionType,
      network::switches::kHostResolverRules,
      network::switches::kIgnoreCertificateErrorsSPKIList,
      network::switches::kLogNetLog,
      network::switches::kNoReferrers,
      service_manager::switches::kNoSandbox,
#if defined(OS_MACOSX)
      service_manager::switches::kEnableSandboxLogging,
#endif
      switches::kIgnoreCertificateErrors,
      switches::kOverrideUseSoftwareGLForTests,
      switches::kOverrideEnabledCdmInterfaceVersion,
      switches::kProxyServer,
      switches::kUseFakeDeviceForMediaStream,
      switches::kUseFileForFakeVideoCapture,
      switches::kUseMockCertVerifierForTesting,
      switches::kUtilityStartupDialog,
      switches::kUseGL,
#if defined(OS_ANDROID)
      switches::kOrderfileMemoryOptimization,
#endif
      // These flags are used by the audio service:
      switches::kAudioBufferSize,
      switches::kAudioServiceQuitTimeoutMs,
      switches::kDisableAudioOutput,
      switches::kFailAudioStreamCreation,
      switches::kMuteAudio,
      switches::kUseFileForFakeAudioCapture,
#if defined(OS_LINUX) || defined(OS_FREEBSD) || defined(OS_SOLARIS)
      switches::kAlsaInputDevice,
      switches::kAlsaOutputDevice,
#endif
#if defined(OS_WIN)
      switches::kEnableExclusiveAudio,
      switches::kForceWaveAudio,
      switches::kTrySupportedChannelLayouts,
      switches::kWaveOutBuffers,
#endif
    };
    cmd_line->CopySwitchesFrom(browser_command_line, kSwitchNames,
                               arraysize(kSwitchNames));

    network_session_configurator::CopyNetworkSwitches(browser_command_line,
                                                      cmd_line.get());

    if (has_cmd_prefix) {
      // Launch the utility child process with some prefix
      // (usually "xterm -e gdb --args").
      cmd_line->PrependWrapper(browser_command_line.GetSwitchValueNative(
          switches::kUtilityCmdPrefix));
    }

    const bool is_service = service_identity_.has_value();
    if (is_service) {
      GetContentClient()->browser()->AdjustUtilityServiceProcessCommandLine(
          *service_identity_, cmd_line.get());
    }

    process_->Launch(std::make_unique<UtilitySandboxedProcessLauncherDelegate>(
                         sandbox_type_, env_),
                     std::move(cmd_line), true);
  }

  return true;
}

bool UtilityProcessHost::OnMessageReceived(const IPC::Message& message) {
  if (!client_.get())
    return true;

  client_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          base::IgnoreResult(&UtilityProcessHostClient::OnMessageReceived),
          client_.get(), message));

  return true;
}

void UtilityProcessHost::OnProcessLaunched() {
  launched_ = true;
  if (launch_callback_)
    std::move(launch_callback_).Run(process_->GetProcess().Pid());
}

void UtilityProcessHost::OnProcessLaunchFailed(int error_code) {
  if (!client_.get())
    return;

  client_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&UtilityProcessHostClient::OnProcessLaunchFailed, client_,
                     error_code));
}

void UtilityProcessHost::OnProcessCrashed(int exit_code) {
  if (!client_.get())
    return;

  client_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&UtilityProcessHostClient::OnProcessCrashed,
                                client_, exit_code));
}

void UtilityProcessHost::NotifyAndDelete(int error_code) {
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&UtilityProcessHost::NotifyLaunchFailedAndDelete,
                     weak_ptr_factory_.GetWeakPtr(), error_code));
}

// static
void UtilityProcessHost::NotifyLaunchFailedAndDelete(
    base::WeakPtr<UtilityProcessHost> host,
    int error_code) {
  if (!host)
    return;

  host->OnProcessLaunchFailed(error_code);
  delete host.get();
}

}  // namespace content
