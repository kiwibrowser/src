// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/accelerator.h"

namespace ui {
namespace ws {

Accelerator::Accelerator(uint32_t id, const mojom::EventMatcher& matcher)
    : id_(id),
      accelerator_phase_(matcher.accelerator_phase),
      event_matcher_(matcher),
      weak_factory_(this) {}

Accelerator::~Accelerator() {}

bool Accelerator::MatchesEvent(const ui::Event& event,
                               const ui::mojom::AcceleratorPhase phase) const {
  if (accelerator_phase_ != phase)
    return false;
  if (!event_matcher_.MatchesEvent(event))
    return false;
  return true;
}

bool Accelerator::EqualEventMatcher(const Accelerator* other) const {
  return accelerator_phase_ == other->accelerator_phase_ &&
         event_matcher_.Equals(other->event_matcher_);
}

}  // namespace ws
}  // namespace ui
