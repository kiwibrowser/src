// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/public/interfaces/ime/ime_struct_traits.h"

#include "mojo/public/cpp/base/string16_mojom_traits.h"
#include "ui/gfx/range/mojo/range_struct_traits.h"

namespace mojo {

// static
bool StructTraits<ui::mojom::CandidateWindowPropertiesDataView,
                  ui::CandidateWindow::CandidateWindowProperty>::
    Read(ui::mojom::CandidateWindowPropertiesDataView data,
         ui::CandidateWindow::CandidateWindowProperty* out) {
  if (data.is_null())
    return false;
  if (!data.ReadAuxiliaryText(&out->auxiliary_text))
    return false;
  out->page_size = data.page_size();
  out->is_vertical = data.vertical();
  out->is_auxiliary_text_visible = data.auxiliary_text_visible();
  out->cursor_position = data.cursor_position();
  out->is_cursor_visible = data.cursor_visible();
  out->show_window_at_composition =
      data.window_position() ==
      ui::mojom::CandidateWindowPosition::kComposition;
  return true;
}

// static
bool StructTraits<ui::mojom::CandidateWindowEntryDataView,
                  ui::CandidateWindow::Entry>::
    Read(ui::mojom::CandidateWindowEntryDataView data,
         ui::CandidateWindow::Entry* out) {
  return !data.is_null() && data.ReadValue(&out->value) &&
         data.ReadLabel(&out->label) && data.ReadAnnotation(&out->annotation) &&
         data.ReadDescriptionTitle(&out->description_title) &&
         data.ReadDescriptionBody(&out->description_body);
}

// static
bool StructTraits<ui::mojom::ImeTextSpanDataView, ui::ImeTextSpan>::Read(
    ui::mojom::ImeTextSpanDataView data,
    ui::ImeTextSpan* out) {
  if (data.is_null())
    return false;
  if (!data.ReadType(&out->type))
    return false;
  out->start_offset = data.start_offset();
  out->end_offset = data.end_offset();
  out->underline_color = data.underline_color();
  if (!data.ReadThickness(&out->thickness))
    return false;
  out->background_color = data.background_color();
  out->suggestion_highlight_color = data.suggestion_highlight_color();
  if (!data.ReadSuggestions(&out->suggestions))
    return false;
  return true;
}

// static
bool StructTraits<ui::mojom::CompositionTextDataView, ui::CompositionText>::
    Read(ui::mojom::CompositionTextDataView data, ui::CompositionText* out) {
  return !data.is_null() && data.ReadText(&out->text) &&
         data.ReadImeTextSpans(&out->ime_text_spans) &&
         data.ReadSelection(&out->selection);
}

// static
ui::mojom::ImeTextSpanType
EnumTraits<ui::mojom::ImeTextSpanType, ui::ImeTextSpan::Type>::ToMojom(
    ui::ImeTextSpan::Type ime_text_span_type) {
  switch (ime_text_span_type) {
    case ui::ImeTextSpan::Type::kComposition:
      return ui::mojom::ImeTextSpanType::kComposition;
    case ui::ImeTextSpan::Type::kSuggestion:
      return ui::mojom::ImeTextSpanType::kSuggestion;
    case ui::ImeTextSpan::Type::kMisspellingSuggestion:
      return ui::mojom::ImeTextSpanType::kMisspellingSuggestion;
  }

  NOTREACHED();
  return ui::mojom::ImeTextSpanType::kComposition;
}

// static
bool EnumTraits<ui::mojom::ImeTextSpanType, ui::ImeTextSpan::Type>::FromMojom(
    ui::mojom::ImeTextSpanType type,
    ui::ImeTextSpan::Type* out) {
  switch (type) {
    case ui::mojom::ImeTextSpanType::kComposition:
      *out = ui::ImeTextSpan::Type::kComposition;
      return true;
    case ui::mojom::ImeTextSpanType::kSuggestion:
      *out = ui::ImeTextSpan::Type::kSuggestion;
      return true;
    case ui::mojom::ImeTextSpanType::kMisspellingSuggestion:
      *out = ui::ImeTextSpan::Type::kMisspellingSuggestion;
      return true;
  }

  NOTREACHED();
  return false;
}

// static
ui::mojom::ImeTextSpanThickness EnumTraits<
    ui::mojom::ImeTextSpanThickness,
    ui::ImeTextSpan::Thickness>::ToMojom(ui::ImeTextSpan::Thickness thickness) {
  switch (thickness) {
    case ui::ImeTextSpan::Thickness::kNone:
      return ui::mojom::ImeTextSpanThickness::kNone;
    case ui::ImeTextSpan::Thickness::kThin:
      return ui::mojom::ImeTextSpanThickness::kThin;
    case ui::ImeTextSpan::Thickness::kThick:
      return ui::mojom::ImeTextSpanThickness::kThick;
  }

  NOTREACHED();
  return ui::mojom::ImeTextSpanThickness::kThin;
}

// static
bool EnumTraits<ui::mojom::ImeTextSpanThickness, ui::ImeTextSpan::Thickness>::
    FromMojom(ui::mojom::ImeTextSpanThickness input,
              ui::ImeTextSpan::Thickness* out) {
  switch (input) {
    case ui::mojom::ImeTextSpanThickness::kNone:
      *out = ui::ImeTextSpan::Thickness::kNone;
      return true;
    case ui::mojom::ImeTextSpanThickness::kThin:
      *out = ui::ImeTextSpan::Thickness::kThin;
      return true;
    case ui::mojom::ImeTextSpanThickness::kThick:
      *out = ui::ImeTextSpan::Thickness::kThick;
      return true;
  }

  NOTREACHED();
  return false;
}

// static
ui::mojom::TextInputMode
EnumTraits<ui::mojom::TextInputMode, ui::TextInputMode>::ToMojom(
    ui::TextInputMode text_input_mode) {
  switch (text_input_mode) {
    case ui::TEXT_INPUT_MODE_DEFAULT:
      return ui::mojom::TextInputMode::kDefault;
    case ui::TEXT_INPUT_MODE_NONE:
      return ui::mojom::TextInputMode::kNone;
    case ui::TEXT_INPUT_MODE_TEXT:
      return ui::mojom::TextInputMode::kText;
    case ui::TEXT_INPUT_MODE_TEL:
      return ui::mojom::TextInputMode::kTel;
    case ui::TEXT_INPUT_MODE_URL:
      return ui::mojom::TextInputMode::kUrl;
    case ui::TEXT_INPUT_MODE_EMAIL:
      return ui::mojom::TextInputMode::kEmail;
    case ui::TEXT_INPUT_MODE_NUMERIC:
      return ui::mojom::TextInputMode::kNumeric;
    case ui::TEXT_INPUT_MODE_DECIMAL:
      return ui::mojom::TextInputMode::kDecimal;
    case ui::TEXT_INPUT_MODE_SEARCH:
      return ui::mojom::TextInputMode::kSearch;
  }
  NOTREACHED();
  return ui::mojom::TextInputMode::kDefault;
}

// static
bool EnumTraits<ui::mojom::TextInputMode, ui::TextInputMode>::FromMojom(
    ui::mojom::TextInputMode input,
    ui::TextInputMode* out) {
  switch (input) {
    case ui::mojom::TextInputMode::kDefault:
      *out = ui::TEXT_INPUT_MODE_DEFAULT;
      return true;
    case ui::mojom::TextInputMode::kNone:
      *out = ui::TEXT_INPUT_MODE_NONE;
      return true;
    case ui::mojom::TextInputMode::kText:
      *out = ui::TEXT_INPUT_MODE_TEXT;
      return true;
    case ui::mojom::TextInputMode::kTel:
      *out = ui::TEXT_INPUT_MODE_TEL;
      return true;
    case ui::mojom::TextInputMode::kUrl:
      *out = ui::TEXT_INPUT_MODE_URL;
      return true;
    case ui::mojom::TextInputMode::kEmail:
      *out = ui::TEXT_INPUT_MODE_EMAIL;
      return true;
    case ui::mojom::TextInputMode::kNumeric:
      *out = ui::TEXT_INPUT_MODE_NUMERIC;
      return true;
    case ui::mojom::TextInputMode::kDecimal:
      *out = ui::TEXT_INPUT_MODE_DECIMAL;
      return true;
    case ui::mojom::TextInputMode::kSearch:
      *out = ui::TEXT_INPUT_MODE_SEARCH;
      return true;
  }
  return false;
}

#define UI_TO_MOJO_TYPE_CASE(name) \
  case ui::TEXT_INPUT_TYPE_##name: \
    return ui::mojom::TextInputType::name

// static
ui::mojom::TextInputType
EnumTraits<ui::mojom::TextInputType, ui::TextInputType>::ToMojom(
    ui::TextInputType text_input_type) {
  switch (text_input_type) {
    UI_TO_MOJO_TYPE_CASE(NONE);
    UI_TO_MOJO_TYPE_CASE(TEXT);
    UI_TO_MOJO_TYPE_CASE(PASSWORD);
    UI_TO_MOJO_TYPE_CASE(SEARCH);
    UI_TO_MOJO_TYPE_CASE(EMAIL);
    UI_TO_MOJO_TYPE_CASE(NUMBER);
    UI_TO_MOJO_TYPE_CASE(TELEPHONE);
    UI_TO_MOJO_TYPE_CASE(URL);
    UI_TO_MOJO_TYPE_CASE(DATE);
    UI_TO_MOJO_TYPE_CASE(DATE_TIME);
    UI_TO_MOJO_TYPE_CASE(DATE_TIME_LOCAL);
    UI_TO_MOJO_TYPE_CASE(MONTH);
    UI_TO_MOJO_TYPE_CASE(TIME);
    UI_TO_MOJO_TYPE_CASE(WEEK);
    UI_TO_MOJO_TYPE_CASE(TEXT_AREA);
    UI_TO_MOJO_TYPE_CASE(CONTENT_EDITABLE);
    UI_TO_MOJO_TYPE_CASE(DATE_TIME_FIELD);
  }
  NOTREACHED();
  return ui::mojom::TextInputType::NONE;
}

#undef UI_TO_MOJO_TYPE_CASE

#define MOJO_TO_UI_TYPE_CASE(name)     \
  case ui::mojom::TextInputType::name: \
    *out = ui::TEXT_INPUT_TYPE_##name; \
    return true;

// static
bool EnumTraits<ui::mojom::TextInputType, ui::TextInputType>::FromMojom(
    ui::mojom::TextInputType input,
    ui::TextInputType* out) {
  switch (input) {
    MOJO_TO_UI_TYPE_CASE(NONE);
    MOJO_TO_UI_TYPE_CASE(TEXT);
    MOJO_TO_UI_TYPE_CASE(PASSWORD);
    MOJO_TO_UI_TYPE_CASE(SEARCH);
    MOJO_TO_UI_TYPE_CASE(EMAIL);
    MOJO_TO_UI_TYPE_CASE(NUMBER);
    MOJO_TO_UI_TYPE_CASE(TELEPHONE);
    MOJO_TO_UI_TYPE_CASE(URL);
    MOJO_TO_UI_TYPE_CASE(DATE);
    MOJO_TO_UI_TYPE_CASE(DATE_TIME);
    MOJO_TO_UI_TYPE_CASE(DATE_TIME_LOCAL);
    MOJO_TO_UI_TYPE_CASE(MONTH);
    MOJO_TO_UI_TYPE_CASE(TIME);
    MOJO_TO_UI_TYPE_CASE(WEEK);
    MOJO_TO_UI_TYPE_CASE(TEXT_AREA);
    MOJO_TO_UI_TYPE_CASE(CONTENT_EDITABLE);
    MOJO_TO_UI_TYPE_CASE(DATE_TIME_FIELD);
  }
#undef MOJO_TO_UI_TYPE_CASE
  return false;
}

}  // namespace mojo
