// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_ACCELERATOR_H_
#define SERVICES_UI_WS_ACCELERATOR_H_

#include <stdint.h>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "services/ui/public/interfaces/event_matcher.mojom.h"
#include "services/ui/ws/event_matcher.h"

namespace ui {
class Event;
}

namespace ui {
namespace ws {

// An Accelerator encompasses an id defined by the client, along with a unique
// mojom::EventMatcher. See WindowManagerClient.
//
// This provides a WeakPtr, as the client might delete the accelerator between
// an event having been matched and the dispatch of the accelerator to the
// client.
class Accelerator {
 public:
  Accelerator(uint32_t id, const mojom::EventMatcher& matcher);
  ~Accelerator();

  // Returns true if |event| and |phase | matches the definition in the
  // mojom::EventMatcher used for initialization.
  bool MatchesEvent(const ui::Event& event,
                    const ui::mojom::AcceleratorPhase phase) const;

  // Returns true if |other| was created with an identical mojom::EventMatcher.
  bool EqualEventMatcher(const Accelerator* other) const;

  base::WeakPtr<Accelerator> GetWeakPtr() { return weak_factory_.GetWeakPtr(); }

  uint32_t id() const { return id_; }

 private:
  uint32_t id_;
  ui::mojom::AcceleratorPhase accelerator_phase_;
  EventMatcher event_matcher_;
  base::WeakPtrFactory<Accelerator> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(Accelerator);
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_ACCELERATOR_H_
