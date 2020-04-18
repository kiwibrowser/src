// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/test/mock_render_process.h"

#include "ui/gfx/geometry/rect.h"
#include "ui/surface/transport_dib.h"

namespace content {

MockRenderProcess::MockRenderProcess()
    : enabled_bindings_(0) {
}

MockRenderProcess::~MockRenderProcess() {
}

void MockRenderProcess::AddBindings(int bindings) {
  enabled_bindings_ |= bindings;
}

int MockRenderProcess::GetEnabledBindings() const {
  return enabled_bindings_;
}

}  // namespace content
