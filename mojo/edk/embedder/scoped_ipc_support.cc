// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/embedder/scoped_ipc_support.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread_restrictions.h"
#include "mojo/edk/system/core.h"

namespace mojo {
namespace edk {

namespace {

void ShutdownIPCSupport(const base::Closure& callback) {
  Core::Get()->RequestShutdown(callback);
}

}  // namespace

ScopedIPCSupport::ScopedIPCSupport(
    scoped_refptr<base::TaskRunner> io_thread_task_runner,
    ShutdownPolicy shutdown_policy) : shutdown_policy_(shutdown_policy) {
  Core::Get()->SetIOTaskRunner(io_thread_task_runner);
}

ScopedIPCSupport::~ScopedIPCSupport() {
  if (shutdown_policy_ == ShutdownPolicy::FAST) {
    ShutdownIPCSupport(base::DoNothing());
    return;
  }

  base::WaitableEvent shutdown_event(
      base::WaitableEvent::ResetPolicy::MANUAL,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  ShutdownIPCSupport(base::Bind(&base::WaitableEvent::Signal,
                                base::Unretained(&shutdown_event)));

  base::ThreadRestrictions::ScopedAllowWait allow_io;
  shutdown_event.Wait();
}

}  // namespace edk
}  // namespace mojo
