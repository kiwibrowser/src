// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/lifetime/browser_shutdown.h"

#include <stdint.h>

#include <map>
#include <memory>
#include <string>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread.h"
#include "base/threading/thread_restrictions.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/browser/about_flags.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/lifetime/switch_utils.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_metrics.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "components/crash/core/common/crash_key.h"
#include "components/metrics/metrics_service.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/tracing/common/tracing_switches.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/tracing_controller.h"
#include "printing/buildflags/buildflags.h"
#include "rlz/buildflags/buildflags.h"

#if defined(OS_WIN)
#include "chrome/browser/first_run/upgrade_util_win.h"
#include "chrome/browser/win/browser_util.h"
#endif

#if !defined(OS_ANDROID) && !defined(OS_CHROMEOS)
#include "chrome/browser/first_run/upgrade_util.h"
#endif

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/boot_times_recorder.h"
#include "chrome/browser/lifetime/termination_notification.h"
#endif

#if BUILDFLAG(ENABLE_BACKGROUND_MODE)
#include "chrome/browser/background/background_mode_manager.h"
#endif

#if BUILDFLAG(ENABLE_PRINT_PREVIEW) && !defined(OS_CHROMEOS)
#include "chrome/browser/service_process/service_process_control.h"
#endif

#if BUILDFLAG(ENABLE_RLZ)
#include "components/rlz/rlz_tracker.h"
#endif

using base::TimeDelta;

namespace browser_shutdown {
namespace {

// Whether the browser is trying to quit (e.g., Quit chosen from menu).
bool g_trying_to_quit = false;

base::Time* g_shutdown_started = nullptr;
ShutdownType g_shutdown_type = NOT_VALID;
int g_shutdown_num_processes;
int g_shutdown_num_processes_slow;

constexpr char kShutdownMsFile[] = "chrome_shutdown_ms.txt";

base::FilePath GetShutdownMsPath() {
  base::FilePath shutdown_ms_file;
  base::PathService::Get(chrome::DIR_USER_DATA, &shutdown_ms_file);
  return shutdown_ms_file.AppendASCII(kShutdownMsFile);
}

const char* ToShutdownTypeString(ShutdownType type) {
  switch (type) {
    case NOT_VALID:
      NOTREACHED();
      return "";
    case WINDOW_CLOSE:
      return "close";
    case BROWSER_EXIT:
      return "exit";
    case END_SESSION:
      return "end";
  }
  return "";
}

}  // namespace

void RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(prefs::kShutdownType, NOT_VALID);
  registry->RegisterIntegerPref(prefs::kShutdownNumProcesses, 0);
  registry->RegisterIntegerPref(prefs::kShutdownNumProcessesSlow, 0);
  registry->RegisterBooleanPref(prefs::kRestartLastSessionOnShutdown, false);
}

ShutdownType GetShutdownType() {
  return g_shutdown_type;
}

void OnShutdownStarting(ShutdownType type) {
  if (g_shutdown_type != NOT_VALID)
    return;

  static crash_reporter::CrashKeyString<8> shutdown_type_key("shutdown-type");
  shutdown_type_key.Set(ToShutdownTypeString(type));

#if !defined(OS_CHROMEOS)
  // Start the shutdown tracing. Note that On ChromeOS this has already been
  // called in AttemptUserExit().
  StartShutdownTracing();
#endif

  g_shutdown_type = type;
  // For now, we're only counting the number of renderer processes
  // since we can't safely count the number of plugin processes from this
  // thread, and we'd really like to avoid anything which might add further
  // delays to shutdown time.
  DCHECK(!g_shutdown_started);
  g_shutdown_started = new base::Time(base::Time::Now());

  // Call FastShutdown on all of the RenderProcessHosts.  This will be
  // a no-op in some cases, so we still need to go through the normal
  // shutdown path for the ones that didn't exit here.
  g_shutdown_num_processes = 0;
  g_shutdown_num_processes_slow = 0;
  for (content::RenderProcessHost::iterator i(
          content::RenderProcessHost::AllHostsIterator());
       !i.IsAtEnd(); i.Advance()) {
    ++g_shutdown_num_processes;
    if (!i.GetCurrentValue()->FastShutdownIfPossible())
      ++g_shutdown_num_processes_slow;
  }
}

#if !defined(OS_ANDROID)
bool ShutdownPreThreadsStop() {
#if defined(OS_CHROMEOS)
  chromeos::BootTimesRecorder::Get()->AddLogoutTimeMarker(
      "BrowserShutdownStarted", false);
#endif
#if BUILDFLAG(ENABLE_PRINT_PREVIEW) && !defined(OS_CHROMEOS)
  // Shutdown the IPC channel to the service processes.
  ServiceProcessControl::GetInstance()->Disconnect();
#endif

  // WARNING: During logoff/shutdown (WM_ENDSESSION) we may not have enough
  // time to get here. If you have something that *must* happen on end session,
  // consider putting it in BrowserProcessImpl::EndSession.
  PrefService* prefs = g_browser_process->local_state();

  // Log the amount of times the user switched profiles during this session.
  ProfileMetrics::LogNumberOfProfileSwitches();

  metrics::MetricsService* metrics = g_browser_process->metrics_service();
  if (metrics)
    metrics->RecordCompletedSessionEnd();

  bool restart_last_session = RecordShutdownInfoPrefs();

  prefs->CommitPendingWrite();

#if BUILDFLAG(ENABLE_RLZ)
  // Cleanup any statics created by RLZ. Must be done before NotificationService
  // is destroyed.
  rlz::RLZTracker::CleanupRlz();
#endif

  return restart_last_session;
}

bool RecordShutdownInfoPrefs() {
  PrefService* prefs = g_browser_process->local_state();
  if (g_shutdown_type > NOT_VALID && g_shutdown_num_processes > 0) {
    // Record the shutdown info so that we can put it into a histogram at next
    // startup.
    prefs->SetInteger(prefs::kShutdownType, g_shutdown_type);
    prefs->SetInteger(prefs::kShutdownNumProcesses, g_shutdown_num_processes);
    prefs->SetInteger(prefs::kShutdownNumProcessesSlow,
                      g_shutdown_num_processes_slow);
  }

  // Check local state for the restart flag so we can restart the session later.
  bool restart_last_session = false;
  if (prefs->HasPrefPath(prefs::kRestartLastSessionOnShutdown)) {
    restart_last_session =
        prefs->GetBoolean(prefs::kRestartLastSessionOnShutdown);
    prefs->ClearPref(prefs::kRestartLastSessionOnShutdown);
  }
  return restart_last_session;
}

void ShutdownPostThreadsStop(int shutdown_flags) {
  delete g_browser_process;
  g_browser_process = nullptr;

  // crbug.com/95079 - This needs to happen after the browser process object
  // goes away.
  ProfileManager::NukeDeletedProfilesFromDisk();

#if defined(OS_CHROMEOS)
  chromeos::BootTimesRecorder::Get()->AddLogoutTimeMarker("BrowserDeleted",
                                                          true);
#endif

#if defined(OS_WIN)
  if (!browser_util::IsBrowserAlreadyRunning() &&
      g_shutdown_type != END_SESSION) {
    upgrade_util::SwapNewChromeExeIfPresent();
  }
#endif

  if (shutdown_flags & RESTART_LAST_SESSION) {
#if defined(OS_CHROMEOS)
    NOTIMPLEMENTED();
#else
    // Make sure to relaunch the browser with the original command line plus
    // the Restore Last Session flag. Note that Chrome can be launched (ie.
    // through ShellExecute on Windows) with a switch argument terminator at
    // the end (double dash, as described in b/1366444) plus a URL,
    // which prevents us from appending to the command line directly (issue
    // 46182). We therefore use GetSwitches to copy the command line (it stops
    // at the switch argument terminator).
    base::CommandLine old_cl(*base::CommandLine::ForCurrentProcess());
    auto new_cl = std::make_unique<base::CommandLine>(old_cl.GetProgram());
    base::CommandLine::SwitchMap switches = old_cl.GetSwitches();
    // Remove the switches that shouldn't persist across restart.
    about_flags::RemoveFlagsSwitches(&switches);
    switches::RemoveSwitchesForAutostart(&switches);
    // Append the old switches to the new command line.
    for (const auto& it : switches) {
      const auto& switch_name = it.first;
      const auto& switch_value = it.second;
      if (switch_value.empty())
        new_cl->AppendSwitch(switch_name);
      else
        new_cl->AppendSwitchNative(switch_name, switch_value);
    }
    if (shutdown_flags & RESTART_IN_BACKGROUND)
      new_cl->AppendSwitch(switches::kNoStartupWindow);

    upgrade_util::RelaunchChromeBrowser(*new_cl);
#endif  // defined(OS_CHROMEOS)
  }

  if (g_shutdown_type > NOT_VALID && g_shutdown_num_processes > 0) {
    // Measure total shutdown time as late in the process as possible
    // and then write it to a file to be read at startup.
    // We can't use prefs since all services are shutdown at this point.
    TimeDelta shutdown_delta = base::Time::Now() - *g_shutdown_started;
    std::string shutdown_ms =
        base::Int64ToString(shutdown_delta.InMilliseconds());
    int len = static_cast<int>(shutdown_ms.length()) + 1;
    base::FilePath shutdown_ms_file = GetShutdownMsPath();
    // Note: ReadLastShutdownFile() is done as a BLOCK_SHUTDOWN task so there's
    // an implicit sequencing between it and this write which happens after
    // threads have been stopped (and thus TaskScheduler::Shutdown() is
    // complete).
    base::WriteFile(shutdown_ms_file, shutdown_ms.c_str(), len);
  }

#if defined(OS_CHROMEOS)
  NotifyAndTerminate(false /* fast_path */);
#endif
}
#endif  // !defined(OS_ANDROID)

void ReadLastShutdownFile(ShutdownType type,
                          int num_procs,
                          int num_procs_slow) {
  base::AssertBlockingAllowed();

  base::FilePath shutdown_ms_file = GetShutdownMsPath();
  std::string shutdown_ms_str;
  int64_t shutdown_ms = 0;
  if (base::ReadFileToString(shutdown_ms_file, &shutdown_ms_str))
    base::StringToInt64(shutdown_ms_str, &shutdown_ms);
  base::DeleteFile(shutdown_ms_file, false);

  if (type == NOT_VALID || shutdown_ms == 0 || num_procs == 0)
    return;

  if (type == WINDOW_CLOSE) {
    UMA_HISTOGRAM_MEDIUM_TIMES("Shutdown.window_close.time2",
                               TimeDelta::FromMilliseconds(shutdown_ms));
    UMA_HISTOGRAM_TIMES("Shutdown.window_close.time_per_process",
                        TimeDelta::FromMilliseconds(shutdown_ms / num_procs));
  } else if (type == BROWSER_EXIT) {
    UMA_HISTOGRAM_MEDIUM_TIMES("Shutdown.browser_exit.time2",
                               TimeDelta::FromMilliseconds(shutdown_ms));
    UMA_HISTOGRAM_TIMES("Shutdown.browser_exit.time_per_process",
                        TimeDelta::FromMilliseconds(shutdown_ms / num_procs));
  } else if (type == END_SESSION) {
    UMA_HISTOGRAM_MEDIUM_TIMES("Shutdown.end_session.time2",
                               TimeDelta::FromMilliseconds(shutdown_ms));
    UMA_HISTOGRAM_TIMES("Shutdown.end_session.time_per_process",
                        TimeDelta::FromMilliseconds(shutdown_ms / num_procs));
  } else {
    NOTREACHED();
  }
  UMA_HISTOGRAM_COUNTS_100("Shutdown.renderers.total", num_procs);
  UMA_HISTOGRAM_COUNTS_100("Shutdown.renderers.slow", num_procs_slow);
}

void ReadLastShutdownInfo() {
  PrefService* prefs = g_browser_process->local_state();
  ShutdownType type =
      static_cast<ShutdownType>(prefs->GetInteger(prefs::kShutdownType));
  int num_procs = prefs->GetInteger(prefs::kShutdownNumProcesses);
  int num_procs_slow = prefs->GetInteger(prefs::kShutdownNumProcessesSlow);
  // clear the prefs immediately so we don't pick them up on a future run
  prefs->SetInteger(prefs::kShutdownType, NOT_VALID);
  prefs->SetInteger(prefs::kShutdownNumProcesses, 0);
  prefs->SetInteger(prefs::kShutdownNumProcessesSlow, 0);

  UMA_HISTOGRAM_ENUMERATION("Shutdown.ShutdownType", type, kNumShutdownTypes);

  base::PostTaskWithTraits(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BACKGROUND,
       base::TaskShutdownBehavior::BLOCK_SHUTDOWN},
      base::BindOnce(&ReadLastShutdownFile, type, num_procs, num_procs_slow));
}

void SetTryingToQuit(bool quitting) {
  g_trying_to_quit = quitting;

  if (quitting)
    return;

  // Reset the restart-related preferences. They get set unconditionally through
  // calls such as chrome::AttemptRestart(), and need to be reset if the restart
  // attempt is cancelled.
  PrefService* pref_service = g_browser_process->local_state();
  if (pref_service) {
#if !defined(OS_ANDROID)
    pref_service->ClearPref(prefs::kWasRestarted);
#endif  // !defined(OS_ANDROID)
    pref_service->ClearPref(prefs::kRestartLastSessionOnShutdown);
  }

#if BUILDFLAG(ENABLE_BACKGROUND_MODE)
  BackgroundModeManager::set_should_restart_in_background(false);
#endif  // BUILDFLAG(ENABLE_BACKGROUND_MODE)
}

bool IsTryingToQuit() {
  return g_trying_to_quit;
}

void StartShutdownTracing() {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kTraceShutdown)) {
    base::trace_event::TraceConfig trace_config(
        command_line.GetSwitchValueASCII(switches::kTraceShutdown), "");
    content::TracingController::GetInstance()->StartTracing(
        trace_config,
        content::TracingController::StartTracingDoneCallback());
  }
  TRACE_EVENT0("shutdown", "StartShutdownTracing");
}

}  // namespace browser_shutdown
