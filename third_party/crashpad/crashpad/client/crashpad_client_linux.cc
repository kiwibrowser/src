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

#include "client/crashpad_client.h"

#include <fcntl.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "client/client_argv_handling.h"
#include "util/file/file_io.h"
#include "util/linux/exception_handler_client.h"
#include "util/linux/exception_information.h"
#include "util/linux/scoped_pr_set_ptracer.h"
#include "util/misc/from_pointer_cast.h"
#include "util/posix/double_fork_and_exec.h"
#include "util/posix/signals.h"

namespace crashpad {

namespace {

std::string FormatArgumentInt(const std::string& name, int value) {
  return base::StringPrintf("--%s=%d", name.c_str(), value);
}

std::string FormatArgumentAddress(const std::string& name, void* addr) {
  return base::StringPrintf("--%s=%p", name.c_str(), addr);
}

class SignalHandler {
 public:
  virtual void HandleCrashFatal(int signo,
                                siginfo_t* siginfo,
                                void* context) = 0;
  virtual bool HandleCrashNonFatal(int signo,
                                   siginfo_t* siginfo,
                                   void* context) = 0;

  void SetFirstChanceHandler(CrashpadClient::FirstChanceHandler handler) {
    first_chance_handler_ = handler;
  }

 protected:
  SignalHandler() = default;
  ~SignalHandler() = default;

  CrashpadClient::FirstChanceHandler first_chance_handler_ = nullptr;
};

// Launches a single use handler to snapshot this process.
class LaunchAtCrashHandler : public SignalHandler {
 public:
  static LaunchAtCrashHandler* Get() {
    static LaunchAtCrashHandler* instance = new LaunchAtCrashHandler();
    return instance;
  }

  bool Initialize(std::vector<std::string>* argv_in) {
    argv_strings_.swap(*argv_in);

    argv_strings_.push_back(FormatArgumentAddress("trace-parent-with-exception",
                                                  &exception_information_));

    ConvertArgvStrings(argv_strings_, &argv_);
    return Signals::InstallCrashHandlers(HandleCrash, 0, nullptr);
  }

  bool HandleCrashNonFatal(int signo,
                           siginfo_t* siginfo,
                           void* context) override {
    if (first_chance_handler_ &&
        first_chance_handler_(
            signo, siginfo, static_cast<ucontext_t*>(context))) {
      return true;
    }

    exception_information_.siginfo_address =
        FromPointerCast<decltype(exception_information_.siginfo_address)>(
            siginfo);
    exception_information_.context_address =
        FromPointerCast<decltype(exception_information_.context_address)>(
            context);
    exception_information_.thread_id = syscall(SYS_gettid);

    ScopedPrSetPtracer set_ptracer(getpid(), /* may_log= */ false);

    pid_t pid = fork();
    if (pid < 0) {
      return false;
    }
    if (pid == 0) {
      execv(argv_[0], const_cast<char* const*>(argv_.data()));
      _exit(EXIT_FAILURE);
    }

    int status;
    waitpid(pid, &status, 0);
    return false;
  }

  void HandleCrashFatal(int signo, siginfo_t* siginfo, void* context) override {
    if (HandleCrashNonFatal(signo, siginfo, context)) {
      return;
    }
    Signals::RestoreHandlerAndReraiseSignalOnReturn(siginfo, nullptr);
  }

 private:
  LaunchAtCrashHandler() = default;

  ~LaunchAtCrashHandler() = delete;

  static void HandleCrash(int signo, siginfo_t* siginfo, void* context) {
    auto state = Get();
    state->HandleCrashFatal(signo, siginfo, context);
  }

  std::vector<std::string> argv_strings_;
  std::vector<const char*> argv_;
  ExceptionInformation exception_information_;

  DISALLOW_COPY_AND_ASSIGN(LaunchAtCrashHandler);
};

// A pointer to the currently installed crash signal handler. This allows
// the static method CrashpadClient::DumpWithoutCrashing to simulate a crash
// using the currently configured crash handling strategy.
static SignalHandler* g_crash_handler;

}  // namespace

CrashpadClient::CrashpadClient() {}

CrashpadClient::~CrashpadClient() {}

bool CrashpadClient::StartHandler(
    const base::FilePath& handler,
    const base::FilePath& database,
    const base::FilePath& metrics_dir,
    const std::string& url,
    const std::map<std::string, std::string>& annotations,
    const std::vector<std::string>& arguments,
    bool restartable,
    bool asynchronous_start) {
  // TODO(jperaza): Implement this after the Android/Linux ExceptionHandlerSever
  // supports accepting new connections.
  // https://crashpad.chromium.org/bug/30
  NOTREACHED();
  return false;
}

// static
bool CrashpadClient::StartHandlerAtCrash(
    const base::FilePath& handler,
    const base::FilePath& database,
    const base::FilePath& metrics_dir,
    const std::string& url,
    const std::map<std::string, std::string>& annotations,
    const std::vector<std::string>& arguments) {
  std::vector<std::string> argv;
  BuildHandlerArgvStrings(
      handler, database, metrics_dir, url, annotations, arguments, &argv);

  auto signal_handler = LaunchAtCrashHandler::Get();
  if (signal_handler->Initialize(&argv)) {
    DCHECK(!g_crash_handler);
    g_crash_handler = signal_handler;
    return true;
  }
  return false;
}

// static
bool CrashpadClient::StartHandlerForClient(
    const base::FilePath& handler,
    const base::FilePath& database,
    const base::FilePath& metrics_dir,
    const std::string& url,
    const std::map<std::string, std::string>& annotations,
    const std::vector<std::string>& arguments,
    int socket) {
  std::vector<std::string> argv;
  BuildHandlerArgvStrings(
      handler, database, metrics_dir, url, annotations, arguments, &argv);

  argv.push_back(FormatArgumentInt("initial-client", socket));

  return DoubleForkAndExec(argv, socket, true, nullptr);
}

// static
void CrashpadClient::DumpWithoutCrash(NativeCPUContext* context) {
  DCHECK(g_crash_handler);

#if defined(ARCH_CPU_ARMEL)
  memset(context->uc_regspace, 0, sizeof(context->uc_regspace));
#elif defined(ARCH_CPU_ARM64)
  memset(context->uc_mcontext.__reserved,
         0,
         sizeof(context->uc_mcontext.__reserved));
#endif

  siginfo_t siginfo;
  siginfo.si_signo = Signals::kSimulatedSigno;
  siginfo.si_errno = 0;
  siginfo.si_code = 0;
  g_crash_handler->HandleCrashNonFatal(
      siginfo.si_signo, &siginfo, reinterpret_cast<void*>(context));
}

// static
void CrashpadClient::SetFirstChanceExceptionHandler(
    FirstChanceHandler handler) {
  DCHECK(g_crash_handler);
  g_crash_handler->SetFirstChanceHandler(handler);
}

}  // namespace crashpad
