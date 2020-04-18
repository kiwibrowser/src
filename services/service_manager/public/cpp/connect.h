// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_PUBLIC_CPP_CONNECT_H_
#define SERVICES_SERVICE_MANAGER_PUBLIC_CPP_CONNECT_H_

#include <utility>

#include "services/service_manager/public/mojom/interface_provider.mojom.h"

namespace service_manager {

// Binds |ptr| to a remote implementation of Interface from |interfaces|.
template <typename Interface>
inline void GetInterface(mojom::InterfaceProvider* interfaces,
                         mojo::InterfacePtr<Interface>* ptr) {
  mojo::MessagePipe pipe;
  ptr->Bind(mojo::InterfacePtrInfo<Interface>(std::move(pipe.handle0), 0u));
  interfaces->GetInterface(Interface::Name_, std::move(pipe.handle1));
}

}  // namespace service_manager

#endif  // SERVICES_SERVICE_MANAGER_PUBLIC_CPP_CONNECT_H_
