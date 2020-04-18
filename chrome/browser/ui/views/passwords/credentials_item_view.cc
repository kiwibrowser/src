// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/passwords/credentials_item_view.h"

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/passwords/manage_passwords_view_utils.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/browser/ui/views/harmony/chrome_typography.h"
#include "chrome/grit/theme_resources.h"
#include "components/autofill/core/common/password_form.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/path.h"
#include "ui/views/border.h"
#include "ui/views/bubble/tooltip_icon.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"

namespace {

gfx::Size GetTextLabelsSize(const views::Label* upper_label,
                            const views::Label* lower_label) {
  gfx::Size upper_label_size = upper_label ? upper_label->GetPreferredSize()
                                           : gfx::Size();
  gfx::Size lower_label_size = lower_label ? lower_label->GetPreferredSize()
                                           : gfx::Size();
  return gfx::Size(std::max(upper_label_size.width(), lower_label_size.width()),
                   upper_label_size.height() + lower_label_size.height());
}

class CircularImageView : public views::ImageView {
 public:
  CircularImageView() = default;

 private:
  // views::ImageView:
  void OnPaint(gfx::Canvas* canvas) override;

  DISALLOW_COPY_AND_ASSIGN(CircularImageView);
};

void CircularImageView::OnPaint(gfx::Canvas* canvas) {
  // Display the avatar picture as a circle.
  gfx::Rect bounds(GetImageBounds());
  gfx::Path circular_mask;
  circular_mask.addCircle(
      SkIntToScalar(bounds.x() + bounds.right()) / 2,
      SkIntToScalar(bounds.y() + bounds.bottom()) / 2,
      SkIntToScalar(std::min(bounds.height(), bounds.width())) / 2);
  canvas->ClipPath(circular_mask, true);
  ImageView::OnPaint(canvas);
}

}  // namespace

CredentialsItemView::CredentialsItemView(
    views::ButtonListener* button_listener,
    const base::string16& upper_text,
    const base::string16& lower_text,
    SkColor hover_color,
    const autofill::PasswordForm* form,
    network::mojom::URLLoaderFactory* loader_factory)
    : Button(button_listener),
      form_(form),
      upper_label_(nullptr),
      lower_label_(nullptr),
      info_icon_(nullptr),
      hover_color_(hover_color),
      weak_ptr_factory_(this) {
  set_notify_enter_exit_on_child(true);
  // Create an image-view for the avatar. Make sure it ignores events so that
  // the parent can receive the events instead.
  image_view_ = new CircularImageView;
  image_view_->set_can_process_events_within_subtree(false);
  gfx::Image image = ui::ResourceBundle::GetSharedInstance().GetImageNamed(
      IDR_PROFILE_AVATAR_PLACEHOLDER_LARGE);
  DCHECK(image.Width() >= kAvatarImageSize &&
         image.Height() >= kAvatarImageSize);
  UpdateAvatar(image.AsImageSkia());
  if (form_->icon_url.is_valid()) {
    // Fetch the actual avatar.
    AccountAvatarFetcher* fetcher = new AccountAvatarFetcher(
        form_->icon_url, weak_ptr_factory_.GetWeakPtr());
    fetcher->Start(loader_factory);
  }
  AddChildView(image_view_);

  // TODO(tapted): Check these (and the STYLE_ values below) against the spec on
  // http://crbug.com/651681.
  const int kLabelContext = ChromeLayoutProvider::Get()->IsHarmonyMode()
                                ? CONTEXT_BODY_TEXT_SMALL
                                : CONTEXT_DEPRECATED_SMALL;

  if (!upper_text.empty()) {
    upper_label_ = new views::Label(upper_text, kLabelContext,
                                    views::style::STYLE_PRIMARY);
    upper_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    AddChildView(upper_label_);
  }

  if (!lower_text.empty()) {
    lower_label_ = new views::Label(lower_text, kLabelContext, STYLE_SECONDARY);
    lower_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    lower_label_->SetMultiLine(true);
    AddChildView(lower_label_);
  }

  if (form_->is_public_suffix_match) {
    info_icon_ = new views::TooltipIcon(
        base::UTF8ToUTF16(form_->origin.GetOrigin().spec()));
    AddChildView(info_icon_);
  }

  if (!upper_text.empty() && !lower_text.empty())
    SetAccessibleName(upper_text + base::ASCIIToUTF16("\n") + lower_text);
  else
    SetAccessibleName(upper_text + lower_text);

  SetFocusBehavior(FocusBehavior::ALWAYS);
}

CredentialsItemView::~CredentialsItemView() = default;

void CredentialsItemView::UpdateAvatar(const gfx::ImageSkia& image) {
  image_view_->SetImage(ScaleImageForAccountAvatar(image));
}

void CredentialsItemView::SetLowerLabelColor(SkColor color) {
  if (lower_label_)
    lower_label_->SetEnabledColor(color);
}

void CredentialsItemView::SetHoverColor(SkColor color) {
  hover_color_ = color;
}

int CredentialsItemView::GetPreferredHeight() const {
  return GetPreferredSize().height();
}

gfx::Size CredentialsItemView::CalculatePreferredSize() const {
  gfx::Size labels_size = GetTextLabelsSize(upper_label_, lower_label_);
  gfx::Size size = gfx::Size(kAvatarImageSize + labels_size.width(),
                             std::max(kAvatarImageSize, labels_size.height()));
  const gfx::Insets insets(GetInsets());
  size.Enlarge(insets.width(), insets.height());
  size.Enlarge(ChromeLayoutProvider::Get()->GetDistanceMetric(
                   views::DISTANCE_RELATED_LABEL_HORIZONTAL),
               0);

  // Make the size at least as large as the minimum size needed by the border.
  size.SetToMax(border() ? border()->GetMinimumSize() : gfx::Size());
  return size;
}

int CredentialsItemView::GetHeightForWidth(int w) const {
  return View::GetHeightForWidth(w);
}

void CredentialsItemView::Layout() {
  gfx::Rect child_area(GetLocalBounds());
  child_area.Inset(GetInsets());

  gfx::Size image_size(image_view_->GetPreferredSize());
  image_size.SetToMin(child_area.size());
  gfx::Point image_origin(child_area.origin());
  image_origin.Offset(0, (child_area.height() - image_size.height()) / 2);
  image_view_->SetBoundsRect(gfx::Rect(image_origin, image_size));

  gfx::Size upper_size =
      upper_label_ ? upper_label_->GetPreferredSize() : gfx::Size();
  gfx::Size lower_size =
      lower_label_ ? lower_label_->GetPreferredSize() : gfx::Size();
  int y_offset = (child_area.height() -
      (upper_size.height() + lower_size.height())) / 2;
  gfx::Point label_origin(image_origin.x() + image_size.width() +
                              ChromeLayoutProvider::Get()->GetDistanceMetric(
                                  views::DISTANCE_RELATED_LABEL_HORIZONTAL),
                          child_area.origin().y() + y_offset);
  if (upper_label_)
    upper_label_->SetBoundsRect(gfx::Rect(label_origin, upper_size));
  if (lower_label_) {
    label_origin.Offset(0, upper_size.height());
    lower_label_->SetBoundsRect(gfx::Rect(label_origin, lower_size));
  }
  if (info_icon_) {
    info_icon_->SizeToPreferredSize();
    info_icon_->SetPosition(
        gfx::Point(child_area.right() - info_icon_->width(),
                   child_area.CenterPoint().y() - info_icon_->height() / 2));
  }
}

void CredentialsItemView::OnPaintBackground(gfx::Canvas* canvas) {
  if (state() == STATE_PRESSED || state() == STATE_HOVERED)
    canvas->DrawColor(hover_color_);
}
