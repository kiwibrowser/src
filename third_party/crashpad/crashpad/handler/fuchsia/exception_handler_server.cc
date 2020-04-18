// Copyright 2017 The Crashpad Authors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "handler/fuchsia/exception_handler_server.h"

#include <zircon/syscalls/exception.h>
#include <zircon/syscalls/port.h>

#include <utility>

#include "base/fuchsia/fuchsia_logging.h"
#include "base/logging.h"
#include "handler/fuchsia/crash_report_exception_handler.h"
#include "util/fuchsia/system_exception_port_key.h"

namespace crashpad {

ExceptionHandlerServer::ExceptionHandlerServer(
    base::ScopedZxHandle root_job,
    base::ScopedZxHandle exception_port)
    : root_job_(std::move(root_job)),
      exception_port_(std::move(exception_port)) {}

ExceptionHandlerServer::~ExceptionHandlerServer() = default;

void ExceptionHandlerServer::Run(CrashReportExceptionHandler* handler) {
  while (true) {
    zx_port_packet_t packet;
    zx_status_t status =
        zx_port_wait(exception_port_.get(), ZX_TIME_INFINITE, &packet, 1);
    if (status != ZX_OK) {
      ZX_LOG(ERROR, status) << "zx_port_wait, aborting";
      return;
    }

    if (packet.key != kSystemExceptionPortKey) {
      LOG(ERROR) << "unexpected packet key, ignoring";
      continue;
    }

    bool result =
        handler->HandleException(packet.exception.pid, packet.exception.tid);
    if (!result) {
      LOG(ERROR) << "HandleException failed";
    }
  }
}

}  // namespace crashpad
