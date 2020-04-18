// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_APP_LIST_VIEWS_APP_LIST_ITEM_VIEW_H_
#define UI_APP_LIST_VIEWS_APP_LIST_ITEM_VIEW_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ash/app_list/model/app_list_item_observer.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string16.h"
#include "base/timer/timer.h"
#include "ui/app_list/app_list_export.h"
#include "ui/app_list/views/app_list_view_context_menu.h"
#include "ui/app_list/views/image_shadow_animator.h"
#include "ui/views/context_menu_controller.h"
#include "ui/views/controls/button/button.h"

namespace views {
class ImageView;
class Label;
class ProgressBar;
}  // namespace views

namespace app_list {

class AppListItem;
class AppListViewDelegate;
class AppsGridView;

class APP_LIST_EXPORT AppListItemView
    : public views::Button,
      public views::ContextMenuController,
      public AppListItemObserver,
      public ImageShadowAnimator::Delegate,
      public AppListViewContextMenu::Delegate {
 public:
  // Internal class name.
  static const char kViewClassName[];

  AppListItemView(AppsGridView* apps_grid_view,
                  AppListItem* item,
                  AppListViewDelegate* delegate);
  ~AppListItemView() override;

  // Set the icon of this image, adding a drop shadow if |has_shadow|.
  void SetIcon(const gfx::ImageSkia& icon);

  void SetItemName(const base::string16& display_name,
                   const base::string16& full_name);
  void SetItemIsInstalling(bool is_installing);
  bool is_highlighted() { return is_highlighted_; }  // for unit test
  void SetItemIsHighlighted(bool is_highlighted);
  void SetItemPercentDownloaded(int percent_downloaded);

  void CancelContextMenu();

  gfx::ImageSkia GetDragImage();
  void OnDragEnded();
  gfx::Point GetDragImageOffset();

  void SetAsAttemptedFolderTarget(bool is_target_folder);

  AppListItem* item() const { return item_weak_; }

  views::ImageView* icon() { return icon_; }

  views::Label* title() { return title_; }

  // In a synchronous drag the item view isn't informed directly of the drag
  // ending, so the runner of the drag should call this.
  void OnSyncDragEnd();

  // Returns the icon bounds relative to AppListItemView.
  const gfx::Rect& GetIconBounds() const;

  // Sets UI state to dragging state.
  void SetDragUIState();

  // Returns the icon bounds for the given |target_bounds| as
  // the assuming bounds of this view.
  gfx::Rect GetIconBoundsForTargetViewBounds(const gfx::Rect& target_bounds);

  // If the item is not in a folder, not highlighted, not being dragged, and not
  // having something dropped onto it, enables subpixel AA for the title.
  void SetTitleSubpixelAA();

  // views::Button overrides:
  void OnGestureEvent(ui::GestureEvent* event) override;

  // views::View overrides:
  bool GetTooltipText(const gfx::Point& p,
                      base::string16* tooltip) const override;

  // ImageShadowAnimator::Delegate overrides:
  void ImageShadowAnimationProgressed(ImageShadowAnimator* animator) override;

 private:
  enum UIState {
    UI_STATE_NORMAL,              // Normal UI (icon + label)
    UI_STATE_DRAGGING,            // Dragging UI (scaled icon only)
    UI_STATE_DROPPING_IN_FOLDER,  // Folder dropping preview UI
  };

  // Get icon from |item_| and schedule background processing.
  void UpdateIcon();

  // Update the tooltip text from |item_|.
  void UpdateTooltip();

  void SetUIState(UIState state);

  // Scales up app icon if |scale_up| is true; otherwise, scale it back to
  // normal size.
  void ScaleAppIcon(bool scale_up);

  // Sets |touch_dragging_| flag and updates UI.
  void SetTouchDragging(bool touch_dragging);
  // Sets |mouse_dragging_| flag and updates UI. Only to be called on
  // |mouse_drag_timer_|.
  void SetMouseDragging(bool mouse_dragging);

  // Invoked when |mouse_drag_timer_| fires to show dragging UI.
  void OnMouseDragTimer();

  // Invoked when |touch_drag_timer_| fires to show dragging UI.
  void OnTouchDragTimer(const gfx::Point& tap_down_location,
                        const gfx::Point& tap_down_root_location);

  // Records the context menu user journey time.
  void OnContextMenuClosed(const base::TimeTicks& open_time);

  // Callback invoked when a context menu is received after calling
  // |AppListViewDelegate::GetContextMenuModel|.
  void OnContextMenuModelReceived(const gfx::Point& point,
                                  ui::MenuSourceType source_type,
                                  std::vector<ash::mojom::MenuItemPtr> menu);

  // views::ContextMenuController overrides:
  void ShowContextMenuForView(views::View* source,
                              const gfx::Point& point,
                              ui::MenuSourceType source_type) override;

  // views::Button overrides:
  void StateChanged(ButtonState old_state) override;
  bool ShouldEnterPushedState(const ui::Event& event) override;
  void PaintButtonContents(gfx::Canvas* canvas) override;

  // views::View overrides:
  const char* GetClassName() const override;
  void Layout() override;
  gfx::Size CalculatePreferredSize() const override;
  bool OnKeyPressed(const ui::KeyEvent& event) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  bool OnMouseDragged(const ui::MouseEvent& event) override;
  void OnFocus() override;
  void OnBlur() override;

  // AppListItemObserver overrides:
  void ItemIconChanged() override;
  void ItemNameChanged() override;
  void ItemIsInstallingChanged() override;
  void ItemPercentDownloadedChanged() override;
  void ItemBeingDestroyed() override;

  // AppListViewContextMenu::Delegate overrides;
  void ExecuteCommand(int command_id, int event_flags) override;

  const bool is_folder_;
  const bool is_in_folder_;

  AppListItem* item_weak_;  // Owned by AppListModel. Can be NULL.

  AppListViewDelegate* delegate_;     // Unowned.
  AppsGridView* apps_grid_view_;      // Parent view, owns this.
  views::ImageView* icon_;            // Strongly typed child view.
  views::Label* title_;               // Strongly typed child view.
  views::ProgressBar* progress_bar_;  // Strongly typed child view.

  AppListViewContextMenu context_menu_;

  UIState ui_state_ = UI_STATE_NORMAL;

  // True if scroll gestures should contribute to dragging.
  bool touch_dragging_ = false;

  // True if the app is enabled for drag/drop operation by mouse.
  bool mouse_dragging_ = false;
  // True if the drag host proxy is crated for mouse dragging.
  bool mouse_drag_proxy_created_ = false;

  std::unique_ptr<ImageShadowAnimator> shadow_animator_;

  bool is_installing_ = false;
  bool is_highlighted_ = false;

  base::string16 tooltip_text_;

  // A timer to defer showing drag UI when mouse is pressed.
  base::OneShotTimer mouse_drag_timer_;
  // A timer to defer showing drag UI when the app item is touch pressed.
  base::OneShotTimer touch_drag_timer_;

  base::WeakPtrFactory<AppListItemView> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(AppListItemView);
};

}  // namespace app_list

#endif  // UI_APP_LIST_VIEWS_APP_LIST_ITEM_VIEW_H_
