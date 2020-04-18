// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_LIBGTKUI_NAV_BUTTON_PROVIDER_GTK3_H_
#define CHROME_BROWSER_UI_LIBGTKUI_NAV_BUTTON_PROVIDER_GTK3_H_

#include <map>

#include "chrome/browser/ui/frame_button_display_types.h"
#include "chrome/browser/ui/libgtkui/libgtkui_export.h"
#include "chrome/browser/ui/views/nav_button_provider.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/controls/button/button.h"

namespace libgtkui {

class LIBGTKUI_EXPORT NavButtonProviderGtk3 : public views::NavButtonProvider {
 public:
  NavButtonProviderGtk3();
  ~NavButtonProviderGtk3() override;

  // views::NavButtonProvider:
  void RedrawImages(int top_area_height, bool maximized, bool active) override;
  gfx::ImageSkia GetImage(chrome::FrameButtonDisplayType type,
                          views::Button::ButtonState state) const override;
  gfx::Insets GetNavButtonMargin(
      chrome::FrameButtonDisplayType type) const override;
  gfx::Insets GetTopAreaSpacing() const override;
  int GetInterNavButtonSpacing() const override;
  std::unique_ptr<views::Background> CreateAvatarButtonBackground(
      const views::Button* avatar_button) const override;
  void CalculateCaptionButtonLayout(
      const gfx::Size& content_size,
      int top_area_height,
      gfx::Size* caption_button_size,
      gfx::Insets* caption_button_spacing) const override;

 private:
  std::map<chrome::FrameButtonDisplayType,
           gfx::ImageSkia[views::Button::STATE_COUNT]>
      button_images_;
  std::map<chrome::FrameButtonDisplayType, gfx::Insets> button_margins_;
  gfx::Insets top_area_spacing_;
  int inter_button_spacing_;
};

}  // namespace libgtkui

#endif  // CHROME_BROWSER_UI_LIBGTKUI_NAV_BUTTON_PROVIDER_GTK3_H_
