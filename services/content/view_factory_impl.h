// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_CONTENT_VIEW_FACTORY_IMPL_H_
#define SERVICES_CONTENT_VIEW_FACTORY_IMPL_H_

#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/content/public/mojom/view_factory.mojom.h"

namespace content {

class Service;

class ViewFactoryImpl : public mojom::ViewFactory {
 public:
  ViewFactoryImpl(Service* service, mojom::ViewFactoryRequest request);
  ~ViewFactoryImpl() override;

 private:
  // mojom::ViewFactory:
  void CreateView(mojom::ViewParamsPtr params,
                  mojom::ViewRequest request,
                  mojom::ViewClientPtr client) override;

  Service* const service_;
  mojo::Binding<mojom::ViewFactory> binding_;

  DISALLOW_COPY_AND_ASSIGN(ViewFactoryImpl);
};

}  // namespace content

#endif  // SERVICES_CONTENT_VIEW_FACTORY_IMPL_H_
