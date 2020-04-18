// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/crash/app_state_tracker.h"

#include "base/no_destructor.h"
#include "chromecast/crash/cast_crash_keys.h"
#include "components/crash/core/common/crash_key.h"

namespace {

struct CurrentAppState {
  std::string previous_app;
  std::string current_app;
  std::string last_launched_app;
};

CurrentAppState* GetAppState() {
  static base::NoDestructor<CurrentAppState> app_state;
  return app_state.get();
}

}  // namespace

namespace chromecast {

// static
std::string AppStateTracker::GetLastLaunchedApp() {
  return GetAppState()->last_launched_app;
}

// static
std::string AppStateTracker::GetCurrentApp() {
  return GetAppState()->current_app;
}

// static
std::string AppStateTracker::GetPreviousApp() {
  return GetAppState()->previous_app;
}

// static
void AppStateTracker::SetLastLaunchedApp(const std::string& app_id) {
  GetAppState()->last_launched_app = app_id;

  crash_keys::last_app.Set(app_id);
}

// static
void AppStateTracker::SetCurrentApp(const std::string& app_id) {
  CurrentAppState* app_state = GetAppState();
  app_state->previous_app = app_state->current_app;
  app_state->current_app = app_id;

  static crash_reporter::CrashKeyString<64> current_app("current_app");
  current_app.Set(app_id);

  crash_keys::previous_app.Set(app_state->previous_app);
}

}  // namespace chromecast
