// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/test/mock_display_client.h"

namespace viz {

MockDisplayClient::MockDisplayClient() : binding_(this) {}

MockDisplayClient::~MockDisplayClient() = default;

mojom::DisplayClientPtr MockDisplayClient::BindInterfacePtr() {
  mojom::DisplayClientPtr ptr;
  binding_.Bind(MakeRequest(&ptr));
  return ptr;
}

}  // namespace viz
