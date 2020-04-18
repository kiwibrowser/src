// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_THUMBNAILS_THUMBNAIL_TAB_HELPER_H_
#define CHROME_BROWSER_THUMBNAILS_THUMBNAIL_TAB_HELPER_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "base/time/time.h"
#include "chrome/browser/thumbnails/thumbnailing_context.h"
#include "content/public/browser/render_widget_host_observer.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "ui/base/page_transition_types.h"

namespace content {
class NavigationHandle;
class RenderViewHost;
}  // namespace content

namespace thumbnails {
class ThumbnailService;
}  // namespace thumbnails

class ThumbnailTabHelper
    : public content::RenderWidgetHostObserver,
      public content::WebContentsObserver,
      public content::WebContentsUserData<ThumbnailTabHelper> {
 public:
  ~ThumbnailTabHelper() override;

 private:
  explicit ThumbnailTabHelper(content::WebContents* contents);
  friend class content::WebContentsUserData<ThumbnailTabHelper>;

  enum class TriggerReason {
    TAB_HIDDEN,
    NAVIGATING_AWAY,
  };

  // Used for UMA histograms. Don't change or delete entries, and only add new
  // ones at the end.
  enum class Outcome {
    SUCCESS = 0,
    NOT_ATTEMPTED_PENDING_NAVIGATION,
    NOT_ATTEMPTED_NO_PAINT_YET,
    NOT_ATTEMPTED_IN_PROGRESS,
    NOT_ATTEMPTED_NO_WEBCONTENTS,
    NOT_ATTEMPTED_NO_URL,
    NOT_ATTEMPTED_SHOULD_NOT_ACQUIRE,
    NOT_ATTEMPTED_VIEW_NOT_AVAILABLE,
    NOT_ATTEMPTED_EMPTY_RECT,
    CANCELED,
    READBACK_FAILED,
    // Add new entries here!
    COUNT
  };

  // content::RenderWidgetHostObserver overrides.
  void RenderWidgetHostVisibilityChanged(content::RenderWidgetHost* widget_host,
                                         bool became_visible) override;
  void RenderWidgetHostDestroyed(
      content::RenderWidgetHost* widget_host) override;

  // content::WebContentsObserver overrides.
  void RenderViewCreated(content::RenderViewHost* render_view_host) override;
  void RenderViewHostChanged(content::RenderViewHost* old_host,
                             content::RenderViewHost* new_host) override;
  void RenderViewDeleted(content::RenderViewHost* render_view_host) override;
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DocumentAvailableInMainFrame() override;
  void DocumentOnLoadCompletedInMainFrame() override;
  void DidFirstVisuallyNonEmptyPaint() override;
  void DidStartLoading() override;
  void NavigationStopped() override;

  void StartWatchingRenderViewHost(content::RenderViewHost* render_view_host);
  void StopWatchingRenderViewHost(content::RenderViewHost* render_view_host);

  // Starts the process of capturing a thumbnail of the current tab contents if
  // necessary and possible.
  void StartThumbnailCaptureIfNecessary(TriggerReason trigger);

  // Creates a thumbnail from the web contents bitmap.
  void ProcessCapturedBitmap(TriggerReason trigger, const SkBitmap& bitmap);

  // Passes the thumbnail to the thumbnail service.
  void StoreThumbnail(const SkBitmap& thumbnail);

  // Cleans up after thumbnail generation has ended.
  void CleanUpFromThumbnailGeneration();

  // Called when the current tab gets hidden.
  void TabHidden();

  scoped_refptr<thumbnails::ThumbnailService> GetThumbnailService();

  static void LogThumbnailingOutcome(TriggerReason trigger, Outcome outcome);

  ScopedObserver<content::RenderWidgetHost, content::RenderWidgetHostObserver>
      observer_;

  bool did_navigation_finish_;
  bool has_received_document_since_navigation_finished_;
  bool has_painted_since_document_received_;

  ui::PageTransition page_transition_;
  bool load_interrupted_;

  scoped_refptr<thumbnails::ThumbnailingContext> thumbnailing_context_;
  bool waiting_for_capture_;

  base::TimeTicks copy_from_surface_start_time_;

  base::WeakPtrFactory<ThumbnailTabHelper> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ThumbnailTabHelper);
};

#endif  // CHROME_BROWSER_THUMBNAILS_THUMBNAIL_TAB_HELPER_H_
