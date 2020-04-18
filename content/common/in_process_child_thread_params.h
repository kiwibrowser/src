// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_IN_PROCESS_CHILD_THREAD_PARAMS_H_
#define CONTENT_COMMON_IN_PROCESS_CHILD_THREAD_PARAMS_H_

#include <string>

#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "content/common/content_export.h"
#include "mojo/edk/embedder/outgoing_broker_client_invitation.h"

namespace content {

// Tells ChildThreadImpl to run in in-process mode. There are a couple of
// parameters to run in the mode: An emulated io task runner used by
// ChnanelMojo, an IPC channel name to open.
class CONTENT_EXPORT InProcessChildThreadParams {
 public:
  InProcessChildThreadParams(
      scoped_refptr<base::SingleThreadTaskRunner> io_runner,
      mojo::edk::OutgoingBrokerClientInvitation* broker_client_invitation,
      const std::string& service_request_token);
  InProcessChildThreadParams(const InProcessChildThreadParams& other);
  ~InProcessChildThreadParams();

  scoped_refptr<base::SingleThreadTaskRunner> io_runner() const {
    return io_runner_;
  }

  mojo::edk::OutgoingBrokerClientInvitation* broker_client_invitation() const {
    return broker_client_invitation_;
  }

  const std::string& service_request_token() const {
    return service_request_token_;
  }

 private:
  scoped_refptr<base::SingleThreadTaskRunner> io_runner_;
  mojo::edk::OutgoingBrokerClientInvitation* const broker_client_invitation_;
  std::string service_request_token_;
};

}  // namespace content

#endif  // CONTENT_COMMON_IN_PROCESS_CHILD_THREAD_PARAMS_H_
