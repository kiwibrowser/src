// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/platform_window/mojo/ime_type_converters.h"

#include <stdint.h>

#include "base/macros.h"

namespace mojo {

#define TEXT_INPUT_TYPE_ASSERT(NAME)                                    \
  static_assert(static_cast<int32_t>(ui::mojom::TextInputType::NAME) == \
                    static_cast<int32_t>(ui::TEXT_INPUT_TYPE_##NAME),   \
                "TEXT_INPUT_TYPE must match")
TEXT_INPUT_TYPE_ASSERT(NONE);
TEXT_INPUT_TYPE_ASSERT(TEXT);
TEXT_INPUT_TYPE_ASSERT(PASSWORD);
TEXT_INPUT_TYPE_ASSERT(SEARCH);
TEXT_INPUT_TYPE_ASSERT(EMAIL);
TEXT_INPUT_TYPE_ASSERT(NUMBER);
TEXT_INPUT_TYPE_ASSERT(TELEPHONE);
TEXT_INPUT_TYPE_ASSERT(URL);
TEXT_INPUT_TYPE_ASSERT(DATE);
TEXT_INPUT_TYPE_ASSERT(DATE_TIME);
TEXT_INPUT_TYPE_ASSERT(DATE_TIME_LOCAL);
TEXT_INPUT_TYPE_ASSERT(MONTH);
TEXT_INPUT_TYPE_ASSERT(TIME);
TEXT_INPUT_TYPE_ASSERT(WEEK);
TEXT_INPUT_TYPE_ASSERT(TEXT_AREA);
TEXT_INPUT_TYPE_ASSERT(CONTENT_EDITABLE);
TEXT_INPUT_TYPE_ASSERT(DATE_TIME_FIELD);
TEXT_INPUT_TYPE_ASSERT(MAX);

#define TEXT_INPUT_FLAG_ASSERT(NAME)                                    \
  static_assert(static_cast<int32_t>(ui::mojom::TextInputFlag::NAME) == \
                    static_cast<int32_t>(ui::TEXT_INPUT_FLAG_##NAME),   \
                "TEXT_INPUT_FLAG must match")
TEXT_INPUT_FLAG_ASSERT(NONE);
TEXT_INPUT_FLAG_ASSERT(AUTOCOMPLETE_ON);
TEXT_INPUT_FLAG_ASSERT(AUTOCOMPLETE_OFF);
TEXT_INPUT_FLAG_ASSERT(AUTOCORRECT_ON);
TEXT_INPUT_FLAG_ASSERT(AUTOCORRECT_OFF);
TEXT_INPUT_FLAG_ASSERT(SPELLCHECK_ON);
TEXT_INPUT_FLAG_ASSERT(SPELLCHECK_OFF);
TEXT_INPUT_FLAG_ASSERT(AUTOCAPITALIZE_NONE);
TEXT_INPUT_FLAG_ASSERT(AUTOCAPITALIZE_CHARACTERS);
TEXT_INPUT_FLAG_ASSERT(AUTOCAPITALIZE_WORDS);
TEXT_INPUT_FLAG_ASSERT(AUTOCAPITALIZE_SENTENCES);

// static
ui::mojom::TextInputType
TypeConverter<ui::mojom::TextInputType, ui::TextInputType>::Convert(
    const ui::TextInputType& input) {
  return static_cast<ui::mojom::TextInputType>(input);
}

// static
ui::TextInputType
TypeConverter<ui::TextInputType, ui::mojom::TextInputType>::Convert(
    const ui::mojom::TextInputType& input) {
  return static_cast<ui::TextInputType>(input);
}

// static
ui::TextInputState
TypeConverter<ui::TextInputState, ui::mojom::TextInputStatePtr>::Convert(
    const ui::mojom::TextInputStatePtr& input) {
  return ui::TextInputState(
      ConvertTo<ui::TextInputType>(input->type), input->flags,
      input->text.has_value() ? input->text.value() : std::string(),
      input->selection_start, input->selection_end, input->composition_start,
      input->composition_end, input->can_compose_inline);
}

}  // namespace mojo
