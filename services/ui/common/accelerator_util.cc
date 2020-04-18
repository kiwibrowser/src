// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/common/accelerator_util.h"

namespace ui {

mojom::EventMatcherPtr CreateKeyMatcher(ui::mojom::KeyboardCode code,
                                        int flags) {
  mojom::EventMatcherPtr matcher(mojom::EventMatcher::New());
  matcher->type_matcher = mojom::EventTypeMatcher::New();
  matcher->flags_matcher = mojom::EventFlagsMatcher::New();
  matcher->ignore_flags_matcher = mojom::EventFlagsMatcher::New();
  // Ignoring these makes most accelerator scenarios more straight forward. Code
  // that needs to check them can override this setting.
  matcher->ignore_flags_matcher->flags = ui::mojom::kEventFlagCapsLockOn |
                                         ui::mojom::kEventFlagScrollLockOn |
                                         ui::mojom::kEventFlagNumLockOn;
  matcher->key_matcher = mojom::KeyEventMatcher::New();
  matcher->type_matcher->type = ui::mojom::EventType::KEY_PRESSED;
  matcher->flags_matcher->flags = flags;
  matcher->key_matcher->keyboard_code = code;
  return matcher;
}

std::vector<ui::mojom::WmAcceleratorPtr> CreateAcceleratorVector(
    uint32_t id,
    ui::mojom::EventMatcherPtr event_matcher) {
  std::vector<ui::mojom::WmAcceleratorPtr> accelerators;
  accelerators.push_back(CreateAccelerator(id, std::move(event_matcher)));
  return accelerators;
}

ui::mojom::WmAcceleratorPtr CreateAccelerator(
    uint32_t id,
    ui::mojom::EventMatcherPtr event_matcher) {
  ui::mojom::WmAcceleratorPtr accelerator_ptr = ui::mojom::WmAccelerator::New();
  accelerator_ptr->id = id;
  accelerator_ptr->event_matcher = std::move(event_matcher);
  return accelerator_ptr;
}

}  // namespace ui
