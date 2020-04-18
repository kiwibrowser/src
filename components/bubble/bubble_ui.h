// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_BUBBLE_BUBBLE_UI_H_
#define COMPONENTS_BUBBLE_BUBBLE_UI_H_

#include "components/bubble/bubble_reference.h"

class BubbleUi {
 public:
  virtual ~BubbleUi() {}

  // Should display the bubble UI. BubbleReference is passed in so that the
  // bubble UI can notify the BubbleManager if it needs to close.
  virtual void Show(BubbleReference bubble_reference) = 0;

  // Should close the bubble UI.
  virtual void Close() = 0;

  // Should update the bubble UI's position.
  // Important to verify that an anchor is still available.
  // ex: fullscreen might not have a location bar in views.
  virtual void UpdateAnchorPosition() = 0;
};

#endif  // COMPONENTS_BUBBLE_BUBBLE_UI_H_
