// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_MODEL_H_
#define IOS_CLEAN_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_MODEL_H_

#import <CoreGraphics/CoreGraphics.h>
#include <cmath>

#include "base/macros.h"
#include "base/observer_list.h"
#import "ios/chrome/browser/ui/broadcaster/chrome_broadcast_observer_bridge.h"

class FullscreenModelObserver;

// Model object used to calculate fullscreen state.
class FullscreenModel : public ChromeBroadcastObserverInterface {
 public:
  FullscreenModel();
  ~FullscreenModel() override;

  // Adds and removes FullscreenModelObservers.
  void AddObserver(FullscreenModelObserver* observer) {
    observers_.AddObserver(observer);
  }
  void RemoveObserver(FullscreenModelObserver* observer) {
    observers_.RemoveObserver(observer);
  }

  // The progress value calculated by the model.
  CGFloat progress() const { return progress_; }

  // Whether fullscreen is disabled.  When disabled, the toolbar is completely
  // visible.
  bool enabled() const { return disabled_counter_ == 0U; }

  // Whether the base offset has been recorded after state has been invalidated
  // by navigations or toolbar height changes.
  bool has_base_offset() const { return !std::isnan(base_offset_); }

  // The base offset against which the fullscreen progress is being calculated.
  CGFloat base_offset() const { return base_offset_; }

  // Increments and decrements |disabled_counter_| for features that require the
  // toolbar be completely visible.
  void IncrementDisabledCounter();
  void DecrementDisabledCounter();

  // Recalculates the fullscreen progress for a new navigation.
  void ResetForNavigation();

  // Called when a scroll end animation finishes.  |progress| is the fullscreen
  // progress corresponding to the final state of the aniamtion.
  void AnimationEndedWithProgress(CGFloat progress);

  // Setter for the toolbar height to use in calculations.  Setting this resets
  // the model to a fully visible state.
  void SetToolbarHeight(CGFloat toolbar_height);
  CGFloat GetToolbarHeight() const;

  // Setter for the height of the scroll view displaying the main content.
  void SetScrollViewHeight(CGFloat scroll_view_height);
  CGFloat GetScrollViewHeight() const;

  // Setter for the current height of the rendered page.
  void SetContentHeight(CGFloat content_height);
  CGFloat GetContentHeight() const;

  // Setter for the top content inset of the scroll view displaying the main
  // content.
  void SetTopContentInset(CGFloat top_inset);
  CGFloat GetTopContentInset() const;

  // Setter for the current vertical content offset.  Setting this will
  // recalculate the progress value.
  void SetYContentOffset(CGFloat y_content_offset);
  CGFloat GetYContentOffset() const;

  // Setter for whether the scroll view is scrolling.  If a scroll event ends
  // and the progress value is not 0.0 or 1.0, the model will round to the
  // nearest value.
  void SetScrollViewIsScrolling(bool scrolling);
  bool IsScrollViewScrolling() const;

  // Setter for whether the scroll view is zooming.
  void SetScrollViewIsZooming(bool zooming);
  bool IsScrollViewZooming() const;

  // Setter for whether the scroll view is being dragged.
  void SetScrollViewIsDragging(bool dragging);
  bool IsScrollViewDragging() const;

 private:
  // Returns how a scroll to the current |y_content_offset_| from |from_offset|
  // should be handled.
  enum class ScrollAction : short {
    kIgnore,                       // Ignore the scroll.
    kUpdateBaseOffset,             // Update |base_offset_| only.
    kUpdateProgress,               // Update |progress_| only.
    kUpdateBaseOffsetAndProgress,  // Update |bse_offset_| and |progress_|.
  };
  ScrollAction ActionForScrollFromOffset(CGFloat from_offset) const;

  // Updates the base offset given the current y content offset, progress, and
  // toolbar height.
  void UpdateBaseOffset();

  // Updates the progress value given the current y content offset, base offset,
  // and toolbar height.
  void UpdateProgress();

  // Setter for |progress_|.  Notifies observers of the new value if
  // |notify_observers| is true.
  void SetProgress(CGFloat progress);

  // ChromeBroadcastObserverInterface:
  void OnScrollViewSizeBroadcasted(CGSize scroll_view_size) override;
  void OnScrollViewContentSizeBroadcasted(CGSize content_size) override;
  void OnScrollViewContentInsetBroadcasted(UIEdgeInsets content_inset) override;
  void OnContentScrollOffsetBroadcasted(CGFloat offset) override;
  void OnScrollViewIsScrollingBroadcasted(bool scrolling) override;
  void OnScrollViewIsZoomingBroadcasted(bool zooming) override;
  void OnScrollViewIsDraggingBroadcasted(bool dragging) override;
  void OnToolbarHeightBroadcasted(CGFloat toolbar_height) override;

  // The observers for this model.
  base::ObserverList<FullscreenModelObserver> observers_;
  // The percentage of the toolbar that should be visible, where 1.0 denotes a
  // fully visible toolbar and 0.0 denotes a completely hidden one.
  CGFloat progress_ = 0.0;
  // The base offset from which to calculate fullscreen state.  When |locked_|
  // is false, it is reset to the current offset after each scroll event.
  CGFloat base_offset_ = NAN;
  // The height of the toolbar being shown or hidden by this model.
  CGFloat toolbar_height_ = 0.0;
  // The current vertical content offset of the main content.
  CGFloat y_content_offset_ = 0.0;
  // The height of the scroll view displaying the current page.
  CGFloat scroll_view_height_ = 0.0;
  // The height of the current page's rendered content.
  CGFloat content_height_ = 0.0;
  // The top inset of the scroll view displaying the current page.
  CGFloat top_inset_ = 0.0;
  // How many currently-running features require the toolbar be visible.
  size_t disabled_counter_ = 0;
  // Whether the main content is being scrolled.
  bool scrolling_ = false;
  // Whether the scroll view is zooming.
  bool zooming_ = false;
  // Whether the main content is being dragged.
  bool dragging_ = false;
  // The number of FullscreenModelObserver callbacks currently being executed.
  size_t observer_callback_count_ = 0;

  DISALLOW_COPY_AND_ASSIGN(FullscreenModel);
};

#endif  // IOS_CLEAN_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_MODEL_H_
