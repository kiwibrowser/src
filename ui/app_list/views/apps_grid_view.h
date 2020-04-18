// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_APP_LIST_VIEWS_APPS_GRID_VIEW_H_
#define UI_APP_LIST_VIEWS_APPS_GRID_VIEW_H_

#include <stddef.h>

#include <set>
#include <string>
#include <tuple>

#include "ash/app_list/model/app_list_model.h"
#include "ash/app_list/model/app_list_model_observer.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/timer/timer.h"
#include "build/build_config.h"
#include "ui/app_list/app_list_export.h"
#include "ui/app_list/pagination_model.h"
#include "ui/app_list/pagination_model_observer.h"
#include "ui/app_list/views/app_list_view.h"
#include "ui/base/models/list_model_observer.h"
#include "ui/compositor/layer_animation_observer.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/views/animation/bounds_animator.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/view.h"
#include "ui/views/view_model.h"

namespace views {
class ButtonListener;
}

namespace app_list {

namespace test {
class AppsGridViewTestApi;
}

class ApplicationDragAndDropHost;
class AppListItemView;
class AppsGridViewFolderDelegate;
class ContentsView;
class IndicatorChipView;
class SuggestionsContainerView;
class PaginationController;
class PulsingBlockView;
class ExpandArrowView;

// AppsGridView displays a grid for AppListItemList sub model.
class APP_LIST_EXPORT AppsGridView : public views::View,
                                     public views::ButtonListener,
                                     public AppListItemListObserver,
                                     public PaginationModelObserver,
                                     public AppListModelObserver,
                                     public ui::ImplicitAnimationObserver {
 public:
  enum Pointer {
    NONE,
    MOUSE,
    TOUCH,
  };

  AppsGridView(ContentsView* contents_view,
               AppsGridViewFolderDelegate* folder_delegate);
  ~AppsGridView() override;

  // Sets fixed layout parameters. After setting this, CalculateLayout below
  // is no longer called to dynamically choosing those layout params.
  void SetLayout(int cols, int rows_per_page);

  int cols() const { return cols_; }
  int rows_per_page() const { return rows_per_page_; }

  // Returns the size of a tile view including its padding.
  gfx::Size GetTotalTileSize() const;

  // Returns the padding around a tile view.
  gfx::Insets GetTilePadding() const;

  // Returns the size of the entire tile grid without padding.
  gfx::Size GetTileGridSizeWithoutPadding() const;

  // This resets the grid view to a fresh state for showing the app list.
  void ResetForShowApps();

  // All items in this view become unfocusable if |disabled| is true. This is
  // used to trap focus within the folder when it is opened.
  void DisableFocusForShowingActiveFolder(bool disabled);

  // Sets |model| to use. Note this does not take ownership of |model|.
  void SetModel(AppListModel* model);

  // Sets the |item_list| to render. Note this does not take ownership of
  // |item_list|.
  void SetItemList(AppListItemList* item_list);

  void SetSelectedView(AppListItemView* view);
  void ClearSelectedView(AppListItemView* view);
  void ClearAnySelectedView();
  bool IsSelectedView(const AppListItemView* view) const;
  bool has_selected_view() const { return selected_view_ != nullptr; }
  views::View* GetSelectedView() const;

  void InitiateDrag(AppListItemView* view,
                    Pointer pointer,
                    const gfx::Point& location,
                    const gfx::Point& root_location);

  void StartDragAndDropHostDragAfterLongPress(Pointer pointer);
  void TryStartDragAndDropHostDrag(Pointer pointer,
                                   const gfx::Point& grid_location);

  // Called from AppListItemView when it receives a drag event. Returns true
  // if the drag is still happening.
  bool UpdateDragFromItem(Pointer pointer, const ui::LocatedEvent& event);

  // Called when the user is dragging an app. |point| is in grid view
  // coordinates.
  void UpdateDrag(Pointer pointer, const gfx::Point& point);
  void EndDrag(bool cancel);
  bool IsDraggedView(const AppListItemView* view) const;
  void ClearDragState();
  void SetDragViewVisible(bool visible);

  // Set the drag and drop host for application links.
  void SetDragAndDropHostOfCurrentAppList(
      ApplicationDragAndDropHost* drag_and_drop_host);

  // Return true if the |bounds_animator_| is animating |view|.
  bool IsAnimatingView(AppListItemView* view);

  bool has_dragged_view() const { return drag_view_ != nullptr; }
  bool dragging() const { return drag_pointer_ != NONE; }
  const AppListItemView* drag_view() const { return drag_view_; }

  // Gets the PaginationModel used for the grid view.
  PaginationModel* pagination_model() { return &pagination_model_; }

  // Overridden from views::View:
  gfx::Size CalculatePreferredSize() const override;
  void Layout() override;
  bool OnKeyPressed(const ui::KeyEvent& event) override;
  void ViewHierarchyChanged(
      const ViewHierarchyChangedDetails& details) override;
  bool GetDropFormats(
      int* formats,
      std::set<ui::Clipboard::FormatType>* format_types) override;
  bool CanDrop(const OSExchangeData& data) override;
  int OnDragUpdated(const ui::DropTargetEvent& event) override;
  const char* GetClassName() const override;

  // Updates the visibility of app list items according to |app_list_state| and
  // |is_in_drag|.
  void UpdateControlVisibility(AppListViewState app_list_state,
                               bool is_in_drag);

  // Overridden from ui::EventHandler:
  void OnGestureEvent(ui::GestureEvent* event) override;

  // Stops the timer that triggers a page flip during a drag.
  void StopPageFlipTimer();

  // Returns the ideal bounds of an AppListItemView in AppsGridView coordinates.
  const gfx::Rect& GetIdealBounds(AppListItemView* view) const;

  // Returns the item view of the item at |index|.
  AppListItemView* GetItemViewAt(int index) const;

  // Schedules an animation to show or hide the view.
  void ScheduleShowHideAnimation(bool show);

  // Called to initiate drag for reparenting a folder item in root level grid
  // view.
  // Both |drag_view_rect| and |drag_pint| is in the coordinates of root level
  // grid view.
  void InitiateDragFromReparentItemInRootLevelGridView(
      AppListItemView* original_drag_view,
      const gfx::Rect& drag_view_rect,
      const gfx::Point& drag_point,
      bool has_native_drag);

  // Updates drag in the root level grid view when receiving the drag event
  // dispatched from the hidden grid view for reparenting a folder item.
  void UpdateDragFromReparentItem(Pointer pointer,
                                  const gfx::Point& drag_point);

  // Dispatches the drag event from hidden grid view to the top level grid view.
  void DispatchDragEventForReparent(Pointer pointer,
                                    const gfx::Point& drag_point);

  // Handles EndDrag event dispatched from the hidden folder grid view in the
  // root level grid view to end reparenting a folder item.
  // |events_forwarded_to_drag_drop_host|: True if the dragged item is dropped
  // to the drag_drop_host, eg. dropped on shelf.
  // |cancel_drag|: True if the drag is ending because it has been canceled.
  void EndDragFromReparentItemInRootLevel(
      bool events_forwarded_to_drag_drop_host,
      bool cancel_drag);

  // Handles EndDrag event in the hidden folder grid view to end reparenting
  // a folder item.
  void EndDragForReparentInHiddenFolderGridView();

  // Called when the folder item associated with the grid view is removed.
  // The grid view must be inside a folder view.
  void OnFolderItemRemoved();

  // Updates the opacity of all the items in the grid during dragging.
  void UpdateOpacity();

  // Passes scroll information from AppListView to the PaginationController,
  // returns true if this scroll would change pages.
  bool HandleScrollFromAppListView(int offset, ui::EventType type);

  // Returns whether the event is within a slot that is occupied by an app icon.
  bool IsEventNearAppIcon(const ui::LocatedEvent& event);

  // Returns the first app list item view in the selected page in the folder.
  AppListItemView* GetCurrentPageFirstItemViewInFolder();

  // Returns the last app list item view in the selected page in the folder.
  AppListItemView* GetCurrentPageLastItemViewInFolder();

  // Return the view model for test purposes.
  const views::ViewModelT<AppListItemView>* view_model_for_test() const {
    return &view_model_;
  }

  // For test: Return if the drag and drop handler was set.
  bool has_drag_and_drop_host_for_test() {
    return nullptr != drag_and_drop_host_;
  }

  // For test: Return if the drag and drop operation gets dispatched.
  bool forward_events_to_drag_and_drop_host_for_test() {
    return forward_events_to_drag_and_drop_host_;
  }

  void set_folder_delegate(AppsGridViewFolderDelegate* folder_delegate) {
    folder_delegate_ = folder_delegate;
  }

  bool is_in_folder() const { return !!folder_delegate_; }

  AppListItemView* activated_folder_item_view() const {
    return activated_folder_item_view_;
  }

  const AppListModel* model() const { return model_; }

  SuggestionsContainerView* suggestions_container_for_test() const {
    return suggestions_container_;
  }

  void set_page_flip_delay_in_ms_for_testing(int page_flip_delay_in_ms) {
    page_flip_delay_in_ms_ = page_flip_delay_in_ms;
  }

  ExpandArrowView* expand_arrow_view_for_test() const {
    return expand_arrow_view_;
  }

 private:
  class FadeoutLayerDelegate;
  friend class test::AppsGridViewTestApi;

  enum DropAttempt {
    DROP_FOR_NONE,
    DROP_FOR_REORDER,
    DROP_FOR_FOLDER,
  };

  // Represents the index to an item view in the grid.
  struct Index {
    Index() : page(-1), slot(-1) {}
    Index(int page, int slot) : page(page), slot(slot) {}

    bool operator==(const Index& other) const {
      return page == other.page && slot == other.slot;
    }
    bool operator!=(const Index& other) const {
      return page != other.page || slot != other.slot;
    }
    bool operator<(const Index& other) const {
      return std::tie(page, slot) < std::tie(other.page, other.slot);
    }

    int page;  // Which page an item view is on.
    int slot;  // Which slot in the page an item view is in.
  };

  // Updates suggestions from app list model.
  void UpdateSuggestions();

  // Returns all apps tiles per page based on |page|.
  int TilesPerPage(int page) const;

  // Updates from model.
  void Update();

  // Updates page splits for item views.
  void UpdatePaging();

  // Updates the number of pulsing block views based on AppListModel status and
  // number of apps.
  void UpdatePulsingBlockViews();

  AppListItemView* CreateViewForItemAtIndex(size_t index);

  // Returns true if the event was handled by the pagination controller.
  bool HandleScroll(int offset, ui::EventType type);

  // Convert between the model index and the visual index. The model index
  // is the index of the item in AppListModel. The visual index is the Index
  // struct above with page/slot info of where to display the item.
  Index GetIndexFromModelIndex(int model_index) const;
  int GetModelIndexFromIndex(const Index& index) const;

  // Ensures the view is visible. Note that if there is a running page
  // transition, this does nothing.
  void EnsureViewVisible(const Index& index);

  void SetSelectedItemByIndex(const Index& index);
  bool IsValidIndex(const Index& index) const;

  Index GetIndexOfView(const AppListItemView* view) const;
  AppListItemView* GetViewAtIndex(const Index& index) const;

  // Gets the index of the AppListItemView at the end of the view model.
  Index GetLastViewIndex() const;

  // Calculates the offset for |page_of_view| based on current page and
  // transition target page.
  const gfx::Vector2d CalculateTransitionOffset(int page_of_view) const;

  void CalculateIdealBounds();
  void AnimateToIdealBounds();

  // Invoked when the given |view|'s current bounds and target bounds are on
  // different rows. To avoid moving diagonally, |view| would be put into a
  // slot prior |target| and fade in while moving to |target|. In the meanwhile,
  // a layer copy of |view| would start at |current| and fade out while moving
  // to succeeding slot of |current|. |animate_current| controls whether to run
  // fading out animation from |current|. |animate_target| controls whether to
  // run fading in animation to |target|.
  void AnimationBetweenRows(AppListItemView* view,
                            bool animate_current,
                            const gfx::Rect& current,
                            bool animate_target,
                            const gfx::Rect& target);

  // Extracts drag location info from |root_location| into |drag_point|.
  void ExtractDragLocation(const gfx::Point& root_location,
                           gfx::Point* drag_point);

  // Updates |reorder_drop_target_|, |folder_drop_target_| and |drop_attempt_|
  // based on |drag_view_|'s position.
  void CalculateDropTarget();

  // If |point| is a valid folder drop target, returns true and sets
  // |drop_target| to the index of the view to do a folder drop for.
  bool CalculateFolderDropTarget(const gfx::Point& point,
                                 Index* drop_target) const;

  // Calculates the reorder target |point| and sets |drop_target| to the index
  // of the view to reorder.
  void CalculateReorderDropTarget(const gfx::Point& point,
                                  Index* drop_target) const;

  // Prepares |drag_and_drop_host_| for dragging. |grid_location| contains
  // the drag point in this grid view's coordinates.
  void StartDragAndDropHostDrag(const gfx::Point& grid_location);

  // Dispatch the drag and drop update event to the dnd host (if needed).
  void DispatchDragEventToDragAndDropHost(
      const gfx::Point& location_in_screen_coordinates);

  // Starts the page flip timer if |drag_point| is in left/right side page flip
  // zone or is over page switcher.
  void MaybeStartPageFlipTimer(const gfx::Point& drag_point);

  // Invoked when |page_flip_timer_| fires.
  void OnPageFlipTimer();

  // Updates |model_| to move item represented by |item_view| to |target| slot.
  void MoveItemInModel(AppListItemView* item_view, const Index& target);

  // Updates |model_| to move item represented by |item_view| into a folder
  // containing item located at |target| slot, also update |view_model_| for
  // the related view changes.
  void MoveItemToFolder(AppListItemView* item_view, const Index& target);

  // Updates both data model and view_model_ for re-parenting a folder item to a
  // new position in top level item list.
  void ReparentItemForReorder(AppListItemView* item_view, const Index& target);

  // Updates both data model and view_model_ for re-parenting a folder item
  // to anther folder target. Returns whether the reparent succeeded.
  bool ReparentItemToAnotherFolder(AppListItemView* item_view,
                                   const Index& target);

  // If there is only 1 item left in the source folder after reparenting an item
  // from it, updates both data model and view_model_ for removing last item
  // from the source folder and removes the source folder.
  void RemoveLastItemFromReparentItemFolderIfNecessary(
      const std::string& source_folder_id);

  // If user does not drop the re-parenting folder item to any valid target,
  // cancel the re-parenting action, let the item go back to its original
  // parent folder with UI animation.
  void CancelFolderItemReparent(AppListItemView* drag_item_view);

  // Cancels any context menus showing for app items on the current page.
  void CancelContextMenusOnCurrentPage();

  // Removes the AppListItemView at |index| in |view_model_| and deletes it.
  void DeleteItemViewAtIndex(int index);

  // Returns true if |point| lies within the bounds of this grid view plus a
  // buffer area surrounding it that can trigger drop target change.
  bool IsPointWithinDragBuffer(const gfx::Point& point) const;

  // Returns true if |point| lies within the bounds of this grid view plus a
  // buffer area surrounding it that can trigger page flip.
  bool IsPointWithinPageFlipBuffer(const gfx::Point& point) const;

  // Returns whether |point| is in the bottom drag buffer, and not over the
  // shelf.
  bool IsPointWithinBottomDragBuffer(const gfx::Point& point) const;

  // Overridden from views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  // Overridden from AppListItemListObserver:
  void OnListItemAdded(size_t index, AppListItem* item) override;
  void OnListItemRemoved(size_t index, AppListItem* item) override;
  void OnListItemMoved(size_t from_index,
                       size_t to_index,
                       AppListItem* item) override;
  void OnAppListItemHighlight(size_t index, bool highlight) override;

  // Overridden from PaginationModelObserver:
  void TotalPagesChanged() override;
  void SelectedPageChanged(int old_selected, int new_selected) override;
  void TransitionStarted() override;
  void TransitionChanged() override;
  void TransitionEnded() override;

  // Overridden from AppListModelObserver:
  void OnAppListModelStatusChanged() override;

  // ui::ImplicitAnimationObserver overrides:
  void OnImplicitAnimationsCompleted() override;

  // Hide a given view temporarily without losing (mouse) events and / or
  // changing the size of it. If |immediate| is set the change will be
  // immediately applied - otherwise it will change gradually.
  // If |hide| is set the view will get hidden, otherwise it gets shown.
  void SetViewHidden(AppListItemView* view, bool hide, bool immediate);

  // Whether the folder drag-and-drop UI should be enabled.
  bool EnableFolderDragDropUI();

  // Returns the size of the entire tile grid.
  gfx::Size GetTileGridSize() const;

  // Returns the slot number which the given |point| falls into or the closest
  // slot if |point| is outside the page's bounds.
  Index GetNearestTileIndexForPoint(const gfx::Point& point) const;

  // Gets height on top of the all apps tiles for |page|.
  int GetHeightOnTopOfAllAppsTiles(int page) const;

  // Gets the bounds of the tile located at |index|, where |index| contains the
  // page/slot info.
  gfx::Rect GetExpectedTileBounds(const Index& index) const;

  // Gets the item view currently displayed at |slot| on the current page. If
  // there is no item displayed at |slot|, returns NULL. Note that this finds an
  // item *displayed* at a slot, which may differ from the item's location in
  // the model (as it may have been temporarily moved during a drag operation).
  AppListItemView* GetViewDisplayedAtSlotOnCurrentPage(int slot) const;

  // Sets state of the view with |target_index| to |is_target_folder| for
  // dropping |drag_view_|.
  void SetAsFolderDroppingTarget(const Index& target_index,
                                 bool is_target_folder);

  // Invoked when |reorder_timer_| fires to show re-order preview UI.
  void OnReorderTimer();

  // Invoked when |folder_item_reparent_timer_| fires.
  void OnFolderItemReparentTimer();

  // Invoked when |folder_dropping_timer_| fires to show folder dropping
  // preview UI.
  void OnFolderDroppingTimer();

  // Updates drag state for dragging inside a folder's grid view.
  void UpdateDragStateInsideFolder(Pointer pointer,
                                   const gfx::Point& drag_point);

  // Returns true if drag event is happening in the root level AppsGridView
  // for reparenting a folder item.
  bool IsDraggingForReparentInRootLevelGridView() const;

  // Returns true if drag event is happening in the hidden AppsGridView of the
  // folder during reparenting a folder item.
  bool IsDraggingForReparentInHiddenGridView() const;

  // Returns the target icon bounds for |drag_item_view| to fly back
  // to its parent |folder_item_view| in animation.
  gfx::Rect GetTargetIconRectInFolder(AppListItemView* drag_item_view,
                                      AppListItemView* folder_item_view);

  // Returns true if the grid view is under an OEM folder.
  bool IsUnderOEMFolder();

  // Handle focus movement triggered by arrow up and down in PEEKING state.
  bool HandleFocusMovementInPeekingState(bool arrow_up);

  // Handle focus movement triggered by arrow up and down in FULLSCREEN_ALL_APPS
  // state.
  bool HandleFocusMovementInFullscreenAllAppsState(bool arrow_up);

  // Update number of columns and rows for apps within a folder.
  void UpdateColsAndRowsForFolder();

  // Gets the index offset of an AppLitItemView as a child in this view. This is
  // used to correct the order of the item views after moving the items into the
  // new positions. As a result, a focus movement bug is resolved. (See
  // https://crbug.com/791758)
  size_t GetAppListItemViewIndexOffset() const;

  AppListModel* model_ = nullptr;         // Owned by AppListView.
  AppListItemList* item_list_ = nullptr;  // Not owned.

  // This can be NULL. Only grid views inside folders have a folder delegate.
  AppsGridViewFolderDelegate* folder_delegate_ = nullptr;

  PaginationModel pagination_model_;
  // Must appear after |pagination_model_|.
  std::unique_ptr<PaginationController> pagination_controller_;

  // Created by AppListMainView, owned by views hierarchy.
  ContentsView* contents_view_ = nullptr;

  // Views below are owned by views hierarchy.
  SuggestionsContainerView* suggestions_container_ = nullptr;
  IndicatorChipView* all_apps_indicator_ = nullptr;
  ExpandArrowView* expand_arrow_view_ = nullptr;

  int cols_ = 0;
  int rows_per_page_ = 0;

  // List of app item views. There is a view per item in |model_|.
  views::ViewModelT<AppListItemView> view_model_;

  // List of pulsing block views.
  views::ViewModelT<PulsingBlockView> pulsing_blocks_model_;

  AppListItemView* selected_view_ = nullptr;

  AppListItemView* drag_view_ = nullptr;

  // The index of the drag_view_ when the drag starts.
  Index drag_view_init_index_;

  // The point where the drag started in AppListItemView coordinates.
  gfx::Point drag_view_offset_;

  // The point where the drag started in GridView coordinates.
  gfx::Point drag_start_grid_view_;

  // The location of |drag_view_| when the drag started.
  gfx::Point drag_view_start_;

  // Page the drag started on.
  int drag_start_page_ = -1;

  Pointer drag_pointer_ = NONE;

  // The most recent reorder drop target.
  Index reorder_drop_target_;

  // The most recent folder drop target.
  Index folder_drop_target_;

  // The index where an empty slot has been left as a placeholder for the
  // reorder drop target. This updates when the reorder animation triggers.
  Index reorder_placeholder_;

  // The current action that ending a drag will perform.
  DropAttempt drop_attempt_ = DROP_FOR_NONE;

  // Timer for re-ordering the |drop_target_| and |drag_view_|.
  base::OneShotTimer reorder_timer_;

  // Timer for dropping |drag_view_| into the folder containing
  // the |drop_target_|.
  base::OneShotTimer folder_dropping_timer_;

  // Timer for dragging a folder item out of folder container ink bubble.
  base::OneShotTimer folder_item_reparent_timer_;

  // An application target drag and drop host which accepts dnd operations.
  ApplicationDragAndDropHost* drag_and_drop_host_ = nullptr;

  // The drag operation is currently inside the dnd host and events get
  // forwarded.
  bool forward_events_to_drag_and_drop_host_ = false;

  // Last mouse drag location in this view's coordinates.
  gfx::Point last_drag_point_;

  // Timer to auto flip page when dragging an item near the left/right edges.
  base::OneShotTimer page_flip_timer_;

  // Target page to switch to when |page_flip_timer_| fires.
  int page_flip_target_ = -1;

  views::BoundsAnimator bounds_animator_;

  // The most recent activated folder item view.
  AppListItemView* activated_folder_item_view_ = nullptr;

  // Tracks if drag_view_ is dragged out of the folder container bubble
  // when dragging a item inside a folder.
  bool drag_out_of_folder_container_ = false;

  // True if the drag_view_ item is a folder item being dragged for reparenting.
  bool dragging_for_reparent_item_ = false;

  std::unique_ptr<FadeoutLayerDelegate> fadeout_layer_delegate_;

  // Delay in milliseconds of when |page_flip_timer_| should fire after user
  // drags an item near the edges.
  int page_flip_delay_in_ms_;

  // True if it is the end gesture from shelf dragging.
  bool is_end_gesture_ = false;

  // The compositor frame number when animation starts.
  int pagination_animation_start_frame_number_;

  DISALLOW_COPY_AND_ASSIGN(AppsGridView);
};

}  // namespace app_list

#endif  // UI_APP_LIST_VIEWS_APPS_GRID_VIEW_H_
