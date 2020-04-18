// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_FRAME_CUSTOM_FRAME_HEADER_H_
#define ASH_FRAME_CUSTOM_FRAME_HEADER_H_

#include "ash/ash_export.h"
#include "ash/frame/frame_header.h"
#include "base/callback.h"
#include "base/macros.h"
#include "ui/gfx/image/image_skia.h"

namespace ash {

// Helper class for drawing a custom frame (such as for a themed Chrome Browser
// frame).
class ASH_EXPORT CustomFrameHeader : public FrameHeader {
 public:
  class AppearanceProvider {
   public:
    virtual ~AppearanceProvider() = default;
    virtual SkColor GetFrameHeaderColor(bool active) = 0;
    virtual gfx::ImageSkia GetFrameHeaderImage(bool active) = 0;
    virtual gfx::ImageSkia GetFrameHeaderOverlayImage(bool active) = 0;
    virtual bool IsTabletMode() = 0;
  };

  // BrowserFrameHeaderAsh does not take ownership of any of the parameters.
  // |target_widget| is the widget that the caption buttons act on.
  // |view| is the view into which |this| will paint. |back_button| can be
  // nullptr, and the frame will not have a back button.
  CustomFrameHeader(views::Widget* target_widget,
                    views::View* view,
                    AppearanceProvider* appearance_provider,
                    bool incognito,
                    FrameCaptionButtonContainerView* caption_button_container);
  ~CustomFrameHeader() override;

 protected:
  // FrameHeader:
  void DoPaintHeader(gfx::Canvas* canvas) override;
  void DoSetFrameColors(SkColor active_frame_color,
                        SkColor inactive_frame_color) override;
  AshLayoutSize GetButtonLayoutSize() const override;
  SkColor GetTitleColor() const override;
  SkColor GetCurrentFrameColor() const override;

 private:
  // Paints the frame image for the |active| state based on the current value of
  // the activation animation.
  void PaintFrameImages(gfx::Canvas* canvas, bool active);

  AppearanceProvider* appearance_provider_ = nullptr;

  // Whether the header is for an incognito browser window.
  bool is_incognito_ = false;

  DISALLOW_COPY_AND_ASSIGN(CustomFrameHeader);
};

}  // namespace ash

#endif  // ASH_FRAME_CUSTOM_FRAME_HEADER_H_
