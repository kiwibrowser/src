// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "cc/input/layer_selection_bound.h"

namespace cc {

LayerSelectionBound::LayerSelectionBound()
    : type(gfx::SelectionBound::EMPTY), layer_id(0), hidden(false) {}

LayerSelectionBound::~LayerSelectionBound() = default;

bool LayerSelectionBound::operator==(const LayerSelectionBound& other) const {
  return type == other.type && layer_id == other.layer_id &&
         edge_top == other.edge_top && edge_bottom == other.edge_bottom &&
         hidden == other.hidden;
}

bool LayerSelectionBound::operator!=(const LayerSelectionBound& other) const {
  return !(*this == other);
}

}  // namespace cc
