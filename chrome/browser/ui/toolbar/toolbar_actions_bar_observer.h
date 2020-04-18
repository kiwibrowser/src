// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_ACTIONS_BAR_OBSERVER_H_
#define CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_ACTIONS_BAR_OBSERVER_H_

class ToolbarActionsBarObserver {
 public:
  // Called when the toolbar actions bar is destroyed.
  virtual void OnToolbarActionsBarDestroyed() {}

  // Called when a toolbar action drag-and-drop sequence has completed.
  virtual void OnToolbarActionDragDone() {}

  // Called when the toolbar actions bar started to resize (since resizes are
  // often animated, chances are the bar did not finish resizing).
  virtual void OnToolbarActionsBarDidStartResize() {}

  // Called when the delegate of the toolbar actions bar finishes animating.
  virtual void OnToolbarActionsBarAnimationEnded() {}

  virtual ~ToolbarActionsBarObserver() {}
};

#endif  // CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_ACTIONS_BAR_OBSERVER_H_
