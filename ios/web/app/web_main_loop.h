// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_APP_WEB_MAIN_LOOP_H_
#define IOS_WEB_APP_WEB_MAIN_LOOP_H_

#include <memory>

#include "base/callback_helpers.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "ios/web/public/app/task_scheduler_init_params_callback.h"

namespace base {
class PowerMonitor;
}  // namespace base

namespace web {
class CookieNotificationBridge;
class ServiceManagerContext;
class WebMainParts;
class WebThreadImpl;

// Implements the main web loop stages called from WebMainRunner.
// See comments in web_main_parts.h for additional info.
class WebMainLoop {
 public:
  explicit WebMainLoop();
  virtual ~WebMainLoop();

  void Init();

  void EarlyInitialization();
  void MainMessageLoopStart();

  // Creates and starts running the tasks needed to complete startup. The
  // |init_params_callback| may be null or supply InitParams to be used to start
  // the global TaskScheduler instead of using the defaults.
  void CreateStartupTasks(TaskSchedulerInitParamsCallback init_params_callback);

  // Performs the shutdown sequence, starting with PostMainMessageLoopRun
  // through stopping threads to PostDestroyThreads.
  void ShutdownThreadsAndCleanUp();

  int GetResultCode() const { return result_code_; }

 private:
  void InitializeMainThread();

  // Called just before creating the threads
  int PreCreateThreads();

  // Creates all secondary threads. The |init_params_callback| may be null or
  // supply InitParams to be used to start the global TaskScheduler instead of
  // using the defaults.
  int CreateThreads(TaskSchedulerInitParamsCallback init_params_callback);

  // Called right after the web threads have been started.
  int WebThreadsStarted();

  // Called just before attaching to the main message loop.
  int PreMainMessageLoopRun();

  // Members initialized on construction ---------------------------------------
  int result_code_;
  // True if the non-UI threads were created.
  bool created_threads_;

  // Members initialized in |MainMessageLoopStart()| ---------------------------
  // The MessageLoop and NetworkChangeNotifier are not owned by the WebMainLoop
  // but still need to be destroyed in correct order so use ScopedClosureRunner.
  base::ScopedClosureRunner destroy_message_loop_;
  std::unique_ptr<base::PowerMonitor> power_monitor_;
  base::ScopedClosureRunner destroy_network_change_notifier_;

  // Destroy parts_ before main_message_loop_ (required) and before other
  // classes constructed in web (but after main_thread_).
  std::unique_ptr<WebMainParts> parts_;

  // Members initialized in |InitializeMainThread()| ---------------------------
  // This must get destroyed after other threads that are created in parts_.
  std::unique_ptr<WebThreadImpl> main_thread_;

  // Members initialized in |RunMainMessageLoopParts()| ------------------------
  std::unique_ptr<WebThreadImpl> io_thread_;

  // Members initialized in |WebThreadsStarted()| --------------------------
  std::unique_ptr<CookieNotificationBridge> cookie_notification_bridge_;
  std::unique_ptr<ServiceManagerContext> service_manager_context_;

  DISALLOW_COPY_AND_ASSIGN(WebMainLoop);
};

}  // namespace web

#endif  // IOS_WEB_APP_WEB_MAIN_LOOP_H_
