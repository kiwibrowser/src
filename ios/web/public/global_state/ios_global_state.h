// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_GLOBAL_STATE_IOS_GLOBAL_STATE_H_
#define IOS_WEB_PUBLIC_GLOBAL_STATE_IOS_GLOBAL_STATE_H_

#include "base/task_scheduler/task_scheduler.h"

namespace ios_global_state {

// Contains parameters passed to |Create|.
struct CreateParams {
  CreateParams() : install_at_exit_manager(false), argc(0), argv(nullptr) {}

  bool install_at_exit_manager;

  int argc;
  const char** argv;
};

// Creates global state for iOS. This should be called as early as possible in
// the application lifecycle. It is safe to call this method more than once, the
// initialization will only be performed once.
//
// An AtExitManager will only be created if |register_exit_manager| is true. If
// |register_exit_manager| is false, an AtExitManager must already exist before
// calling |Create|.
// |argc| and |argv| may be set to the command line options which were passed to
// the application.
//
// Since the initialization will only be performed the first time this method is
// called, the values of all the parameters will be ignored after the first
// call.
void Create(const CreateParams& create_params);

// Creates a message loop for the UI thread and attaches it. It is safe to call
// this method more than once, the initialization will only be performed once.
void BuildMessageLoop();

// Destroys the message loop create by BuildMessageLoop. It is safe to call
// multiple time.
void DestroyMessageLoop();

// Creates a network change notifier.  It is safe to call this method more than
// once, the initialization will only be performed once.
void CreateNetworkChangeNotifier();

// Destroys the network change notifier created by CreateNetworkChangeNotifier.
// It is safe to call this method multiple time.
void DestroyNetworkChangeNotifier();

// Starts a global base::TaskScheduler. This method must be called to start
// the Task Scheduler that is created in |Create|. If |init_params| is null,
// default InitParams will be used. It is safe to call this method more than
// once, the task scheduler will only be started once.
void StartTaskScheduler(base::TaskScheduler::InitParams* init_params);

// Destroys the AtExitManager if one was created in |Create|. It is safe to call
// this method even if |install_at_exit_manager| was false in the CreateParams
// passed to |Create|. It is safe to call this method more than once, the
// AtExitManager will be destroyed on the first call.
void DestroyAtExitManager();

}  // namespace ios_global_state

#endif  // IOS_WEB_PUBLIC_GLOBAL_STATE_IOS_GLOBAL_STATE_H_
