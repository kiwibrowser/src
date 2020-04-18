// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web/public/web_state/web_state_interface_provider.h"

namespace web {

WebStateInterfaceProvider::WebStateInterfaceProvider() : binding_(this) {}
WebStateInterfaceProvider::~WebStateInterfaceProvider() = default;

void WebStateInterfaceProvider::Bind(
    service_manager::mojom::InterfaceProviderRequest request) {
  binding_.Bind(std::move(request));
}

void WebStateInterfaceProvider::GetInterface(
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle handle) {
  registry_.BindInterface(interface_name, std::move(handle));
}

}  // namespace web
