// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/chrome_cleaner/mock_chrome_cleaner_process_win.h"

#include <utility>
#include <vector>

#include "base/bind_helpers.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/threading/thread.h"
#include "mojo/edk/embedder/connection_params.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/incoming_broker_client_invitation.h"
#include "mojo/edk/embedder/outgoing_broker_client_invitation.h"
#include "mojo/edk/embedder/platform_channel_pair.h"
#include "mojo/edk/embedder/scoped_ipc_support.h"
#include "mojo/edk/embedder/transport_protocol.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace safe_browsing {

namespace {

using ::chrome_cleaner::mojom::ChromePrompt;
using ::chrome_cleaner::mojom::ChromePromptPtr;
using ::chrome_cleaner::mojom::ChromePromptPtrInfo;
using ::chrome_cleaner::mojom::PromptAcceptance;

constexpr char kCrashPointSwitch[] = "mock-crash-point";
constexpr char kUwsFoundSwitch[] = "mock-uws-found";
constexpr char kRebootRequiredSwitch[] = "mock-reboot-required";
constexpr char kRegistryKeysReportingSwitch[] = "registry-keys-reporting";
constexpr char kExpectedUserResponseSwitch[] = "mock-expected-user-response";

}  // namespace

// static
bool MockChromeCleanerProcess::Options::FromCommandLine(
    const base::CommandLine& command_line,
    Options* options) {
  int registry_keys_reporting_int = -1;
  if (!base::StringToInt(
          command_line.GetSwitchValueASCII(kRegistryKeysReportingSwitch),
          &registry_keys_reporting_int) ||
      registry_keys_reporting_int < 0 ||
      registry_keys_reporting_int >=
          static_cast<int>(RegistryKeysReporting::kNumRegistryKeysReporting)) {
    return false;
  }

  options->SetReportedResults(
      command_line.HasSwitch(kUwsFoundSwitch),
      static_cast<RegistryKeysReporting>(registry_keys_reporting_int));
  options->set_reboot_required(command_line.HasSwitch(kRebootRequiredSwitch));

  if (command_line.HasSwitch(kCrashPointSwitch)) {
    int crash_point_int = 0;
    if (base::StringToInt(command_line.GetSwitchValueASCII(kCrashPointSwitch),
                          &crash_point_int) &&
        crash_point_int >= 0 &&
        crash_point_int < static_cast<int>(CrashPoint::kNumCrashPoints)) {
      options->set_crash_point(static_cast<CrashPoint>(crash_point_int));
    } else {
      return false;
    }
  }

  if (command_line.HasSwitch(kExpectedUserResponseSwitch)) {
    int expected_response_int = 0;
    if (base::StringToInt(
            command_line.GetSwitchValueASCII(kExpectedUserResponseSwitch),
            &expected_response_int) &&
        expected_response_int >= 0 &&
        expected_response_int <
            static_cast<int>(PromptAcceptance::NUM_VALUES)) {
      options->set_expected_user_response(
          static_cast<PromptAcceptance>(expected_response_int));
    } else {
      return false;
    }
  }

  return true;
}

MockChromeCleanerProcess::Options::Options() = default;

MockChromeCleanerProcess::Options::Options(const Options& other)
    : files_to_delete_(other.files_to_delete_),
      registry_keys_(other.registry_keys_),
      reboot_required_(other.reboot_required_),
      crash_point_(other.crash_point_),
      registry_keys_reporting_(other.registry_keys_reporting_),
      expected_user_response_(other.expected_user_response_) {}

MockChromeCleanerProcess::Options& MockChromeCleanerProcess::Options::operator=(
    const Options& other) {
  files_to_delete_ = other.files_to_delete_;
  registry_keys_ = other.registry_keys_;
  reboot_required_ = other.reboot_required_;
  crash_point_ = other.crash_point_;
  registry_keys_reporting_ = other.registry_keys_reporting_;
  expected_user_response_ = other.expected_user_response_;
  return *this;
}

MockChromeCleanerProcess::Options::~Options() {}

void MockChromeCleanerProcess::Options::AddSwitchesToCommandLine(
    base::CommandLine* command_line) const {
  if (!files_to_delete_.empty())
    command_line->AppendSwitch(kUwsFoundSwitch);

  if (reboot_required())
    command_line->AppendSwitch(kRebootRequiredSwitch);

  if (crash_point() != CrashPoint::kNone) {
    command_line->AppendSwitchASCII(
        kCrashPointSwitch, base::IntToString(static_cast<int>(crash_point())));
  }

  command_line->AppendSwitchASCII(
      kRegistryKeysReportingSwitch,
      base::IntToString(static_cast<int>(registry_keys_reporting())));

  if (expected_user_response() != PromptAcceptance::UNSPECIFIED) {
    command_line->AppendSwitchASCII(
        kExpectedUserResponseSwitch,
        base::IntToString(static_cast<int>(expected_user_response())));
  }
}

void MockChromeCleanerProcess::Options::SetReportedResults(
    bool has_files_to_remove,
    RegistryKeysReporting registry_keys_reporting) {
  if (!has_files_to_remove)
    files_to_delete_.clear();
  if (has_files_to_remove) {
    files_to_delete_.push_back(
        base::FilePath(FILE_PATH_LITERAL("/path/to/file1.exe")));
    files_to_delete_.push_back(
        base::FilePath(FILE_PATH_LITERAL("/path/to/other/file2.exe")));
    files_to_delete_.push_back(
        base::FilePath(FILE_PATH_LITERAL("/path/to/some file.dll")));
  }

  registry_keys_reporting_ = registry_keys_reporting;
  switch (registry_keys_reporting) {
    case RegistryKeysReporting::kUnsupported:
      // Defined as an optional object in which a registry keys vector is not
      // present.
      registry_keys_ = base::Optional<std::vector<base::string16>>();
      break;

    case RegistryKeysReporting::kNotReported:
      // Defined as an optional object in which an empty registry keys vector is
      // present.
      registry_keys_ =
          base::Optional<std::vector<base::string16>>(base::in_place);
      break;

    case RegistryKeysReporting::kReported:
      // Defined as an optional object in which a non-empty registry keys vector
      // is present.
      registry_keys_ = base::Optional<std::vector<base::string16>>({
          L"HKCU:32\\Software\\Some\\Unwanted Software",
          L"HKCU:32\\Software\\Another\\Unwanted Software",
      });
      break;

    default:
      NOTREACHED();
  }
}

int MockChromeCleanerProcess::Options::ExpectedExitCode(
    PromptAcceptance received_prompt_acceptance) const {
  if (crash_point() != CrashPoint::kNone)
    return kDeliberateCrashExitCode;

  if (files_to_delete_.empty())
    return kNothingFoundExitCode;

  if (received_prompt_acceptance == PromptAcceptance::ACCEPTED_WITH_LOGS ||
      received_prompt_acceptance == PromptAcceptance::ACCEPTED_WITHOUT_LOGS) {
    return reboot_required() ? kRebootRequiredExitCode
                             : kRebootNotRequiredExitCode;
  }

  return kDeclinedExitCode;
}

MockChromeCleanerProcess::MockChromeCleanerProcess(
    const Options& options,
    const std::string& chrome_mojo_pipe_token)
    : options_(options), chrome_mojo_pipe_token_(chrome_mojo_pipe_token) {}

int MockChromeCleanerProcess::Run() {
  // We use EXPECT_*() macros to get good log lines, but since this code is run
  // in a separate process, failing a check in an EXPECT_*() macro will not fail
  // the test. Therefore, we use ::testing::Test::HasFailure() to detect
  // EXPECT_*() failures and return an error code that indicates that the test
  // should fail.
  EXPECT_FALSE(chrome_mojo_pipe_token_.empty());
  if (::testing::Test::HasFailure())
    return kInternalTestFailureExitCode;

  if (options_.crash_point() == CrashPoint::kOnStartup)
    exit(kDeliberateCrashExitCode);

  mojo::edk::Init();
  base::Thread::Options thread_options(base::MessageLoop::TYPE_IO, 0);
  base::Thread io_thread("IPCThread");
  EXPECT_TRUE(io_thread.StartWithOptions(thread_options));
  if (::testing::Test::HasFailure())
    return kInternalTestFailureExitCode;

  mojo::edk::ScopedIPCSupport ipc_support(
      io_thread.task_runner(),
      mojo::edk::ScopedIPCSupport::ShutdownPolicy::CLEAN);

  auto invitation =
      mojo::edk::IncomingBrokerClientInvitation::AcceptFromCommandLine(
          mojo::edk::TransportProtocol::kLegacy);
  ChromePromptPtrInfo prompt_ptr_info(
      invitation->ExtractMessagePipe(chrome_mojo_pipe_token_), 0);

  if (options_.crash_point() == CrashPoint::kAfterConnection)
    exit(kDeliberateCrashExitCode);

  base::MessageLoop message_loop;
  base::RunLoop run_loop;
  // After the response from the parent process is received, this will post a
  // task to unblock the child process's main thread.
  auto quit_closure = base::BindOnce(
      [](scoped_refptr<base::SequencedTaskRunner> main_runner,
         base::Closure quit_closure) {
        main_runner->PostTask(FROM_HERE, std::move(quit_closure));
      },
      base::SequencedTaskRunnerHandle::Get(),
      base::Passed(run_loop.QuitClosure()));

  io_thread.task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&MockChromeCleanerProcess::SendScanResults,
                     base::Unretained(this), std::move(prompt_ptr_info),
                     base::Passed(&quit_closure)));

  run_loop.Run();

  EXPECT_NE(received_prompt_acceptance_, PromptAcceptance::UNSPECIFIED);
  EXPECT_EQ(received_prompt_acceptance_, options_.expected_user_response());
  if (::testing::Test::HasFailure())
    return kInternalTestFailureExitCode;
  return options_.ExpectedExitCode(received_prompt_acceptance_);
}

void MockChromeCleanerProcess::SendScanResults(
    ChromePromptPtrInfo prompt_ptr_info,
    base::OnceClosure quit_closure) {
  // This pointer will be deleted by PromptUserCallback.
  chrome_prompt_ptr_ = new ChromePromptPtr();
  chrome_prompt_ptr_->Bind(std::move(prompt_ptr_info));

  if (options_.crash_point() == CrashPoint::kAfterRequestSent) {
    // This task is posted to the IPC thread so that it will happen after the
    // request is sent to the parent process and before the response gets
    // handled on the IPC thread.
    base::SequencedTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind([]() { exit(kDeliberateCrashExitCode); }));
  }

  (*chrome_prompt_ptr_)
      ->PromptUser(
          options_.files_to_delete(), options_.registry_keys(),
          base::BindOnce(&MockChromeCleanerProcess::PromptUserCallback,
                         base::Unretained(this), std::move(quit_closure)));
}

void MockChromeCleanerProcess::PromptUserCallback(
    base::OnceClosure quit_closure,
    PromptAcceptance prompt_acceptance) {
  delete chrome_prompt_ptr_;
  chrome_prompt_ptr_ = nullptr;

  received_prompt_acceptance_ = prompt_acceptance;

  if (options_.crash_point() == CrashPoint::kAfterResponseReceived)
    exit(kDeliberateCrashExitCode);

  std::move(quit_closure).Run();
}

}  // namespace safe_browsing
