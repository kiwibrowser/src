// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/omnibox/omnibox_match_cell_view.h"

#include <algorithm>

#include "base/feature_list.h"
#include "base/macros.h"
#include "base/metrics/field_trial_params.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/omnibox/omnibox_theme.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/omnibox/omnibox_popup_contents_view.h"
#include "chrome/browser/ui/views/omnibox/omnibox_text_view.h"
#include "chrome/browser/ui/views/omnibox/rounded_omnibox_results_frame.h"
#include "chrome/grit/generated_resources.h"
#include "components/omnibox/browser/omnibox_field_trial.h"
#include "extensions/common/image_util.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/gfx/image/canvas_image_source.h"
#include "ui/views/controls/image_view.h"

namespace {

// The minimum vertical margin that should be used above and below each
// suggestion.
static constexpr int kMinVerticalMargin = 1;

// The vertical padding to provide each RenderText in addition to the height of
// the font. Where possible, RenderText uses this additional space to vertically
// center the cap height of the font instead of centering the entire font.
static constexpr int kVerticalPadding = 4;

// TODO(dschuyler): Perhaps this should be based on the font size
// instead of hardcoded to 2 dp (e.g. by adding a space in an
// appropriate font to the beginning of the description, then reducing
// the additional padding here to zero).
static constexpr int kAnswerIconToTextPadding = 2;

// The edge length of the rich suggestions images.
static constexpr int kRichImageSize = 32;
static constexpr int kRichImageCornerRadius = 4;

// Returns the horizontal offset that ensures icons align vertically with the
// Omnibox icon.
int GetIconAlignmentOffset() {
  // The horizontal bounds of a result is the width of the selection highlight
  // (i.e. the views::Background). The traditional popup is designed with its
  // selection shape mimicking the internal shape of the omnibox border. Inset
  // to be consistent with the border drawn in BackgroundWith1PxBorder.
  int offset = LocationBarView::GetBorderThicknessDip();

  // The touch-optimized popup selection always fills the results frame. So to
  // align icons, inset additionally by the frame alignment inset on the left.
  if (ui::MaterialDesignController::IsTouchOptimizedUiEnabled())
    offset += RoundedOmniboxResultsFrame::kLocationBarAlignmentInsets.left();
  return offset;
}

// Returns the margins that should appear at the top and bottom of the result.
// |is_old_style_answer| indicates whether the vertical margin is for a omnibox
// result displaying an answer to the query.
gfx::Insets GetVerticalInsets(int text_height, bool is_old_style_answer) {
  // Regardless of the text size, we ensure a minimum size for the content line
  // here. This minimum is larger for hybrid mouse/touch devices to ensure an
  // adequately sized touch target.
  const int min_height_for_icon =
      GetLayoutConstant(LOCATION_BAR_ICON_SIZE) +
      (OmniboxFieldTrial::GetSuggestionVerticalMargin() * 2);
  const int min_height_for_text = text_height + 2 * kMinVerticalMargin;
  int min_height = std::max(min_height_for_icon, min_height_for_text);

  // Make sure the minimum height of an omnibox result matches the height of the
  // location bar view / non-results section of the omnibox popup in touch.
  if (ui::MaterialDesignController::IsTouchOptimizedUiEnabled()) {
    min_height = std::max(
        min_height, RoundedOmniboxResultsFrame::GetNonResultSectionHeight());
    if (is_old_style_answer) {
      // Answer matches apply the normal margin at the top and the minimum
      // allowable margin at the bottom.
      const int top_margin = gfx::ToCeiledInt((min_height - text_height) / 2.f);
      return gfx::Insets(top_margin, 0, kMinVerticalMargin, 0);
    }
  }

  const int total_margin = min_height - text_height;
  // Ceiling the top margin to account for |total_margin| being an odd number.
  const int top_margin = gfx::ToCeiledInt(total_margin / 2.f);
  const int bottom_margin = total_margin - top_margin;
  return gfx::Insets(top_margin, 0, bottom_margin, 0);
}

// Returns the padding width between elements.
int HorizontalPadding() {
  return GetLayoutConstant(LOCATION_BAR_ELEMENT_PADDING) +
         GetLayoutConstant(LOCATION_BAR_ICON_INTERIOR_PADDING);
}

////////////////////////////////////////////////////////////////////////////////
// PlaceholderImageSource:

class PlaceholderImageSource : public gfx::CanvasImageSource {
 public:
  PlaceholderImageSource(const gfx::Size& canvas_size, SkColor color);
  ~PlaceholderImageSource() override;

  // CanvasImageSource override:
  void Draw(gfx::Canvas* canvas) override;

 private:
  SkColor color_;
  gfx::Size size_;

  DISALLOW_COPY_AND_ASSIGN(PlaceholderImageSource);
};

PlaceholderImageSource::PlaceholderImageSource(const gfx::Size& canvas_size,
                                               SkColor color)
    : gfx::CanvasImageSource(canvas_size, false),
      color_(color),
      size_(canvas_size) {}

PlaceholderImageSource::~PlaceholderImageSource() = default;

void PlaceholderImageSource::Draw(gfx::Canvas* canvas) {
  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStrokeAndFill_Style);
  flags.setColor(color_);
  canvas->sk_canvas()->drawRoundRect(gfx::RectToSkRect(gfx::Rect(size_)),
                                     kRichImageCornerRadius,
                                     kRichImageCornerRadius, flags);
}

////////////////////////////////////////////////////////////////////////////////
// OmniboxImageView:

class OmniboxImageView : public views::ImageView {
 public:
  OmniboxImageView() = default;

  bool CanProcessEventsWithinSubtree() const override { return false; }

 private:
  // views::ImageView:
  void OnPaint(gfx::Canvas* canvas) override;

  DISALLOW_COPY_AND_ASSIGN(OmniboxImageView);
};

void OmniboxImageView::OnPaint(gfx::Canvas* canvas) {
  gfx::Path mask;
  mask.addRoundRect(gfx::RectToSkRect(GetImageBounds()), kRichImageCornerRadius,
                    kRichImageCornerRadius);
  canvas->ClipPath(mask, true);
  ImageView::OnPaint(canvas);
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// OmniboxMatchCellView:

OmniboxMatchCellView::OmniboxMatchCellView(OmniboxResultView* result_view)
    : is_old_style_answer_(false),
      is_rich_suggestion_(false),
      is_search_type_(false) {
  AddChildView(icon_view_ = new OmniboxImageView());
  AddChildView(image_view_ = new OmniboxImageView());
  AddChildView(content_view_ = new OmniboxTextView(result_view));
  AddChildView(description_view_ = new OmniboxTextView(result_view));
  AddChildView(separator_view_ = new OmniboxTextView(result_view));

  const base::string16& separator =
      l10n_util::GetStringUTF16(IDS_AUTOCOMPLETE_MATCH_DESCRIPTION_SEPARATOR);
  separator_view_->SetText(separator);
}

OmniboxMatchCellView::~OmniboxMatchCellView() = default;

gfx::Size OmniboxMatchCellView::CalculatePreferredSize() const {
  const int text_height = content_view_->GetLineHeight() + kVerticalPadding;
  int height = text_height +
               GetVerticalInsets(text_height, is_old_style_answer_).height();
  if (is_rich_suggestion_ || is_old_style_answer_) {
    height += GetDescriptionHeight();
  }
  return gfx::Size(0, height);
}

bool OmniboxMatchCellView::CanProcessEventsWithinSubtree() const {
  return false;
}

int OmniboxMatchCellView::GetDescriptionHeight() const {
  int icon_width = icon_view_->width();
  int answer_icon_size = image_view_->GetImage().isNull()
                             ? 0
                             : image_view_->height() + kAnswerIconToTextPadding;
  int deduction = GetIconAlignmentOffset() + icon_width +
                  (HorizontalPadding() * 3) + answer_icon_size;
  int description_width = std::max(width() - deduction, 0);
  return description_view_->GetHeightForWidth(description_width) +
         kVerticalPadding;
}

void OmniboxMatchCellView::OnMatchUpdate(const OmniboxResultView* result_view,
                                         const AutocompleteMatch& match) {
  is_old_style_answer_ = !!match.answer;
  is_rich_suggestion_ =
      (OmniboxFieldTrial::IsNewAnswerLayoutEnabled() && !!match.answer) ||
      (base::FeatureList::IsEnabled(omnibox::kOmniboxRichEntitySuggestions) &&
       !match.image_url.empty());
  is_search_type_ = AutocompleteMatch::IsSearchType(match.type);

  // Set up the small icon.
  if (is_rich_suggestion_) {
    icon_view_->SetSize(gfx::Size());
  } else {
    icon_view_->SetSize(icon_view_->CalculatePreferredSize());
  }

  // Set up the separator.
  if (is_old_style_answer_ || is_rich_suggestion_) {
    separator_view_->SetSize(gfx::Size());
  } else {
    separator_view_->SetSize(separator_view_->CalculatePreferredSize());
  }

  // Set up the larger image.
  if (!is_rich_suggestion_) {
    // An entry with |is_old_style_answer_| may use the image_view_. But it's
    // set when the image arrives (later).
    image_view_->SetImage(gfx::ImageSkia());
    image_view_->SetSize(gfx::Size());
  } else {
    SkColor color = result_view->GetColor(OmniboxPart::RESULTS_BACKGROUND);
    extensions::image_util::ParseHexColorString(match.image_dominant_color,
                                                &color);
    color = SkColorSetA(color, 0x40);  // 25% transparency (arbitrary).
    const gfx::Size size = gfx::Size(kRichImageSize, kRichImageSize);
    image_view_->SetImage(
        gfx::CanvasImageSource::MakeImageSkia<PlaceholderImageSource>(size,
                                                                      color));
    image_view_->SetSize(size);
  }
}

const char* OmniboxMatchCellView::GetClassName() const {
  return "OmniboxMatchCellView";
}

void OmniboxMatchCellView::Layout() {
  views::View::Layout();
  if (is_rich_suggestion_) {
    LayoutRichSuggestion();
  } else if (is_old_style_answer_) {
    LayoutOldStyleAnswer();
  } else {
    LayoutSplit();
  }
}

void OmniboxMatchCellView::LayoutOldStyleAnswer() {
  const int text_height = content_view_->GetLineHeight() + kVerticalPadding;
  int x = GetIconAlignmentOffset() + HorizontalPadding();
  int y = GetVerticalInsets(text_height, /*is_old_style_answer=*/true).top();
  icon_view_->SetPosition(
      gfx::Point(x, y + (text_height - icon_view_->height()) / 2));
  x += icon_view_->width() + HorizontalPadding();
  content_view_->SetBounds(x, y, width() - x, text_height);
  y += text_height;
  if (!image_view_->GetImage().isNull()) {
    // The description may be multi-line. Using the view height results in
    // an image that's too large, so we use the line height here instead.
    int image_edge_length = description_view_->GetLineHeight();
    image_view_->SetBounds(x, y + (kVerticalPadding / 2), image_edge_length,
                           image_edge_length);
    image_view_->SetImageSize(gfx::Size(image_edge_length, image_edge_length));
    x += image_view_->width() + kAnswerIconToTextPadding;
  }
  int description_width = width() - x;
  description_view_->SetBounds(
      x, y, description_width,
      description_view_->GetHeightForWidth(description_width) +
          kVerticalPadding);
}

void OmniboxMatchCellView::LayoutRichSuggestion() {
  const int text_height = content_view_->GetLineHeight() + kVerticalPadding;
  int x = GetIconAlignmentOffset() + HorizontalPadding();
  int y = GetVerticalInsets(text_height, /*is_old_style_answer=*/false).top();
  image_view_->SetImageSize(gfx::Size(kRichImageSize, kRichImageSize));
  image_view_->SetBounds(x, y + (text_height * 2 - kRichImageSize) / 2,
                         kRichImageSize, kRichImageSize);
  x += kRichImageSize + HorizontalPadding();
  content_view_->SetBounds(x, y, width() - x, text_height);
  y += text_height;
  int description_width = width() - x;
  description_view_->SetBounds(
      x, y, description_width,
      description_view_->GetHeightForWidth(description_width) +
          kVerticalPadding);
}

void OmniboxMatchCellView::LayoutSplit() {
  const int text_height = content_view_->GetLineHeight() + kVerticalPadding;
  int x = GetIconAlignmentOffset() + HorizontalPadding();
  icon_view_->SetSize(icon_view_->CalculatePreferredSize());
  int y = GetVerticalInsets(text_height, /*is_old_style_answer=*/false).top();
  icon_view_->SetPosition(
      gfx::Point(x, y + (text_height - icon_view_->height()) / 2));
  x += icon_view_->width() + HorizontalPadding();
  int content_width = content_view_->CalculatePreferredSize().width();
  int description_width = description_view_->CalculatePreferredSize().width();
  gfx::Size separator_size = separator_view_->CalculatePreferredSize();
  OmniboxPopupModel::ComputeMatchMaxWidths(
      content_width, separator_size.width(), description_width, width() - x,
      /*description_on_separate_line=*/false, !is_search_type_, &content_width,
      &description_width);
  content_view_->SetBounds(x, y, content_width, text_height);
  if (description_width != 0) {
    x += content_view_->width();
    separator_view_->SetSize(separator_size);
    separator_view_->SetBounds(x, y, separator_view_->width(), text_height);
    x += separator_view_->width();
    description_view_->SetBounds(x, y, description_width, text_height);
  } else {
    description_view_->SetSize(gfx::Size());
    separator_view_->SetSize(gfx::Size());
  }
}
