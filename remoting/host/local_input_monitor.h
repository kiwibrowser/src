// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_LOCAL_INPUT_MONITOR_H_
#define REMOTING_HOST_LOCAL_INPUT_MONITOR_H_

#include <memory>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"

namespace base {
class SingleThreadTaskRunner;
}  // namespace base

namespace remoting {

class ClientSessionControl;

// Monitors the local input to notify about mouse movements. On Mac and Linux
// catches the disconnection keyboard shortcut (Ctlr-Alt-Esc) and invokes
// SessionController::Delegate::DisconnectSession() when this key combination is
// pressed.
//
// TODO(sergeyu): Refactor shortcut code to a separate class.
class LocalInputMonitor {
 public:
  virtual ~LocalInputMonitor() {}

  // Creates a platform-specific instance of LocalInputMonitor.
  // |client_session_control| is called on the |caller_task_runner| thread.
  static std::unique_ptr<LocalInputMonitor> Create(
      scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> input_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
      base::WeakPtr<ClientSessionControl> client_session_control);

 protected:
  LocalInputMonitor() {}
};

}  // namespace remoting

#endif  // REMOTING_HOST_LOCAL_INPUT_MONITOR_H_
