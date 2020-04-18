// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/service_process_util_posix.h"

#include <string.h>

#include <utility>

#include "base/bind.h"
#include "base/location.h"
#include "base/message_loop/message_loop_current.h"
#include "base/posix/eintr_wrapper.h"
#include "base/synchronization/waitable_event.h"
#include "chrome/common/multi_process_lock.h"

namespace {
int g_signal_socket = -1;
}

// Attempts to take a lock named |name|. If |waiting| is true then this will
// make multiple attempts to acquire the lock.
// Caller is responsible for ownership of the MultiProcessLock.
MultiProcessLock* TakeNamedLock(const std::string& name, bool waiting) {
  std::unique_ptr<MultiProcessLock> lock(MultiProcessLock::Create(name));
  if (lock == NULL) return NULL;
  bool got_lock = false;
  for (int i = 0; i < 10; ++i) {
    if (lock->TryLock()) {
      got_lock = true;
      break;
    }
    if (!waiting) break;
    base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(100 * i));
  }
  if (!got_lock) {
    lock.reset();
  }
  return lock.release();
}

ServiceProcessTerminateMonitor::ServiceProcessTerminateMonitor(
    const base::Closure& terminate_task)
    : terminate_task_(terminate_task) {
}

ServiceProcessTerminateMonitor::~ServiceProcessTerminateMonitor() {
}

void ServiceProcessTerminateMonitor::OnFileCanReadWithoutBlocking(int fd) {
  if (!terminate_task_.is_null()) {
    int buffer;
    int length = read(fd, &buffer, sizeof(buffer));
    if ((length == sizeof(buffer)) && (buffer == kTerminateMessage)) {
      terminate_task_.Run();
      terminate_task_.Reset();
    } else if (length > 0) {
      DLOG(ERROR) << "Unexpected read: " << buffer;
    } else if (length == 0) {
      DLOG(ERROR) << "Unexpected fd close";
    } else if (length < 0) {
      DPLOG(ERROR) << "read";
    }
  }
}

void ServiceProcessTerminateMonitor::OnFileCanWriteWithoutBlocking(int fd) {
  NOTIMPLEMENTED();
}

// "Forced" Shutdowns on POSIX are done via signals. The magic signal for
// a shutdown is SIGTERM. "write" is a signal safe function. PLOG(ERROR) is
// not, but we don't ever expect it to be called.
static void SigTermHandler(int sig, siginfo_t* info, void* uap) {
  // TODO(dmaclach): add security here to make sure that we are being shut
  //                 down by an appropriate process.
  int message = ServiceProcessTerminateMonitor::kTerminateMessage;
  if (write(g_signal_socket, &message, sizeof(message)) < 0) {
    DPLOG(ERROR) << "write";
  }
}

ServiceProcessState::StateData::StateData()
    : watcher(FROM_HERE), set_action(false) {
  memset(sockets, -1, sizeof(sockets));
  memset(&old_action, 0, sizeof(old_action));
}

void ServiceProcessState::StateData::SignalReady(base::WaitableEvent* signal,
                                                 bool* success) {
  DCHECK(task_runner->BelongsToCurrentThread());
  DCHECK_EQ(g_signal_socket, -1);
  DCHECK(!signal->IsSignaled());
  *success = base::MessageLoopCurrentForIO::Get()->WatchFileDescriptor(
      sockets[0], true, base::MessagePumpForIO::WATCH_READ, &watcher,
      terminate_monitor.get());
  if (!*success) {
    DLOG(ERROR) << "WatchFileDescriptor";
    signal->Signal();
    return;
  }
  g_signal_socket = sockets[1];

  // Set up signal handler for SIGTERM.
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_sigaction = SigTermHandler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = SA_SIGINFO;
  *success = sigaction(SIGTERM, &action, &old_action) == 0;
  if (!*success) {
    DPLOG(ERROR) << "sigaction";
    signal->Signal();
    return;
  }

  // If the old_action is not default, somebody else has installed a
  // a competing handler. Our handler is going to override it so it
  // won't be called. If this occurs it needs to be fixed.
  DCHECK_EQ(old_action.sa_handler, SIG_DFL);
  set_action = true;

#if defined(OS_MACOSX)
  *success = WatchExecutable();
  if (!*success) {
    DLOG(ERROR) << "WatchExecutable";
    signal->Signal();
    return;
  }
#elif defined(OS_POSIX)
  initializing_lock.reset();
#endif  // OS_POSIX
  signal->Signal();
}

ServiceProcessState::StateData::~StateData() {
  // StateData is destroyed on the thread that called SignalReady() (if any) to
  // satisfy the requirement that base::FilePathWatcher is destroyed in sequence
  // with base::FilePathWatcher::Watch().
  DCHECK(!task_runner || task_runner->BelongsToCurrentThread());

  if (sockets[0] != -1) {
    if (IGNORE_EINTR(close(sockets[0]))) {
      DPLOG(ERROR) << "close";
    }
  }
  if (sockets[1] != -1) {
    if (IGNORE_EINTR(close(sockets[1]))) {
      DPLOG(ERROR) << "close";
    }
  }
  if (set_action) {
    if (sigaction(SIGTERM, &old_action, NULL) < 0) {
      DPLOG(ERROR) << "sigaction";
    }
  }
  g_signal_socket = -1;
}

void ServiceProcessState::CreateState() {
  DCHECK(!state_);
  state_ = new StateData();
}

bool ServiceProcessState::SignalReady(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    const base::Closure& terminate_task) {
  DCHECK(task_runner);
  DCHECK(state_);

#if !defined(OS_MACOSX)
  state_->running_lock.reset(TakeServiceRunningLock(true));
  if (state_->running_lock.get() == NULL) {
    return false;
  }
#endif
  state_->terminate_monitor.reset(
      new ServiceProcessTerminateMonitor(terminate_task));
  if (pipe(state_->sockets) < 0) {
    DPLOG(ERROR) << "pipe";
    return false;
  }
  base::WaitableEvent signal_ready(
      base::WaitableEvent::ResetPolicy::MANUAL,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  bool success = false;

  state_->task_runner = std::move(task_runner);
  state_->task_runner->PostTask(
      FROM_HERE, base::Bind(&ServiceProcessState::StateData::SignalReady,
                            base::Unretained(state_), &signal_ready, &success));
  signal_ready.Wait();
  return success;
}

void ServiceProcessState::TearDownState() {
  if (state_ && state_->task_runner)
    state_->task_runner->DeleteSoon(FROM_HERE, state_);
  else
    delete state_;
  state_ = nullptr;
}
