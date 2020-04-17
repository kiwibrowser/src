// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/content/service.h"

#include <utility>

#include "base/bind.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/content/public/mojom/view_factory.mojom.h"
#include "services/content/service_delegate.h"
#include "services/content/view_factory_impl.h"
#include "services/content/view_impl.h"
#include "services/service_manager/public/cpp/service_context.h"

namespace content {

Service::Service(ServiceDelegate* delegate) : delegate_(delegate) {
  binders_.AddInterface(base::BindRepeating(
      [](Service* service, mojom::ViewFactoryRequest request) {
        service->AddViewFactory(
            std::make_unique<ViewFactoryImpl>(service, std::move(request)));
      },
      this));
}

Service::~Service() {
  delegate_->WillDestroyServiceInstance(this);
}

void Service::ForceQuit() {
  // Ensure that all bound interfaces are disconnected and no further interface
  // requests will be handled.
  view_factories_.clear();
  views_.clear();
  binders_.RemoveInterface<mojom::ViewFactory>();

  // Force-disconnect from the Service Mangager. Under normal circumstances
  // (i.e. in non-test code), the call below destroys |this|.
  context()->QuitNow();
}

void Service::AddViewFactory(std::unique_ptr<ViewFactoryImpl> factory) {
  auto* raw_factory = factory.get();
  view_factories_.emplace(raw_factory, std::move(factory));
}

void Service::RemoveViewFactory(ViewFactoryImpl* factory) {
  view_factories_.erase(factory);
}

void Service::AddView(std::unique_ptr<ViewImpl> view) {
  auto* raw_view = view.get();
  views_.emplace(raw_view, std::move(view));
}

void Service::RemoveView(ViewImpl* view) {
  views_.erase(view);
}

void Service::OnBindInterface(const service_manager::BindSourceInfo& source,
                              const std::string& interface_name,
                              mojo::ScopedMessagePipeHandle pipe) {
  binders_.BindInterface(interface_name, std::move(pipe));
}

}  // namespace content
