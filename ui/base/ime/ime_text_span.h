// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_IME_IME_TEXT_SPAN_H_
#define UI_BASE_IME_IME_TEXT_SPAN_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/ime/ui_base_ime_export.h"

namespace ui {

// Intentionally keep sync with blink::WebImeTextSpan defined in:
// third_party/WebKit/public/web/WebImeTextSpan.h

struct UI_BASE_IME_EXPORT ImeTextSpan {
  enum class Type {
    // Creates a composition marker.
    kComposition,
    // Creates a suggestion marker that isn't cleared after the user picks a
    // replacement.
    kSuggestion,
    // Creates a suggestion marker that is cleared after the user picks a
    // replacement, and will be ignored if added to an element with spell
    // checking disabled.
    kMisspellingSuggestion,
  };

  enum class Thickness {
    kNone,
    kThin,
    kThick,
  };

  // The default constructor is used by generated Mojo code.
  ImeTextSpan();
  // TODO(huangs): remove this constructor.
  ImeTextSpan(uint32_t start_offset,
              uint32_t end_offset,
              Thickness thickness);
  ImeTextSpan(
      Type type,
      uint32_t start_offset,
      uint32_t end_offset,
      Thickness thickness,
      SkColor background_color,
      SkColor suggestion_highlight_color = SK_ColorTRANSPARENT,
      const std::vector<std::string>& suggestions = std::vector<std::string>());

  ImeTextSpan(const ImeTextSpan& rhs);

  ~ImeTextSpan();

  bool operator==(const ImeTextSpan& rhs) const {
    return (this->type == rhs.type) &&
           (this->start_offset == rhs.start_offset) &&
           (this->end_offset == rhs.end_offset) &&
           (this->underline_color == rhs.underline_color) &&
           (this->thickness == rhs.thickness) &&
           (this->background_color == rhs.background_color) &&
           (this->suggestion_highlight_color ==
            rhs.suggestion_highlight_color) &&
           (this->suggestions == rhs.suggestions);
  }

  bool operator!=(const ImeTextSpan& rhs) const { return !(*this == rhs); }

  Type type;
  uint32_t start_offset;
  uint32_t end_offset;
  SkColor underline_color = SK_ColorTRANSPARENT;
  Thickness thickness;
  SkColor background_color;
  SkColor suggestion_highlight_color;
  std::vector<std::string> suggestions;
};

typedef std::vector<ImeTextSpan> ImeTextSpans;

}  // namespace ui

#endif  // UI_BASE_IME_IME_TEXT_SPAN_H_
