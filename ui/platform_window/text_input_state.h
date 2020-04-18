// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_PLATFORM_WINDOW_TEXT_INPUT_STATE_H_
#define UI_PLATFORM_WINDOW_TEXT_INPUT_STATE_H_

#include <string>

#include "ui/base/ime/text_input_flags.h"
#include "ui/base/ime/text_input_type.h"

namespace ui {

// Text input info which is based on blink::WebTextInputInfo.
struct TextInputState {
  TextInputState();
  TextInputState(TextInputType type,
                 int flags,
                 const std::string& text,
                 int selection_start,
                 int selection_end,
                 int composition_start,
                 int composition_end,
                 bool can_compose_inline);
  TextInputState(const TextInputState& other);
  bool operator==(const TextInputState& other) const;

  // The type of input field.
  TextInputType type;

  // The flags of the input field (autocorrect, autocomplete, etc.).
  int flags;

  // The value of the input field.
  std::string text;

  // The cursor position of the current selection start, or the caret position
  // if nothing is selected.
  int selection_start;

  // The cursor position of the current selection end, or the caret position
  // if nothing is selected.
  int selection_end;

  // The start position of the current composition, or -1 if there is none.
  int composition_start;

  // The end position of the current composition, or -1 if there is none.
  int composition_end;

  // Whether or not inline composition can be performed for the current input.
  bool can_compose_inline;
};

}  // namespace ui

#endif  // UI_PLATFORM_WINDOW_TEXT_INPUT_STATE_H_
