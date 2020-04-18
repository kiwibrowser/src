// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/content_accelerators/accelerator_util.h"

#include "build/build_config.h"
#include "third_party/blink/public/platform/web_input_event.h"
#include "ui/events/blink/blink_event_util.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"

namespace ui {

ui::Accelerator GetAcceleratorFromNativeWebKeyboardEvent(
    const content::NativeWebKeyboardEvent& event) {
  ui::Accelerator accelerator(
      static_cast<ui::KeyboardCode>(event.windows_key_code),
      WebEventModifiersToEventFlags(event.GetModifiers()));
  if (event.GetType() == blink::WebInputEvent::kKeyUp)
    accelerator.set_key_state(Accelerator::KeyState::RELEASED);
  return accelerator;
}

}  // namespace ui
