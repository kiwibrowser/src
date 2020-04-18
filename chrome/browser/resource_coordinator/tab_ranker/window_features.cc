// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/tab_ranker/window_features.h"

namespace tab_ranker {

WindowFeatures::WindowFeatures(SessionID window_id,
                               metrics::WindowMetricsEvent::Type type)
    : window_id(window_id), type(type) {}

WindowFeatures::WindowFeatures(const WindowFeatures& other) = default;

WindowFeatures::~WindowFeatures() = default;

bool WindowFeatures::operator==(const WindowFeatures& other) const {
  return window_id == other.window_id && type == other.type &&
         show_state == other.show_state && is_active == other.is_active &&
         tab_count == other.tab_count;
}

bool WindowFeatures::operator!=(const WindowFeatures& other) const {
  return !operator==(other);
}

}  // namespace tab_ranker
