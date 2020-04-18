// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_WEB_STATE_WEB_STATE_INTERFACE_PROVIDER_H_
#define IOS_WEB_WEB_STATE_WEB_STATE_INTERFACE_PROVIDER_H_

#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/mojom/interface_provider.mojom.h"

namespace web {

class WebStateInterfaceProvider
    : public service_manager::mojom::InterfaceProvider {
 public:
  WebStateInterfaceProvider();
  ~WebStateInterfaceProvider() override;

  void Bind(service_manager::mojom::InterfaceProviderRequest request);

  service_manager::BinderRegistry* registry() { return &registry_; }

 private:
  // service_manager::mojom::InterfaceProvider:
  void GetInterface(const std::string& interface_name,
                    mojo::ScopedMessagePipeHandle handle) override;

  service_manager::BinderRegistry registry_;

  mojo::Binding<service_manager::mojom::InterfaceProvider> binding_;

  DISALLOW_COPY_AND_ASSIGN(WebStateInterfaceProvider);
};

}  // namespace web

#endif  // IOS_WEB_WEB_STATE_WEB_STATE_INTERFACE_PROVIDER_H_
