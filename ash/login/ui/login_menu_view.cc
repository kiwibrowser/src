// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/login/ui/login_menu_view.h"

#include "ash/login/ui/hover_notifier.h"
#include "ash/login/ui/non_accessible_view.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/scrollbar/overlay_scroll_bar.h"
#include "ui/views/layout/box_layout.h"

namespace ash {

constexpr int kMenuItemWidthDp = 178;
constexpr int kMenuItemHeightDp = 28;
constexpr int kRegularMenuItemLeftPaddingDp = 2;
constexpr int kGroupMenuItemLeftPaddingDp = 10;
constexpr int kNonEmptyHeight = 1;

namespace {

constexpr SkColor kMenuBackgroundColor = SkColorSetRGB(0x3C, 0x40, 0x43);

class MenuItemView : public views::Button, public views::ButtonListener {
 public:
  MenuItemView(const LoginMenuView::Item& item,
               const LoginMenuView::OnHighLight& on_highlight)
      : views::Button(this), item_(item), on_highlight_(on_highlight) {
    SetFocusBehavior(FocusBehavior::ALWAYS);
    SetLayoutManager(
        std::make_unique<views::BoxLayout>(views::BoxLayout::kHorizontal));
    SetPreferredSize(gfx::Size(kMenuItemWidthDp, kMenuItemHeightDp));

    auto* spacing = new NonAccessibleView();
    spacing->SetPreferredSize(gfx::Size(item.is_group
                                            ? kRegularMenuItemLeftPaddingDp
                                            : kGroupMenuItemLeftPaddingDp,
                                        kNonEmptyHeight));
    AddChildView(spacing);

    views::Label* label = new views::Label(base::UTF8ToUTF16(item.title));
    label->SetEnabledColor(SK_ColorWHITE);
    label->SetSubpixelRenderingEnabled(false);
    label->SetAutoColorReadabilityEnabled(false);
    label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    AddChildView(label);

    if (item.selected)
      SetBackground(views::CreateSolidBackground(SK_ColorGRAY));

    hover_notifier_ = std::make_unique<HoverNotifier>(
        this,
        base::BindRepeating(&MenuItemView::OnHover, base::Unretained(this)));
  }

  ~MenuItemView() override = default;

  // views::View:
  int GetHeightForWidth(int w) const override {
    // Make row height fixed avoiding layout manager adjustments.
    return GetPreferredSize().height();
  }

  // views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override {
    DCHECK(sender == this);
    if (item_.is_group)
      return;

    on_highlight_.Run(true /*by_selection*/);
    GetWidget()->Close();
  }

  void OnHover(bool has_hover) {
    if (has_hover && !item_.is_group)
      RequestFocus();
  }

  void OnFocus() override {
    ScrollViewToVisible();
    on_highlight_.Run(false /*by_selection*/);
  }

  const LoginMenuView::Item& item() const { return item_; }

 private:
  const LoginMenuView::Item item_;
  const LoginMenuView::OnHighLight on_highlight_;
  std::unique_ptr<HoverNotifier> hover_notifier_;

  DISALLOW_COPY_AND_ASSIGN(MenuItemView);
};

class LoginScrollBar : public views::OverlayScrollBar {
 public:
  LoginScrollBar() : OverlayScrollBar(false) {}

  // OverlayScrollBar:
  bool OnKeyPressed(const ui::KeyEvent& event) override {
    // Let LoginMenuView to handle up/down keypress.
    return false;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(LoginScrollBar);
};

}  // namespace

LoginMenuView::TestApi::TestApi(LoginMenuView* view) : view_(view) {}

LoginMenuView::TestApi::~TestApi() = default;

views::View* LoginMenuView::TestApi::contents() const {
  return view_->contents_;
}

LoginMenuView::Item::Item() = default;

LoginMenuView::LoginMenuView(const std::vector<Item>& items,
                             views::View* anchor_view,
                             const OnSelect& on_select)
    : LoginBaseBubbleView(anchor_view), on_select_(on_select) {
  set_can_activate(true);
  set_margins(gfx::Insets());
  set_color(kMenuBackgroundColor);
  SetFocusBehavior(views::View::FocusBehavior::ALWAYS);

  scroller_ = new views::ScrollView();
  scroller_->SetBackgroundColor(SK_ColorTRANSPARENT);
  scroller_->set_draw_overflow_indicator(false);
  scroller_->ClipHeightTo(kMenuItemHeightDp, kMenuItemHeightDp * 5);
  AddChildView(scroller_);

  views::BoxLayout* box_layout = SetLayoutManager(
      std::make_unique<views::BoxLayout>(views::BoxLayout::kVertical));
  box_layout->SetFlexForView(scroller_, 1);

  contents_ = new NonAccessibleView();
  views::BoxLayout* layout = contents_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(views::BoxLayout::kVertical));
  layout->SetDefaultFlex(1);
  layout->set_minimum_cross_axis_size(kMenuItemWidthDp);
  layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::MAIN_AXIS_ALIGNMENT_CENTER);

  for (size_t i = 0; i < items.size(); i++) {
    const Item& item = items[i];
    contents_->AddChildView(new MenuItemView(
        item, base::BindRepeating(&LoginMenuView::OnHighLightChange,
                                  base::Unretained(this), i)));

    if (item.selected)
      selected_index_ = i;
  }
  scroller_->SetContents(contents_);
  scroller_->SetVerticalScrollBar(new LoginScrollBar());
}

LoginMenuView::~LoginMenuView() = default;

void LoginMenuView::OnHighLightChange(int item_index, bool by_selection) {
  selected_index_ = item_index;
  views::View* highlight_item = contents_->child_at(item_index);
  for (views::View* child : contents_->GetChildrenInZOrder()) {
    child->SetBackground(views::CreateSolidBackground(
        child == highlight_item ? SK_ColorGRAY : SK_ColorTRANSPARENT));
  }

  if (by_selection) {
    MenuItemView* menu_view = static_cast<MenuItemView*>(highlight_item);
    on_select_.Run(menu_view->item());
  }
  contents_->SchedulePaint();
}

int LoginMenuView::FindNextItem(bool reverse) {
  int delta = reverse ? -1 : 1;
  int current_index = selected_index_ + delta;
  while (current_index >= 0 && current_index < contents_->child_count()) {
    MenuItemView* menu_view =
        static_cast<MenuItemView*>(contents_->child_at(current_index));
    if (!menu_view->item().is_group)
      break;
    current_index += delta;
  }

  if (current_index < 0 || current_index == contents_->child_count())
    return selected_index_;
  return current_index;
}

void LoginMenuView::OnFocus() {
  // Forward the focus to the selected child view.
  contents_->child_at(selected_index_)->RequestFocus();
}

bool LoginMenuView::OnKeyPressed(const ui::KeyEvent& event) {
  const ui::KeyboardCode key = event.key_code();
  if (key == ui::VKEY_UP || key == ui::VKEY_DOWN) {
    contents_->child_at(FindNextItem(key == ui::VKEY_UP))->RequestFocus();
    return true;
  }

  return false;
}

void LoginMenuView::VisibilityChanged(View* starting_from, bool is_visible) {
  if (is_visible)
    contents_->child_at(selected_index_)->RequestFocus();
}

}  // namespace ash
