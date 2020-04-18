// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_ASSOCIATED_INTERFACE_PROVIDER_IMPL_H_
#define CONTENT_COMMON_ASSOCIATED_INTERFACE_PROVIDER_IMPL_H_

#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"

#include <stdint.h>
#include <memory>

#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "content/common/associated_interfaces.mojom.h"

namespace content {

class AssociatedInterfaceProviderImpl
    : public blink::AssociatedInterfaceProvider {
 public:
  // Binds this to a remote mojom::AssociatedInterfaceProvider.
  //
  // |task_runner| must belong to the same thread. It will be used to dispatch
  // all callbacks and connection error notification.
  explicit AssociatedInterfaceProviderImpl(
      mojom::AssociatedInterfaceProviderAssociatedPtr proxy,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner = nullptr);

  // Constructs a local provider with no remote interfaces. This is useful in
  // conjunction with OverrideBinderForTesting(), in test environments where
  // there may not be a remote |mojom::AssociatedInterfaceProvider| available.
  //
  // |task_runner| must belong to the same thread. It will be used to dispatch
  // all callbacks and connection error notification.
  explicit AssociatedInterfaceProviderImpl(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner);
  ~AssociatedInterfaceProviderImpl() override;

  // AssociatedInterfaceProvider:
  void GetInterface(const std::string& name,
                    mojo::ScopedInterfaceEndpointHandle handle) override;
  void OverrideBinderForTesting(
      const std::string& name,
      const base::Callback<void(mojo::ScopedInterfaceEndpointHandle)>& binder)
      override;

 private:
  class LocalProvider;

  mojom::AssociatedInterfaceProviderAssociatedPtr proxy_;

  std::unique_ptr<LocalProvider> local_provider_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(AssociatedInterfaceProviderImpl);
};

}  // namespace content

#endif  // CONTENT_COMMON_ASSOCIATED_INTERFACE_PROVIDER_IMPL_H_
