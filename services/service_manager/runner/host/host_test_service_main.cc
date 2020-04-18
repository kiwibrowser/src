// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/system/message_pipe.h"
#include "services/service_manager/public/c/main.h"

MojoResult ServiceMain(MojoHandle service_request_handle) {
  mojo::ScopedMessagePipeHandle service_request_pipe;
  service_request_pipe.reset(mojo::MessagePipeHandle(service_request_handle));
  return MOJO_RESULT_OK;
}
