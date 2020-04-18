// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/associated_interface_provider_impl.h"
#include "base/callback.h"
#include "base/run_loop.h"
#include "mojo/public/cpp/bindings/associated_binding.h"

namespace content {

class AssociatedInterfaceProviderImpl::LocalProvider
    : public mojom::AssociatedInterfaceProvider {
 public:
  LocalProvider(mojom::AssociatedInterfaceProviderAssociatedPtr* proxy,
                scoped_refptr<base::SingleThreadTaskRunner> task_runner)
      : associated_interface_provider_binding_(this) {
    associated_interface_provider_binding_.Bind(
        mojo::MakeRequestAssociatedWithDedicatedPipe(proxy),
        std::move(task_runner));
  }

  ~LocalProvider() override {}

  void SetBinderForName(
      const std::string& name,
      const base::Callback<void(mojo::ScopedInterfaceEndpointHandle)>& binder) {
    binders_[name] = binder;
  }

 private:
  // mojom::AssociatedInterfaceProvider:
  void GetAssociatedInterface(
      const std::string& name,
      mojom::AssociatedInterfaceAssociatedRequest request) override {
    auto it = binders_.find(name);
    if (it != binders_.end())
      it->second.Run(request.PassHandle());
  }

  using BinderMap =
      std::map<std::string,
               base::Callback<void(mojo::ScopedInterfaceEndpointHandle)>>;
  BinderMap binders_;

  mojo::AssociatedBinding<mojom::AssociatedInterfaceProvider>
      associated_interface_provider_binding_;
};

AssociatedInterfaceProviderImpl::AssociatedInterfaceProviderImpl(
    mojom::AssociatedInterfaceProviderAssociatedPtr proxy,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : proxy_(std::move(proxy)), task_runner_(std::move(task_runner)) {
  DCHECK(proxy_.is_bound());
}

AssociatedInterfaceProviderImpl::AssociatedInterfaceProviderImpl(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : local_provider_(std::make_unique<LocalProvider>(&proxy_, task_runner)),
      task_runner_(std::move(task_runner)) {}

AssociatedInterfaceProviderImpl::~AssociatedInterfaceProviderImpl() {}

void AssociatedInterfaceProviderImpl::GetInterface(
    const std::string& name,
    mojo::ScopedInterfaceEndpointHandle handle) {
  proxy_->GetAssociatedInterface(
      name, mojom::AssociatedInterfaceAssociatedRequest(std::move(handle)));
}

void AssociatedInterfaceProviderImpl::OverrideBinderForTesting(
    const std::string& name,
    const base::Callback<void(mojo::ScopedInterfaceEndpointHandle)>& binder) {
  if (!local_provider_) {
    DCHECK(proxy_.is_bound());
    proxy_.reset();
    local_provider_ = std::make_unique<LocalProvider>(&proxy_, task_runner_);
  }
  local_provider_->SetBinderForName(name, binder);
}

}  // namespace content
