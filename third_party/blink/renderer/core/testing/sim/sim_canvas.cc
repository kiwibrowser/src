// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/testing/sim/sim_canvas.h"

#include "third_party/blink/renderer/platform/graphics/color.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/core/SkRRect.h"
#include "third_party/skia/include/core/SkRect.h"

namespace blink {

size_t SimCanvas::Commands::DrawCount(CommandType type,
                                      const String& color_string) const {
  Color color;
  if (!color_string.IsNull())
    CHECK(color.SetFromString(color_string));

  size_t count = 0;
  for (auto& command : commands_) {
    if (command.type == type &&
        (color_string.IsNull() || command.color == color.Rgb())) {
      count++;
    }
  }
  return count;
}

static int g_depth = 0;

class DrawScope {
 public:
  DrawScope() { ++g_depth; }
  ~DrawScope() { --g_depth; }
};

SimCanvas::SimCanvas(int width, int height) : SkCanvas(width, height) {}

void SimCanvas::AddCommand(CommandType type, RGBA32 color) {
  if (g_depth > 1)
    return;
  commands_.commands_.push_back(Commands::Command{type, color});
}

void SimCanvas::onDrawRect(const SkRect& rect, const SkPaint& paint) {
  DrawScope scope;
  AddCommand(CommandType::kRect, paint.getColor());
  SkCanvas::onDrawRect(rect, paint);
}

void SimCanvas::onDrawOval(const SkRect& oval, const SkPaint& paint) {
  DrawScope scope;
  AddCommand(CommandType::kShape, paint.getColor());
  SkCanvas::onDrawOval(oval, paint);
}

void SimCanvas::onDrawRRect(const SkRRect& rrect, const SkPaint& paint) {
  DrawScope scope;
  AddCommand(CommandType::kShape, paint.getColor());
  SkCanvas::onDrawRRect(rrect, paint);
}

void SimCanvas::onDrawPath(const SkPath& path, const SkPaint& paint) {
  DrawScope scope;
  AddCommand(CommandType::kShape, paint.getColor());
  SkCanvas::onDrawPath(path, paint);
}

void SimCanvas::onDrawImage(const SkImage* image,
                            SkScalar left,
                            SkScalar top,
                            const SkPaint* paint) {
  DrawScope scope;
  AddCommand(CommandType::kImage);
  SkCanvas::onDrawImage(image, left, top, paint);
}

void SimCanvas::onDrawImageRect(const SkImage* image,
                                const SkRect* src,
                                const SkRect& dst,
                                const SkPaint* paint,
                                SrcRectConstraint constraint) {
  DrawScope scope;
  AddCommand(CommandType::kImage);
  SkCanvas::onDrawImageRect(image, src, dst, paint, constraint);
}

void SimCanvas::onDrawText(const void* text,
                           size_t byte_length,
                           SkScalar x,
                           SkScalar y,
                           const SkPaint& paint) {
  DrawScope scope;
  AddCommand(CommandType::kText, paint.getColor());
  SkCanvas::onDrawText(text, byte_length, x, y, paint);
}

void SimCanvas::onDrawPosText(const void* text,
                              size_t byte_length,
                              const SkPoint pos[],
                              const SkPaint& paint) {
  DrawScope scope;
  AddCommand(CommandType::kText, paint.getColor());
  SkCanvas::onDrawPosText(text, byte_length, pos, paint);
}

void SimCanvas::onDrawPosTextH(const void* text,
                               size_t byte_length,
                               const SkScalar xpos[],
                               SkScalar const_y,
                               const SkPaint& paint) {
  DrawScope scope;
  AddCommand(CommandType::kText, paint.getColor());
  SkCanvas::onDrawPosTextH(text, byte_length, xpos, const_y, paint);
}

void SimCanvas::onDrawTextOnPath(const void* text,
                                 size_t byte_length,
                                 const SkPath& path,
                                 const SkMatrix* matrix,
                                 const SkPaint& paint) {
  DrawScope scope;
  AddCommand(CommandType::kText, paint.getColor());
  SkCanvas::onDrawTextOnPath(text, byte_length, path, matrix, paint);
}

void SimCanvas::onDrawTextBlob(const SkTextBlob* blob,
                               SkScalar x,
                               SkScalar y,
                               const SkPaint& paint) {
  DrawScope scope;
  AddCommand(CommandType::kText, paint.getColor());
  SkCanvas::onDrawTextBlob(blob, x, y, paint);
}

}  // namespace blink
