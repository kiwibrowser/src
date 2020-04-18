// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/csspaint/document_paint_definition.h"

namespace blink {

DocumentPaintDefinition::DocumentPaintDefinition(CSSPaintDefinition* definition)
    : paint_definition_(definition), registered_definitions_count_(1u) {
  DCHECK(definition);
}

DocumentPaintDefinition::~DocumentPaintDefinition() = default;

bool DocumentPaintDefinition::RegisterAdditionalPaintDefinition(
    const CSSPaintDefinition& other) {
  if (GetPaintRenderingContext2DSettings().alpha() !=
          other.GetPaintRenderingContext2DSettings().alpha() ||
      NativeInvalidationProperties() != other.NativeInvalidationProperties() ||
      CustomInvalidationProperties() != other.CustomInvalidationProperties() ||
      InputArgumentTypes() != other.InputArgumentTypes())
    return false;
  registered_definitions_count_++;
  return true;
}

void DocumentPaintDefinition::Trace(blink::Visitor* visitor) {
  visitor->Trace(paint_definition_);
}

}  // namespace blink
