// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_EMBEDDED_OBJECT_PAINT_INVALIDATOR_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_EMBEDDED_OBJECT_PAINT_INVALIDATOR_H_

#include "third_party/blink/renderer/platform/graphics/paint_invalidation_reason.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class LayoutEmbeddedObject;
struct PaintInvalidatorContext;

class EmbeddedObjectPaintInvalidator {
  STACK_ALLOCATED();

 public:
  EmbeddedObjectPaintInvalidator(const LayoutEmbeddedObject& embedded_object,
                                 const PaintInvalidatorContext& context)
      : embedded_object_(embedded_object), context_(context) {}

  PaintInvalidationReason InvalidatePaint();

 private:
  const LayoutEmbeddedObject& embedded_object_;
  const PaintInvalidatorContext& context_;
};

}  // namespace blink

#endif
