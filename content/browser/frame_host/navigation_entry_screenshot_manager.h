// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_FRAME_HOST_NAVIGATION_ENTRY_SCREENSHOT_MANAGER_H_
#define CONTENT_BROWSER_FRAME_HOST_NAVIGATION_ENTRY_SCREENSHOT_MANAGER_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "content/common/content_export.h"

class SkBitmap;

namespace content {

class NavigationControllerImpl;
class NavigationEntryImpl;
class RenderViewHost;
class ScreenshotData;

// NavigationEntryScreenshotManager takes care of taking image-captures for the
// current navigation entry of a NavigationControllerImpl, and managing these
// captured images. These image-captures are used for history navigation using
// overscroll gestures.
class CONTENT_EXPORT NavigationEntryScreenshotManager {
 public:
  explicit NavigationEntryScreenshotManager(
      NavigationControllerImpl* controller);
  virtual ~NavigationEntryScreenshotManager();

  // Takes a screenshot of the last-committed entry of the controller.
  void TakeScreenshot();

  // Clears screenshots of all navigation entries.
  void ClearAllScreenshots();

 protected:
  // Overridden by tests to be notified when a screenshot will be taken. Tests
  // can override OnScreenshotSet() to be notified after the screenshot is
  // taken.
  virtual void WillTakeScreenshot(RenderViewHost* host) {}

  // Called after a screenshot has been set on an NavigationEntryImpl.
  // Overridden in tests to get notified of when a screenshot is set.
  virtual void OnScreenshotSet(NavigationEntryImpl* entry);

  NavigationControllerImpl* owner() { return owner_; }

  void SetMinScreenshotIntervalMS(int interval_ms);

  // The callback invoked when taking the screenshot of the page is complete.
  // This sets the screenshot on the navigation entry.
  void OnScreenshotTaken(int unique_id, const SkBitmap& bitmap);

  // Returns the number of entries with screenshots.
  int GetScreenshotCount() const;

 private:
  // This is called when the screenshot data has beene encoded to PNG in a
  // worker thread.
  void OnScreenshotEncodeComplete(int unique_id,
                                  scoped_refptr<ScreenshotData> data);

  // Removes the screenshot for the entry, returning true if the entry had a
  // screenshot.
  bool ClearScreenshot(NavigationEntryImpl* entry);

  // The screenshots in the NavigationEntryImpls can accumulate and consume a
  // large amount of memory. This function makes sure that the memory
  // consumption is within a certain limit.
  void PurgeScreenshotsIfNecessary();

  // The navigation controller that owns this screenshot-manager.
  NavigationControllerImpl* owner_;

  base::Time last_screenshot_time_;
  int min_screenshot_interval_ms_;

  // Taking a screenshot and encoding them can be async. So use a weakptr for
  // the callback to make sure that the screenshot/encoding completion callback
  // does not trigger on a destroyed NavigationEntryScreenshotManager.
  base::WeakPtrFactory<NavigationEntryScreenshotManager> screenshot_factory_;

  DISALLOW_COPY_AND_ASSIGN(NavigationEntryScreenshotManager);
};

}  // namespace content

#endif  // CONTENT_BROWSER_FRAME_HOST_NAVIGATION_ENTRY_SCREENSHOT_MANAGER_H_
