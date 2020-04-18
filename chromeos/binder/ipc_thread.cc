// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/binder/ipc_thread.h"

#include "base/message_loop/message_loop.h"
#include "base/message_loop/message_loop_current.h"
#include "chromeos/binder/command_broker.h"
#include "chromeos/binder/driver.h"

namespace binder {

// IpcThreadPoller
IpcThreadPoller::IpcThreadPoller(ThreadType type, Driver* driver)
    : type_(type),
      driver_(driver),
      command_broker_(driver),
      watcher_(FROM_HERE) {}

IpcThreadPoller::~IpcThreadPoller() {
  if (!command_broker_.ExitLooper()) {
    LOG(ERROR) << "Failed to exit looper.";
  }
  if (!driver_->NotifyCurrentThreadExiting()) {
    LOG(ERROR) << "Failed to send thread exit.";
  }
}

bool IpcThreadPoller::Initialize() {
  if (type_ == THREAD_TYPE_MAIN) {
    if (!command_broker_.EnterLooper()) {
      LOG(ERROR) << "Failed to enter looper.";
      return false;
    }
  } else {
    if (!command_broker_.RegisterLooper()) {
      LOG(ERROR) << "Failed to register looper.";
      return false;
    }
  }
  if (!base::MessageLoopCurrentForIO::Get()->WatchFileDescriptor(
          driver_->GetFD(), true, base::MessagePumpForIO::WATCH_READ, &watcher_,
          this)) {
    LOG(ERROR) << "Failed to initialize watcher.";
    return false;
  }
  return true;
}

void IpcThreadPoller::OnFileCanReadWithoutBlocking(int fd) {
  bool success = command_broker_.PollCommands();
  LOG_IF(ERROR, !success) << "PollCommands() failed.";
}

void IpcThreadPoller::OnFileCanWriteWithoutBlocking(int fd) {
  NOTREACHED();
}

// MainIpcThread
MainIpcThread::MainIpcThread() : base::Thread("BinderMainThread") {}

MainIpcThread::~MainIpcThread() {
  Stop();
}

bool MainIpcThread::Start() {
  base::Thread::Options options;
  options.message_loop_type = base::MessageLoop::TYPE_IO;
  return StartWithOptions(options);
}

void MainIpcThread::Init() {
  driver_.reset(new Driver());
  if (!driver_->Initialize()) {
    LOG(ERROR) << "Failed to initialize driver.";
    return;
  }
  poller_.reset(
      new IpcThreadPoller(IpcThreadPoller::THREAD_TYPE_MAIN, driver_.get()));
  if (!poller_->Initialize()) {
    LOG(ERROR) << "Failed to initialize poller.";
    return;
  }
  initialized_ = true;
}

void MainIpcThread::CleanUp() {
  poller_.reset();
  driver_.reset();
}

// SubIpcThread
SubIpcThread::SubIpcThread(MainIpcThread* main_thread)
    : base::Thread("BinderSubThread"), main_thread_(main_thread) {}

SubIpcThread::~SubIpcThread() {
  Stop();
}

bool SubIpcThread::Start() {
  base::Thread::Options options;
  options.message_loop_type = base::MessageLoop::TYPE_IO;
  return StartWithOptions(options);
}

void SubIpcThread::Init() {
  // Wait for the main thread to finish initialization.
  if (!main_thread_->WaitUntilThreadStarted()) {
    LOG(ERROR) << "Failed to wait for the main thread.";
    return;
  }
  poller_.reset(new IpcThreadPoller(IpcThreadPoller::THREAD_TYPE_SUB,
                                    main_thread_->driver()));
  if (!poller_->Initialize()) {
    LOG(ERROR) << "Failed to initialize poller.";
    return;
  }
  initialized_ = true;
}

void SubIpcThread::CleanUp() {
  poller_.reset();
}

}  // namespace binder
