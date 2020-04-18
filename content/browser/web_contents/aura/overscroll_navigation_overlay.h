// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEB_CONTENTS_AURA_OVERSCROLL_NAVIGATION_OVERLAY_H_
#define CONTENT_BROWSER_WEB_CONTENTS_AURA_OVERSCROLL_NAVIGATION_OVERLAY_H_

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "content/browser/web_contents/aura/overscroll_window_animation.h"
#include "content/browser/web_contents/aura/types.h"
#include "content/browser/web_contents/web_contents_view_aura.h"
#include "content/common/content_export.h"
#include "content/public/browser/web_contents_observer.h"
#include "ui/gfx/image/image.h"

namespace content {

class OverscrollNavigationOverlayTest;

// When a history navigation is triggered at the end of an overscroll
// navigation, it is necessary to show the history-screenshot until the page is
// done navigating and painting. This class accomplishes this by calling the
// navigation and creating, showing and destroying the screenshot window on top
// of the page until the page has completed loading and painting. When the
// overscroll completes, this screenshot window is returned by
// OnOverscrollComplete and |window_| is set to own it.
// There are two overscroll cases, for the first one the main window is the web
// contents window. At this stage, |window_| is null. The second case is
// triggered if the user overscrolls after |window_| is set, before the page
// finishes loading. When this happens, |window_| is the main window.
class CONTENT_EXPORT OverscrollNavigationOverlay
    : public WebContentsObserver,
      public OverscrollWindowAnimation::Delegate {
 public:
  OverscrollNavigationOverlay(WebContentsImpl* web_contents,
                              aura::Window* web_contents_window);

  ~OverscrollNavigationOverlay() override;

  // Returns a pointer to the relay delegate we own.
  OverscrollControllerDelegate* relay_delegate() { return owa_.get(); }

 private:
  friend class OverscrollNavigationOverlayTest;
  FRIEND_TEST_ALL_PREFIXES(OverscrollNavigationOverlayTest, WithScreenshot);
  FRIEND_TEST_ALL_PREFIXES(OverscrollNavigationOverlayTest, WithoutScreenshot);
  FRIEND_TEST_ALL_PREFIXES(OverscrollNavigationOverlayTest, CannotNavigate);
  FRIEND_TEST_ALL_PREFIXES(OverscrollNavigationOverlayTest, ForwardNavigation);
  FRIEND_TEST_ALL_PREFIXES(OverscrollNavigationOverlayTest,
                           ForwardNavigationCancelled);
  FRIEND_TEST_ALL_PREFIXES(OverscrollNavigationOverlayTest, CancelNavigation);
  FRIEND_TEST_ALL_PREFIXES(OverscrollNavigationOverlayTest,
                           CancelAfterSuccessfulNavigation);
  FRIEND_TEST_ALL_PREFIXES(OverscrollNavigationOverlayTest, OverlayWindowSwap);
  FRIEND_TEST_ALL_PREFIXES(OverscrollNavigationOverlayTest,
                           CloseDuringAnimation);
  FRIEND_TEST_ALL_PREFIXES(OverscrollNavigationOverlayTest,
                           ImmediateLoadOnNavigate);

  // Resets state and starts observing |web_contents_| for page load/paint
  // updates. This function makes sure that the screenshot window is stacked
  // on top, so that it hides the content window behind it, and destroys the
  // screenshot window when the page is done loading/painting.
  // This should be called immediately after initiating the navigation,
  // otherwise the overlay may be dismissed prematurely.
  void StartObserving();

  // Stop observing the page and start the final overlay fade-out animation if
  // there's no active overscroll window animation.
  void StopObservingIfDone();

  // Creates a window that shows a history-screenshot and is stacked relative to
  // the current overscroll |direction_| with the given |bounds|.
  std::unique_ptr<aura::Window> CreateOverlayWindow(const gfx::Rect& bounds);

  // Returns an image with the history-screenshot for the previous or next page,
  // according to the given |direction|.
  const gfx::Image GetImageForDirection(NavigationDirection direction) const;

  // Overridden from OverscrollWindowAnimation::Delegate:
  std::unique_ptr<aura::Window> CreateFrontWindow(
      const gfx::Rect& bounds) override;
  std::unique_ptr<aura::Window> CreateBackWindow(
      const gfx::Rect& bounds) override;
  aura::Window* GetMainWindow() const override;
  void OnOverscrollCompleting() override;
  void OnOverscrollCompleted(std::unique_ptr<aura::Window> window) override;
  void OnOverscrollCancelled() override;

  // Overridden from WebContentsObserver:
  void DidFirstVisuallyNonEmptyPaint() override;
  void DidStopLoading() override;

  // The current overscroll direction.
  NavigationDirection direction_;

  // The web contents that are being navigated.
  WebContentsImpl* web_contents_;

  // The overlay window that shows a screenshot during an overscroll gesture and
  // handles overscroll events during the second overscroll case.
  std::unique_ptr<aura::Window> window_;

  bool loading_complete_;
  bool received_paint_update_;

  // URL of the NavigationEntry we are navigating to. This is needed to
  // filter on WebContentsObserver callbacks and is used to dismiss the overlay
  // when the relevant page loads and paints.
  GURL pending_entry_url_;

  // Manages the overscroll animations.
  std::unique_ptr<OverscrollWindowAnimation> owa_;

  // The window that hosts the web contents.
  aura::Window* web_contents_window_;

  DISALLOW_COPY_AND_ASSIGN(OverscrollNavigationOverlay);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEB_CONTENTS_AURA_OVERSCROLL_NAVIGATION_OVERLAY_H_
