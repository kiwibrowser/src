// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_TEST_MOCK_WIDGET_IMPL_H_
#define CONTENT_TEST_MOCK_WIDGET_IMPL_H_

#include "content/common/widget.mojom.h"
#include "content/test/mock_widget_input_handler.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_request.h"

namespace content {

class MockWidgetImpl : public mojom::Widget {
 public:
  explicit MockWidgetImpl(mojo::InterfaceRequest<mojom::Widget> request);
  ~MockWidgetImpl() override;

  void SetupWidgetInputHandler(mojom::WidgetInputHandlerRequest request,
                               mojom::WidgetInputHandlerHostPtr host) override;

  MockWidgetInputHandler* input_handler() { return input_handler_.get(); }

 private:
  mojo::Binding<mojom::Widget> binding_;
  std::unique_ptr<MockWidgetInputHandler> input_handler_;

  DISALLOW_COPY_AND_ASSIGN(MockWidgetImpl);
};

}  // namespace content

#endif  // CONTENT_TEST_MOCK_WIDGET_IMPL_H_
