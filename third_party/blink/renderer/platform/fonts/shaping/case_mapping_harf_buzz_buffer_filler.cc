// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/fonts/shaping/case_mapping_harf_buzz_buffer_filler.h"

#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

static const uint16_t* ToUint16(const UChar* src) {
  // FIXME: This relies on undefined behavior however it works on the
  // current versions of all compilers we care about and avoids making
  // a copy of the string.
  static_assert(sizeof(UChar) == sizeof(uint16_t),
                "UChar should be the same size as uint16_t");
  return reinterpret_cast<const uint16_t*>(src);
}

CaseMappingHarfBuzzBufferFiller::CaseMappingHarfBuzzBufferFiller(
    CaseMapIntend case_map_intend,
    AtomicString locale,
    hb_buffer_t* harf_buzz_buffer,
    const UChar* buffer,
    unsigned buffer_length,
    unsigned start_index,
    unsigned num_characters)
    : harf_buzz_buffer_(harf_buzz_buffer) {
  if (case_map_intend == CaseMapIntend::kKeepSameCase) {
    hb_buffer_add_utf16(harf_buzz_buffer_, ToUint16(buffer), buffer_length,
                        start_index, num_characters);
  } else {
    String case_mapped_text;
    if (case_map_intend == CaseMapIntend::kUpperCase) {
      case_mapped_text = String(buffer, buffer_length).UpperUnicode(locale);
    } else {
      case_mapped_text = String(buffer, buffer_length).LowerUnicode(locale);
    }

    if (case_mapped_text.length() != buffer_length) {
      FillSlowCase(case_map_intend, locale, buffer, buffer_length, start_index,
                   num_characters);
      return;
    }

    DCHECK_EQ(case_mapped_text.length(), buffer_length);
    DCHECK(!case_mapped_text.Is8Bit());
    hb_buffer_add_utf16(harf_buzz_buffer_,
                        ToUint16(case_mapped_text.Characters16()),
                        buffer_length, start_index, num_characters);
  }
}

// TODO(drott): crbug.com/623940 Fix lack of context sensitive case mapping
// here.
void CaseMappingHarfBuzzBufferFiller::FillSlowCase(
    CaseMapIntend case_map_intend,
    AtomicString locale,
    const UChar* buffer,
    unsigned buffer_length,
    unsigned start_index,
    unsigned num_characters) {
  // Record pre-context.
  hb_buffer_add_utf16(harf_buzz_buffer_, ToUint16(buffer), buffer_length,
                      start_index, 0);

  for (unsigned char_index = start_index;
       char_index < start_index + num_characters;) {
    unsigned new_char_index = char_index;
    U16_FWD_1(buffer, new_char_index, num_characters);
    String char_by_char(&buffer[char_index], new_char_index - char_index);
    String case_mapped_char;
    if (case_map_intend == CaseMapIntend::kUpperCase)
      case_mapped_char = char_by_char.UpperUnicode(locale);
    else
      case_mapped_char = char_by_char.LowerUnicode(locale);

    for (unsigned j = 0; j < case_mapped_char.length();) {
      UChar32 codepoint = 0;
      U16_NEXT(case_mapped_char.Characters16(), j, case_mapped_char.length(),
               codepoint);
      // Add all characters of the case mapping result at the same cluster
      // position.
      hb_buffer_add(harf_buzz_buffer_, codepoint, char_index);
    }
    char_index = new_char_index;
  }

  // Record post-context
  hb_buffer_add_utf16(harf_buzz_buffer_, ToUint16(buffer), buffer_length,
                      start_index + num_characters, 0);
}

}  // namespace blink
