// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/frame/default_frame_header.h"

#include <memory>

#include "ash/frame/caption_buttons/frame_back_button.h"
#include "ash/frame/caption_buttons/frame_caption_button_container_view.h"
#include "ash/public/cpp/shell_window_ids.h"
#include "ash/test/ash_test_base.h"
#include "ui/views/test/test_views.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/non_client_view.h"

using views::NonClientFrameView;
using views::Widget;

namespace ash {

using DefaultFrameHeaderTest = AshTestBase;

// Ensure the title text is vertically aligned with the window icon.
TEST_F(DefaultFrameHeaderTest, TitleIconAlignment) {
  std::unique_ptr<Widget> w = CreateTestWidget(
      nullptr, kShellWindowId_DefaultContainer, gfx::Rect(1, 2, 3, 4));
  FrameCaptionButtonContainerView container(w.get());
  views::StaticSizedView window_icon(gfx::Size(16, 16));
  window_icon.SetBounds(0, 0, 16, 16);
  w->SetBounds(gfx::Rect(0, 0, 500, 500));
  w->Show();

  DefaultFrameHeader frame_header(w.get(), w->non_client_view()->frame_view(),
                                  &container);
  frame_header.SetLeftHeaderView(&window_icon);
  frame_header.LayoutHeader();
  gfx::Rect title_bounds = frame_header.GetTitleBounds();
  EXPECT_EQ(window_icon.bounds().CenterPoint().y(),
            title_bounds.CenterPoint().y());
}

TEST_F(DefaultFrameHeaderTest, BackButtonAlignment) {
  std::unique_ptr<Widget> w = CreateTestWidget(
      nullptr, kShellWindowId_DefaultContainer, gfx::Rect(1, 2, 3, 4));
  FrameCaptionButtonContainerView container(w.get());
  FrameBackButton back;

  DefaultFrameHeader frame_header(w.get(), w->non_client_view()->frame_view(),
                                  &container);
  frame_header.SetBackButton(&back);
  frame_header.LayoutHeader();
  gfx::Rect title_bounds = frame_header.GetTitleBounds();
  // The back button should be positioned at the left edge, and
  // vertically centered.
  EXPECT_EQ(back.bounds().CenterPoint().y(), title_bounds.CenterPoint().y());
  EXPECT_EQ(0, back.bounds().x());
}

// Ensure the right frame colors are used.
TEST_F(DefaultFrameHeaderTest, FrameColors) {
  std::unique_ptr<Widget> w = CreateTestWidget(
      nullptr, kShellWindowId_DefaultContainer, gfx::Rect(1, 2, 3, 4));
  FrameCaptionButtonContainerView container(w.get());
  views::StaticSizedView window_icon(gfx::Size(16, 16));
  window_icon.SetBounds(0, 0, 16, 16);
  w->SetBounds(gfx::Rect(0, 0, 500, 500));
  w->Show();

  DefaultFrameHeader frame_header(w.get(), w->non_client_view()->frame_view(),
                                  &container);

  // Check frame color is sensitive to mode.
  SkColor active = SkColorSetRGB(70, 70, 70);
  SkColor inactive = SkColorSetRGB(200, 200, 200);
  frame_header.SetFrameColors(active, inactive);
  frame_header.mode_ = FrameHeader::MODE_ACTIVE;
  EXPECT_EQ(active, frame_header.GetCurrentFrameColor());
  frame_header.mode_ = FrameHeader::MODE_INACTIVE;
  EXPECT_EQ(inactive, frame_header.GetCurrentFrameColor());
}

}  // namespace ash
