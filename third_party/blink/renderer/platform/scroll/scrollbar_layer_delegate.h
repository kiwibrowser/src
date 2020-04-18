// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCROLL_SCROLLBAR_LAYER_DELEGATE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCROLL_SCROLLBAR_LAYER_DELEGATE_H_

#include <memory>

#include "base/macros.h"
#include "cc/input/scrollbar.h"
#include "cc/paint/paint_canvas.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/platform_export.h"

namespace blink {

class Scrollbar;
class ScrollbarTheme;

// Implementation of cc::Scrollbar, providing a delegate to query about
// scrollbar state and to paint the image in the scrollbar.
class PLATFORM_EXPORT ScrollbarLayerDelegate : public cc::Scrollbar {
 public:
  ScrollbarLayerDelegate(blink::Scrollbar& scrollbar,
                         float device_scale_factor);
  ~ScrollbarLayerDelegate() override;

  // cc::Scrollbar implementation.
  cc::ScrollbarOrientation Orientation() const override;
  bool IsLeftSideVerticalScrollbar() const override;
  bool HasThumb() const override;
  bool IsOverlay() const override;
  gfx::Point Location() const override;
  int ThumbThickness() const override;
  int ThumbLength() const override;
  gfx::Rect TrackRect() const override;
  float ThumbOpacity() const override;
  bool NeedsPaintPart(cc::ScrollbarPart part) const override;
  bool HasTickmarks() const override;
  void PaintPart(cc::PaintCanvas* canvas,
                 cc::ScrollbarPart part,
                 const gfx::Rect& content_rect) override;

  bool UsesNinePatchThumbResource() const override;
  gfx::Size NinePatchThumbCanvasSize() const override;
  gfx::Rect NinePatchThumbAperture() const override;

 private:
  // Accessed by main and compositor threads, e.g., the compositor thread
  // checks |Orientation()|.
  CrossThreadPersistent<blink::Scrollbar> scrollbar_;

  ScrollbarTheme& theme_;
  float device_scale_factor_;

  DISALLOW_COPY_AND_ASSIGN(ScrollbarLayerDelegate);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCROLL_SCROLLBAR_LAYER_DELEGATE_H_
