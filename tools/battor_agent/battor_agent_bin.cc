// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file provides a thin binary wrapper around the BattOr Agent
// library. This binary wrapper provides a means for non-C++ tracing
// controllers, such as Telemetry and Android Systrace, to issue high-level
// tracing commands to the BattOr through an interactive shell.
//
// Example usage of how an external trace controller might use this binary:
//
// 1) Telemetry's PowerTracingAgent is told to start recording power samples
// 2) PowerTracingAgent opens up a BattOr agent binary subprocess
// 3) PowerTracingAgent sends the subprocess the StartTracing message via
//    STDIN
// 4) PowerTracingAgent waits for the subprocess to write a line to STDOUT
//    ('Done.' if successful, some error message otherwise)
// 5) If the last command was successful, PowerTracingAgent waits for the
//    duration of the trace
// 6) When the tracing should end, PowerTracingAgent records the clock sync
//    start timestamp and sends the subprocess the
//    'RecordClockSyncMark <marker>' message via STDIN.
// 7) PowerTracingAgent waits for the subprocess to write a line to STDOUT
//    ('Done.' if successful, some error message otherwise)
// 8) If the last command was successful, PowerTracingAgent records the clock
//    sync end timestamp and sends the subprocess the StopTracing message via
//    STDIN
// 9) PowerTracingAgent continues to read trace output lines from STDOUT until
//    the binary exits with an exit code of 1 (indicating failure) or the
//    'Done.' line is printed to STDOUT, signaling the last line of the trace
// 10) PowerTracingAgent returns the battery trace to the Telemetry trace
//     controller

#include <stdint.h>

#include <fstream>
#include <iomanip>
#include <iostream>

#include "base/at_exit.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_tokenizer.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/task_scheduler.h"
#include "base/threading/thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "tools/battor_agent/battor_agent.h"
#include "tools/battor_agent/battor_error.h"
#include "tools/battor_agent/battor_finder.h"

using std::endl;

namespace battor {

namespace {

const char kIoThreadName[] = "BattOr IO Thread";

const char kUsage[] =
    "Start the battor_agent shell with:\n"
    "\n"
    "  battor_agent <switches>\n"
    "\n"
    "Switches: \n"
    "  --battor-path=<path> Uses the specified BattOr path.\n"
    "  --interactive Enables interactive power profiling."
    "\n"
    "Once in the shell, you can issue the following commands:\n"
    "\n"
    "  StartTracing\n"
    "  StopTracing <optional file path>\n"
    "  SupportsExplicitClockSync\n"
    "  RecordClockSyncMarker <marker>\n"
    "  GetFirmwareGitHash\n"
    "  Exit\n"
    "  Help\n"
    "\n";

// The command line switch used to enable interactive mode where starting and
// stopping is easily toggled.
const char kInteractiveSwitch[] = "interactive";

void PrintSupportsExplicitClockSync() {
  std::cout << BattOrAgent::SupportsExplicitClockSync() << endl;
}

// Logs the error and exits with an error code.
void HandleError(battor::BattOrError error) {
  if (error != BATTOR_ERROR_NONE)
    LOG(FATAL) << "Fatal error when communicating with the BattOr: "
               << BattOrErrorToString(error);
}

// Prints an error message and exits due to a required thread failing to start.
void ExitFromThreadStartFailure(const std::string& thread_name) {
  LOG(FATAL) << "Failed to start " << thread_name;
}

std::vector<std::string> TokenizeString(std::string cmd) {
  base::StringTokenizer tokenizer(cmd, " ");
  std::vector<std::string> tokens;
  while (tokenizer.GetNext())
    tokens.push_back(tokenizer.token());
  return tokens;
}

}  // namespace

// Wrapper class containing all state necessary for an independent binary to
// use a BattOrAgent to communicate with a BattOr.
class BattOrAgentBin : public BattOrAgent::Listener {
 public:
  BattOrAgentBin() : io_thread_(kIoThreadName) {}

  ~BattOrAgentBin() { DCHECK(!agent_); }

  // Starts the interactive BattOr agent shell and eventually returns an exit
  // code.
  int Run(int argc, char* argv[]) {
    // If we don't have any BattOr to use, exit.
    std::string path = BattOrFinder::FindBattOr();
    if (path.empty()) {
      std::cout << "Unable to find a BattOr." << endl;
#if defined(OS_WIN)
      std::cout << "Try \"--battor-path=<path>\" to specify the COM port where "
                   "the BattOr can be found, typically COM3."
                << endl;
#endif
      exit(1);
    }

    SetUp(path);

    if (base::CommandLine::ForCurrentProcess()->HasSwitch(kInteractiveSwitch)) {
      interactive_ = true;
      std::cout << "Type <Enter> to toggle tracing, type Exit or Ctrl+C "
                   "to quit, or Help for help."
                << endl;
    }

    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::Bind(&BattOrAgentBin::RunNextCommand, base::Unretained(this)));
    ui_thread_run_loop_.Run();

    TearDown();
    return 0;
  }

  // Performs any setup necessary for the BattOr binary to run.
  void SetUp(const std::string& path) {
    base::Thread::Options io_thread_options;
    io_thread_options.message_loop_type = base::MessageLoopForIO::TYPE_IO;
    if (!io_thread_.StartWithOptions(io_thread_options)) {
      ExitFromThreadStartFailure(kIoThreadName);
    }

    // Block until the creation of the BattOrAgent is complete. This doesn't
    // seem necessary because we're posting the creation to the IO thread
    // before posting any commands, so we're guaranteed that the creation
    // will happen first. However, the crashes that happen without this sync
    // mechanism in place say otherwise.
    base::WaitableEvent done(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                             base::WaitableEvent::InitialState::NOT_SIGNALED);
    io_thread_.task_runner()->PostTask(
        FROM_HERE,
        base::Bind(&BattOrAgentBin::CreateAgent, base::Unretained(this), path,
                   base::ThreadTaskRunnerHandle::Get(), &done));
    done.Wait();
  }

  // Performs any cleanup necessary after the BattOr binary is done running.
  void TearDown() {
    base::WaitableEvent done(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                             base::WaitableEvent::InitialState::NOT_SIGNALED);
    io_thread_.task_runner()->PostTask(
        FROM_HERE, base::Bind(&BattOrAgentBin::DeleteAgent,
                              base::Unretained(this), &done));
    done.Wait();
  }

  void RunNextCommand() {
    std::string cmd;
    std::getline(std::cin, cmd);

    if (interactive_) {
      if (cmd == "") {
        cmd = is_tracing_ ? "StopTracing" : "StartTracing";
        std::cout << cmd << endl;
        is_tracing_ = !is_tracing_;
      }
    }

    if (cmd == "StartTracing") {
      StartTracing();
    } else if (cmd.find("StopTracing") != std::string::npos) {
      std::vector<std::string> tokens = TokenizeString(cmd);

      if (tokens[0] != "StopTracing" || tokens.size() > 2) {
        std::cout << "Invalid StopTracing command." << endl;
        std::cout << kUsage << endl;
        PostRunNextCommand();
        return;
      }

      // tokens[1] contains the optional output file argument, which allows
      // users to dump the trace to a file instead instead of to STDOUT.
      std::string trace_output_file =
          tokens.size() == 2 ? tokens[1] : std::string();

      StopTracing(trace_output_file);
      if (interactive_) {
        PostRunNextCommand();
      }
    } else if (cmd == "SupportsExplicitClockSync") {
      PrintSupportsExplicitClockSync();
      PostRunNextCommand();
    } else if (cmd.find("RecordClockSyncMarker") != std::string::npos) {
      std::vector<std::string> tokens = TokenizeString(cmd);
      if (tokens.size() != 2 || tokens[0] != "RecordClockSyncMarker") {
        std::cout << "Invalid RecordClockSyncMarker command." << endl;
        std::cout << kUsage << endl;
        PostRunNextCommand();
        return;
      }

      RecordClockSyncMarker(tokens[1]);
    } else if (cmd == "GetFirmwareGitHash") {
      GetFirmwareGitHash();
      return;
    } else if (cmd == "Exit" || std::cin.eof()) {
      ui_thread_message_loop_.task_runner()->PostTask(
          FROM_HERE, ui_thread_run_loop_.QuitClosure());
    } else {
      std::cout << kUsage << endl;
      PostRunNextCommand();
    }
  }

  void PostRunNextCommand() {
    ui_thread_message_loop_.task_runner()->PostTask(
        FROM_HERE,
        base::Bind(&BattOrAgentBin::RunNextCommand, base::Unretained(this)));
  }

  void GetFirmwareGitHash() {
    io_thread_.task_runner()->PostTask(
        FROM_HERE,
        base::Bind(&BattOrAgent::GetFirmwareGitHash,
                   base::Unretained(agent_.get())));
  }

  void OnGetFirmwareGitHashComplete(const std::string& firmware_git_hash,
                                    BattOrError error) override {
    if (error == BATTOR_ERROR_NONE)
      std::cout << firmware_git_hash << endl;
    else
      HandleError(error);

    PostRunNextCommand();
  }

  void StartTracing() {
    io_thread_.task_runner()->PostTask(
        FROM_HERE,
        base::Bind(&BattOrAgent::StartTracing, base::Unretained(agent_.get())));
  }

  void OnStartTracingComplete(BattOrError error) override {
    if (error == BATTOR_ERROR_NONE)
      std::cout << "Done." << endl;
    else
      HandleError(error);

    PostRunNextCommand();
  }

  void StopTracing(const std::string& trace_output_file) {
    trace_output_file_ = trace_output_file;
    io_thread_.task_runner()->PostTask(
        FROM_HERE,
        base::Bind(&BattOrAgent::StopTracing, base::Unretained(agent_.get())));
  }

  std::string BattOrResultsToSummary(const BattOrResults& results) {
    const uint32_t samples_per_second = results.GetSampleRate();

    // Print a summary of a BattOr trace. These summaries are intended for human
    // consumption and are subject to change at any moment. The summary is
    // printed when using interactive mode.
    std::stringstream trace_summary;
    // Display floating-point numbers without exponents, in a five-character
    // field, with two digits of precision. ie;
    // 12.39
    //  8.40
    trace_summary << std::fixed << std::setw(5) << std::setprecision(2);

    // Scan through the sample data to summarize it. Report on average power and
    // second-by-second power including min-second, median-second, and
    // max-second.
    double total_power = 0.0;
    int num_seconds = 0;
    std::vector<double> power_by_seconds;
    const std::vector<float>& samples = results.GetPowerSamples();
    for (size_t i = 0; i < samples.size(); i += samples_per_second) {
      size_t loop_count = samples.size() - i;
      if (loop_count > samples_per_second)
        loop_count = samples_per_second;

      double second_power = 0.0;
      for (size_t j = i; j < i + loop_count; ++j) {
        total_power += samples[i];
        second_power += samples[i];
      }

      // Print/store results for full seconds.
      if (loop_count == samples_per_second) {
        // Calculate power for one second in watts.
        second_power /= samples_per_second;
        trace_summary << "Second " << std::setw(2) << num_seconds
                      << " average power: " << std::setw(5) << second_power
                      << " W" << std::endl;
        ++num_seconds;
        power_by_seconds.push_back(second_power);
      }
    }
    // Calculate average power in watts.
    const double average_power_W = total_power / samples.size();
    const double duration_sec =
        static_cast<double>(samples.size()) / samples_per_second;
    trace_summary << "Average power over " << duration_sec
                  << " s : " << average_power_W << " W" << std::endl;
    std::sort(power_by_seconds.begin(), power_by_seconds.end());
    if (power_by_seconds.size() >= 3) {
      trace_summary << "Summary of power-by-seconds:" << std::endl
                    << "Minimum: " << power_by_seconds[0] << std::endl
                    << "Median:  "
                    << power_by_seconds[power_by_seconds.size() / 2]
                    << std::endl
                    << "Maximum: "
                    << power_by_seconds[power_by_seconds.size() - 1]
                    << std::endl;
    } else {
      trace_summary << "Too short a trace to generate per-second summary.";
    }

    return trace_summary.str();
  }

  void OnStopTracingComplete(const BattOrResults& results,
                             BattOrError error) override {
    if (error == BATTOR_ERROR_NONE) {
      std::string output_file = trace_output_file_;
      if (trace_output_file_.empty()) {
        // Save the detailed results in case they are needed.
        base::FilePath default_path;
        base::PathService::Get(base::DIR_USER_DESKTOP, &default_path);
        default_path = default_path.Append(FILE_PATH_LITERAL("trace_data.txt"));
        output_file = default_path.AsUTF8Unsafe().c_str();
        std::cout << "Saving detailed results to " << output_file << std::endl;
      }

      if (interactive_) {
        // Print a summary of the trace.
        std::cout << BattOrResultsToSummary(results) << endl;
      }

      std::ofstream trace_stream(output_file);
      if (!trace_stream.is_open()) {
        std::cout << "Tracing output file \"" << output_file
                  << "\" could not be opened." << endl;
        exit(1);
      }
      trace_stream << results.ToString();
      trace_stream.close();
      std::cout << "Done." << endl;
    } else {
      HandleError(error);
    }

    if (!interactive_) {
      ui_thread_message_loop_.task_runner()->PostTask(
          FROM_HERE, ui_thread_run_loop_.QuitClosure());
    }
  }

  void RecordClockSyncMarker(const std::string& marker) {
    io_thread_.task_runner()->PostTask(
        FROM_HERE, base::Bind(&BattOrAgent::RecordClockSyncMarker,
                              base::Unretained(agent_.get()), marker));
  }

  void OnRecordClockSyncMarkerComplete(BattOrError error) override {
    if (error == BATTOR_ERROR_NONE)
      std::cout << "Done." << endl;
    else
      HandleError(error);

    PostRunNextCommand();
  }

  // Postable task for creating the BattOrAgent. Because the BattOrAgent has
  // uber thread safe dependencies, all interactions with it, including creating
  // and deleting it, MUST happen on the IO thread.
  void CreateAgent(
      const std::string& path,
      scoped_refptr<base::SingleThreadTaskRunner> ui_thread_task_runner,
      base::WaitableEvent* done) {
    agent_.reset(new BattOrAgent(path, this, ui_thread_task_runner));
    done->Signal();
  }

  // Postable task for deleting the BattOrAgent. See the comment for
  // CreateAgent() above regarding why this is necessary.
  void DeleteAgent(base::WaitableEvent* done) {
    agent_.reset();
    done->Signal();
  }

 private:
  // NOTE: ui_thread_message_loop_ must appear before ui_thread_run_loop_ here
  // because ui_thread_run_loop_ checks for the current MessageLoop during
  // initialization.
  base::MessageLoopForUI ui_thread_message_loop_;
  base::RunLoop ui_thread_run_loop_;

  // Threads needed for serial communication.
  base::Thread io_thread_;

  // The agent capable of asynchronously communicating with the BattOr.
  std::unique_ptr<BattOrAgent> agent_;

  std::string trace_output_file_;

  // When true user can Start/Stop tracing by typing Enter.
  bool interactive_ = false;
  // Toggle to support alternating starting/stopping tracing.
  bool is_tracing_ = false;
};

}  // namespace battor

int main(int argc, char* argv[]) {
  base::AtExitManager exit_manager;
  base::CommandLine::Init(argc, argv);
  battor::BattOrAgentBin bin;
  base::TaskScheduler::CreateAndStartWithDefaultParams("battor_agent");
  return bin.Run(argc, argv);
}
