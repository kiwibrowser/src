// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SAFE_BROWSING_CHROME_CLEANER_REPORTER_RUNNER_WIN_H_
#define CHROME_BROWSER_SAFE_BROWSING_CHROME_CLEANER_REPORTER_RUNNER_WIN_H_

#include <limits.h>
#include <stdint.h>

#include <memory>
#include <queue>
#include <string>

#include "base/callback.h"
#include "base/command_line.h"
#include "base/time/time.h"
#include "base/version.h"
#include "components/chrome_cleaner/public/constants/constants.h"

namespace base {
class TaskRunner;
}

namespace safe_browsing {

class ChromeCleanerController;

// A special exit code identifying a failure to run the reporter.
const int kReporterNotLaunchedExitCode = INT_MAX;

// The number of days to wait before triggering another reporter run.
const int kDaysBetweenSuccessfulSwReporterRuns = 7;
// The number of days to wait before sending out reporter logs.
const int kDaysBetweenReporterLogsSent = 7;

// Identifies if an invocation was created during periodic reporter runs
// or because the user explicitly initiated a cleanup. The invocation type
// controls whether a prompt dialog will be shown to the user and under what
// conditions logs may be uploaded to Google.
//
// These values are used to send UMA information and are replicated in the
// enums.xml file, so the order MUST NOT CHANGE.
enum class SwReporterInvocationType {
  // Default value that should never be used for valid invocations.
  kUnspecified,
  // Periodic runs of the reporter are initiated by Chrome after startup.
  // If removable unwanted software is found the user may be prompted to
  // run the Chrome Cleanup tool. Logs from the software reporter will only
  // be uploaded if the user has opted-into SBER2 and if unwanted software
  // is found on the system. The cleaner process in scanning mode will not
  // upload logs.
  kPeriodicRun,
  // User-initiated runs in which the user has opted-out of sending details
  // to Google. Those runs are intended to be completely driven from the
  // Settings page, so a prompt dialog will not be shown to the user if
  // removable unwanted software is found. Logs will not be uploaded from the
  // reporter, even if the user has opted into SBER2, and cleaner logs will not
  // be uploaded.
  kUserInitiatedWithLogsDisallowed,
  // User-initiated runs in which the user has not opted-out of sending
  // details to Google. Those runs are intended to be completely driven from
  // the Settings page, so a prompt dialog will not be shown to the user if
  // removable unwanted software is found. Logs will be uploaded from both
  // the reporter and the cleaner in scanning mode (which will only run if
  // unwanted software is found by the reporter).
  kUserInitiatedWithLogsAllowed,

  kMax,
};

bool IsUserInitiated(SwReporterInvocationType invocation_type);

// Parameters used to invoke the sw_reporter component.
class SwReporterInvocation {
 public:
  // Flags to control behaviours the Software Reporter should support by
  // default. These flags are set in the Reporter installer, and experimental
  // versions of the reporter will turn on the behaviours that are not yet
  // supported.
  using Behaviours = uint32_t;
  enum : Behaviours {
    BEHAVIOUR_LOG_EXIT_CODE_TO_PREFS = 0x2,
    BEHAVIOUR_TRIGGER_PROMPT = 0x4,

    BEHAVIOURS_ENABLED_BY_DEFAULT =
        BEHAVIOUR_LOG_EXIT_CODE_TO_PREFS | BEHAVIOUR_TRIGGER_PROMPT,
  };

  explicit SwReporterInvocation(const base::CommandLine& command_line);
  SwReporterInvocation(const SwReporterInvocation& invocation);
  void operator=(const SwReporterInvocation& invocation);

  // Fluent interface methods, intended to be used during initialization.
  // Sample usage:
  //   auto invocation = SwReporterInvocation(command_line)
  //       .WithSuffix("MySuffix")
  //       .WithSupportedBehaviours(
  //           SwReporterInvocation::Behaviours::BEHAVIOUR_TRIGGER_PROMPT);
  SwReporterInvocation& WithSuffix(const std::string& suffix);
  SwReporterInvocation& WithSupportedBehaviours(
      Behaviours supported_behaviours);

  bool operator==(const SwReporterInvocation& other) const;

  const base::CommandLine& command_line() const;
  base::CommandLine& mutable_command_line();

  Behaviours supported_behaviours() const;
  bool BehaviourIsSupported(Behaviours intended_behaviour) const;

  // Experimental versions of the reporter will write metrics to registry keys
  // ending in |suffix_|. Those metrics should be copied to UMA histograms also
  // ending in |suffix_|. For the canonical version, |suffix_| will be empty.
  std::string suffix() const;

  // Indicates if the invocation type allows logs to be uploaded by the
  // reporter process.
  bool reporter_logs_upload_enabled() const;
  void set_reporter_logs_upload_enabled(bool reporter_logs_upload_enabled);

  // Indicates if the invocation type allows logs to be uploaded by the
  // cleaner process in scanning mode.
  bool cleaner_logs_upload_enabled() const;
  void set_cleaner_logs_upload_enabled(bool cleaner_logs_upload_enabled);

  chrome_cleaner::ChromePromptValue chrome_prompt() const;
  void set_chrome_prompt(chrome_cleaner::ChromePromptValue chrome_prompt);

 private:
  base::CommandLine command_line_;

  Behaviours supported_behaviours_ = BEHAVIOURS_ENABLED_BY_DEFAULT;

  std::string suffix_;

  bool reporter_logs_upload_enabled_ = false;
  bool cleaner_logs_upload_enabled_ = false;

  chrome_cleaner::ChromePromptValue chrome_prompt_ =
      chrome_cleaner::ChromePromptValue::kUnspecified;
};

// These values are used to send UMA information and are replicated in the
// enums.xml file, so the order MUST NOT CHANGE.
enum class SwReporterInvocationResult {
  kUnspecified,
  // Tried to start a new run, but a user-initiated run was already
  // happening. The UI should never allow this to happen.
  kNotScheduled,
  // The reporter process timed-out while running.
  kTimedOut,
  // The on-demand reporter run failed to download a new version of the reporter
  // component.
  kComponentNotAvailable,
  // The reporter failed to start.
  kProcessFailedToLaunch,
  // The reporter ended with a failure.
  kGeneralFailure,
  // The reporter ran successfully, but didn't find cleanable unwanted software.
  kNothingFound,
  // A periodic reporter sequence ran successfully and found cleanable unwanted
  // software, but the user shouldn't be prompted at this time.
  kCleanupNotOffered,
  // The reporter ran successfully and found cleanable unwanted software, and
  // a cleanup should be offered. A notification with this result should be
  // immediately followed by an attempt to run the cleaner in scanning mode.
  kCleanupToBeOffered,

  kMax,
};

// Called when all reporter invocations have completed, with a result parameter
// indicating if they succeeded.
using OnReporterSequenceDone =
    base::OnceCallback<void(SwReporterInvocationResult result)>;

class SwReporterInvocationSequence {
 public:
  using Queue = std::queue<SwReporterInvocation>;

  explicit SwReporterInvocationSequence(
      const base::Version& version = base::Version());
  SwReporterInvocationSequence(SwReporterInvocationSequence&& queue);
  SwReporterInvocationSequence(
      const SwReporterInvocationSequence& invocations_sequence);
  virtual ~SwReporterInvocationSequence();

  void PushInvocation(const SwReporterInvocation& invocation);

  void operator=(SwReporterInvocationSequence&& queue);

  void NotifySequenceDone(SwReporterInvocationResult result);

  base::Version version() const;

  const Queue& container() const;
  Queue& mutable_container();

 private:
  base::Version version_;
  Queue container_;
  // Invoked the first time this sequence run finishes or when the object
  // gets destroyed if it's never invoked.
  OnReporterSequenceDone on_sequence_done_;
};

// Tries to run the given invocations. If this runs successfully, than any
// calls made in the next |kDaysBetweenSuccessfulSwReporterRuns| days will be
// ignored.
//
// Each "run" of the sw_reporter component may aggregate the results of several
// executions of the tool with different command lines. |invocations| is the
// queue of SwReporters to execute as a single "run". When a new try is
// scheduled the entire queue is executed.
void MaybeStartSwReporter(SwReporterInvocationType invocation_type,
                          SwReporterInvocationSequence&& invocations);

// Returns true if the sw_reporter is allowed to run due to enterprise policies.
bool SwReporterIsAllowedByPolicy();

// Returns true if the sw_reported is allowed to report back results due to
// enterprise policies.
bool SwReporterReportingIsAllowedByPolicy();

// A delegate used by tests to implement test doubles (e.g., stubs, fakes, or
// mocks).
//
// TODO(crbug.com/776538): Replace this with a proper delegate that defines the
// default behaviour to be overriden (instead of defined) by tests.
class SwReporterTestingDelegate {
 public:
  virtual ~SwReporterTestingDelegate() {}

  // Invoked by tests in place of base::LaunchProcess.
  virtual int LaunchReporter(const SwReporterInvocation& invocation) = 0;

  // Invoked by tests to override the current time.
  // See Now() in reporter_runner_win.cc.
  virtual base::Time Now() const = 0;

  // A task runner used to spawn the reporter process (which blocks).
  // See ReporterRunner::ScheduleNextInvocation().
  virtual base::TaskRunner* BlockingTaskRunner() const = 0;

  // Invoked by tests to return a mock to the cleaner controller.
  virtual ChromeCleanerController* GetCleanerController() = 0;

  // Invoked by tests in place of the actual creation of the dialog controller.
  virtual void CreateChromeCleanerDialogController() = 0;
};

// Set a delegate for testing. The implementation will not take ownership of
// |delegate| - it must remain valid until this function is called again to
// reset the delegate. If |delegate| is nullptr, any previous delegate is
// cleared.
void SetSwReporterTestingDelegate(SwReporterTestingDelegate* delegate);

}  // namespace safe_browsing

#endif  // CHROME_BROWSER_SAFE_BROWSING_CHROME_CLEANER_REPORTER_RUNNER_WIN_H_
