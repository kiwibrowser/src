// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_BROWSER_CAST_CONTENT_WINDOW_H_
#define CHROMECAST_BROWSER_CAST_CONTENT_WINDOW_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "chromecast/graphics/cast_window_manager.h"
#include "content/public/browser/web_contents.h"
#include "ui/events/event.h"

namespace chromecast {
namespace shell {

enum class VisibilityType {
  UNKNOWN = 0,
  FULL_SCREEN = 1,
  PARTIAL_OUT = 2,
  HIDDEN = 3
};

enum class VisibilityPriority {
  // Default priority. It is up to system to decide how to show the activity.
  DEFAULT = 0,

  // The activity wants to occupy the full screen for some period of time and
  // then become hidden after a timeout.
  TRANSIENT_ACTIVITY = 1,

  // A high priority interruption occupies half of the screen if a sticky
  // activity is showing on the screen. Otherwise, it occupies the full screen.
  HIGH_PRIORITY_INTERRUPTION = 2,

  // The activity wants to be persistently visible. Unlike TRANSIENT_ACTIVITY,
  // there should be no timeout.
  STICKY_ACTIVITY = 3,

  // The activity should not be visible.
  HIDDEN = 4,
};

enum class GestureType { NO_GESTURE = 0, GO_BACK = 1 };

// Class that represents the "window" a WebContents is displayed in cast_shell.
// For Linux, this represents an Aura window. For Android, this is a Activity.
// See CastContentWindowAura and CastContentWindowAndroid.
class CastContentWindow {
 public:
  class Delegate {
   public:
    // Notify window destruction.
    virtual void OnWindowDestroyed() {}

    // Notifies that a key event was triggered on the window.
    virtual void OnKeyEvent(const ui::KeyEvent& key_event) {}

    // Check to see if the gesture can be handled by the delegate. This is
    // called prior to ConsumeGesture().
    virtual bool CanHandleGesture(GestureType gesture_type) = 0;

    // Consume and handle a UI gesture. Returns whether the gesture was
    // handled or not.
    virtual bool ConsumeGesture(GestureType gesture_type) = 0;

    // Notify visibility change for this window.
    virtual void OnVisibilityChange(VisibilityType visibility_type) {}

    // Returns app ID of cast activity or application.
    virtual std::string GetId() = 0;

   protected:
    virtual ~Delegate() {}
  };

  // Creates the platform specific CastContentWindow. |delegate| should outlive
  // the created CastContentWindow.
  static std::unique_ptr<CastContentWindow> Create(
      CastContentWindow::Delegate* delegate,
      bool is_headless,
      bool enable_touch_input);

  virtual ~CastContentWindow() {}

  // Creates a full-screen window for |web_contents| and displays it if
  // |is_visible| is true.
  // |web_contents| should outlive this CastContentWindow.
  // |window_manager| should outlive this CastContentWindow.
  // TODO(seantopping): This method probably shouldn't exist; this class should
  // use RAII instead.
  virtual void CreateWindowForWebContents(
      content::WebContents* web_contents,
      CastWindowManager* window_manager,
      bool is_visible,
      CastWindowManager::WindowId z_order,
      VisibilityPriority visibility_priority) = 0;

  // Enables touch input to be routed to the window's WebContents.
  virtual void EnableTouchInput(bool enabled) = 0;

  // Cast activity or application calls it to request for a visibility priority
  // change.
  virtual void RequestVisibility(VisibilityPriority visibility_priority) = 0;

  // Cast activity or application calls it to request for moving out of the
  // screen.
  virtual void RequestMoveOut() = 0;
};

}  // namespace shell
}  // namespace chromecast

#endif  // CHROMECAST_BROWSER_CAST_CONTENT_WINDOW_H_
