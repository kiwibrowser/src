// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/content/view_factory_impl.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "services/content/service.h"
#include "services/content/view_impl.h"

namespace content {

ViewFactoryImpl::ViewFactoryImpl(Service* service,
                                 mojom::ViewFactoryRequest request)
    : service_(service), binding_(this, std::move(request)) {
  binding_.set_connection_error_handler(base::BindOnce(
      &Service::RemoveViewFactory, base::Unretained(service_), this));
}

ViewFactoryImpl::~ViewFactoryImpl() = default;

void ViewFactoryImpl::CreateView(mojom::ViewParamsPtr params,
                                 mojom::ViewRequest request,
                                 mojom::ViewClientPtr client) {
  service_->AddView(std::make_unique<ViewImpl>(
      service_, std::move(params), std::move(request), std::move(client)));
}

}  // namespace content
