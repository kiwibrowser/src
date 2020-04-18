// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/simple_connection_filter.h"

namespace content {

SimpleConnectionFilter::SimpleConnectionFilter(
    std::unique_ptr<service_manager::BinderRegistry> registry)
    : registry_(std::move(registry)) {}

SimpleConnectionFilter::~SimpleConnectionFilter() {}

void SimpleConnectionFilter::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle* interface_pipe,
    service_manager::Connector* connector) {
  registry_->TryBindInterface(interface_name, interface_pipe);
}

SimpleConnectionFilterWithSourceInfo::SimpleConnectionFilterWithSourceInfo(
    std::unique_ptr<service_manager::BinderRegistryWithArgs<
        const service_manager::BindSourceInfo&>> registry)
    : registry_(std::move(registry)) {}

SimpleConnectionFilterWithSourceInfo::~SimpleConnectionFilterWithSourceInfo() {}

void SimpleConnectionFilterWithSourceInfo::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle* interface_pipe,
    service_manager::Connector* connector) {
  registry_->TryBindInterface(interface_name, interface_pipe, source_info);
}

}  // namespace content
