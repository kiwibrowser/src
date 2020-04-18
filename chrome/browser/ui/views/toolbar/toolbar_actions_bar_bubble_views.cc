// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/toolbar/toolbar_actions_bar_bubble_views.h"

#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/grit/locale_settings.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/image_button_factory.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/link.h"
#include "ui/views/layout/box_layout.h"

namespace {
const int kIconSize = 16;
}

ToolbarActionsBarBubbleViews::ToolbarActionsBarBubbleViews(
    views::View* anchor_view,
    const gfx::Point& anchor_point,
    bool anchored_to_action,
    std::unique_ptr<ToolbarActionsBarBubbleDelegate> delegate)
    : views::BubbleDialogDelegateView(anchor_view,
                                      views::BubbleBorder::TOP_RIGHT),
      delegate_(std::move(delegate)),
      anchored_to_action_(anchored_to_action) {
  set_close_on_deactivate(delegate_->ShouldCloseOnDeactivate());
  if (!anchor_view)
    SetAnchorRect(gfx::Rect(anchor_point, gfx::Size()));
  chrome::RecordDialogCreation(chrome::DialogIdentifier::TOOLBAR_ACTIONS_BAR);
}

ToolbarActionsBarBubbleViews::~ToolbarActionsBarBubbleViews() {}

void ToolbarActionsBarBubbleViews::Show() {
  // Passing the Widget pointer (via GetWidget()) below in the lambda is safe
  // because the controller, which eventually invokes the callback passed to
  // OnBubbleShown, will never outlive the bubble view. This is because the
  // ToolbarActionsBarBubbleView owns the ToolbarActionsBarBubbleDelegate.
  // The ToolbarActionsBarBubbleDelegate is an ExtensionMessageBubbleBridge,
  // which owns the ExtensionMessageBubbleController.
  delegate_->OnBubbleShown(
      base::Bind([](views::Widget* widget) { widget->Close(); }, GetWidget()));
  GetWidget()->Show();
}

views::View* ToolbarActionsBarBubbleViews::CreateExtraView() {
  std::unique_ptr<ToolbarActionsBarBubbleDelegate::ExtraViewInfo>
      extra_view_info = delegate_->GetExtraViewInfo();

  if (!extra_view_info)
    return nullptr;

  std::unique_ptr<views::ImageView> icon;
  if (extra_view_info->resource) {
    icon = std::make_unique<views::ImageView>();
    icon->SetImage(gfx::CreateVectorIcon(*extra_view_info->resource, kIconSize,
                                         gfx::kChromeIconGrey));
  }

  std::unique_ptr<views::View> extra_view;
  const base::string16& text = extra_view_info->text;
  if (!text.empty()) {
    if (extra_view_info->is_learn_more) {
      image_button_ = views::CreateVectorImageButton(this);
      image_button_->SetFocusForPlatform();
      image_button_->SetTooltipText(text);
      views::SetImageFromVectorIcon(image_button_,
                                    vector_icons::kHelpOutlineIcon);
      extra_view.reset(image_button_);
    } else {
      extra_view = std::make_unique<views::Label>(text);
    }
  }

  if (icon && extra_view) {
    views::View* parent = new views::View();
    parent->SetLayoutManager(std::make_unique<views::BoxLayout>(
        views::BoxLayout::kHorizontal, gfx::Insets(),
        ChromeLayoutProvider::Get()->GetDistanceMetric(
            views::DISTANCE_RELATED_CONTROL_VERTICAL)));
    parent->AddChildView(icon.release());
    parent->AddChildView(extra_view.release());
    return parent;
  }

  return icon ? static_cast<views::View*>(icon.release())
              : static_cast<views::View*>(extra_view.release());
}

base::string16 ToolbarActionsBarBubbleViews::GetWindowTitle() const {
  return delegate_->GetHeadingText();
}

bool ToolbarActionsBarBubbleViews::ShouldShowCloseButton() const {
  return true;
}

bool ToolbarActionsBarBubbleViews::Cancel() {
  DCHECK(!delegate_notified_of_close_);
  delegate_notified_of_close_ = true;
  delegate_->OnBubbleClosed(
      ToolbarActionsBarBubbleDelegate::CLOSE_DISMISS_USER_ACTION);
  return true;
}

bool ToolbarActionsBarBubbleViews::Accept() {
  DCHECK(!delegate_notified_of_close_);
  delegate_notified_of_close_ = true;
  delegate_->OnBubbleClosed(ToolbarActionsBarBubbleDelegate::CLOSE_EXECUTE);
  return true;
}

bool ToolbarActionsBarBubbleViews::Close() {
  // If the user took any action, the delegate will have been notified already.
  // Otherwise, this was dismissal due to deactivation.
  if (!delegate_notified_of_close_) {
    delegate_notified_of_close_ = true;
    delegate_->OnBubbleClosed(
        ToolbarActionsBarBubbleDelegate::CLOSE_DISMISS_DEACTIVATION);
  }
  return true;
}

void ToolbarActionsBarBubbleViews::Init() {
  base::string16 body_text_string = delegate_->GetBodyText(anchored_to_action_);
  base::string16 item_list = delegate_->GetItemListText();
  if (body_text_string.empty() && item_list.empty())
    return;

  ChromeLayoutProvider* provider = ChromeLayoutProvider::Get();
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::kVertical, gfx::Insets(),
      provider->GetDistanceMetric(views::DISTANCE_RELATED_CONTROL_VERTICAL)));

  int width = provider->GetDistanceMetric(
                  ChromeDistanceMetric::DISTANCE_BUBBLE_PREFERRED_WIDTH) -
              margins().width();

  if (!body_text_string.empty()) {
    body_text_ = new views::Label(body_text_string);
    body_text_->SetMultiLine(true);
    body_text_->SizeToFit(width);
    body_text_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    AddChildView(body_text_);
  }

  if (!item_list.empty()) {
    item_list_ = new views::Label(item_list);
    item_list_->SetBorder(views::CreateEmptyBorder(
        0,
        provider->GetDistanceMetric(views::DISTANCE_RELATED_CONTROL_HORIZONTAL),
        0, 0));
    item_list_->SetMultiLine(true);
    item_list_->SizeToFit(width);
    item_list_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    AddChildView(item_list_);
  }
}

int ToolbarActionsBarBubbleViews::GetDialogButtons() const {
  int buttons = ui::DIALOG_BUTTON_NONE;
  if (!delegate_->GetActionButtonText().empty())
    buttons |= ui::DIALOG_BUTTON_OK;
  if (!delegate_->GetDismissButtonText().empty())
    buttons |= ui::DIALOG_BUTTON_CANCEL;
  return buttons;
}

int ToolbarActionsBarBubbleViews::GetDefaultDialogButton() const {
  return delegate_->GetDefaultDialogButton();
}

base::string16 ToolbarActionsBarBubbleViews::GetDialogButtonLabel(
    ui::DialogButton button) const {
  return button == ui::DIALOG_BUTTON_OK ? delegate_->GetActionButtonText()
                                        : delegate_->GetDismissButtonText();
}

void ToolbarActionsBarBubbleViews::ButtonPressed(views::Button* sender,
                                                 const ui::Event& event) {
  DCHECK(!delegate_notified_of_close_);
  delegate_notified_of_close_ = true;
  delegate_->OnBubbleClosed(ToolbarActionsBarBubbleDelegate::CLOSE_LEARN_MORE);
  // Note that the Widget may or may not already be closed at this point,
  // depending on delegate_->ShouldCloseOnDeactivate(). Widget::Close() protects
  // against multiple calls (so long as they are not nested), and Widget
  // destruction is asynchronous, so it is safe to call Close() again.
  GetWidget()->Close();
}
