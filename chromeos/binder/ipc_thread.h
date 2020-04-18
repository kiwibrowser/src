// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_BINDER_IPC_THREAD_H_
#define CHROMEOS_BINDER_IPC_THREAD_H_

#include <memory>

#include "base/macros.h"
#include "base/message_loop/message_pump_for_io.h"
#include "base/threading/thread.h"
#include "chromeos/binder/command_broker.h"
#include "chromeos/chromeos_export.h"

namespace binder {

class Driver;

// IpcThreadPoller watches the driver for incoming commands, and polls and
// handles them when necessary.
class CHROMEOS_EXPORT IpcThreadPoller
    : public base::MessagePumpForIO::FdWatcher {
 public:
  enum ThreadType {
    THREAD_TYPE_MAIN,  // The thread owns the driver instance.
    THREAD_TYPE_SUB,   // The thread is not the owner of the driver instance.
  };

  IpcThreadPoller(ThreadType type, Driver* driver);
  ~IpcThreadPoller() override;

  Driver* driver() { return driver_; }
  CommandBroker* command_broker() { return &command_broker_; }

  // Initializes this object.
  // Returns true on success.
  bool Initialize();

  // base::MessageLoopIO::Watcher overrides:
  void OnFileCanReadWithoutBlocking(int fd) override;
  void OnFileCanWriteWithoutBlocking(int fd) override;

 private:
  ThreadType type_;
  Driver* driver_;
  CommandBroker command_broker_;
  base::MessagePumpForIO::FdWatchController watcher_;
  DISALLOW_COPY_AND_ASSIGN(IpcThreadPoller);
};

// MainIpcThread manages binder-related resources (e.g. binder driver FD) and
// handles incoming binder commands.
class CHROMEOS_EXPORT MainIpcThread : public base::Thread {
 public:
  MainIpcThread();
  ~MainIpcThread() override;

  Driver* driver() { return driver_.get(); }
  CommandBroker* command_broker() { return poller_->command_broker(); }
  bool initialized() const { return initialized_; }

  // Starts this thread.
  // Returns true on success.
  bool Start();

 protected:
  // base::Thread overrides:
  void Init() override;
  void CleanUp() override;

 private:
  std::unique_ptr<Driver> driver_;
  std::unique_ptr<IpcThreadPoller> poller_;
  bool initialized_ = false;

  DISALLOW_COPY_AND_ASSIGN(MainIpcThread);
};

// SubIpcThread does the same thing as MainIpcThread, but relies on the driver
// instance owned by the main thread.
class CHROMEOS_EXPORT SubIpcThread : public base::Thread {
 public:
  // Constructs a sub thread with a Driver instance owned by the main thread.
  explicit SubIpcThread(MainIpcThread* main_thread);

  ~SubIpcThread() override;

  Driver* driver() { return poller_->driver(); }
  CommandBroker* command_broker() { return poller_->command_broker(); }
  bool initialized() const { return initialized_; }

  // Starts this thread.
  // Returns true on success.
  bool Start();

 protected:
  // base::Thread overrides:
  void Init() override;
  void CleanUp() override;

 private:
  MainIpcThread* main_thread_;
  std::unique_ptr<IpcThreadPoller> poller_;
  bool initialized_ = false;

  DISALLOW_COPY_AND_ASSIGN(SubIpcThread);
};

}  // namespace binder

#endif  // CHROMEOS_BINDER_IPC_THREAD_H_
