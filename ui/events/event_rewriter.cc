// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/event_rewriter.h"

#include "ui/events/event_source.h"

namespace ui {

EventDispatchDetails EventRewriter::SendEventToEventSource(EventSource* source,
                                                           Event* event) const {
  return source->SendEventToSinkFromRewriter(event, this);
}

}  // namespace ui
