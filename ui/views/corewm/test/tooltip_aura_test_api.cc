// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/corewm/test/tooltip_aura_test_api.h"

#include "ui/gfx/render_text.h"
#include "ui/views/corewm/tooltip_aura.h"

namespace views {
namespace corewm {
namespace test {

gfx::RenderText* TooltipAuraTestApi::GetRenderText() {
  return tooltip_aura_->GetRenderTextForTest();
}

}  // namespace test
}  // namespace corewm
}  // namespace views
