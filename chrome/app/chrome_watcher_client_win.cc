// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/app/chrome_watcher_client_win.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/macros.h"
#include "components/browser_watcher/watcher_client_win.h"

namespace {

// Because we can only bind parameters from the left, |parent_process| must be
// the last parameter of the method that we bind into a
// BrowserWatcherClient::CommandLineGenerator. The ChromeWatcherClient API is
// more intuitive if the ChromeWatcherClient::CommandLineGenerator takes
// |parent_process| as its second parameter, so we use this intermediate
// function to swap the order.
base::CommandLine InvokeCommandLineGenerator(
    const ChromeWatcherClient::CommandLineGenerator& command_line_generator,
    HANDLE on_initialized_event,
    HANDLE parent_process) {
  return command_line_generator.Run(parent_process, ::GetCurrentThreadId(),
                                    on_initialized_event);
}

}  // namespace

ChromeWatcherClient::ChromeWatcherClient(
    const CommandLineGenerator& command_line_generator)
    : command_line_generator_(command_line_generator) {
}

ChromeWatcherClient::~ChromeWatcherClient() {
}

bool ChromeWatcherClient::LaunchWatcher() {
  // Create an inheritable event that the child process will signal when it has
  // completed initialization.
  SECURITY_ATTRIBUTES on_initialized_event_attributes = {
      sizeof(SECURITY_ATTRIBUTES),  // nLength
      nullptr,                      // lpSecurityDescriptor
      TRUE                          // bInheritHandle
  };
  on_initialized_event_.Set(::CreateEvent(&on_initialized_event_attributes,
                                          TRUE,  // manual reset
                                          FALSE, nullptr));
  if (!on_initialized_event_.IsValid()) {
    DPLOG(ERROR) << "Failed to create an event.";
    return false;
  }

  // Configure the basic WatcherClient, binding in the initialization event
  // HANDLE.
  browser_watcher::WatcherClient watcher_client(
      base::Bind(&InvokeCommandLineGenerator, command_line_generator_,
                 on_initialized_event_.Get()));
  // Indicate that the event HANDLE should be inherited.
  watcher_client.AddInheritedHandle(on_initialized_event_.Get());
  // Launch the watcher.
  watcher_client.LaunchWatcher();
  // Grab a handle to the watcher so that we may later wait on its
  // initialization.
  process_ = watcher_client.process().Duplicate();
  if (!process_.IsValid())
    on_initialized_event_.Close();
  return process_.IsValid();
}

bool ChromeWatcherClient::EnsureInitialized() {
  if (!process_.IsValid())
    return false;

  DCHECK(on_initialized_event_.IsValid());

  HANDLE handles[] = {on_initialized_event_.Get(), process_.Handle()};
  DWORD result = ::WaitForMultipleObjects(arraysize(handles), handles,
                                          FALSE, INFINITE);

  switch (result) {
    case WAIT_OBJECT_0:
      return true;
    case WAIT_OBJECT_0 + 1:
      LOG(ERROR) << "Chrome watcher process failed to launch.";
      return false;
    case WAIT_FAILED:
      DPLOG(ERROR) << "Failure while waiting on Chrome watcher process launch.";
      return false;
    default:
      NOTREACHED() << "Unexpected result while waiting on Chrome watcher "
                      "process launch: " << result;
      return false;
  }
}

bool ChromeWatcherClient::WaitForExit(int* exit_code) {
  return process_.IsValid() && process_.WaitForExit(exit_code);
}

bool ChromeWatcherClient::WaitForExitWithTimeout(base::TimeDelta timeout,
                                                 int* exit_code) {
  return process_.IsValid() &&
         process_.WaitForExitWithTimeout(timeout, exit_code);
}
