// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/bookmarks/bookmark_bubble_view.h"

#include "base/metrics/user_metrics.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "build/buildflag.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/platform_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/bookmarks/bookmark_bubble_observer.h"
#include "chrome/browser/ui/bookmarks/bookmark_editor.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/sync/sync_promo_ui.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/browser/ui/views/harmony/textfield_layout.h"
#include "chrome/browser/ui/views/sync/bubble_sync_promo_view.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "components/signin/core/browser/signin_buildflags.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/widget/widget.h"

#if defined(OS_WIN)
#include "chrome/browser/ui/views/desktop_ios_promotion/desktop_ios_promotion_bubble_view.h"
#include "chrome/browser/ui/views/desktop_ios_promotion/desktop_ios_promotion_footnote_view.h"
#endif

#if BUILDFLAG(ENABLE_DICE_SUPPORT)
#include "chrome/browser/signin/account_consistency_mode_manager.h"
#include "chrome/browser/ui/views/sync/dice_bubble_sync_promo_view.h"
#endif

using base::UserMetricsAction;
using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;

BookmarkBubbleView* BookmarkBubbleView::bookmark_bubble_ = nullptr;

// static
views::Widget* BookmarkBubbleView::ShowBubble(
    views::View* anchor_view,
    const gfx::Rect& anchor_rect,
    gfx::NativeView parent_window,
    bookmarks::BookmarkBubbleObserver* observer,
    std::unique_ptr<BubbleSyncPromoDelegate> delegate,
    Profile* profile,
    const GURL& url,
    bool already_bookmarked) {
  if (bookmark_bubble_)
    return nullptr;

  bookmark_bubble_ =
      new BookmarkBubbleView(anchor_view, observer, std::move(delegate),
                             profile, url, !already_bookmarked);
  // Bookmark bubble should always anchor TOP_RIGHT, but the
  // LocationBarBubbleDelegateView does not know that and may use different
  // arrow anchoring.
  bookmark_bubble_->set_arrow(views::BubbleBorder::TOP_RIGHT);
  if (!anchor_view) {
    bookmark_bubble_->SetAnchorRect(anchor_rect);
    bookmark_bubble_->set_parent_window(parent_window);
  }
  views::Widget* bubble_widget =
      views::BubbleDialogDelegateView::CreateBubble(bookmark_bubble_);
  bubble_widget->Show();
  // Select the entire title textfield contents when the bubble is first shown.
  bookmark_bubble_->name_field_->SelectAll(true);
  bookmark_bubble_->SetArrowPaintType(views::BubbleBorder::PAINT_TRANSPARENT);

  if (bookmark_bubble_->observer_) {
    BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile);
    const BookmarkNode* node = model->GetMostRecentlyAddedUserNodeForURL(url);
    bookmark_bubble_->observer_->OnBookmarkBubbleShown(node);
  }
  return bubble_widget;
}

// static
void BookmarkBubbleView::Hide() {
  if (bookmark_bubble_)
    bookmark_bubble_->GetWidget()->Close();
}

BookmarkBubbleView::~BookmarkBubbleView() {
  if (apply_edits_) {
    ApplyEdits();
  } else if (remove_bookmark_) {
    BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile_);
    const BookmarkNode* node = model->GetMostRecentlyAddedUserNodeForURL(url_);
    if (node)
      model->Remove(node);
  }
}

// ui::DialogModel -------------------------------------------------------------

base::string16 BookmarkBubbleView::GetDialogButtonLabel(
    ui::DialogButton button) const {
#if defined(OS_WIN)
  if (is_showing_ios_promotion_)
    return ios_promo_view_->GetDialogButtonLabel(button);
#endif
  return l10n_util::GetStringUTF16((button == ui::DIALOG_BUTTON_OK)
                                       ? IDS_DONE
                                       : IDS_BOOKMARK_BUBBLE_REMOVE_BOOKMARK);
}

// views::WidgetDelegate -------------------------------------------------------

views::View* BookmarkBubbleView::GetInitiallyFocusedView() {
  return name_field_;
}

base::string16 BookmarkBubbleView::GetWindowTitle() const {
#if defined(OS_WIN)
  if (is_showing_ios_promotion_) {
    return desktop_ios_promotion::GetPromoTitle(
        desktop_ios_promotion::PromotionEntryPoint::BOOKMARKS_BUBBLE);
  }
#endif
  return l10n_util::GetStringUTF16(newly_bookmarked_
                                       ? IDS_BOOKMARK_BUBBLE_PAGE_BOOKMARKED
                                       : IDS_BOOKMARK_BUBBLE_PAGE_BOOKMARK);
}

bool BookmarkBubbleView::ShouldShowCloseButton() const {
  return true;
}

gfx::ImageSkia BookmarkBubbleView::GetWindowIcon() {
#if defined(OS_WIN)
  if (is_showing_ios_promotion_)
    return ios_promo_view_->GetWindowIcon();
#endif
  return gfx::ImageSkia();
}

bool BookmarkBubbleView::ShouldShowWindowIcon() const {
  return is_showing_ios_promotion_;
}

void BookmarkBubbleView::WindowClosing() {
  // We have to reset |bubble_| here, not in our destructor, because we'll be
  // destroyed asynchronously and the shown state will be checked before then.
  DCHECK_EQ(bookmark_bubble_, this);
  bookmark_bubble_ = NULL;
  is_showing_ios_promotion_ = false;

  if (observer_)
    observer_->OnBookmarkBubbleHidden();
}

// views::DialogDelegate -------------------------------------------------------

views::View* BookmarkBubbleView::CreateExtraView() {
  edit_button_ = views::MdTextButton::CreateSecondaryUiButton(
      this, l10n_util::GetStringUTF16(IDS_BOOKMARK_BUBBLE_OPTIONS));
  edit_button_->AddAccelerator(ui::Accelerator(ui::VKEY_E, ui::EF_ALT_DOWN));
  return edit_button_;
}

bool BookmarkBubbleView::GetExtraViewPadding(int* padding) {
  *padding = ChromeLayoutProvider::Get()->GetDistanceMetric(
      DISTANCE_UNRELATED_CONTROL_HORIZONTAL_LARGE);
  return true;
}

views::View* BookmarkBubbleView::CreateFootnoteView() {
#if defined(OS_WIN)
  if (!is_showing_ios_promotion_ &&
      desktop_ios_promotion::IsEligibleForIOSPromotion(
          profile_,
          desktop_ios_promotion::PromotionEntryPoint::BOOKMARKS_FOOTNOTE)) {
    footnote_view_ = new DesktopIOSPromotionFootnoteView(profile_, this);
    return footnote_view_;
  }
#endif
  if (!SyncPromoUI::ShouldShowSyncPromo(profile_))
    return nullptr;

#if BUILDFLAG(ENABLE_DICE_SUPPORT)
  if (AccountConsistencyModeManager::IsDiceEnabledForProfile(profile_)) {
    footnote_view_ = new DiceBubbleSyncPromoView(
        profile_, delegate_.get(),
        signin_metrics::AccessPoint::ACCESS_POINT_BOOKMARK_BUBBLE,
        IDS_BOOKMARK_DICE_PROMO_SIGNIN_MESSAGE,
        IDS_BOOKMARK_DICE_PROMO_SYNC_MESSAGE,
        false /* signin_button_prominent */);
  } else {
    footnote_view_ = new BubbleSyncPromoView(
        delegate_.get(),
        signin_metrics::AccessPoint::ACCESS_POINT_BOOKMARK_BUBBLE,
        IDS_BOOKMARK_SYNC_PROMO_LINK, IDS_BOOKMARK_SYNC_PROMO_MESSAGE);
  }
#else
  footnote_view_ = new BubbleSyncPromoView(
      delegate_.get(),
      signin_metrics::AccessPoint::ACCESS_POINT_BOOKMARK_BUBBLE,
      IDS_BOOKMARK_SYNC_PROMO_LINK, IDS_BOOKMARK_SYNC_PROMO_MESSAGE);
#endif
  return footnote_view_;
}

bool BookmarkBubbleView::Cancel() {
#if defined(OS_WIN)
  if (is_showing_ios_promotion_)
    return ios_promo_view_->Cancel();
#endif
  base::RecordAction(UserMetricsAction("BookmarkBubble_Unstar"));
  // Set this so we remove the bookmark after the window closes.
  remove_bookmark_ = true;
  apply_edits_ = false;
  return true;
}

bool BookmarkBubbleView::Accept() {
#if defined(OS_WIN)
  if (is_showing_ios_promotion_)
    return ios_promo_view_->Accept();
  using desktop_ios_promotion::PromotionEntryPoint;
  if (desktop_ios_promotion::IsEligibleForIOSPromotion(
          profile_, PromotionEntryPoint::BOOKMARKS_BUBBLE)) {
    ShowIOSPromotion(PromotionEntryPoint::BOOKMARKS_BUBBLE);
    return false;
  }
#endif
  return true;
}

bool BookmarkBubbleView::Close() {
  // Allow closing when activation lost. Default would call Accept().
  return true;
}

void BookmarkBubbleView::UpdateButton(views::LabelButton* button,
                                      ui::DialogButton type) {
  LocationBarBubbleDelegateView::UpdateButton(button, type);
  if (type == ui::DIALOG_BUTTON_CANCEL)
    button->AddAccelerator(ui::Accelerator(ui::VKEY_R, ui::EF_ALT_DOWN));
}

// views::View -----------------------------------------------------------------

const char* BookmarkBubbleView::GetClassName() const {
  return "BookmarkBubbleView";
}

// views::ButtonListener -------------------------------------------------------

void BookmarkBubbleView::ButtonPressed(views::Button* sender,
                                       const ui::Event& event) {
  base::RecordAction(UserMetricsAction("BookmarkBubble_Edit"));
  ShowEditor();
}

// views::ComboboxListener -----------------------------------------------------

void BookmarkBubbleView::OnPerformAction(views::Combobox* combobox) {
  if (combobox->selected_index() + 1 == folder_model()->GetItemCount()) {
    base::RecordAction(UserMetricsAction("BookmarkBubble_EditFromCombobox"));
    ShowEditor();
  }
}

// DesktopIOSPromotionFootnoteDelegate -----------------------------------------

void BookmarkBubbleView::OnIOSPromotionFootnoteLinkClicked() {
#if defined(OS_WIN)
  ShowIOSPromotion(
      desktop_ios_promotion::PromotionEntryPoint::FOOTNOTE_FOLLOWUP_BUBBLE);
#endif
}

// views::BubbleDialogDelegateView ---------------------------------------------

void BookmarkBubbleView::Init() {
  using views::GridLayout;

  SetLayoutManager(std::make_unique<views::FillLayout>());
  bookmark_contents_view_ = new views::View();
  GridLayout* layout = bookmark_contents_view_->SetLayoutManager(
      std::make_unique<views::GridLayout>(bookmark_contents_view_));

  constexpr int kColumnId = 0;
  ConfigureTextfieldStack(layout, kColumnId);
  name_field_ = AddFirstTextfieldRow(
      layout, l10n_util::GetStringUTF16(IDS_BOOKMARK_BUBBLE_NAME_LABEL),
      kColumnId);
  name_field_->SetText(GetBookmarkName());
  name_field_->SetAccessibleName(
      l10n_util::GetStringUTF16(IDS_BOOKMARK_AX_BUBBLE_NAME_LABEL));

  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile_);
  auto parent_folder_model = std::make_unique<RecentlyUsedFoldersComboModel>(
      model, model->GetMostRecentlyAddedUserNodeForURL(url_));

  parent_combobox_ = AddComboboxRow(
      layout, l10n_util::GetStringUTF16(IDS_BOOKMARK_BUBBLE_FOLDER_LABEL),
      std::move(parent_folder_model), kColumnId);
  parent_combobox_->set_listener(this);
  parent_combobox_->SetAccessibleName(
      l10n_util::GetStringUTF16(IDS_BOOKMARK_AX_BUBBLE_FOLDER_LABEL));

  AddChildView(bookmark_contents_view_);
}

// Private methods -------------------------------------------------------------

BookmarkBubbleView::BookmarkBubbleView(
    views::View* anchor_view,
    bookmarks::BookmarkBubbleObserver* observer,
    std::unique_ptr<BubbleSyncPromoDelegate> delegate,
    Profile* profile,
    const GURL& url,
    bool newly_bookmarked)
    : LocationBarBubbleDelegateView(anchor_view, gfx::Point(), nullptr),
      observer_(observer),
      delegate_(std::move(delegate)),
      profile_(profile),
      url_(url),
      newly_bookmarked_(newly_bookmarked) {
  chrome::RecordDialogCreation(chrome::DialogIdentifier::BOOKMARK);
  set_margins(ChromeLayoutProvider::Get()->GetDialogInsetsForContentType(
      views::CONTROL, views::CONTROL));
}

base::string16 BookmarkBubbleView::GetBookmarkName() {
  BookmarkModel* bookmark_model =
      BookmarkModelFactory::GetForBrowserContext(profile_);
  const BookmarkNode* node =
      bookmark_model->GetMostRecentlyAddedUserNodeForURL(url_);
  if (node)
    return node->GetTitle();
  else
    NOTREACHED();
  return base::string16();
}

void BookmarkBubbleView::ShowEditor() {
  const BookmarkNode* node =
      BookmarkModelFactory::GetForBrowserContext(profile_)
          ->GetMostRecentlyAddedUserNodeForURL(url_);
  gfx::NativeWindow native_parent =
      anchor_widget() ? anchor_widget()->GetNativeWindow()
                      : platform_util::GetTopLevel(parent_window());
  DCHECK(native_parent);

  Profile* profile = profile_;
  ApplyEdits();
  GetWidget()->Close();

  if (node && native_parent)
    BookmarkEditor::Show(native_parent, profile,
                         BookmarkEditor::EditDetails::EditNode(node),
                         BookmarkEditor::SHOW_TREE);
}

void BookmarkBubbleView::ApplyEdits() {
  // Set this to make sure we don't attempt to apply edits again.
  apply_edits_ = false;

  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile_);
  const BookmarkNode* node = model->GetMostRecentlyAddedUserNodeForURL(url_);
  if (node) {
    const base::string16 new_title = name_field_->text();
    if (new_title != node->GetTitle()) {
      model->SetTitle(node, new_title);
      base::RecordAction(
          UserMetricsAction("BookmarkBubble_ChangeTitleInBubble"));
    }
    folder_model()->MaybeChangeParent(node, parent_combobox_->selected_index());
  }
}

#if defined(OS_WIN)
void BookmarkBubbleView::ShowIOSPromotion(
    desktop_ios_promotion::PromotionEntryPoint entry_point) {
  DCHECK(!is_showing_ios_promotion_);
  edit_button_->SetVisible(false);

  // If edits are to be applied we must do so before removing the content.
  if (apply_edits_)
    ApplyEdits();

  delete bookmark_contents_view_;
  bookmark_contents_view_ = nullptr;
  delete footnote_view_;
  footnote_view_ = nullptr;
  is_showing_ios_promotion_ = true;
  ios_promo_view_ = new DesktopIOSPromotionBubbleView(profile_, entry_point);
  AddChildView(ios_promo_view_);
  set_margins(ChromeLayoutProvider::Get()->GetDialogInsetsForContentType(
      views::TEXT, views::TEXT));
  GetWidget()->UpdateWindowIcon();
  GetWidget()->UpdateWindowTitle();
  DialogModelChanged();
  SizeToContents();
}
#endif
