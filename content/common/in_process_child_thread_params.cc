// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/in_process_child_thread_params.h"

namespace content {

InProcessChildThreadParams::InProcessChildThreadParams(
    scoped_refptr<base::SingleThreadTaskRunner> io_runner,
    mojo::edk::OutgoingBrokerClientInvitation* broker_client_invitation,
    const std::string& service_request_token)
    : io_runner_(std::move(io_runner)),
      broker_client_invitation_(broker_client_invitation),
      service_request_token_(service_request_token) {}

InProcessChildThreadParams::InProcessChildThreadParams(
    const InProcessChildThreadParams& other) = default;

InProcessChildThreadParams::~InProcessChildThreadParams() {
}

}  // namespace content
