// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MEDIA_INTERFACE_PROVIDER_H_
#define MEDIA_MOJO_SERVICES_MEDIA_INTERFACE_PROVIDER_H_

#include "media/mojo/services/media_mojo_export.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/mojom/interface_provider.mojom.h"

namespace media {

class MEDIA_MOJO_EXPORT MediaInterfaceProvider
    : public service_manager::mojom::InterfaceProvider {
 public:
  explicit MediaInterfaceProvider(
      service_manager::mojom::InterfaceProviderRequest request);
  ~MediaInterfaceProvider() override;

  service_manager::BinderRegistry* registry() { return &registry_; }

 private:
  // service_manager::mojom::InterfaceProvider:
  void GetInterface(const std::string& interface_name,
                    mojo::ScopedMessagePipeHandle handle) override;

  service_manager::BinderRegistry registry_;

  mojo::Binding<service_manager::mojom::InterfaceProvider> binding_;

  DISALLOW_COPY_AND_ASSIGN(MediaInterfaceProvider);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MEDIA_BINDER_REGISTRY_H_
