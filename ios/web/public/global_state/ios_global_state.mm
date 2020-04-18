// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web/public/global_state/ios_global_state.h"

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/message_loop/message_loop_current.h"
#include "base/task_scheduler/initialization_util.h"
#include "net/base/network_change_notifier.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

base::AtExitManager* g_exit_manager = nullptr;
base::MessageLoopForUI* g_message_loop = nullptr;
net::NetworkChangeNotifier* g_network_change_notifer = nullptr;

base::TaskScheduler::InitParams GetDefaultTaskSchedulerInitParams() {
  return base::TaskScheduler::InitParams(
      base::SchedulerWorkerPoolParams(
          base::RecommendedMaxNumberOfThreadsInPool(2, 8, 0.1, 0),
          base::TimeDelta::FromSeconds(30)),
      base::SchedulerWorkerPoolParams(
          base::RecommendedMaxNumberOfThreadsInPool(2, 8, 0.1, 0),
          base::TimeDelta::FromSeconds(30)),
      base::SchedulerWorkerPoolParams(
          base::RecommendedMaxNumberOfThreadsInPool(3, 8, 0.3, 0),
          base::TimeDelta::FromSeconds(30)),
      base::SchedulerWorkerPoolParams(
          base::RecommendedMaxNumberOfThreadsInPool(3, 8, 0.3, 0),
          base::TimeDelta::FromSeconds(60)));
}

}  // namespace

namespace ios_global_state {

void Create(const CreateParams& create_params) {
  static dispatch_once_t once_token;
  dispatch_once(&once_token, ^{
    if (create_params.install_at_exit_manager) {
      g_exit_manager = new base::AtExitManager();
    }
    base::CommandLine::Init(create_params.argc, create_params.argv);

    base::TaskScheduler::Create("Browser");
  });
}

void BuildMessageLoop() {
  static dispatch_once_t once_token;
  dispatch_once(&once_token, ^{
    // Create a MessageLoop if one does not already exist for the current
    // thread.
    if (!base::MessageLoopCurrent::Get()) {
      g_message_loop = new base::MessageLoopForUI();
    }
    base::MessageLoopCurrentForUI::Get()->Attach();
  });
}

void DestroyMessageLoop() {
  delete g_message_loop;
  g_message_loop = nullptr;
}

void CreateNetworkChangeNotifier() {
  static dispatch_once_t once_token;
  dispatch_once(&once_token, ^{
    g_network_change_notifer = net::NetworkChangeNotifier::Create();
  });
}

void DestroyNetworkChangeNotifier() {
  delete g_network_change_notifer;
  g_network_change_notifer = nullptr;
}

void StartTaskScheduler(base::TaskScheduler::InitParams* params) {
  static dispatch_once_t once_token;
  dispatch_once(&once_token, ^{
    auto init_params = params ? *params : GetDefaultTaskSchedulerInitParams();
    base::TaskScheduler::GetInstance()->Start(init_params);
  });
}

void DestroyAtExitManager() {
  delete g_exit_manager;
  g_exit_manager = nullptr;
}

}  // namespace ios_global_state
