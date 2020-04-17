// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_CONTENT_VIEW_IMPL_H_
#define SERVICES_CONTENT_VIEW_IMPL_H_

#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/content/public/mojom/view.mojom.h"
#include "services/content/public/mojom/view_factory.mojom.h"

namespace content {

class Service;
class ViewDelegate;

class ViewImpl : public mojom::View {
 public:
  ViewImpl(Service* service,
           mojom::ViewParamsPtr params,
           mojom::ViewRequest request,
           mojom::ViewClientPtr client);
  ~ViewImpl() override;

 private:
  // mojom::ContentView:
  void Navigate(const GURL& url) override;

  Service* const service_;

  mojo::Binding<mojom::View> binding_;
  mojom::ViewClientPtr client_;

  std::unique_ptr<ViewDelegate> delegate_;

  DISALLOW_COPY_AND_ASSIGN(ViewImpl);
};

}  // namespace content

#endif  // SERVICES_CONTENT_VIEW_IMPL_H_
