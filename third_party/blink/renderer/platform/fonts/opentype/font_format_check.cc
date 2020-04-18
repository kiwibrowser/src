// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/fonts/opentype/font_format_check.h"

#include "SkTypeface.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

// Include HarfBuzz to have a cross-platform way to retrieve table tags without
// having to rely on the platform being able to instantiate this font format.
#include <hb.h>

namespace blink {

FontFormatCheck::FontFormatCheck(sk_sp<SkData> sk_data) {
  std::unique_ptr<hb_blob_t, std::function<void(hb_blob_t*)>> font_blob(
      hb_blob_create(reinterpret_cast<const char*>(sk_data->bytes()),
                     sk_data->size(), HB_MEMORY_MODE_READONLY, nullptr,
                     nullptr),
      [](hb_blob_t* blob) { hb_blob_destroy(blob); });
  std::unique_ptr<hb_face_t, std::function<void(hb_face_t*)>> face(
      hb_face_create(font_blob.get(), 0),
      [](hb_face_t* face) { hb_face_destroy(face); });

  unsigned table_count = 0;
  table_count = hb_face_get_table_tags(face.get(), 0, nullptr, nullptr);
  table_tags_.resize(table_count);
  if (!hb_face_get_table_tags(face.get(), 0, &table_count, table_tags_.data()))
    table_tags_.resize(0);
}

bool FontFormatCheck::IsVariableFont() {
  return table_tags_.size() && table_tags_.Contains(HB_TAG('f', 'v', 'a', 'r'));
}

bool FontFormatCheck::IsCbdtCblcColorFont() {
  return table_tags_.size() &&
         table_tags_.Contains(HB_TAG('C', 'B', 'D', 'T')) &&
         table_tags_.Contains(HB_TAG('C', 'B', 'L', 'C'));
}

bool FontFormatCheck::IsSbixColorFont() {
  return table_tags_.size() && table_tags_.Contains(HB_TAG('s', 'b', 'i', 'x'));
}

bool FontFormatCheck::IsCff2OutlineFont() {
  return table_tags_.size() && table_tags_.Contains(HB_TAG('C', 'F', 'F', '2'));
}

FontFormatCheck::VariableFontSubType FontFormatCheck::ProbeVariableFont(
    sk_sp<SkTypeface> typeface) {
  if (!typeface->getTableSize(
          SkFontTableTag(SkSetFourByteTag('f', 'v', 'a', 'r'))))
    return VariableFontSubType::kNotVariable;

  if (typeface->getTableSize(
          SkFontTableTag(SkSetFourByteTag('C', 'F', 'F', '2'))))
    return VariableFontSubType::kVariableCFF2;
  return VariableFontSubType::kVariableTrueType;
}

}  // namespace blink
