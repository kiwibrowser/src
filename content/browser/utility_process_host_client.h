// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_UTILITY_PROCESS_HOST_CLIENT_H_
#define CONTENT_BROWSER_UTILITY_PROCESS_HOST_CLIENT_H_

#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"

namespace IPC {
class Message;
}

namespace content {

// An interface to be implemented by consumers of the utility process to
// get results back.  All functions are called on the thread passed along
// to UtilityProcessHost.
class UtilityProcessHostClient
    : public base::RefCountedThreadSafe<UtilityProcessHostClient> {
 public:
  // Called when the process has crashed.
  virtual void OnProcessCrashed(int exit_code) {}

  // Called when the process fails to launch. |error_code| is one of
  // LaunchResultCode or sandbox::ResultCode containing the error.
  virtual void OnProcessLaunchFailed(int error_code) {}

  // Allow the client to filter IPC messages.
  virtual bool OnMessageReceived(const IPC::Message& message) = 0;

 protected:
  friend class base::RefCountedThreadSafe<UtilityProcessHostClient>;

  virtual ~UtilityProcessHostClient() {}
};

};  // namespace content

#endif  // CONTENT_BROWSER_UTILITY_PROCESS_HOST_CLIENT_H_
