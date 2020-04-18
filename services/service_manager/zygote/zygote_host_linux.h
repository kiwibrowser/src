// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_ZYGOTE_ZYGOTE_HOST_LINUX_H_
#define SERVICES_SERVICE_MANAGER_ZYGOTE_ZYGOTE_HOST_LINUX_H_

#include <unistd.h>

#include "base/component_export.h"
#include "base/process/process.h"

namespace service_manager {

// https://chromium.googlesource.com/chromium/src/+/master/docs/linux_zygote.md

// The zygote host is an interface, in the browser process, to the zygote
// process.
class ZygoteHost {
 public:
  // Returns the singleton instance.
  static COMPONENT_EXPORT(SERVICE_MANAGER_ZYGOTE) ZygoteHost* GetInstance();

  virtual ~ZygoteHost() {}

  // Returns the pid of the Zygote process.
  virtual bool IsZygotePid(pid_t pid) = 0;

  // Returns an int which is a bitmask of kSandboxLinux* values. Only valid
  // after the first render has been forked.
  virtual int GetRendererSandboxStatus() const = 0;

  // Adjust the OOM score of the given renderer's PID.  The allowed
  // range for the score is [0, 1000], where higher values are more
  // likely to be killed by the OOM killer.
  virtual void AdjustRendererOOMScore(base::ProcessHandle process_handle,
                                      int score) = 0;
};

}  // namespace service_manager

#endif  // SERVICES_SERVICE_MANAGER_ZYGOTE_ZYGOTE_HOST_LINUX_H_
