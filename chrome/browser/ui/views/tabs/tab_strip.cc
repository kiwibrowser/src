// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/tabs/tab_strip.h"

#include <stddef.h>

#include <algorithm>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "base/compiler_specific.h"
#include "base/containers/adapters.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "cc/paint/paint_flags.h"
#include "chrome/browser/defaults.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/view_ids.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/tabs/new_tab_button.h"
#include "chrome/browser/ui/views/tabs/stacked_tab_strip_layout.h"
#include "chrome/browser/ui/views/tabs/tab.h"
#include "chrome/browser/ui/views/tabs/tab_drag_controller.h"
#include "chrome/browser/ui/views/tabs/tab_strip_controller.h"
#include "chrome/browser/ui/views/tabs/tab_strip_layout.h"
#include "chrome/browser/ui/views/tabs/tab_strip_observer.h"
#include "chrome/browser/ui/views/touch_uma/touch_uma.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/theme_resources.h"
#include "third_party/skia/include/core/SkColorFilter.h"
#include "third_party/skia/include/effects/SkLayerDrawLooper.h"
#include "third_party/skia/include/pathops/SkPathOps.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/default_theme_provider.h"
#include "ui/base/dragdrop/drag_drop_types.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/models/list_selection_model.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/compositor/compositing_recorder.h"
#include "ui/compositor/paint_recorder.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/animation/animation_container.h"
#include "ui/gfx/animation/throb_animation.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/path.h"
#include "ui/gfx/scoped_canvas.h"
#include "ui/gfx/skia_util.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/masked_targeter_delegate.h"
#include "ui/views/mouse_watcher_view_host.h"
#include "ui/views/rect_based_targeting_utils.h"
#include "ui/views/view_model_utils.h"
#include "ui/views/view_targeter.h"
#include "ui/views/widget/root_view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/non_client_view.h"

#if defined(OS_WIN)
#include "base/win/windows_version.h"
#include "ui/display/win/screen_win.h"
#include "ui/gfx/win/hwnd_util.h"
#include "ui/views/win/hwnd_util.h"
#endif

using base::UserMetricsAction;
using ui::DropTargetEvent;
using MD = ui::MaterialDesignController;

namespace {

const int kTabStripAnimationVSlop = 40;

// Inverse ratio of the width of a tab edge to the width of the tab. When
// hovering over the left or right edge of a tab, the drop indicator will
// point between tabs.
const int kTabEdgeRatioInverse = 4;

// Size of the drop indicator.
int drop_indicator_width;
int drop_indicator_height;

// Max number of stacked tabs.
const int kMaxStackedCount = 4;

// Padding between stacked tabs.
const int kStackedPadding = 6;

// See UpdateLayoutTypeFromMouseEvent() for a description of these.
#if !defined(OS_CHROMEOS)
const int kMouseMoveTimeMS = 200;
const int kMouseMoveCountBeforeConsiderReal = 3;
#endif

// Amount of time we delay before resizing after a close from a touch.
const int kTouchResizeLayoutTimeMS = 2000;

#if defined(OS_MACOSX)
const int kPinnedToNonPinnedOffset = 2;
#else
const int kPinnedToNonPinnedOffset = 3;
#endif

TabSizeInfo* g_tab_size_info = nullptr;

enum NewTabButtonPosition {
  LEADING,     // Pinned to the leading edge of the tabstrip region.
  AFTER_TABS,  // After the last tab.
  TRAILING,    // Pinned to the trailing edge of the tabstrip region.
};

// Animation delegate used for any automatic tab movement.  Hides the tab if it
// is not fully visible within the tabstrip area, to prevent overflow clipping.
class TabAnimationDelegate : public gfx::AnimationDelegate {
 public:
  TabAnimationDelegate(TabStrip* tab_strip, Tab* tab);
  ~TabAnimationDelegate() override;

  void AnimationProgressed(const gfx::Animation* animation) override;

 protected:
  TabStrip* tab_strip() { return tab_strip_; }
  Tab* tab() { return tab_; }

 private:
  TabStrip* const tab_strip_;
  Tab* const tab_;

  DISALLOW_COPY_AND_ASSIGN(TabAnimationDelegate);
};

TabAnimationDelegate::TabAnimationDelegate(TabStrip* tab_strip, Tab* tab)
    : tab_strip_(tab_strip), tab_(tab) {}

TabAnimationDelegate::~TabAnimationDelegate() {}

void TabAnimationDelegate::AnimationProgressed(
    const gfx::Animation* animation) {
  tab_->SetVisible(tab_strip_->ShouldTabBeVisible(tab_));
}

// Animation delegate used when a dragged tab is released. When done sets the
// dragging state to false.
class ResetDraggingStateDelegate : public TabAnimationDelegate {
 public:
  ResetDraggingStateDelegate(TabStrip* tab_strip, Tab* tab);
  ~ResetDraggingStateDelegate() override;

  void AnimationEnded(const gfx::Animation* animation) override;
  void AnimationCanceled(const gfx::Animation* animation) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ResetDraggingStateDelegate);
};

ResetDraggingStateDelegate::ResetDraggingStateDelegate(TabStrip* tab_strip,
                                                       Tab* tab)
    : TabAnimationDelegate(tab_strip, tab) {}

ResetDraggingStateDelegate::~ResetDraggingStateDelegate() {}

void ResetDraggingStateDelegate::AnimationEnded(
    const gfx::Animation* animation) {
  tab()->set_dragging(false);
  AnimationProgressed(animation);  // Forces tab visibility to update.
}

void ResetDraggingStateDelegate::AnimationCanceled(
    const gfx::Animation* animation) {
  AnimationEnded(animation);
}

// If |dest| contains the point |point_in_source| the event handler from |dest|
// is returned. Otherwise NULL is returned.
views::View* ConvertPointToViewAndGetEventHandler(
    views::View* source,
    views::View* dest,
    const gfx::Point& point_in_source) {
  gfx::Point dest_point(point_in_source);
  views::View::ConvertPointToTarget(source, dest, &dest_point);
  return dest->HitTestPoint(dest_point)
             ? dest->GetEventHandlerForPoint(dest_point)
             : NULL;
}

// Gets a tooltip handler for |point_in_source| from |dest|. Note that |dest|
// should return NULL if it does not contain the point.
views::View* ConvertPointToViewAndGetTooltipHandler(
    views::View* source,
    views::View* dest,
    const gfx::Point& point_in_source) {
  gfx::Point dest_point(point_in_source);
  views::View::ConvertPointToTarget(source, dest, &dest_point);
  return dest->GetTooltipHandlerForPoint(dest_point);
}

TabDragController::EventSource EventSourceFromEvent(
    const ui::LocatedEvent& event) {
  return event.IsGestureEvent() ? TabDragController::EVENT_SOURCE_TOUCH
                                : TabDragController::EVENT_SOURCE_MOUSE;
}

const TabSizeInfo& GetTabSizeInfo() {
  if (g_tab_size_info)
    return *g_tab_size_info;

  g_tab_size_info = new TabSizeInfo;
  g_tab_size_info->pinned_tab_width = Tab::GetPinnedWidth();
  g_tab_size_info->min_active_width = Tab::GetMinimumActiveSize().width();
  g_tab_size_info->min_inactive_width = Tab::GetMinimumInactiveSize().width();
  g_tab_size_info->max_size = Tab::GetStandardSize();
  g_tab_size_info->tab_overlap = Tab::GetOverlap();
  g_tab_size_info->pinned_to_normal_offset =
      TabStrip::GetPinnedToNonPinnedOffset();
  return *g_tab_size_info;
}

}  // namespace

///////////////////////////////////////////////////////////////////////////////
// TabStrip::RemoveTabDelegate
//
// AnimationDelegate used when removing a tab. Does the necessary cleanup when
// done.
class TabStrip::RemoveTabDelegate : public TabAnimationDelegate {
 public:
  RemoveTabDelegate(TabStrip* tab_strip, Tab* tab);

  void AnimationEnded(const gfx::Animation* animation) override;
  void AnimationCanceled(const gfx::Animation* animation) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(RemoveTabDelegate);
};

TabStrip::RemoveTabDelegate::RemoveTabDelegate(TabStrip* tab_strip, Tab* tab)
    : TabAnimationDelegate(tab_strip, tab) {}

void TabStrip::RemoveTabDelegate::AnimationEnded(
    const gfx::Animation* animation) {
  DCHECK(tab()->closing());
  tab_strip()->RemoveAndDeleteTab(tab());

  // Send the Container a message to simulate a mouse moved event at the current
  // mouse position. This tickles the Tab the mouse is currently over to show
  // the "hot" state of the close button.  Note that this is not required (and
  // indeed may crash!) for removes spawned by non-mouse closes and
  // drag-detaches.
  if (!tab_strip()->IsDragSessionActive() &&
      tab_strip()->ShouldHighlightCloseButtonAfterRemove()) {
    // The widget can apparently be null during shutdown.
    views::Widget* widget = tab_strip()->GetWidget();
    if (widget)
      widget->SynthesizeMouseMoveEvent();
  }
}

void TabStrip::RemoveTabDelegate::AnimationCanceled(
    const gfx::Animation* animation) {
  AnimationEnded(animation);
}

///////////////////////////////////////////////////////////////////////////////
// TabStrip, public:

TabStrip::TabStrip(std::unique_ptr<TabStripController> controller)
    : controller_(std::move(controller)),
      current_inactive_width_(Tab::GetStandardSize().width()),
      current_active_width_(Tab::GetStandardSize().width()),
      animation_container_(new gfx::AnimationContainer()),
      bounds_animator_(this) {
  Init();
  SetEventTargeter(std::make_unique<views::ViewTargeter>(this));
}

TabStrip::~TabStrip() {
  // The animations may reference the tabs. Shut down the animation before we
  // delete the tabs.
  StopAnimating(false);

  DestroyDragController();

  // Make sure we unhook ourselves as a message loop observer so that we don't
  // crash in the case where the user closes the window after closing a tab
  // but before moving the mouse.
  RemoveMessageLoopObserver();

  // The children (tabs) may callback to us from their destructor. Delete them
  // so that if they call back we aren't in a weird state.
  RemoveAllChildViews(true);
}

// static
bool TabStrip::ShouldDrawStrokes() {
  return !MD::IsRefreshUi();
}

// static
int TabStrip::GetPinnedToNonPinnedOffset() {
  return MD::IsRefreshUi() ? 0 : kPinnedToNonPinnedOffset;
}

void TabStrip::AddObserver(TabStripObserver* observer) {
  observers_.AddObserver(observer);
}

void TabStrip::RemoveObserver(TabStripObserver* observer) {
  observers_.RemoveObserver(observer);
}

int TabStrip::GetTabsMaxX() const {
  // There might be no tabs yet during startup.
  return tab_count() ? ideal_bounds(tab_count() - 1).right() : 0;
}

void TabStrip::SetBackgroundOffset(const gfx::Point& offset) {
  for (int i = 0; i < tab_count(); ++i)
    tab_at(i)->set_background_offset(offset);
  new_tab_button_->set_background_offset(offset);
}

bool TabStrip::IsRectInWindowCaption(const gfx::Rect& rect) {
  views::View* v = GetEventHandlerForRect(rect);

  // If there is no control at this location, claim the hit was in the title
  // bar to get a move action.
  if (v == this)
    return true;

  // Check to see if the rect intersects the non-button parts of the new tab
  // button. The button has a non-rectangular shape, so if it's not in the
  // visual portions of the button we treat it as a click to the caption.
  gfx::RectF rect_in_new_tab_coords_f(rect);
  View::ConvertRectToTarget(this, new_tab_button_, &rect_in_new_tab_coords_f);
  gfx::Rect rect_in_new_tab_coords =
      gfx::ToEnclosingRect(rect_in_new_tab_coords_f);
  if (new_tab_button_->GetLocalBounds().Intersects(rect_in_new_tab_coords) &&
      !new_tab_button_->HitTestRect(rect_in_new_tab_coords))
    return true;

  // All other regions, including the new Tab button, should be considered part
  // of the containing Window's client area so that regular events can be
  // processed for them.
  return false;
}

bool TabStrip::IsPositionInWindowCaption(const gfx::Point& point) {
  return IsRectInWindowCaption(gfx::Rect(point, gfx::Size(1, 1)));
}

bool TabStrip::IsTabStripCloseable() const {
  return !IsDragSessionActive();
}

bool TabStrip::IsTabStripEditable() const {
  return !IsDragSessionActive() && !IsActiveDropTarget();
}

bool TabStrip::IsTabCrashed(int tab_index) const {
  return tab_at(tab_index)->data().IsCrashed();
}

bool TabStrip::TabHasNetworkError(int tab_index) const {
  return tab_at(tab_index)->data().network_state == TabNetworkState::kError;
}

TabAlertState TabStrip::GetTabAlertState(int tab_index) const {
  return tab_at(tab_index)->data().alert_state;
}

void TabStrip::UpdateLoadingAnimations() {
  for (int i = 0; i < tab_count(); i++)
    tab_at(i)->StepLoadingAnimation();
}

void TabStrip::SetStackedLayout(bool stacked_layout) {
  if (stacked_layout == stacked_layout_)
    return;

  const int active_index = controller_->GetActiveIndex();
  int active_center = 0;
  if (active_index != -1) {
    active_center =
        ideal_bounds(active_index).x() + ideal_bounds(active_index).width() / 2;
  }
  stacked_layout_ = stacked_layout;
  SetResetToShrinkOnExit(false);
  SwapLayoutIfNecessary();
  // When transitioning to stacked try to keep the active tab centered.
  if (touch_layout_ && active_index != -1) {
    touch_layout_->SetActiveTabLocation(active_center -
                                        ideal_bounds(active_index).width() / 2);
    AnimateToIdealBounds();
  }

  for (int i = 0; i < tab_count(); ++i)
    tab_at(i)->HideCloseButtonForInactiveTabsChanged();
}

bool TabStrip::SizeTabButtonToTopOfTabStrip() {
  // Extend the button to the screen edge in maximized and immersive fullscreen.
  views::Widget* widget = GetWidget();
  return browser_defaults::kSizeTabButtonToTopOfTabStrip ||
         (widget && (widget->IsMaximized() || widget->IsFullscreen()));
}

void TabStrip::StartHighlight(int model_index) {
  tab_at(model_index)->StartPulse();
}

void TabStrip::StopAllHighlighting() {
  for (int i = 0; i < tab_count(); ++i)
    tab_at(i)->StopPulse();
}

void TabStrip::AddTabAt(int model_index, TabRendererData data, bool is_active) {
  Tab* tab = new Tab(this, animation_container_.get());
  AddChildView(tab);
  const bool pinned = data.pinned;
  tab->SetData(std::move(data));
  UpdateTabsClosingMap(model_index, 1);
  tabs_.Add(tab, model_index);
  selected_tabs_.IncrementFrom(model_index);

  if (touch_layout_) {
    GenerateIdealBoundsForPinnedTabs(NULL);
    int add_types = 0;
    if (pinned)
      add_types |= StackedTabStripLayout::kAddTypePinned;
    if (is_active)
      add_types |= StackedTabStripLayout::kAddTypeActive;
    touch_layout_->AddTab(model_index, add_types, GetStartXForNormalTabs());
  }

  // Don't animate the first tab, it looks weird, and don't animate anything
  // if the containing window isn't visible yet.
  if (tab_count() > 1 && GetWidget() && GetWidget()->IsVisible())
    StartInsertTabAnimation(model_index);
  else
    DoLayout();

  SwapLayoutIfNecessary();

  for (TabStripObserver& observer : observers_)
    observer.OnTabAdded(model_index);

  // Stop dragging when a new tab is added and dragging a window. Doing
  // otherwise results in a confusing state if the user attempts to reattach. We
  // could allow this and make TabDragController update itself during the add,
  // but this comes up infrequently enough that it's not worth the complexity.
  //
  // At the start of AddTabAt() the model and tabs are out sync. Any queries to
  // find a tab given a model index can go off the end of |tabs_|. As such, it
  // is important that we complete the drag *after* adding the tab so that the
  // model and tabstrip are in sync.
  if (drag_controller_.get() && !drag_controller_->is_mutating() &&
      drag_controller_->is_dragging_window()) {
    EndDrag(END_DRAG_COMPLETE);
  }
}

void TabStrip::MoveTab(int from_model_index,
                       int to_model_index,
                       TabRendererData data) {
  DCHECK_GT(tabs_.view_size(), 0);
  const Tab* last_tab = GetLastVisibleTab();
  tab_at(from_model_index)->SetData(std::move(data));
  if (touch_layout_) {
    tabs_.MoveViewOnly(from_model_index, to_model_index);
    int pinned_count = 0;
    GenerateIdealBoundsForPinnedTabs(&pinned_count);
    touch_layout_->MoveTab(from_model_index, to_model_index,
                           controller_->GetActiveIndex(),
                           GetStartXForNormalTabs(), pinned_count);
  } else {
    tabs_.Move(from_model_index, to_model_index);
  }
  selected_tabs_.Move(from_model_index, to_model_index, /*length=*/1);

  StartMoveTabAnimation();
  if (MayHideNewTabButtonWhileDragging() &&
      TabDragController::IsAttachedTo(this) &&
      (last_tab != GetLastVisibleTab() || last_tab->dragging())) {
    new_tab_button_->SetVisible(false);
  }
  SwapLayoutIfNecessary();

  for (TabStripObserver& observer : observers_)
    observer.OnTabMoved(from_model_index, to_model_index);
}

void TabStrip::RemoveTabAt(content::WebContents* contents, int model_index) {
  if (touch_layout_) {
    Tab* tab = tab_at(model_index);
    tab->set_closing(true);
    int old_x = tabs_.ideal_bounds(model_index).x();
    // We still need to paint the tab until we actually remove it. Put it in
    // tabs_closing_map_ so we can find it.
    RemoveTabFromViewModel(model_index);
    touch_layout_->RemoveTab(model_index,
                             GenerateIdealBoundsForPinnedTabs(NULL), old_x);
    ScheduleRemoveTabAnimation(tab);
  } else if (in_tab_close_ && model_index != GetModelCount()) {
    StartMouseInitiatedRemoveTabAnimation(model_index);
  } else {
    StartRemoveTabAnimation(model_index);
  }
  SwapLayoutIfNecessary();

  for (TabStripObserver& observer : observers_)
    observer.OnTabRemoved(model_index);

  // Stop dragging when a new tab is removed and dragging a window. Doing
  // otherwise results in a confusing state if the user attempts to reattach. We
  // could allow this and make TabDragController update itself during the
  // remove operation, but this comes up infrequently enough that it's not worth
  // the complexity.
  //
  // At the start of RemoveTabAt() the model and tabs are out sync. Any queries
  // to find a tab given a model index can go off the end of |tabs_|. As such,
  // it is important that we complete the drag *after* removing the tab so that
  // the model and tabstrip are in sync.
  if (contents && drag_controller_.get() && !drag_controller_->is_mutating() &&
      drag_controller_->IsDraggingTab(contents)) {
    EndDrag(END_DRAG_COMPLETE);
  }
}

void TabStrip::SetTabData(int model_index, TabRendererData data) {
  Tab* tab = tab_at(model_index);
  bool pinned_state_changed = tab->data().pinned != data.pinned;
  tab->SetData(std::move(data));

  if (pinned_state_changed) {
    if (touch_layout_) {
      int pinned_tab_count = 0;
      int start_x = GenerateIdealBoundsForPinnedTabs(&pinned_tab_count);
      touch_layout_->SetXAndPinnedCount(start_x, pinned_tab_count);
    }
    if (GetWidget() && GetWidget()->IsVisible())
      StartPinnedTabAnimation();
    else
      DoLayout();
  }
  SwapLayoutIfNecessary();
}

bool TabStrip::ShouldTabBeVisible(const Tab* tab) const {
  // Detached tabs should always be invisible (as they close).
  if (tab->detached())
    return false;

  // When stacking tabs, all tabs should always be visible.
  if (stacked_layout_)
    return true;

  // If the tab is currently clipped, it shouldn't be visible.  Note that we
  // allow dragged tabs to draw over any trailing "New Tab button" region as
  // well, because either the New Tab button will be hidden, or the dragged tabs
  // will be animating back to their normal positions and we don't want to hide
  // them in the New Tab button region in case they re-appear after leaving it.
  // (This prevents flickeriness.)  We never draw non-dragged tabs in New Tab
  // button area, even when the button is invisible, so that they don't appear
  // to "pop in" when the button disappears.
  // TODO: Probably doesn't work for RTL
  int right_edge = tab->bounds().right();
  const bool trailing_new_tab_button =
      (GetNewTabButtonPosition() == TRAILING) ||
      (MayHideNewTabButtonWhileDragging() && !tab->dragging());
  const int tabstrip_right =
      trailing_new_tab_button ? GetTabAreaWidth() : width();
  if (right_edge > tabstrip_right)
    return false;

  // Non-clipped dragging tabs should always be visible.
  if (tab->dragging())
    return true;

  // Let all non-clipped closing tabs be visible.  These will probably finish
  // closing before the user changes the active tab, so there's little reason to
  // try and make the more complex logic below apply.
  if (tab->closing())
    return true;

  // Now we need to check whether the tab isn't currently clipped, but could
  // become clipped if we changed the active tab, widening either this tab or
  // the tabstrip portion before it.

  // Pinned tabs don't change size when activated, so any tab in the pinned tab
  // region is safe.
  if (tab->data().pinned)
    return true;

  // If the active tab is on or before this tab, we're safe.
  if (controller_->GetActiveIndex() <= GetModelIndexOfTab(tab))
    return true;

  // We need to check what would happen if the active tab were to move to this
  // tab or before.
  return (right_edge + current_active_width_ - current_inactive_width_) <=
         tabstrip_right;
}

void TabStrip::PrepareForCloseAt(int model_index, CloseTabSource source) {
  if (!in_tab_close_ && IsAnimating()) {
    // Cancel any current animations. We do this as remove uses the current
    // ideal bounds and we need to know ideal bounds is in a good state.
    StopAnimating(true);
  }

  if (!GetWidget())
    return;

  int model_count = GetModelCount();
  if (model_count > 1 && model_index != model_count - 1) {
    // The user is about to close a tab other than the last tab. Set
    // available_width_for_tabs_ so that if we do a layout we don't position a
    // tab past the end of the second to last tab. We do this so that as the
    // user closes tabs with the mouse a tab continues to fall under the mouse.
    Tab* last_tab = tab_at(model_count - 1);
    Tab* tab_being_removed = tab_at(model_index);
    available_width_for_tabs_ = last_tab->bounds().right() - TabStartX() -
                                tab_being_removed->width() + Tab::GetOverlap();
    if (model_index == 0 && tab_being_removed->data().pinned &&
        !tab_at(1)->data().pinned) {
      available_width_for_tabs_ -= GetPinnedToNonPinnedOffset();
    }
  }

  in_tab_close_ = true;
  resize_layout_timer_.Stop();
  if (source == CLOSE_TAB_FROM_TOUCH) {
    StartResizeLayoutTabsFromTouchTimer();
  } else {
    AddMessageLoopObserver();
  }
}

void TabStrip::SetSelection(const ui::ListSelectionModel& new_selection) {
  if (selected_tabs_.active() != new_selection.active()) {
    if (selected_tabs_.active() >= 0)
      tab_at(selected_tabs_.active())->ActiveStateChanged();
    if (new_selection.active() >= 0)
      tab_at(new_selection.active())->ActiveStateChanged();
  }

  if (touch_layout_) {
    touch_layout_->SetActiveIndex(new_selection.active());
    // Only start an animation if we need to. Otherwise clicking on an
    // unselected tab and dragging won't work because dragging is only allowed
    // if not animating.
    if (!views::ViewModelUtils::IsAtIdealBounds(tabs_))
      AnimateToIdealBounds();
    SchedulePaint();
  } else {
    if (current_inactive_width_ == current_active_width_) {
      // When tabs are wide enough, selecting a new tab cannot change the
      // ideal bounds, so only a repaint is necessary.
      SchedulePaint();
    } else if (IsAnimating()) {
      // The selection change will have modified the ideal bounds of the tabs
      // in |selected_tabs_| and |new_selection|.  We need to recompute.
      // Note: This is safe even if we're in the midst of mouse-based tab
      // closure--we won't expand the tabstrip back to the full window
      // width--because PrepareForCloseAt() will have set
      // |available_width_for_tabs_| already.
      GenerateIdealBounds();
      AnimateToIdealBounds();
    } else {
      // As in the animating case above, the selection change will have
      // affected the desired bounds of the tabs, but since we're not animating
      // we can just snap to the new bounds.
      DoLayout();
    }
  }

  // Use STLSetDifference to get the indices of elements newly selected
  // and no longer selected, since selected_indices() is always sorted.
  ui::ListSelectionModel::SelectedIndices no_longer_selected =
      base::STLSetDifference<ui::ListSelectionModel::SelectedIndices>(
          selected_tabs_.selected_indices(), new_selection.selected_indices());
  ui::ListSelectionModel::SelectedIndices newly_selected =
      base::STLSetDifference<ui::ListSelectionModel::SelectedIndices>(
          new_selection.selected_indices(), selected_tabs_.selected_indices());

  // Fire accessibility events that reflect the changes to selection.
  for (size_t i = 0; i < no_longer_selected.size(); ++i) {
    tab_at(no_longer_selected[i])
        ->NotifyAccessibilityEvent(ax::mojom::Event::kSelectionRemove, true);
  }
  for (size_t i = 0; i < newly_selected.size(); ++i) {
    tab_at(newly_selected[i])
        ->NotifyAccessibilityEvent(ax::mojom::Event::kSelectionAdd, true);
  }
  tab_at(new_selection.active())
      ->NotifyAccessibilityEvent(ax::mojom::Event::kSelection, true);
  selected_tabs_ = new_selection;
}

void TabStrip::SetTabNeedsAttention(int model_index, bool attention) {
  tab_at(model_index)->SetTabNeedsAttention(attention);
}

int TabStrip::GetModelIndexOfTab(const Tab* tab) const {
  return tabs_.GetIndexOfView(tab);
}

int TabStrip::GetModelCount() const {
  return controller_->GetCount();
}

bool TabStrip::IsValidModelIndex(int model_index) const {
  return controller_->IsValidIndex(model_index);
}

bool TabStrip::IsDragSessionActive() const {
  return drag_controller_.get() != NULL;
}

bool TabStrip::IsActiveDropTarget() const {
  for (int i = 0; i < tab_count(); ++i) {
    Tab* tab = tab_at(i);
    if (tab->dragging())
      return true;
  }
  return false;
}

SkAlpha TabStrip::GetInactiveAlpha(bool for_new_tab_button) const {
#if defined(OS_CHROMEOS)
  static const SkAlpha kInactiveTabAlphaAsh = 230;
  const SkAlpha base_alpha = kInactiveTabAlphaAsh;
#else
  static const SkAlpha kInactiveTabAlphaGlass = 200;
  static const SkAlpha kInactiveTabAlphaOpaque = 255;
  const SkAlpha base_alpha = TitlebarBackgroundIsTransparent()
                                 ? kInactiveTabAlphaGlass
                                 : kInactiveTabAlphaOpaque;
#endif  // OS_CHROMEOS
  static const double kMultiSelectionMultiplier = 0.6;
  return (for_new_tab_button || (GetSelectionModel().size() <= 1))
             ? base_alpha
             : static_cast<SkAlpha>(kMultiSelectionMultiplier * base_alpha);
}

bool TabStrip::IsAnimating() const {
  return bounds_animator_.IsAnimating();
}

void TabStrip::StopAnimating(bool layout) {
  if (!IsAnimating())
    return;

  bounds_animator_.Cancel();

  if (layout)
    DoLayout();
}

const ui::ListSelectionModel& TabStrip::GetSelectionModel() const {
  return controller_->GetSelectionModel();
}

bool TabStrip::SupportsMultipleSelection() {
  // TODO: currently only allow single selection in touch layout mode.
  return touch_layout_ == nullptr;
}

bool TabStrip::ShouldHideCloseButtonForInactiveTabs() {
  return touch_layout_ != nullptr || MD::IsRefreshUi();
}

bool TabStrip::ShouldShowCloseButtonOnHover() {
  return MD::IsRefreshUi();
}

bool TabStrip::MaySetClip() {
  // Only touch layout needs to restrict the clip.
  return touch_layout_ || IsStackingDraggedTabs();
}

void TabStrip::SelectTab(Tab* tab) {
  int model_index = GetModelIndexOfTab(tab);
  if (IsValidModelIndex(model_index))
    controller_->SelectTab(model_index);
}

void TabStrip::ExtendSelectionTo(Tab* tab) {
  int model_index = GetModelIndexOfTab(tab);
  if (IsValidModelIndex(model_index))
    controller_->ExtendSelectionTo(model_index);
}

void TabStrip::ToggleSelected(Tab* tab) {
  int model_index = GetModelIndexOfTab(tab);
  if (IsValidModelIndex(model_index))
    controller_->ToggleSelected(model_index);
}

void TabStrip::AddSelectionFromAnchorTo(Tab* tab) {
  int model_index = GetModelIndexOfTab(tab);
  if (IsValidModelIndex(model_index))
    controller_->AddSelectionFromAnchorTo(model_index);
}

void TabStrip::CloseTab(Tab* tab, CloseTabSource source) {
  if (tab->closing()) {
    // If the tab is already closing, close the next tab. We do this so that the
    // user can rapidly close tabs by clicking the close button and not have
    // the animations interfere with that.
    const int closed_tab_index = FindClosingTab(tab).first->first;
    if (closed_tab_index < GetModelCount())
      controller_->CloseTab(closed_tab_index, source);
    return;
  }
  int model_index = GetModelIndexOfTab(tab);
  if (IsValidModelIndex(model_index))
    controller_->CloseTab(model_index, source);
}

void TabStrip::ToggleTabAudioMute(Tab* tab) {
  int model_index = GetModelIndexOfTab(tab);
  if (IsValidModelIndex(model_index))
    controller_->ToggleTabAudioMute(model_index);
}

void TabStrip::ShowContextMenuForTab(Tab* tab,
                                     const gfx::Point& p,
                                     ui::MenuSourceType source_type) {
  controller_->ShowContextMenuForTab(tab, p, source_type);
}

bool TabStrip::IsActiveTab(const Tab* tab) const {
  int model_index = GetModelIndexOfTab(tab);
  return IsValidModelIndex(model_index) &&
         controller_->IsActiveTab(model_index);
}

bool TabStrip::IsTabSelected(const Tab* tab) const {
  int model_index = GetModelIndexOfTab(tab);
  return IsValidModelIndex(model_index) &&
         controller_->IsTabSelected(model_index);
}

bool TabStrip::IsTabPinned(const Tab* tab) const {
  if (tab->closing())
    return false;

  int model_index = GetModelIndexOfTab(tab);
  return IsValidModelIndex(model_index) &&
         controller_->IsTabPinned(model_index);
}

bool TabStrip::IsIncognito() const {
  // There may be no controller in tests.
  return controller_ && controller_->IsIncognito();
}

void TabStrip::MaybeStartDrag(
    Tab* tab,
    const ui::LocatedEvent& event,
    const ui::ListSelectionModel& original_selection) {
  // Don't accidentally start any drag operations during animations if the
  // mouse is down... during an animation tabs are being resized automatically,
  // so the View system can misinterpret this easily if the mouse is down that
  // the user is dragging.
  if (IsAnimating() || tab->closing() ||
      controller_->HasAvailableDragActions() == 0) {
    return;
  }

  int model_index = GetModelIndexOfTab(tab);
  if (!IsValidModelIndex(model_index)) {
    CHECK(false);
    return;
  }
  Tabs tabs;
  int size_to_selected = 0;
  int x = tab->GetMirroredXInView(event.x());
  int y = event.y();
  // Build the set of selected tabs to drag and calculate the offset from the
  // first selected tab.
  for (int i = 0; i < tab_count(); ++i) {
    Tab* other_tab = tab_at(i);
    if (IsTabSelected(other_tab)) {
      tabs.push_back(other_tab);
      if (other_tab == tab) {
        size_to_selected = GetSizeNeededForTabs(tabs);
        x += size_to_selected - tab->width();
      }
    }
  }
  DCHECK(!tabs.empty());
  DCHECK(base::ContainsValue(tabs, tab));
  ui::ListSelectionModel selection_model;
  if (!original_selection.IsSelected(model_index))
    selection_model = original_selection;
  // Delete the existing DragController before creating a new one. We do this as
  // creating the DragController remembers the WebContents delegates and we need
  // to make sure the existing DragController isn't still a delegate.
  drag_controller_.reset();
  TabDragController::MoveBehavior move_behavior = TabDragController::REORDER;
  // Use MOVE_VISIBLE_TABS in the following conditions:
  // . Mouse event generated from touch and the left button is down (the right
  //   button corresponds to a long press, which we want to reorder).
  // . Gesture tap down and control key isn't down.
  // . Real mouse event and control is down. This is mostly for testing.
  DCHECK(event.type() == ui::ET_MOUSE_PRESSED ||
         event.type() == ui::ET_GESTURE_TAP_DOWN);
  if (touch_layout_ &&
      ((event.type() == ui::ET_MOUSE_PRESSED &&
        (((event.flags() & ui::EF_FROM_TOUCH) &&
          static_cast<const ui::MouseEvent&>(event).IsLeftMouseButton()) ||
         (!(event.flags() & ui::EF_FROM_TOUCH) &&
          static_cast<const ui::MouseEvent&>(event).IsControlDown()))) ||
       (event.type() == ui::ET_GESTURE_TAP_DOWN && !event.IsControlDown()))) {
    move_behavior = TabDragController::MOVE_VISIBLE_TABS;
  }

  drag_controller_.reset(new TabDragController);
  drag_controller_->Init(this, tab, tabs, gfx::Point(x, y), event.x(),
                         std::move(selection_model), move_behavior,
                         EventSourceFromEvent(event));
}

void TabStrip::ContinueDrag(views::View* view, const ui::LocatedEvent& event) {
  if (drag_controller_.get() &&
      drag_controller_->event_source() == EventSourceFromEvent(event)) {
    gfx::Point screen_location(event.location());
    views::View::ConvertPointToScreen(view, &screen_location);
    drag_controller_->Drag(screen_location);
  }
}

bool TabStrip::EndDrag(EndDragReason reason) {
  if (!drag_controller_.get())
    return false;
  bool started_drag = drag_controller_->started_drag();
  drag_controller_->EndDrag(reason);
  return started_drag;
}

Tab* TabStrip::GetTabAt(Tab* tab, const gfx::Point& tab_in_tab_coordinates) {
  gfx::Point local_point = tab_in_tab_coordinates;
  ConvertPointToTarget(tab, this, &local_point);

  views::View* view = GetEventHandlerForPoint(local_point);
  if (!view)
    return NULL;  // No tab contains the point.

  // Walk up the view hierarchy until we find a tab, or the TabStrip.
  while (view && view != this && view->id() != VIEW_ID_TAB)
    view = view->parent();

  return view && view->id() == VIEW_ID_TAB ? static_cast<Tab*>(view) : NULL;
}

Tab* TabStrip::GetAdjacentTab(Tab* tab, TabController::Direction direction) {
  const int index = GetModelIndexOfTab(tab);
  if (index < 0)
    return nullptr;
  const int new_index = index + (direction == TabController::FORWARD ? 1 : -1);
  return new_index < 0 || new_index >= tab_count() ? nullptr
                                                   : tab_at(new_index);
}

void TabStrip::OnMouseEventInTab(views::View* source,
                                 const ui::MouseEvent& event) {
  UpdateStackedLayoutFromMouseEvent(source, event);
}

bool TabStrip::ShouldPaintTab(
    const Tab* tab,
    const base::RepeatingCallback<gfx::Path(const gfx::Rect&)>& border_callback,
    gfx::Path* clip) {
  if (!MaySetClip())
    return true;

  int index = GetModelIndexOfTab(tab);
  if (index == -1)
    return true;  // Tab is closing, paint it all.

  int active_index = IsStackingDraggedTabs() ? controller_->GetActiveIndex()
                                             : touch_layout_->active_index();
  if (active_index == tab_count())
    active_index--;

  const int current_x = tab_at(index)->x();
  if (index < active_index) {
    const int next_x = tab_at(index + 1)->x();
    if (current_x == next_x)
      return false;

    if (current_x > next_x)
      return true;  // Can happen during dragging.

    *clip = border_callback.Run(tab_at(index + 1)->bounds());
    clip->offset(SkIntToScalar(next_x - current_x), 0);
  } else if (index > active_index && index > 0) {
    const gfx::Rect& previous_bounds(tab_at(index - 1)->bounds());
    const int previous_x = previous_bounds.x();
    if (current_x == previous_x)
      return false;

    if (current_x < previous_x)
      return true;  // Can happen during dragging.

    if (previous_bounds.right() - Tab::GetOverlap() != current_x) {
      *clip = border_callback.Run(tab_at(index - 1)->bounds());
      clip->offset(SkIntToScalar(previous_x - current_x), 0);
    }
  }
  return true;
}

bool TabStrip::CanPaintThrobberToLayer() const {
  // Disable layer-painting of throbbers if dragging, if any tab animation is in
  // progress, or if stacked tabs are enabled. Also disable in fullscreen: when
  // "immersive" the tab strip could be sliding in or out; for other modes,
  // there's no tab strip.
  const bool dragging = drag_controller_ && drag_controller_->started_drag();
  const views::Widget* widget = GetWidget();
  return widget && !touch_layout_ && !dragging && !IsAnimating() &&
         !widget->IsFullscreen();
}

SkColor TabStrip::GetToolbarTopSeparatorColor() const {
  return controller_->GetToolbarTopSeparatorColor();
}

// Returns the accessible tab name for the tab.
base::string16 TabStrip::GetAccessibleTabName(const Tab* tab) const {
  int model_index = GetModelIndexOfTab(tab);
  if (IsValidModelIndex(model_index))
    return controller_->GetAccessibleTabName(tab);
  return base::string16();
}

int TabStrip::GetBackgroundResourceId(bool* custom_image) const {
  const ui::ThemeProvider* tp = GetThemeProvider();

  if (TitlebarBackgroundIsTransparent()) {
    const int kBackgroundIdGlass = IDR_THEME_TAB_BACKGROUND_V;
    *custom_image = tp->HasCustomImage(kBackgroundIdGlass);
    return kBackgroundIdGlass;
  }

  // If a custom theme does not provide a replacement tab background, but does
  // provide a replacement frame image, HasCustomImage() on the tab background
  // ID will return false, but the theme provider will make a custom image from
  // the frame image.  Furthermore, since the theme provider will create the
  // incognito frame image from the normal frame image, in incognito mode we
  // need to look for a custom incognito _or_ regular frame image.
  const bool incognito = controller_->IsIncognito();
  const int id =
      incognito ? IDR_THEME_TAB_BACKGROUND_INCOGNITO : IDR_THEME_TAB_BACKGROUND;
  *custom_image = tp->HasCustomImage(id) ||
                  tp->HasCustomImage(IDR_THEME_FRAME) ||
                  (incognito && tp->HasCustomImage(IDR_THEME_FRAME_INCOGNITO));
  return id;
}

void TabStrip::MouseMovedOutOfHost() {
  ResizeLayoutTabs();
  if (reset_to_shrink_on_exit_) {
    reset_to_shrink_on_exit_ = false;
    SetStackedLayout(false);
    controller_->StackedLayoutMaybeChanged();
  }
}

///////////////////////////////////////////////////////////////////////////////
// TabStrip, views::View overrides:

void TabStrip::Layout() {
  // Only do a layout if our size changed.
  if (last_layout_size_ == size())
    return;
  if (IsDragSessionActive())
    return;
  DoLayout();
}

void TabStrip::PaintChildren(const views::PaintInfo& paint_info) {
  // The view order doesn't match the paint order (tabs_ contains the tab
  // ordering). Additionally we need to paint the tabs that are closing in
  // |tabs_closing_map_|.
  bool is_dragging = false;
  Tab* active_tab = nullptr;
  Tab* hovered_tab = nullptr;
  Tabs tabs_dragging;
  Tabs selected_tabs;

  {
    // We pass false for |lcd_text_requires_opaque_layer| so that background
    // tab titles will get LCD AA.  These are rendered opaquely on an opaque tab
    // background before the layer is composited, so this is safe.
    ui::CompositingRecorder opacity_recorder(paint_info.context(),
                                             GetInactiveAlpha(false), false);

    PaintClosingTabs(tab_count(), paint_info);

    int active_tab_index = -1;
    for (int i = tab_count() - 1; i >= 0; --i) {
      Tab* tab = tab_at(i);
      if (tab->dragging() && !stacked_layout_) {
        is_dragging = true;
        if (tab->IsActive()) {
          active_tab = tab;
          active_tab_index = i;
        } else {
          tabs_dragging.push_back(tab);
        }
      } else if (!tab->IsActive()) {
        if (!tab->IsSelected()) {
          if (!stacked_layout_) {
            // In Refresh mode, defer the painting of the hovered tab to below.
            if (MD::IsRefreshUi() && tab->IsMouseHovered()) {
              hovered_tab = tab;
            } else {
              tab->Paint(paint_info);
            }
          }
        } else {
          selected_tabs.push_back(tab);
        }
      } else {
        active_tab = tab;
        active_tab_index = i;
      }
      PaintClosingTabs(i, paint_info);
    }

    // Draw from the left and then the right if we're in touch mode.
    if (stacked_layout_ && active_tab_index >= 0) {
      for (int i = 0; i < active_tab_index; ++i) {
        Tab* tab = tab_at(i);
        tab->Paint(paint_info);
      }

      for (int i = tab_count() - 1; i > active_tab_index; --i) {
        Tab* tab = tab_at(i);
        tab->Paint(paint_info);
      }
    }
  }

  // Now selected but not active. We don't want these dimmed if using native
  // frame, so they're painted after initial pass.
  for (size_t i = 0; i < selected_tabs.size(); ++i)
    selected_tabs[i]->Paint(paint_info);

  // If the last hovered tab is still animating and there is no currently
  // hovered tab, make sure it still paints in the right order while it's
  // animating.
  if (!hovered_tab && last_hovered_tab_ &&
      last_hovered_tab_->hover_controller()->ShouldDraw())
    hovered_tab = last_hovered_tab_;

  // The currently hovered tab or the last tab that was hovered should be
  // painted right before the active tab to ensure the highlighted tab shape
  // looks reasonable.
  if (hovered_tab && !is_dragging)
    hovered_tab->Paint(paint_info);

  // Keep track of the last tab that was hovered to that it continues to be
  // painted right before the active tab while the animation is running.
  last_hovered_tab_ = hovered_tab;

  // Next comes the active tab.
  if (active_tab && !is_dragging)
    active_tab->Paint(paint_info);

  // Paint the New Tab button.
  if (new_tab_button_->state() == views::Button::STATE_PRESSED) {
    new_tab_button_->Paint(paint_info);
  } else {
    // Match the inactive tab opacity for non-pressed states.  See comments in
    // NewTabButton::PaintFill() for why we don't do this for the pressed state.
    // This call doesn't need to set |lcd_text_requires_opaque_layer| to false
    // because no text will be drawn.
    ui::CompositingRecorder opacity_recorder(paint_info.context(),
                                             GetInactiveAlpha(true), true);
    new_tab_button_->Paint(paint_info);
  }

  // And the dragged tabs.
  for (size_t i = 0; i < tabs_dragging.size(); ++i)
    tabs_dragging[i]->Paint(paint_info);

  // If the active tab is being dragged, it goes last.
  if (active_tab && is_dragging)
    active_tab->Paint(paint_info);

  // Keep the recording scales consistent for the tab strip and its children.
  // See crbug/753911
  ui::PaintRecorder recorder(paint_info.context(),
                             paint_info.paint_recording_size(),
                             paint_info.paint_recording_scale_x(),
                             paint_info.paint_recording_scale_y(), nullptr);

  if (ShouldDrawStrokes()) {
    gfx::Canvas* canvas = recorder.canvas();
    if (active_tab && active_tab->visible()) {
      canvas->sk_canvas()->clipRect(
          gfx::RectToSkRect(active_tab->GetMirroredBounds()),
          SkClipOp::kDifference);
    }
    BrowserView::Paint1pxHorizontalLine(canvas, GetToolbarTopSeparatorColor(),
                                        GetLocalBounds(), true);
  }
}

const char* TabStrip::GetClassName() const {
  static const char kViewClassName[] = "TabStrip";
  return kViewClassName;
}

gfx::Size TabStrip::CalculatePreferredSize() const {
  int needed_tab_width;
  if (touch_layout_ || adjust_layout_) {
    // For stacked tabs the minimum size is calculated as the size needed to
    // handle showing any number of tabs.
    needed_tab_width = GetLayoutConstant(TAB_STACK_TAB_WIDTH) +
                       (2 * kStackedPadding * kMaxStackedCount);
  } else {
    // Otherwise the minimum width is based on the actual number of tabs.
    const int pinned_tab_count = GetPinnedTabCount();
    needed_tab_width = pinned_tab_count * Tab::GetPinnedWidth();
    const int remaining_tab_count = tab_count() - pinned_tab_count;
    const int min_selected_width = Tab::GetMinimumActiveSize().width();
    const int min_unselected_width = Tab::GetMinimumInactiveSize().width();
    if (remaining_tab_count > 0) {
      needed_tab_width += GetPinnedToNonPinnedOffset() + min_selected_width +
                          ((remaining_tab_count - 1) * min_unselected_width);
    }

    const int overlap = Tab::GetOverlap();
    if (tab_count() > 1)
      needed_tab_width -= (tab_count() - 1) * overlap;

    // Don't let the tabstrip shrink smaller than is necessary to show one tab,
    // and don't force it to be larger than is necessary to show 20 tabs.
    const int largest_min_tab_width =
        min_selected_width + 19 * (min_unselected_width - overlap);
    needed_tab_width = std::min(std::max(needed_tab_width, min_selected_width),
                                largest_min_tab_width);
  }
  return gfx::Size(needed_tab_width + GetFrameGrabWidth() +
                       GetNewTabButtonWidth(IsIncognito()),
                   Tab::GetMinimumInactiveSize().height());
}

void TabStrip::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  node_data->role = ax::mojom::Role::kTabList;
}

views::View* TabStrip::GetTooltipHandlerForPoint(const gfx::Point& point) {
  if (!HitTestPoint(point))
    return NULL;

  if (!touch_layout_) {
    // Return any view that isn't a Tab or this TabStrip immediately. We don't
    // want to interfere.
    views::View* v = View::GetTooltipHandlerForPoint(point);
    if (v && v != this && strcmp(v->GetClassName(), Tab::kViewClassName))
      return v;

    views::View* tab = FindTabHitByPoint(point);
    if (tab)
      return tab;
  } else {
    if (new_tab_button_->visible()) {
      views::View* view =
          ConvertPointToViewAndGetTooltipHandler(this, new_tab_button_, point);
      if (view)
        return view;
    }
    Tab* tab = FindTabForEvent(point);
    if (tab)
      return ConvertPointToViewAndGetTooltipHandler(this, tab, point);
  }
  return this;
}

BrowserRootView::DropIndex TabStrip::GetDropIndex(
    const DropTargetEvent& event) {
  // Force animations to stop, otherwise it makes the index calculation tricky.
  StopAnimating(true);

  // If the UI layout is right-to-left, we need to mirror the mouse
  // coordinates since we calculate the drop index based on the
  // original (and therefore non-mirrored) positions of the tabs.
  const int x = GetMirroredXInView(event.x());
  for (int i = 0; i < tab_count(); ++i) {
    Tab* tab = tab_at(i);
    const int tab_max_x = tab->x() + tab->width();
    const int hot_width = tab->width() / kTabEdgeRatioInverse;
    if (x < tab_max_x) {
      if (x >= (tab_max_x - hot_width))
        return {i + 1, true};
      return {i, x < tab->x() + hot_width};
    }
  }

  // The drop isn't over a tab, add it to the end.
  return {tab_count(), true};
}

views::View* TabStrip::GetViewForDrop() {
  return this;
}

void TabStrip::HandleDragUpdate(
    const base::Optional<BrowserRootView::DropIndex>& index) {
  SetDropArrow(index);
}

void TabStrip::HandleDragExited() {
  SetDropArrow({});
}

///////////////////////////////////////////////////////////////////////////////
// TabStrip, private:

void TabStrip::Init() {
  set_id(VIEW_ID_TAB_STRIP);
  // So we get enter/exit on children to switch stacked layout on and off.
  set_notify_enter_exit_on_child(true);

  new_tab_button_bounds_.set_size(GetLayoutSize(NEW_TAB_BUTTON, IsIncognito()));
  new_tab_button_bounds_.Inset(0, 0, 0, -NewTabButton::GetTopOffset());
  new_tab_button_ = new NewTabButton(this, this);
  new_tab_button_->SetTooltipText(
      l10n_util::GetStringUTF16(IDS_TOOLTIP_NEW_TAB));
  new_tab_button_->SetAccessibleName(
      l10n_util::GetStringUTF16(IDS_ACCNAME_NEWTAB));
  new_tab_button_->SetImageAlignment(views::ImageButton::ALIGN_LEFT,
                                     views::ImageButton::ALIGN_BOTTOM);
  new_tab_button_->SetEventTargeter(
      std::make_unique<views::ViewTargeter>(new_tab_button_));
  AddChildView(new_tab_button_);

  if (drop_indicator_width == 0) {
    // Direction doesn't matter, both images are the same size.
    gfx::ImageSkia* drop_image = GetDropArrowImage(true);
    drop_indicator_width = drop_image->width();
    drop_indicator_height = drop_image->height();
  }
}

void TabStrip::StartInsertTabAnimation(int model_index) {
  PrepareForAnimation();

  // The TabStrip can now use its entire width to lay out Tabs.
  in_tab_close_ = false;
  available_width_for_tabs_ = -1;

  GenerateIdealBounds();

  // Set the current bounds to be the correct place but 0 width.
  Tab* tab = tab_at(model_index);
  if (model_index == 0) {
    tab->SetBounds(TabStartX(), ideal_bounds(model_index).y(), 0,
                   ideal_bounds(model_index).height());
  } else {
    Tab* prev_tab = tab_at(model_index - 1);
    tab->SetBounds(prev_tab->bounds().right() - Tab::GetOverlap(),
                   ideal_bounds(model_index).y(), 0,
                   ideal_bounds(model_index).height());
  }

  // Animate in to the full width.
  AnimateToIdealBounds();
}

void TabStrip::StartMoveTabAnimation() {
  PrepareForAnimation();
  GenerateIdealBounds();
  AnimateToIdealBounds();
}

void TabStrip::StartRemoveTabAnimation(int model_index) {
  PrepareForAnimation();

  // Mark the tab as closing.
  Tab* tab = tab_at(model_index);
  tab->set_closing(true);

  RemoveTabFromViewModel(model_index);

  ScheduleRemoveTabAnimation(tab);
}

void TabStrip::ScheduleRemoveTabAnimation(Tab* tab) {
  // Start an animation for the tabs.
  GenerateIdealBounds();
  AnimateToIdealBounds();

  // Animate the tab being closed to zero width.
  gfx::Rect tab_bounds = tab->bounds();
  tab_bounds.set_width(0);
  bounds_animator_.AnimateViewTo(tab, tab_bounds);
  bounds_animator_.SetAnimationDelegate(
      tab, std::make_unique<RemoveTabDelegate>(this, tab));

  // Don't animate the new tab button when dragging tabs. Otherwise it looks
  // like the new tab button magically appears from beyond the end of the tab
  // strip.
  if (TabDragController::IsAttachedTo(this)) {
    bounds_animator_.StopAnimatingView(new_tab_button_);
    new_tab_button_->SetBoundsRect(new_tab_button_bounds_);
  }
}

void TabStrip::AnimateToIdealBounds() {
  for (int i = 0; i < tab_count(); ++i) {
    Tab* tab = tab_at(i);
    if (!tab->dragging()) {
      bounds_animator_.AnimateViewTo(tab, ideal_bounds(i));
      bounds_animator_.SetAnimationDelegate(
          tab, std::make_unique<TabAnimationDelegate>(this, tab));
    }
  }

  bounds_animator_.AnimateViewTo(new_tab_button_, new_tab_button_bounds_);
}

bool TabStrip::ShouldHighlightCloseButtonAfterRemove() {
  return in_tab_close_;
}

TabStrip::NewTabButtonPosition TabStrip::GetNewTabButtonPosition() const {
  if (!MD::IsRefreshUi())
    return AFTER_TABS;

  const auto* frame_view = static_cast<const BrowserNonClientFrameView*>(
      GetWidget()->non_client_view()->frame_view());
  return frame_view->CaptionButtonsOnLeadingEdge() ? TRAILING : LEADING;
}

int TabStrip::GetNewTabButtonSpacing() const {
  constexpr int kSpacing[] = {-5, -6, 6, 8, 8};
  const auto mode = MD::GetMode();
  int spacing = kSpacing[mode];

  // Not sure if we can reach here with no tabs (e.g. during startup), but not
  // crashing in that case is easy.
  if (MD::IsRefreshUi() && tab_count()) {
    const int adjacent_tab_index =
        (GetNewTabButtonPosition() == LEADING) ? 0 : tab_count() - 1;
    spacing -= tab_at(adjacent_tab_index)->GetCornerRadius();
  }

  return spacing;
}

int TabStrip::GetNewTabButtonWidth(bool is_incognito) const {
  int width = GetLayoutSize(NEW_TAB_BUTTON, is_incognito).width();
  if (GetNewTabButtonPosition() != TRAILING)
    width += GetNewTabButtonSpacing();
  return width;
}

bool TabStrip::MayHideNewTabButtonWhileDragging() const {
  return GetNewTabButtonPosition() == AFTER_TABS;
}

int TabStrip::GetFrameGrabWidth() const {
  // Only Refresh has a grab area.
  if (!MD::IsRefreshUi())
    return 0;

  // The apparent width of the grab area.
  constexpr int kGrabWidth = 50;
  int width = kGrabWidth;

  // There might be no tabs yet during startup.
  if ((GetNewTabButtonPosition() != AFTER_TABS) && tab_count()) {
    // The grab area is adjacent to the last tab.  This tab has mostly empty
    // space where the outer (lower) corners are, which should be treated as
    // part of the grab area, so decrease the size of the remaining grab area by
    // that width.
    width -= tab_at(tab_count() - 1)->GetCornerRadius();
  }

  return width;
}

bool TabStrip::TitlebarBackgroundIsTransparent() const {
#if defined(OS_WIN)
  // Windows 8+ uses transparent window contents (because the titlebar area is
  // drawn by the system and not Chrome), but the actual titlebar is opaque.
  if (base::win::GetVersion() >= base::win::VERSION_WIN8)
    return false;
#endif
  return GetWidget()->ShouldWindowContentsBeTransparent();
}

void TabStrip::DoLayout() {
  last_layout_size_ = size();

  StopAnimating(false);

  SwapLayoutIfNecessary();

  if (touch_layout_)
    touch_layout_->SetWidth(GetTabAreaWidth());

  GenerateIdealBounds();

  views::ViewModelUtils::SetViewBoundsToIdealBounds(tabs_);
  SetTabVisibility();

  SchedulePaint();

  bounds_animator_.StopAnimatingView(new_tab_button_);
  new_tab_button_->SetBoundsRect(new_tab_button_bounds_);
}

void TabStrip::SetTabVisibility() {
  // We could probably be more efficient here by making use of the fact that the
  // tabstrip will always have any visible tabs, and then any invisible tabs, so
  // we could e.g. binary-search for the changeover point.  But since we have to
  // iterate through all the tabs to call SetVisible() anyway, it doesn't seem
  // worth it.
  for (int i = 0; i < tab_count(); ++i) {
    Tab* tab = tab_at(i);
    tab->SetVisible(ShouldTabBeVisible(tab));
  }
  for (const auto& closing_tab : tabs_closing_map_) {
    for (Tab* tab : closing_tab.second)
      tab->SetVisible(ShouldTabBeVisible(tab));
  }
}

void TabStrip::DragActiveTab(const std::vector<int>& initial_positions,
                             int delta) {
  DCHECK_EQ(tab_count(), static_cast<int>(initial_positions.size()));
  if (!touch_layout_) {
    StackDraggedTabs(delta);
    return;
  }
  SetIdealBoundsFromPositions(initial_positions);
  touch_layout_->DragActiveTab(delta);
  DoLayout();
}

void TabStrip::SetIdealBoundsFromPositions(const std::vector<int>& positions) {
  if (static_cast<size_t>(tab_count()) != positions.size())
    return;

  for (int i = 0; i < tab_count(); ++i) {
    gfx::Rect bounds(ideal_bounds(i));
    bounds.set_x(positions[i]);
    tabs_.set_ideal_bounds(i, bounds);
  }
}

void TabStrip::StackDraggedTabs(int delta) {
  DCHECK(!touch_layout_);
  GenerateIdealBounds();
  const int active_index = controller_->GetActiveIndex();
  DCHECK_NE(-1, active_index);
  if (delta < 0) {
    // Drag the tabs to the left, stacking tabs before the active tab.
    const int adjusted_delta =
        std::min(ideal_bounds(active_index).x() -
                     kStackedPadding * std::min(active_index, kMaxStackedCount),
                 -delta);
    for (int i = 0; i <= active_index; ++i) {
      const int min_x = std::min(i, kMaxStackedCount) * kStackedPadding;
      gfx::Rect new_bounds(ideal_bounds(i));
      new_bounds.set_x(std::max(min_x, new_bounds.x() - adjusted_delta));
      tabs_.set_ideal_bounds(i, new_bounds);
    }
    const bool is_active_pinned = tab_at(active_index)->data().pinned;
    const int active_width = ideal_bounds(active_index).width();
    for (int i = active_index + 1; i < tab_count(); ++i) {
      const int max_x =
          ideal_bounds(active_index).x() +
          (kStackedPadding * std::min(i - active_index, kMaxStackedCount));
      gfx::Rect new_bounds(ideal_bounds(i));
      int new_x = std::max(new_bounds.x() + delta, max_x);
      if (new_x == max_x && !tab_at(i)->data().pinned && !is_active_pinned &&
          new_bounds.width() != active_width)
        new_x += (active_width - new_bounds.width());
      new_bounds.set_x(new_x);
      tabs_.set_ideal_bounds(i, new_bounds);
    }
  } else {
    // Drag the tabs to the right, stacking tabs after the active tab.
    const int last_tab_width = ideal_bounds(tab_count() - 1).width();
    const int last_tab_x = GetTabAreaWidth() - last_tab_width;
    if (active_index == tab_count() - 1 &&
        ideal_bounds(tab_count() - 1).x() == last_tab_x)
      return;
    const int adjusted_delta =
        std::min(last_tab_x -
                     kStackedPadding * std::min(tab_count() - active_index - 1,
                                                kMaxStackedCount) -
                     ideal_bounds(active_index).x(),
                 delta);
    for (int last_index = tab_count() - 1, i = last_index; i >= active_index;
         --i) {
      const int max_x =
          last_tab_x -
          std::min(tab_count() - i - 1, kMaxStackedCount) * kStackedPadding;
      gfx::Rect new_bounds(ideal_bounds(i));
      int new_x = std::min(max_x, new_bounds.x() + adjusted_delta);
      // Because of rounding not all tabs are the same width. Adjust the
      // position to accommodate this, otherwise the stacking is off.
      if (new_x == max_x && !tab_at(i)->data().pinned &&
          new_bounds.width() != last_tab_width)
        new_x += (last_tab_width - new_bounds.width());
      new_bounds.set_x(new_x);
      tabs_.set_ideal_bounds(i, new_bounds);
    }
    for (int i = active_index - 1; i >= 0; --i) {
      const int min_x =
          ideal_bounds(active_index).x() -
          std::min(active_index - i, kMaxStackedCount) * kStackedPadding;
      gfx::Rect new_bounds(ideal_bounds(i));
      new_bounds.set_x(std::min(min_x, new_bounds.x() + delta));
      tabs_.set_ideal_bounds(i, new_bounds);
    }
    if ((GetNewTabButtonPosition() == AFTER_TABS) &&
        (ideal_bounds(tab_count() - 1).right() >= new_tab_button_->x()))
      new_tab_button_->SetVisible(false);
  }
  views::ViewModelUtils::SetViewBoundsToIdealBounds(tabs_);
  SchedulePaint();
}

bool TabStrip::IsStackingDraggedTabs() const {
  return drag_controller_.get() && drag_controller_->started_drag() &&
         (drag_controller_->move_behavior() ==
          TabDragController::MOVE_VISIBLE_TABS);
}

void TabStrip::LayoutDraggedTabsAt(const Tabs& tabs,
                                   Tab* active_tab,
                                   const gfx::Point& location,
                                   bool initial_drag) {
  // Immediately hide the new tab button if the last tab is being dragged.
  const Tab* last_visible_tab = GetLastVisibleTab();
  if (MayHideNewTabButtonWhileDragging() && last_visible_tab &&
      last_visible_tab->dragging())
    new_tab_button_->SetVisible(false);
  std::vector<gfx::Rect> bounds;
  CalculateBoundsForDraggedTabs(tabs, &bounds);
  DCHECK_EQ(tabs.size(), bounds.size());
  int active_tab_model_index = GetModelIndexOfTab(active_tab);
  int active_tab_index = static_cast<int>(
      std::find(tabs.begin(), tabs.end(), active_tab) - tabs.begin());
  for (size_t i = 0; i < tabs.size(); ++i) {
    Tab* tab = tabs[i];
    gfx::Rect new_bounds = bounds[i];
    new_bounds.Offset(location.x(), location.y());
    int consecutive_index =
        active_tab_model_index - (active_tab_index - static_cast<int>(i));
    // If this is the initial layout during a drag and the tabs aren't
    // consecutive animate the view into position. Do the same if the tab is
    // already animating (which means we previously caused it to animate).
    if ((initial_drag && GetModelIndexOfTab(tabs[i]) != consecutive_index) ||
        bounds_animator_.IsAnimating(tabs[i])) {
      bounds_animator_.SetTargetBounds(tabs[i], new_bounds);
    } else {
      tab->SetBoundsRect(new_bounds);
    }
  }
  SetTabVisibility();
}

void TabStrip::CalculateBoundsForDraggedTabs(const Tabs& tabs,
                                             std::vector<gfx::Rect>* bounds) {
  const int overlap = Tab::GetOverlap();
  int x = 0;
  for (size_t i = 0; i < tabs.size(); ++i) {
    Tab* tab = tabs[i];
    if (i > 0 && tab->data().pinned != tabs[i - 1]->data().pinned)
      x += GetPinnedToNonPinnedOffset();
    gfx::Rect new_bounds = tab->bounds();
    new_bounds.set_origin(gfx::Point(x, 0));
    bounds->push_back(new_bounds);
    x += tab->width() - overlap;
  }
}

int TabStrip::TabStartX() const {
  return (GetNewTabButtonPosition() == LEADING)
             ? GetNewTabButtonWidth(IsIncognito())
             : 0;
}

int TabStrip::NewTabButtonX() const {
  const auto position = GetNewTabButtonPosition();
  if (position == LEADING)
    return 0;

  // This is not GetTabAreaWidth() because we don't want to subtract the
  // separator region between the tabs and the new tab button.
  const int tab_area_width = width() - new_tab_button_bounds_.width();
  if (position == TRAILING)
    return tab_area_width;

  // For non-stacked tabs the ideal bounds may go outside the bounds of the
  // tabstrip. Constrain the x-coordinate of the new tab button so that it is
  // always visible.
  return std::min(tab_area_width,
                  tabs_.ideal_bounds(tabs_.view_size() - 1).right() +
                      GetNewTabButtonSpacing());
}

int TabStrip::GetSizeNeededForTabs(const Tabs& tabs) {
  int width = 0;
  for (size_t i = 0; i < tabs.size(); ++i) {
    Tab* tab = tabs[i];
    width += tab->width();
    if (i > 0 && tab->data().pinned != tabs[i - 1]->data().pinned)
      width += GetPinnedToNonPinnedOffset();
  }
  if (!tabs.empty())
    width -= Tab::GetOverlap() * (tabs.size() - 1);
  return width;
}

int TabStrip::GetPinnedTabCount() const {
  int pinned_count = 0;
  while (pinned_count < tab_count() && tab_at(pinned_count)->data().pinned)
    pinned_count++;
  return pinned_count;
}

const Tab* TabStrip::GetLastVisibleTab() const {
  for (int i = tab_count() - 1; i >= 0; --i) {
    const Tab* tab = tab_at(i);
    if (tab->visible())
      return tab;
  }
  // While in normal use the tabstrip should always be wide enough to have at
  // least one visible tab, it can be zero-width in tests, meaning we get here.
  return NULL;
}

void TabStrip::RemoveTabFromViewModel(int index) {
  // We still need to paint the tab until we actually remove it. Put it
  // in tabs_closing_map_ so we can find it.
  tabs_closing_map_[index].push_back(tab_at(index));
  UpdateTabsClosingMap(index + 1, -1);
  tabs_.Remove(index);
  selected_tabs_.DecrementFrom(index);
}

void TabStrip::RemoveAndDeleteTab(Tab* tab) {
  std::unique_ptr<Tab> deleter(tab);
  FindClosingTabResult res(FindClosingTab(tab));
  res.first->second.erase(res.second);
  if (res.first->second.empty())
    tabs_closing_map_.erase(res.first);
  if (tab == last_hovered_tab_)
    last_hovered_tab_ = nullptr;
}

void TabStrip::UpdateTabsClosingMap(int index, int delta) {
  if (tabs_closing_map_.empty())
    return;

  if (delta == -1 &&
      tabs_closing_map_.find(index - 1) != tabs_closing_map_.end() &&
      tabs_closing_map_.find(index) != tabs_closing_map_.end()) {
    const Tabs& tabs(tabs_closing_map_[index]);
    tabs_closing_map_[index - 1].insert(tabs_closing_map_[index - 1].end(),
                                        tabs.begin(), tabs.end());
  }
  TabsClosingMap updated_map;
  for (auto& i : tabs_closing_map_) {
    if (i.first > index)
      updated_map[i.first + delta] = i.second;
    else if (i.first < index)
      updated_map[i.first] = i.second;
  }
  if (delta > 0 && tabs_closing_map_.find(index) != tabs_closing_map_.end())
    updated_map[index + delta] = tabs_closing_map_[index];
  tabs_closing_map_.swap(updated_map);
}

void TabStrip::StartedDraggingTabs(const Tabs& tabs) {
  // Let the controller know that the user started dragging tabs.
  controller_->OnStartedDraggingTabs();

  // Hide the new tab button immediately if we didn't originate the drag.
  if (MayHideNewTabButtonWhileDragging() && !drag_controller_)
    new_tab_button_->SetVisible(false);

  PrepareForAnimation();

  // Reset dragging state of existing tabs.
  for (int i = 0; i < tab_count(); ++i)
    tab_at(i)->set_dragging(false);

  for (size_t i = 0; i < tabs.size(); ++i) {
    tabs[i]->set_dragging(true);
    bounds_animator_.StopAnimatingView(tabs[i]);
  }

  // Move the dragged tabs to their ideal bounds.
  GenerateIdealBounds();

  // Sets the bounds of the dragged tabs.
  for (size_t i = 0; i < tabs.size(); ++i) {
    int tab_data_index = GetModelIndexOfTab(tabs[i]);
    DCHECK_NE(-1, tab_data_index);
    tabs[i]->SetBoundsRect(ideal_bounds(tab_data_index));
  }
  SetTabVisibility();
  SchedulePaint();
}

void TabStrip::DraggedTabsDetached() {
  // Let the controller know that the user is not dragging this tabstrip's tabs
  // anymore.
  controller_->OnStoppedDraggingTabs();
  new_tab_button_->SetVisible(true);
}

void TabStrip::StoppedDraggingTabs(const Tabs& tabs,
                                   const std::vector<int>& initial_positions,
                                   bool move_only,
                                   bool completed) {
  // Let the controller know that the user stopped dragging tabs.
  controller_->OnStoppedDraggingTabs();

  new_tab_button_->SetVisible(true);
  if (move_only && touch_layout_) {
    if (completed)
      touch_layout_->SizeToFit();
    else
      SetIdealBoundsFromPositions(initial_positions);
  }
  bool is_first_tab = true;
  for (size_t i = 0; i < tabs.size(); ++i)
    StoppedDraggingTab(tabs[i], &is_first_tab);
}

void TabStrip::StoppedDraggingTab(Tab* tab, bool* is_first_tab) {
  int tab_data_index = GetModelIndexOfTab(tab);
  if (tab_data_index == -1) {
    // The tab was removed before the drag completed. Don't do anything.
    return;
  }

  if (*is_first_tab) {
    *is_first_tab = false;
    PrepareForAnimation();

    // Animate the view back to its correct position.
    GenerateIdealBounds();
    AnimateToIdealBounds();
  }
  bounds_animator_.AnimateViewTo(tab, ideal_bounds(tab_data_index));
  // Install a delegate to reset the dragging state when done. We have to leave
  // dragging true for the tab otherwise it'll draw beneath the new tab button.
  bounds_animator_.SetAnimationDelegate(
      tab, std::make_unique<ResetDraggingStateDelegate>(this, tab));
}

void TabStrip::OwnDragController(TabDragController* controller) {
  // Typically, ReleaseDragController() and OwnDragController() calls are paired
  // via corresponding calls to TabDragController::Detach() and
  // TabDragController::Attach(). There is one exception to that rule: when a
  // drag might start, we create a TabDragController that is owned by the
  // potential source tabstrip in MaybeStartDrag(). If a drag actually starts,
  // we then call Attach() on the source tabstrip, but since the source tabstrip
  // already owns the TabDragController, so we don't need to do anything.
  if (controller != drag_controller_.get())
    drag_controller_.reset(controller);
}

void TabStrip::DestroyDragController() {
  new_tab_button_->SetVisible(true);
  drag_controller_.reset();
}

TabDragController* TabStrip::ReleaseDragController() {
  return drag_controller_.release();
}

TabStrip::FindClosingTabResult TabStrip::FindClosingTab(const Tab* tab) {
  DCHECK(tab->closing());
  for (auto i = tabs_closing_map_.begin(); i != tabs_closing_map_.end(); ++i) {
    Tabs::iterator j = std::find(i->second.begin(), i->second.end(), tab);
    if (j != i->second.end())
      return FindClosingTabResult(i, j);
  }
  NOTREACHED();
  return FindClosingTabResult(tabs_closing_map_.end(), Tabs::iterator());
}

void TabStrip::PaintClosingTabs(int index, const views::PaintInfo& paint_info) {
  if (tabs_closing_map_.find(index) == tabs_closing_map_.end())
    return;
  for (Tab* tab : base::Reversed(tabs_closing_map_[index]))
    tab->Paint(paint_info);
}

void TabStrip::UpdateStackedLayoutFromMouseEvent(views::View* source,
                                                 const ui::MouseEvent& event) {
  if (!adjust_layout_)
    return;

  // The following code attempts to switch to shrink (not stacked) layout when
  // the mouse exits the tabstrip (or the mouse is pressed on a stacked tab) and
  // to stacked layout when a touch device is used. This is made problematic by
  // windows generating mouse move events that do not clearly indicate the move
  // is the result of a touch device. This assumes a real mouse is used if
  // |kMouseMoveCountBeforeConsiderReal| mouse move events are received within
  // the time window |kMouseMoveTimeMS|.  At the time we get a mouse press we
  // know whether its from a touch device or not, but we don't layout then else
  // everything shifts. Instead we wait for the release.
  //
  // TODO(sky): revisit this when touch events are really plumbed through.

  switch (event.type()) {
    case ui::ET_MOUSE_PRESSED:
      mouse_move_count_ = 0;
      last_mouse_move_time_ = base::TimeTicks();
      SetResetToShrinkOnExit((event.flags() & ui::EF_FROM_TOUCH) == 0);
      if (reset_to_shrink_on_exit_ && touch_layout_) {
        gfx::Point tab_strip_point(event.location());
        views::View::ConvertPointToTarget(source, this, &tab_strip_point);
        Tab* tab = FindTabForEvent(tab_strip_point);
        if (tab && touch_layout_->IsStacked(GetModelIndexOfTab(tab))) {
          SetStackedLayout(false);
          controller_->StackedLayoutMaybeChanged();
        }
      }
      break;

    case ui::ET_MOUSE_MOVED: {
#if defined(OS_CHROMEOS)
      // Ash does not synthesize mouse events from touch events.
      SetResetToShrinkOnExit(true);
#else
      gfx::Point location(event.location());
      ConvertPointToTarget(source, this, &location);
      if (location == last_mouse_move_location_)
        return;  // Ignore spurious moves.
      last_mouse_move_location_ = location;
      if ((event.flags() & ui::EF_FROM_TOUCH) == 0 &&
          (event.flags() & ui::EF_IS_SYNTHESIZED) == 0) {
        if ((base::TimeTicks::Now() - last_mouse_move_time_).InMilliseconds() <
            kMouseMoveTimeMS) {
          if (mouse_move_count_++ == kMouseMoveCountBeforeConsiderReal)
            SetResetToShrinkOnExit(true);
        } else {
          mouse_move_count_ = 1;
          last_mouse_move_time_ = base::TimeTicks::Now();
        }
      } else {
        last_mouse_move_time_ = base::TimeTicks();
      }
#endif
      break;
    }

    case ui::ET_MOUSE_RELEASED: {
      gfx::Point location(event.location());
      ConvertPointToTarget(source, this, &location);
      last_mouse_move_location_ = location;
      mouse_move_count_ = 0;
      last_mouse_move_time_ = base::TimeTicks();
      if ((event.flags() & ui::EF_FROM_TOUCH) == ui::EF_FROM_TOUCH) {
        SetStackedLayout(true);
        controller_->StackedLayoutMaybeChanged();
      }
      break;
    }

    default:
      break;
  }
}

void TabStrip::ResizeLayoutTabs() {
  // We've been called back after the TabStrip has been emptied out (probably
  // just prior to the window being destroyed). We need to do nothing here or
  // else GetTabAt below will crash.
  if (tab_count() == 0)
    return;

  // It is critically important that this is unhooked here, otherwise we will
  // keep spying on messages forever.
  RemoveMessageLoopObserver();

  in_tab_close_ = false;
  available_width_for_tabs_ = -1;
  int pinned_tab_count = GetPinnedTabCount();
  if (pinned_tab_count == tab_count()) {
    // Only pinned tabs, we know the tab widths won't have changed (all
    // pinned tabs have the same width), so there is nothing to do.
    return;
  }
  // Don't try and avoid layout based on tab sizes. If tabs are small enough
  // then the width of the active tab may not change, but other widths may
  // have. This is particularly important if we've overflowed (all tabs are at
  // the min).
  StartResizeLayoutAnimation();
}

void TabStrip::ResizeLayoutTabsFromTouch() {
  // Don't resize if the user is interacting with the tabstrip.
  if (!drag_controller_.get())
    ResizeLayoutTabs();
  else
    StartResizeLayoutTabsFromTouchTimer();
}

void TabStrip::StartResizeLayoutTabsFromTouchTimer() {
  resize_layout_timer_.Stop();
  resize_layout_timer_.Start(
      FROM_HERE, base::TimeDelta::FromMilliseconds(kTouchResizeLayoutTimeMS),
      this, &TabStrip::ResizeLayoutTabsFromTouch);
}

void TabStrip::SetTabBoundsForDrag(const std::vector<gfx::Rect>& tab_bounds) {
  StopAnimating(false);
  DCHECK_EQ(tab_count(), static_cast<int>(tab_bounds.size()));
  for (int i = 0; i < tab_count(); ++i)
    tab_at(i)->SetBoundsRect(tab_bounds[i]);
  // Reset the layout size as we've effectively layed out a different size.
  // This ensures a layout happens after the drag is done.
  last_layout_size_ = gfx::Size();
}

void TabStrip::AddMessageLoopObserver() {
  if (!mouse_watcher_.get()) {
    mouse_watcher_ = std::make_unique<views::MouseWatcher>(
        std::make_unique<views::MouseWatcherViewHost>(
            this, gfx::Insets(0, 0, kTabStripAnimationVSlop, 0)),
        this);
  }
  mouse_watcher_->Start();
}

void TabStrip::RemoveMessageLoopObserver() {
  mouse_watcher_.reset(NULL);
}

gfx::Rect TabStrip::GetDropBounds(int drop_index,
                                  bool drop_before,
                                  bool* is_beneath) {
  DCHECK_NE(drop_index, -1);

  const int overlap = Tab::GetOverlap();
  int center_x;
  if (drop_index < tab_count()) {
    Tab* tab = tab_at(drop_index);
    center_x = tab->x() + ((drop_before ? overlap : tab->width()) / 2);
  } else {
    Tab* last_tab = tab_at(drop_index - 1);
    center_x = last_tab->x() + last_tab->width() - (overlap / 2);
  }

  // Mirror the center point if necessary.
  center_x = GetMirroredXInView(center_x);

  // Determine the screen bounds.
  gfx::Point drop_loc(center_x - drop_indicator_width / 2,
                      -drop_indicator_height);
  ConvertPointToScreen(this, &drop_loc);
  gfx::Rect drop_bounds(drop_loc.x(), drop_loc.y(), drop_indicator_width,
                        drop_indicator_height);

  // If the rect doesn't fit on the monitor, push the arrow to the bottom.
  display::Screen* screen = display::Screen::GetScreen();
  display::Display display = screen->GetDisplayMatching(drop_bounds);
  *is_beneath = !display.bounds().Contains(drop_bounds);
  if (*is_beneath)
    drop_bounds.Offset(0, drop_bounds.height() + height());

  return drop_bounds;
}

void TabStrip::SetDropArrow(
    const base::Optional<BrowserRootView::DropIndex>& index) {
  if (!index) {
    controller_->OnDropIndexUpdate(-1, false);
    drop_arrow_.reset();
    return;
  }

  // Let the controller know of the index update.
  controller_->OnDropIndexUpdate(index->value, index->drop_before);

  if (drop_arrow_ && (index == drop_arrow_->index))
    return;

  bool is_beneath;
  gfx::Rect drop_bounds =
      GetDropBounds(index->value, index->drop_before, &is_beneath);

  if (!drop_arrow_) {
    drop_arrow_ = std::make_unique<DropArrow>(*index, !is_beneath, GetWidget());
  } else {
    drop_arrow_->index = *index;
    if (is_beneath == drop_arrow_->point_down) {
      drop_arrow_->point_down = !is_beneath;
      drop_arrow_->arrow_view->SetImage(
          GetDropArrowImage(drop_arrow_->point_down));
    }
  }

  // Reposition the window. Need to show it too as the window is initially
  // hidden.
  drop_arrow_->arrow_window->SetBounds(drop_bounds);
  drop_arrow_->arrow_window->Show();
}

// static
gfx::ImageSkia* TabStrip::GetDropArrowImage(bool is_down) {
  return ui::ResourceBundle::GetSharedInstance().GetImageSkiaNamed(
      is_down ? IDR_TAB_DROP_DOWN : IDR_TAB_DROP_UP);
}

// TabStrip:DropArrow:
// ----------------------------------------------------------

TabStrip::DropArrow::DropArrow(const BrowserRootView::DropIndex& index,
                               bool point_down,
                               views::Widget* context)
    : index(index), point_down(point_down) {
  arrow_view = new views::ImageView;
  arrow_view->SetImage(GetDropArrowImage(point_down));

  arrow_window = new views::Widget;
  views::Widget::InitParams params(views::Widget::InitParams::TYPE_POPUP);
  params.keep_on_top = true;
  params.opacity = views::Widget::InitParams::TRANSLUCENT_WINDOW;
  params.accept_events = false;
  params.bounds = gfx::Rect(drop_indicator_width, drop_indicator_height);
  params.context = context->GetNativeWindow();
  arrow_window->Init(params);
  arrow_window->SetContentsView(arrow_view);
}

TabStrip::DropArrow::~DropArrow() {
  // Close eventually deletes the window, which deletes arrow_view too.
  arrow_window->Close();
}

///////////////////////////////////////////////////////////////////////////////

void TabStrip::PrepareForAnimation() {
  if (!IsDragSessionActive() && !TabDragController::IsAttachedTo(this)) {
    for (int i = 0; i < tab_count(); ++i)
      tab_at(i)->set_dragging(false);
  }
}

void TabStrip::GenerateIdealBounds() {
  if (tab_count() == 0)
    return;  // Should only happen during creation/destruction, ignore.

  const int old_max_x = GetTabsMaxX();

  if (!touch_layout_) {
    const int available_width = (available_width_for_tabs_ < 0)
                                    ? GetTabAreaWidth()
                                    : available_width_for_tabs_;
    const std::vector<gfx::Rect> tabs_bounds = CalculateBounds(
        GetTabSizeInfo(), GetPinnedTabCount(), tab_count(),
        controller_->GetActiveIndex(), TabStartX(), available_width,
        &current_active_width_, &current_inactive_width_);
    DCHECK_EQ(static_cast<size_t>(tab_count()), tabs_bounds.size());

    for (size_t i = 0; i < tabs_bounds.size(); ++i)
      tabs_.set_ideal_bounds(i, tabs_bounds[i]);
  }

  new_tab_button_bounds_.set_origin(gfx::Point(NewTabButtonX(), 0));

  if (GetTabsMaxX() != old_max_x) {
    for (TabStripObserver& observer : observers_)
      observer.OnTabsMaxXChanged();
  }
}

int TabStrip::GenerateIdealBoundsForPinnedTabs(int* first_non_pinned_index) {
  const int num_pinned_tabs = GetPinnedTabCount();

  if (first_non_pinned_index)
    *first_non_pinned_index = num_pinned_tabs;

  const int start_x = TabStartX();
  if (num_pinned_tabs == 0)
    return start_x;

  std::vector<gfx::Rect> tab_bounds(tab_count());
  int non_pinned_x = CalculateBoundsForPinnedTabs(
      GetTabSizeInfo(), num_pinned_tabs, tab_count(), start_x, &tab_bounds);
  for (int i = 0; i < num_pinned_tabs; ++i)
    tabs_.set_ideal_bounds(i, tab_bounds[i]);
  return non_pinned_x;
}

int TabStrip::GetTabAreaWidth() const {
  return width() - GetFrameGrabWidth() - GetNewTabButtonWidth(IsIncognito());
}

void TabStrip::StartResizeLayoutAnimation() {
  PrepareForAnimation();
  GenerateIdealBounds();
  AnimateToIdealBounds();
}

void TabStrip::StartPinnedTabAnimation() {
  in_tab_close_ = false;
  available_width_for_tabs_ = -1;

  PrepareForAnimation();

  GenerateIdealBounds();
  AnimateToIdealBounds();
}

void TabStrip::StartMouseInitiatedRemoveTabAnimation(int model_index) {
  PrepareForAnimation();

  Tab* tab_closing = tab_at(model_index);
  tab_closing->set_closing(true);

  // We still need to paint the tab until we actually remove it. Put it in
  // tabs_closing_map_ so we can find it.
  RemoveTabFromViewModel(model_index);

  GenerateIdealBounds();
  AnimateToIdealBounds();

  gfx::Rect tab_bounds = tab_closing->bounds();
  tab_bounds.set_width(0);
  bounds_animator_.AnimateViewTo(tab_closing, tab_bounds);

  // Register delegate to do cleanup when done.
  bounds_animator_.SetAnimationDelegate(
      tab_closing, std::make_unique<RemoveTabDelegate>(this, tab_closing));
}

bool TabStrip::IsPointInTab(Tab* tab,
                            const gfx::Point& point_in_tabstrip_coords) {
  gfx::Point point_in_tab_coords(point_in_tabstrip_coords);
  View::ConvertPointToTarget(this, tab, &point_in_tab_coords);
  return tab->HitTestPoint(point_in_tab_coords);
}

int TabStrip::GetStartXForNormalTabs() const {
  const int start_x = TabStartX();
  const int pinned_tab_count = GetPinnedTabCount();
  if (pinned_tab_count == 0)
    return start_x;
  return start_x +
         pinned_tab_count * (Tab::GetPinnedWidth() - Tab::GetOverlap()) +
         kPinnedToNonPinnedOffset;
}

// static
void TabStrip::ResetTabSizeInfoForTesting() {
  if (g_tab_size_info) {
    delete g_tab_size_info;
    g_tab_size_info = nullptr;
  }
}

Tab* TabStrip::FindTabForEvent(const gfx::Point& point) {
  DCHECK(touch_layout_);
  int active_tab_index = touch_layout_->active_index();
  Tab* tab = FindTabForEventFrom(point, active_tab_index, -1);
  return tab ? tab : FindTabForEventFrom(point, active_tab_index + 1, 1);
}

Tab* TabStrip::FindTabForEventFrom(const gfx::Point& point,
                                   int start,
                                   int delta) {
  // |start| equals tab_count() when there are only pinned tabs.
  if (start == tab_count())
    start += delta;
  for (int i = start; i >= 0 && i < tab_count(); i += delta) {
    if (IsPointInTab(tab_at(i), point))
      return tab_at(i);
  }
  return NULL;
}

Tab* TabStrip::FindTabHitByPoint(const gfx::Point& point) {
  // The display order doesn't necessarily match the child order, so we iterate
  // in display order.
  for (int i = 0; i < tab_count(); ++i) {
    // If we don't first exclude points outside the current tab, the code below
    // will return the wrong tab if the next tab is selected, the following tab
    // is active, and |point| is in the overlap region between the two.
    Tab* tab = tab_at(i);
    if (!IsPointInTab(tab, point))
      continue;

    // Selected tabs render atop unselected ones, and active tabs render atop
    // everything.  Check whether the next tab renders atop this one and |point|
    // is in the overlap region.
    Tab* next_tab = i < (tab_count() - 1) ? tab_at(i + 1) : nullptr;
    if (next_tab &&
        (next_tab->IsActive() ||
         (next_tab->IsSelected() && !tab->IsSelected())) &&
        IsPointInTab(next_tab, point))
      return next_tab;

    // This is the topmost tab for this point.
    return tab;
  }

  return nullptr;
}

std::vector<int> TabStrip::GetTabXCoordinates() {
  std::vector<int> results;
  for (int i = 0; i < tab_count(); ++i)
    results.push_back(ideal_bounds(i).x());
  return results;
}

void TabStrip::SwapLayoutIfNecessary() {
  bool needs_touch = NeedsTouchLayout();
  bool using_touch = touch_layout_ != NULL;
  if (needs_touch == using_touch)
    return;

  if (needs_touch) {
    gfx::Size tab_size(GetLayoutConstant(TAB_STACK_TAB_WIDTH),
                       GetLayoutConstant(TAB_HEIGHT));

    const int overlap = Tab::GetOverlap();
    touch_layout_.reset(new StackedTabStripLayout(
        tab_size, overlap, kStackedPadding, kMaxStackedCount, &tabs_));
    touch_layout_->SetWidth(GetTabAreaWidth());
    // This has to be after SetWidth() as SetWidth() is going to reset the
    // bounds of the pinned tabs (since StackedTabStripLayout doesn't yet know
    // how many pinned tabs there are).
    GenerateIdealBoundsForPinnedTabs(NULL);
    touch_layout_->SetXAndPinnedCount(GetStartXForNormalTabs(),
                                      GetPinnedTabCount());
    touch_layout_->SetActiveIndex(controller_->GetActiveIndex());

    base::RecordAction(UserMetricsAction("StackedTab_EnteredStackedLayout"));
  } else {
    touch_layout_.reset();
  }
  PrepareForAnimation();
  GenerateIdealBounds();
  SetTabVisibility();
  AnimateToIdealBounds();
}

bool TabStrip::NeedsTouchLayout() const {
  if (!stacked_layout_)
    return false;

  int pinned_tab_count = GetPinnedTabCount();
  int normal_count = tab_count() - pinned_tab_count;
  if (normal_count <= 1 || normal_count == pinned_tab_count)
    return false;
  return (GetLayoutConstant(TAB_STACK_TAB_WIDTH) * normal_count -
          Tab::GetOverlap() * (normal_count - 1)) >
         GetTabAreaWidth() - GetStartXForNormalTabs();
}

void TabStrip::SetResetToShrinkOnExit(bool value) {
  if (!adjust_layout_)
    return;

  // We have to be using stacked layout to reset out of it.
  value &= stacked_layout_;

  if (value == reset_to_shrink_on_exit_)
    return;

  reset_to_shrink_on_exit_ = value;
  // Add an observer so we know when the mouse moves out of the tabstrip.
  if (reset_to_shrink_on_exit_)
    AddMessageLoopObserver();
  else
    RemoveMessageLoopObserver();
}

void TabStrip::ButtonPressed(views::Button* sender, const ui::Event& event) {
  if (sender == new_tab_button_) {
    base::RecordAction(UserMetricsAction("NewTab_Button"));
    UMA_HISTOGRAM_ENUMERATION("Tab.NewTab", TabStripModel::NEW_TAB_BUTTON,
                              TabStripModel::NEW_TAB_ENUM_COUNT);
    if (event.IsMouseEvent()) {
      const ui::MouseEvent& mouse = static_cast<const ui::MouseEvent&>(event);
      if (mouse.IsOnlyMiddleMouseButton()) {
        if (ui::Clipboard::IsSupportedClipboardType(
                ui::CLIPBOARD_TYPE_SELECTION)) {
          ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
          CHECK(clipboard);
          base::string16 clipboard_text;
          clipboard->ReadText(ui::CLIPBOARD_TYPE_SELECTION, &clipboard_text);
          if (!clipboard_text.empty())
            controller_->CreateNewTabWithLocation(clipboard_text);
        }
        return;
      }
    }

    controller_->CreateNewTab();
    if (event.type() == ui::ET_GESTURE_TAP)
      TouchUMA::RecordGestureAction(TouchUMA::GESTURE_NEWTAB_TAP);
  }
}

// Overridden to support automation. See automation_proxy_uitest.cc.
const views::View* TabStrip::GetViewByID(int view_id) const {
  if (tab_count() > 0) {
    if (view_id == VIEW_ID_TAB_LAST)
      return tab_at(tab_count() - 1);
    if ((view_id >= VIEW_ID_TAB_0) && (view_id < VIEW_ID_TAB_LAST)) {
      int index = view_id - VIEW_ID_TAB_0;
      return (index >= 0 && index < tab_count()) ? tab_at(index) : NULL;
    }
  }

  return View::GetViewByID(view_id);
}

bool TabStrip::OnMousePressed(const ui::MouseEvent& event) {
  UpdateStackedLayoutFromMouseEvent(this, event);
  // We can't return true here, else clicking in an empty area won't drag the
  // window.
  return false;
}

bool TabStrip::OnMouseDragged(const ui::MouseEvent& event) {
  ContinueDrag(this, event);
  return true;
}

void TabStrip::OnMouseReleased(const ui::MouseEvent& event) {
  EndDrag(END_DRAG_COMPLETE);
  UpdateStackedLayoutFromMouseEvent(this, event);
}

void TabStrip::OnMouseCaptureLost() {
  EndDrag(END_DRAG_CAPTURE_LOST);
}

void TabStrip::OnMouseMoved(const ui::MouseEvent& event) {
  UpdateStackedLayoutFromMouseEvent(this, event);
}

void TabStrip::OnMouseEntered(const ui::MouseEvent& event) {
  SetResetToShrinkOnExit(true);
}

void TabStrip::OnGestureEvent(ui::GestureEvent* event) {
  SetResetToShrinkOnExit(false);
  switch (event->type()) {
    case ui::ET_GESTURE_SCROLL_END:
    case ui::ET_SCROLL_FLING_START:
    case ui::ET_GESTURE_END:
      EndDrag(END_DRAG_COMPLETE);
      if (adjust_layout_) {
        SetStackedLayout(true);
        controller_->StackedLayoutMaybeChanged();
      }
      break;

    case ui::ET_GESTURE_LONG_PRESS:
      if (drag_controller_.get())
        drag_controller_->SetMoveBehavior(TabDragController::REORDER);
      break;

    case ui::ET_GESTURE_LONG_TAP: {
      EndDrag(END_DRAG_CANCEL);
      gfx::Point local_point = event->location();
      Tab* tab = touch_layout_ ? FindTabForEvent(local_point)
                               : FindTabHitByPoint(local_point);
      if (tab) {
        ConvertPointToScreen(this, &local_point);
        ShowContextMenuForTab(tab, local_point, ui::MENU_SOURCE_TOUCH);
      }
      break;
    }

    case ui::ET_GESTURE_SCROLL_UPDATE:
      ContinueDrag(this, *event);
      break;

    case ui::ET_GESTURE_TAP_DOWN:
      EndDrag(END_DRAG_CANCEL);
      break;

    case ui::ET_GESTURE_TAP: {
      const int active_index = controller_->GetActiveIndex();
      DCHECK_NE(-1, active_index);
      Tab* active_tab = tab_at(active_index);
      TouchUMA::GestureActionType action = TouchUMA::GESTURE_TABNOSWITCH_TAP;
      if (active_tab->tab_activated_with_last_tap_down())
        action = TouchUMA::GESTURE_TABSWITCH_TAP;
      TouchUMA::RecordGestureAction(action);
      break;
    }

    default:
      break;
  }
  event->SetHandled();
}

views::View* TabStrip::TargetForRect(views::View* root, const gfx::Rect& rect) {
  CHECK_EQ(root, this);

  if (!views::UsePointBasedTargeting(rect))
    return views::ViewTargeterDelegate::TargetForRect(root, rect);
  const gfx::Point point(rect.CenterPoint());

  if (!touch_layout_) {
    // Return any view that isn't a Tab or this TabStrip immediately. We don't
    // want to interfere.
    views::View* v = views::ViewTargeterDelegate::TargetForRect(root, rect);
    if (v && v != this && strcmp(v->GetClassName(), Tab::kViewClassName))
      return v;

    views::View* tab = FindTabHitByPoint(point);
    if (tab)
      return tab;
  } else {
    if (new_tab_button_->visible()) {
      views::View* view =
          ConvertPointToViewAndGetEventHandler(this, new_tab_button_, point);
      if (view)
        return view;
    }
    Tab* tab = FindTabForEvent(point);
    if (tab)
      return ConvertPointToViewAndGetEventHandler(this, tab, point);
  }
  return this;
}
