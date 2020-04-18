// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_COMMON_ACCELERATOR_UTIL_H_
#define SERVICES_UI_COMMON_ACCELERATOR_UTIL_H_

#include "services/ui/public/interfaces/event_matcher.mojom.h"
#include "services/ui/public/interfaces/window_manager.mojom.h"
#include "ui/events/mojo/event_constants.mojom.h"
#include "ui/events/mojo/keyboard_codes.mojom.h"

namespace ui {

// |flags| is a bitfield of kEventFlag* and kMouseEventFlag* values in
// input_event_constants.mojom.
mojom::EventMatcherPtr CreateKeyMatcher(ui::mojom::KeyboardCode code,
                                        int flags);

// Construct accelerator vector from the provided |id| and |event_matcher|
std::vector<ui::mojom::WmAcceleratorPtr> CreateAcceleratorVector(
    uint32_t id,
    ui::mojom::EventMatcherPtr event_matcher);

// Construct accelerator from the provided |id| and |event_matcher|
ui::mojom::WmAcceleratorPtr CreateAccelerator(
    uint32_t id,
    ui::mojom::EventMatcherPtr event_matcher);

}  // namespace ui

#endif  // SERVICES_UI_COMMON_ACCELERATOR_UTIL_H_
