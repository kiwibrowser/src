// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/wm/core/accelerator_filter.h"

#include <utility>

#include "build/build_config.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/accelerators/accelerator_history.h"
#include "ui/events/event.h"
#include "ui/wm/core/accelerator_delegate.h"

namespace wm {

////////////////////////////////////////////////////////////////////////////////
// AcceleratorFilter, public:

AcceleratorFilter::AcceleratorFilter(
    std::unique_ptr<AcceleratorDelegate> delegate,
    ui::AcceleratorHistory* accelerator_history)
    : delegate_(std::move(delegate)),
      accelerator_history_(accelerator_history) {
  DCHECK(accelerator_history);
}

AcceleratorFilter::~AcceleratorFilter() {
}

////////////////////////////////////////////////////////////////////////////////
// AcceleratorFilter, EventFilter implementation:

void AcceleratorFilter::OnKeyEvent(ui::KeyEvent* event) {
  const ui::EventType type = event->type();
  DCHECK(event->target());
  if ((type != ui::ET_KEY_PRESSED && type != ui::ET_KEY_RELEASED) ||
      event->is_char() || !event->target()) {
    return;
  }

  ui::Accelerator accelerator(*event);
  accelerator_history_->StoreCurrentAccelerator(accelerator);

  if (delegate_->ProcessAccelerator(*event, accelerator))
    event->StopPropagation();
}

void AcceleratorFilter::OnMouseEvent(ui::MouseEvent* event) {
  if (event->type() == ui::ET_MOUSE_PRESSED ||
      event->type() == ui::ET_MOUSE_RELEASED) {
    accelerator_history_->InterruptCurrentAccelerator();
  }
}

}  // namespace wm
