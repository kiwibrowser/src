// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/test/mock_widget_impl.h"

namespace content {

MockWidgetImpl::MockWidgetImpl(mojo::InterfaceRequest<mojom::Widget> request)
    : binding_(this, std::move(request)) {}

MockWidgetImpl::~MockWidgetImpl() {}

void MockWidgetImpl::SetupWidgetInputHandler(
    mojom::WidgetInputHandlerRequest request,
    mojom::WidgetInputHandlerHostPtr host) {
  input_handler_ = std::make_unique<MockWidgetInputHandler>(std::move(request),
                                                            std::move(host));
}

}  // namespace content
