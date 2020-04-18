// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/omnibox/rounded_omnibox_results_frame.h"

#include "build/build_config.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/omnibox/omnibox_theme.h"
#include "chrome/browser/ui/views/location_bar/background_with_1_px_border.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "ui/compositor/layer.h"
#include "ui/views/painter.h"

#if defined(USE_AURA)
#include "ui/aura/window.h"
#include "ui/aura/window_targeter.h"
#endif

namespace {

// Value from the spec controlling appearance of the shadow.
constexpr int kElevation = 16;

// View at the top of the frame which paints transparent pixels to make a hole
// so that the location bar shows through.
class TopBackgroundView : public views::View {
 public:
  explicit TopBackgroundView(SkColor color) {
    auto background =
        std::make_unique<BackgroundWith1PxBorder>(SK_ColorTRANSPARENT, color);
    background->set_blend_mode(SkBlendMode::kSrc);
    SetBackground(std::move(background));
  }
};

// Insets used to position |contents_| within |contents_host_|.
gfx::Insets GetContentInsets() {
  return gfx::Insets(RoundedOmniboxResultsFrame::GetNonResultSectionHeight(), 0,
                     0, 0);
}

}  // namespace

constexpr gfx::Insets RoundedOmniboxResultsFrame::kLocationBarAlignmentInsets;

RoundedOmniboxResultsFrame::RoundedOmniboxResultsFrame(views::View* contents,
                                                       OmniboxTint tint)
    : contents_(contents) {
  // Host the contents in its own View to simplify layout and clipping.
  contents_host_ = new views::View();
  contents_host_->SetPaintToLayer();
  contents_host_->layer()->SetFillsBoundsOpaquely(false);

  // Use a solid background. Note this is clipped to get rounded corners.
  SkColor background_color =
      GetOmniboxColor(OmniboxPart::RESULTS_BACKGROUND, tint);
  contents_host_->SetBackground(views::CreateSolidBackground(background_color));

  // Use a textured mask to clip contents. This doesn't work on Windows
  // (https://crbug.com/713359), and can't be animated, but it simplifies
  // selection highlights.
  // TODO(tapted): Remove this and have the contents paint a half-rounded rect
  // for the background, and when selecting the bottom row.
  contents_mask_ = views::Painter::CreatePaintedLayer(
      views::Painter::CreateSolidRoundRectPainter(
          SK_ColorBLACK, GetLayoutConstant(LOCATION_BAR_BUBBLE_CORNER_RADIUS)));
  contents_mask_->layer()->SetFillsBoundsOpaquely(false);
  contents_host_->layer()->SetMaskLayer(contents_mask_->layer());

  top_background_ = new TopBackgroundView(background_color);
  contents_host_->AddChildView(top_background_);
  contents_host_->AddChildView(contents_);

  AddChildView(contents_host_);
}

RoundedOmniboxResultsFrame::~RoundedOmniboxResultsFrame() = default;

// static
void RoundedOmniboxResultsFrame::OnBeforeWidgetInit(
    views::Widget::InitParams* params) {
  params->shadow_type = views::Widget::InitParams::SHADOW_TYPE_DROP;
  params->corner_radius = GetLayoutConstant(LOCATION_BAR_BUBBLE_CORNER_RADIUS);
  params->shadow_elevation = kElevation;
  params->name = "RoundedOmniboxResultsFrameWindow";
}

// static
int RoundedOmniboxResultsFrame::GetNonResultSectionHeight() {
  return GetLayoutConstant(LOCATION_BAR_HEIGHT) +
         kLocationBarAlignmentInsets.height();
}

const char* RoundedOmniboxResultsFrame::GetClassName() const {
  return "RoundedOmniboxResultsFrame";
}

void RoundedOmniboxResultsFrame::Layout() {
  // This is called when the Widget resizes due to results changing. Resizing
  // the Widget is fast on ChromeOS, but slow on other platforms, and can't be
  // animated smoothly.
  // TODO(tapted): Investigate using a static Widget size.
  const gfx::Rect bounds = GetLocalBounds();
  contents_host_->SetBoundsRect(bounds);
  contents_mask_->layer()->SetBounds(bounds);

  gfx::Rect top_bounds(bounds);
  top_bounds.set_height(GetNonResultSectionHeight());
  top_bounds.Inset(kLocationBarAlignmentInsets);
  top_background_->SetBoundsRect(top_bounds);

  gfx::Rect results_bounds(bounds);
  results_bounds.Inset(GetContentInsets());
  contents_->SetBoundsRect(results_bounds);
}

void RoundedOmniboxResultsFrame::AddedToWidget() {
#if defined(USE_AURA)
  // Use a ui::EventTargeter that allows mouse and touch events in the top
  // portion of the Widget to pass through to the omnibox beneath it.
  auto results_targeter = std::make_unique<aura::WindowTargeter>();
  results_targeter->SetInsets(GetContentInsets());
  GetWidget()->GetNativeWindow()->SetEventTargeter(std::move(results_targeter));
#endif
}
