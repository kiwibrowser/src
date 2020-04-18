// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/platform_window/text_input_state.h"

namespace ui {

TextInputState::TextInputState()
    : type(TEXT_INPUT_TYPE_NONE),
      flags(TEXT_INPUT_FLAG_NONE),
      selection_start(0),
      selection_end(0),
      composition_start(0),
      composition_end(0),
      can_compose_inline(false) {}

TextInputState::TextInputState(TextInputType type,
                               int flags,
                               const std::string& text,
                               int selection_start,
                               int selection_end,
                               int composition_start,
                               int composition_end,
                               bool can_compose_inline)
    : type(type),
      flags(flags),
      text(text),
      selection_start(selection_start),
      selection_end(selection_end),
      composition_start(composition_start),
      composition_end(composition_end),
      can_compose_inline(can_compose_inline) {}

TextInputState::TextInputState(const TextInputState& other) = default;

bool TextInputState::operator==(const TextInputState& other) const {
  return type == other.type &&
         flags == other.flags &&
         text == other.text &&
         selection_start == other.selection_start &&
         selection_end == other.selection_end &&
         composition_start == other.composition_start &&
         composition_end == other.composition_end &&
         can_compose_inline == other.can_compose_inline;
}

}  // namespace ui
