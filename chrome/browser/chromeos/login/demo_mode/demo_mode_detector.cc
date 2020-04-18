// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/demo_mode/demo_mode_detector.h"

#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "base/sys_info.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/login/ui/login_display_host.h"
#include "chrome/common/pref_names.h"
#include "chromeos/chromeos_switches.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace {
const int kDerelectDetectionTimeoutSeconds = 8 * 60 * 60;  // 8 hours.
const int kDerelectIdleTimeoutSeconds = 5 * 60;            // 5 minutes.
const int kOobeTimerUpdateIntervalSeconds = 5 * 60;        // 5 minutes.
}  // namespace

namespace chromeos {

// static
void DemoModeDetector::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterInt64Pref(prefs::kTimeOnOobe, 0);
}

DemoModeDetector::DemoModeDetector() : weak_ptr_factory_(this) {
  SetupTimeouts();
}

DemoModeDetector::~DemoModeDetector() {}

void DemoModeDetector::InitDetection() {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableDemoMode))
    return;

  const bool has_derelict_switch =
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDerelictDetectionTimeout) ||
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDerelictIdleTimeout);

  if (base::SysInfo::IsRunningOnChromeOS() && !has_derelict_switch) {
    std::string track;
    // We're running on an actual device; if we cannot find our release track
    // value or if the track contains "testimage", don't start demo mode.
    if (!base::SysInfo::GetLsbReleaseValue("CHROMEOS_RELEASE_TRACK", &track) ||
        track.find("testimage") != std::string::npos)
      return;
  }

  if (IsDerelict())
    StartIdleDetection();
  else
    StartOobeTimer();
}

void DemoModeDetector::StopDetection() {
  oobe_timer_.Stop();
  idle_detector_.reset();
}

void DemoModeDetector::StartIdleDetection() {
  if (!idle_detector_) {
    idle_detector_.reset(new IdleDetector(
        base::Bind(&DemoModeDetector::OnIdle, weak_ptr_factory_.GetWeakPtr())));
  }
  idle_detector_->Start(derelict_idle_timeout_);
}

void DemoModeDetector::StartOobeTimer() {
  if (oobe_timer_.IsRunning())
    return;
  oobe_timer_.Start(FROM_HERE, oobe_timer_update_interval_, this,
                    &DemoModeDetector::OnOobeTimerUpdate);
}

void DemoModeDetector::OnIdle() {
  if (demo_launched_)
    return;
  demo_launched_ = true;
  LoginDisplayHost::default_host()->StartDemoAppLaunch();
}

void DemoModeDetector::OnOobeTimerUpdate() {
  time_on_oobe_ += oobe_timer_update_interval_;

  PrefService* prefs = g_browser_process->local_state();
  prefs->SetInt64(prefs::kTimeOnOobe, time_on_oobe_.InSeconds());

  if (IsDerelict()) {
    oobe_timer_.Stop();
    StartIdleDetection();
  }
}

void DemoModeDetector::SetupTimeouts() {
  base::CommandLine* cmdline = base::CommandLine::ForCurrentProcess();
  DCHECK(cmdline);

  PrefService* prefs = g_browser_process->local_state();
  time_on_oobe_ =
      base::TimeDelta::FromSeconds(prefs->GetInt64(prefs::kTimeOnOobe));

  int derelict_detection_timeout;
  if (!cmdline->HasSwitch(switches::kDerelictDetectionTimeout) ||
      !base::StringToInt(
          cmdline->GetSwitchValueASCII(switches::kDerelictDetectionTimeout),
          &derelict_detection_timeout)) {
    derelict_detection_timeout = kDerelectDetectionTimeoutSeconds;
  }
  derelict_detection_timeout_ =
      base::TimeDelta::FromSeconds(derelict_detection_timeout);

  int derelict_idle_timeout;
  if (!cmdline->HasSwitch(switches::kDerelictIdleTimeout) ||
      !base::StringToInt(
          cmdline->GetSwitchValueASCII(switches::kDerelictIdleTimeout),
          &derelict_idle_timeout)) {
    derelict_idle_timeout = kDerelectIdleTimeoutSeconds;
  }
  derelict_idle_timeout_ = base::TimeDelta::FromSeconds(derelict_idle_timeout);

  int oobe_timer_update_interval;
  if (!cmdline->HasSwitch(switches::kOobeTimerInterval) ||
      !base::StringToInt(
          cmdline->GetSwitchValueASCII(switches::kOobeTimerInterval),
          &oobe_timer_update_interval)) {
    oobe_timer_update_interval = kOobeTimerUpdateIntervalSeconds;
  }
  oobe_timer_update_interval_ =
      base::TimeDelta::FromSeconds(oobe_timer_update_interval);

  // In case we'd be derelict before our timer is set to trigger, reduce
  // the interval so we check again when we're scheduled to go derelict.
  oobe_timer_update_interval_ =
      std::max(std::min(oobe_timer_update_interval_,
                        derelict_detection_timeout_ - time_on_oobe_),
               base::TimeDelta::FromSeconds(0));
}

bool DemoModeDetector::IsDerelict() {
  return time_on_oobe_ >= derelict_detection_timeout_;
}

}  // namespace chromeos
