// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_BUBBLE_BUBBLE_CLOSE_REASON_H_
#define COMPONENTS_BUBBLE_BUBBLE_CLOSE_REASON_H_

// List of reasons why a bubble might close. These correspond to various events
// from the UI. Not all platforms will receive all events.
enum BubbleCloseReason {
  // The bubble WILL be closed regardless of return value for |ShouldClose|.
  // Ex: The bubble's parent window is being destroyed.
  BUBBLE_CLOSE_FORCED,

  // Bubble was closed without any user interaction.
  BUBBLE_CLOSE_FOCUS_LOST,

  // User did not interact with the bubble, but changed tab.
  BUBBLE_CLOSE_TABSWITCHED,

  // User did not interact with the bubble, but detached the tab.
  BUBBLE_CLOSE_TABDETACHED,

  // User dismissed the bubble. (ESC, close, etc.)
  BUBBLE_CLOSE_USER_DISMISSED,

  // There has been a navigation event. (Link, URL typed, refresh, etc.)
  BUBBLE_CLOSE_NAVIGATED,

  // The parent window has entered or exited fullscreen mode. Will also be
  // called for immersive fullscreen.
  BUBBLE_CLOSE_FULLSCREEN_TOGGLED,

  // The user selected an affirmative response in the bubble.
  BUBBLE_CLOSE_ACCEPTED,

  // The user selected a negative response in the bubble.
  BUBBLE_CLOSE_CANCELED,

  // A bubble's owning frame is being destroyed.
  BUBBLE_CLOSE_FRAME_DESTROYED,
};

#endif  // COMPONENTS_BUBBLE_BUBBLE_CLOSE_REASON_H_
