// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/views/search_box_view.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "ash/app_list/model/search/search_box_model.h"
#include "ash/app_list/model/search/search_model.h"
#include "ash/public/cpp/app_list/app_list_constants.h"
#include "ash/public/cpp/app_list/vector_icons/vector_icons.h"
#include "ash/public/cpp/wallpaper_types.h"
#include "base/macros.h"
#include "ui/app_list/app_list_util.h"
#include "ui/app_list/app_list_view_delegate.h"
#include "ui/app_list/resources/grit/app_list_resources.h"
#include "ui/app_list/views/app_list_view.h"
#include "ui/app_list/views/contents_view.h"
#include "ui/app_list/views/search_result_base_view.h"
#include "ui/app_list/views/search_result_page_view.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/chromeos/search_box/search_box_constants.h"
#include "ui/chromeos/search_box/search_box_view_delegate.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/keyboard/keyboard_controller.h"
#include "ui/keyboard/keyboard_util.h"
#include "ui/strings/grit/ui_strings.h"
#include "ui/views/border.h"
#include "ui/views/context_menu_controller.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/vector_icons.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

using ash::ColorProfileType;

namespace app_list {

namespace {

constexpr int kPaddingSearchResult = 16;
constexpr int kSearchBoxBorderWidth = 4;

constexpr SkColor kSearchBoxBorderColor =
    SkColorSetARGB(0x3D, 0xFF, 0xFF, 0xFF);

constexpr int kSearchBoxBorderCornerRadiusSearchResult = 4;
constexpr int kCloseIconSize = 24;
constexpr int kSearchBoxFocusBorderCornerRadius = 28;

// Range of the fraction of app list from collapsed to peeking that search box
// should change opacity.
constexpr float kOpacityStartFraction = 0.1f;
constexpr float kOpacityEndFraction = 0.6f;

// Gets the box layout inset horizontal padding for the state of AppListModel.
int GetBoxLayoutPaddingForState(ash::AppListState state) {
  if (state == ash::AppListState::kStateSearchResults)
    return kPaddingSearchResult;
  return search_box::kPadding;
}

}  // namespace

SearchBoxView::SearchBoxView(search_box::SearchBoxViewDelegate* delegate,
                             AppListViewDelegate* view_delegate,
                             AppListView* app_list_view)
    : search_box::SearchBoxViewBase(delegate),
      view_delegate_(view_delegate),
      app_list_view_(app_list_view),
      weak_ptr_factory_(this) {
  set_is_tablet_mode(app_list_view->is_tablet_mode());
}

SearchBoxView::~SearchBoxView() {
  search_model_->search_box()->RemoveObserver(this);
}

void SearchBoxView::ClearSearch() {
  search_box::SearchBoxViewBase::ClearSearch();
  app_list_view_->SetStateFromSearchBoxView(true);
}

views::View* SearchBoxView::GetSelectedViewInContentsView() {
  if (!contents_view())
    return nullptr;
  return static_cast<ContentsView*>(contents_view())->GetSelectedView();
}

void SearchBoxView::HandleSearchBoxEvent(ui::LocatedEvent* located_event) {
  if (located_event->type() == ui::ET_MOUSEWHEEL) {
    if (!app_list_view_->HandleScroll(
            located_event->AsMouseWheelEvent()->offset().y(),
            ui::ET_MOUSEWHEEL)) {
      return;
    }
  }
  search_box::SearchBoxViewBase::HandleSearchBoxEvent(located_event);
}

void SearchBoxView::ModelChanged() {
  if (search_model_)
    search_model_->search_box()->RemoveObserver(this);

  search_model_ = view_delegate_->GetSearchModel();
  DCHECK(search_model_);
  UpdateSearchIcon();
  search_model_->search_box()->AddObserver(this);

  HintTextChanged();
  OnWallpaperColorsChanged();
}

void SearchBoxView::UpdateKeyboardVisibility() {
  if (!is_tablet_mode())
    return;

  keyboard::KeyboardController* const keyboard_controller =
      keyboard::KeyboardController::GetInstance();
  if (!keyboard_controller ||
      is_search_box_active() == keyboard::IsKeyboardVisible()) {
    return;
  }

  if (is_search_box_active()) {
    keyboard_controller->ShowKeyboard(false);
    return;
  }

  keyboard_controller->HideKeyboard(
      keyboard::KeyboardController::HIDE_REASON_MANUAL);
}

void SearchBoxView::UpdateModel(bool initiated_by_user) {
  // Temporarily remove from observer to ignore notifications caused by us.
  search_model_->search_box()->RemoveObserver(this);
  search_model_->search_box()->Update(search_box()->text(), initiated_by_user);
  search_model_->search_box()->SetSelectionModel(
      search_box()->GetSelectionModel());
  search_model_->search_box()->AddObserver(this);
}

void SearchBoxView::UpdateSearchIcon() {
  const gfx::VectorIcon& google_icon =
      is_search_box_active() ? kIcGoogleColorIcon : kIcGoogleBlackIcon;
  const gfx::VectorIcon& icon = search_model_->search_engine_is_google()
                                    ? google_icon
                                    : kIcSearchEngineNotGoogleIcon;
  SetSearchIconImage(gfx::CreateVectorIcon(icon, search_box::kSearchIconSize,
                                           search_box_color()));
}

void SearchBoxView::UpdateSearchBoxBorder() {
  if (search_box()->HasFocus() && !is_search_box_active()) {
    // Show a gray ring around search box to indicate that the search box is
    // selected. Do not show it when search box is active, because blinking
    // cursor already indicates that.
    SetBorder(views::CreateRoundedRectBorder(kSearchBoxBorderWidth,
                                             kSearchBoxFocusBorderCornerRadius,
                                             kSearchBoxBorderColor));
    return;
  }

  // Creates an empty border as a placeholder for colored border so that
  // re-layout won't move views below the search box.
  SetBorder(
      views::CreateEmptyBorder(kSearchBoxBorderWidth, kSearchBoxBorderWidth,
                               kSearchBoxBorderWidth, kSearchBoxBorderWidth));
}

void SearchBoxView::SetupCloseButton() {
  views::ImageButton* close = close_button();
  close->SetImage(views::ImageButton::STATE_NORMAL,
                  gfx::CreateVectorIcon(views::kIcCloseIcon, kCloseIconSize,
                                        search_box_color()));
  close->SetVisible(false);
  close->SetAccessibleName(
      l10n_util::GetStringUTF16(IDS_APP_LIST_CLEAR_SEARCHBOX));
}

void SearchBoxView::SetupBackButton() {
  views::ImageButton* back = back_button();
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  back->SetImage(views::ImageButton::STATE_NORMAL,
                 rb.GetImageSkiaNamed(IDR_APP_LIST_FOLDER_BACK_NORMAL));
  back->SetImageAlignment(views::ImageButton::ALIGN_CENTER,
                          views::ImageButton::ALIGN_MIDDLE);
  back->SetVisible(false);
  base::string16 back_button_label(
      l10n_util::GetStringUTF16(IDS_APP_LIST_BACK));
  back->SetAccessibleName(back_button_label);
  back->SetTooltipText(back_button_label);
}

void SearchBoxView::OnKeyEvent(ui::KeyEvent* event) {
  app_list_view_->RedirectKeyEventToSearchBox(event);

  if (!CanProcessUpDownKeyTraversal(*event))
    return;

  // If focus is in search box view, up key moves focus to the last element of
  // contents view, while down key moves focus to the first element of contents
  // view.
  ContentsView* contents = static_cast<ContentsView*>(contents_view());
  AppListPage* page = contents->GetPageView(contents->GetActivePageIndex());
  views::View* v = event->key_code() == ui::VKEY_UP
                       ? page->GetLastFocusableView()
                       : page->GetFirstFocusableView();
  if (v)
    v->RequestFocus();
  event->SetHandled();
}

void SearchBoxView::UpdateBackground(double progress,
                                     ash::AppListState current_state,
                                     ash::AppListState target_state) {
  SetSearchBoxBackgroundCornerRadius(gfx::Tween::LinearIntValueBetween(
      progress, GetSearchBoxBorderCornerRadiusForState(current_state),
      GetSearchBoxBorderCornerRadiusForState(target_state)));
  const SkColor color = gfx::Tween::ColorValueBetween(
      progress, GetBackgroundColorForState(current_state),
      GetBackgroundColorForState(target_state));
  UpdateBackgroundColor(color);
}

void SearchBoxView::UpdateLayout(double progress,
                                 ash::AppListState current_state,
                                 ash::AppListState target_state) {
  box_layout()->set_inside_border_insets(
      gfx::Insets(0, gfx::Tween::LinearIntValueBetween(
                         progress, GetBoxLayoutPaddingForState(current_state),
                         GetBoxLayoutPaddingForState(target_state))));
  InvalidateLayout();
}

int SearchBoxView::GetSearchBoxBorderCornerRadiusForState(
    ash::AppListState state) const {
  if (state == ash::AppListState::kStateSearchResults &&
      !app_list_view_->is_in_drag()) {
    return kSearchBoxBorderCornerRadiusSearchResult;
  }
  return search_box::kSearchBoxBorderCornerRadius;
}

SkColor SearchBoxView::GetBackgroundColorForState(
    ash::AppListState state) const {
  if (state == ash::AppListState::kStateSearchResults)
    return kCardBackgroundColor;
  return background_color();
}

void SearchBoxView::UpdateOpacity() {
  // The opacity of searchbox is a function of the fractional displacement of
  // the app list from collapsed(0) to peeking(1) state. When the fraction
  // changes from |kOpacityStartFraction| to |kOpaticyEndFraction|, the opacity
  // of searchbox changes from 0.f to 1.0f.
  ContentsView* contents = static_cast<ContentsView*>(contents_view());
  if (!contents->GetPageView(contents->GetActivePageIndex())
           ->ShouldShowSearchBox()) {
    return;
  }
  int app_list_y_position_in_screen =
      contents->app_list_view()->app_list_y_position_in_screen();
  float fraction =
      std::max<float>(0, contents->app_list_view()->GetScreenBottom() -
                             kShelfSize - app_list_y_position_in_screen) /
      (kPeekingAppListHeight - kShelfSize);

  float opacity =
      std::min(std::max((fraction - kOpacityStartFraction) /
                            (kOpacityEndFraction - kOpacityStartFraction),
                        0.f),
               1.0f);

  AppListView* app_list_view = contents->app_list_view();
  bool should_restore_opacity =
      !app_list_view->is_in_drag() &&
      (app_list_view->app_list_state() != AppListViewState::CLOSED);
  // Restores the opacity of searchbox if the gesture dragging ends.
  this->layer()->SetOpacity(should_restore_opacity ? 1.0f : opacity);
  contents->search_results_page_view()->layer()->SetOpacity(
      should_restore_opacity ? 1.0f : opacity);
}

void SearchBoxView::GetWallpaperProminentColors(
    AppListViewDelegate::GetWallpaperProminentColorsCallback callback) {
  view_delegate_->GetWallpaperProminentColors(std::move(callback));
}

void SearchBoxView::OnWallpaperProminentColorsReceived(
    const std::vector<SkColor>& prominent_colors) {
  if (prominent_colors.empty())
    return;
  DCHECK_EQ(static_cast<size_t>(ColorProfileType::NUM_OF_COLOR_PROFILES),
            prominent_colors.size());

  SetSearchBoxColor(
      prominent_colors[static_cast<int>(ColorProfileType::DARK_MUTED)]);
  SetBackgroundColor(
      prominent_colors[static_cast<int>(ColorProfileType::LIGHT_VIBRANT)]);
  UpdateSearchIcon();
  close_button()->SetImage(
      views::Button::STATE_NORMAL,
      gfx::CreateVectorIcon(views::kIcCloseIcon, kCloseIconSize,
                            search_box_color()));
  search_box()->set_placeholder_text_color(search_box_color());
  UpdateBackgroundColor(background_color());
  SchedulePaint();
}

void SearchBoxView::ContentsChanged(views::Textfield* sender,
                                    const base::string16& new_contents) {
  search_box::SearchBoxViewBase::ContentsChanged(sender, new_contents);
  app_list_view_->SetStateFromSearchBoxView(IsSearchBoxTrimmedQueryEmpty());
}

bool SearchBoxView::HandleKeyEvent(views::Textfield* sender,
                                   const ui::KeyEvent& key_event) {
  if (key_event.type() == ui::ET_KEY_PRESSED &&
      key_event.key_code() == ui::VKEY_RETURN) {
    if (!IsSearchBoxTrimmedQueryEmpty()) {
      // Hitting Enter when focus is on search box opens the first result.
      ui::KeyEvent event(key_event);
      views::View* first_result_view =
          static_cast<ContentsView*>(contents_view())
              ->search_results_page_view()
              ->first_result_view();
      if (first_result_view)
        first_result_view->OnKeyEvent(&event);
      return true;
    }

    if (!is_search_box_active()) {
      SetSearchBoxActive(true);
      return true;
    }
    return false;
  }

  if (CanProcessLeftRightKeyTraversal(key_event))
    return ProcessLeftRightKeyTraversalForTextfield(search_box(), key_event);
  return false;
}

bool SearchBoxView::HandleMouseEvent(views::Textfield* sender,
                                     const ui::MouseEvent& mouse_event) {
  if (mouse_event.type() == ui::ET_MOUSEWHEEL) {
    return app_list_view_->HandleScroll(
        (&mouse_event)->AsMouseWheelEvent()->offset().y(), ui::ET_MOUSEWHEEL);
  }
  return search_box::SearchBoxViewBase::HandleMouseEvent(sender, mouse_event);
}

void SearchBoxView::HintTextChanged() {
  const app_list::SearchBoxModel* search_box_model =
      search_model_->search_box();
  search_box()->set_placeholder_text(search_box_model->hint_text());
  search_box()->SetAccessibleName(search_box_model->accessible_name());
  SchedulePaint();
}

void SearchBoxView::SelectionModelChanged() {
  search_box()->SelectSelectionModel(
      search_model_->search_box()->selection_model());
}

void SearchBoxView::Update() {
  search_box()->SetText(search_model_->search_box()->text());
  UpdateCloseButtonVisisbility();
  NotifyQueryChanged();
}

void SearchBoxView::SearchEngineChanged() {
  UpdateSearchIcon();
}

void SearchBoxView::OnWallpaperColorsChanged() {
  GetWallpaperProminentColors(
      base::BindOnce(&SearchBoxView::OnWallpaperProminentColorsReceived,
                     weak_ptr_factory_.GetWeakPtr()));
}

}  // namespace app_list
