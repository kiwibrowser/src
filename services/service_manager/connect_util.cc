// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/connect_util.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "services/service_manager/connect_params.h"
#include "services/service_manager/service_manager.h"

namespace service_manager {

mojo::ScopedMessagePipeHandle BindInterface(
    ServiceManager* service_manager,
    const Identity& source,
    const Identity& target,
    const std::string& interface_name) {
  std::unique_ptr<ConnectParams> params(new ConnectParams);
  params->set_source(source);
  params->set_target(target);
  mojo::MessagePipe pipe;
  params->set_interface_request_info(interface_name, std::move(pipe.handle1));
  params->set_start_service_callback(base::DoNothing());
  service_manager->Connect(std::move(params));
  return std::move(pipe.handle0);
}

}  // namespace service_manager
