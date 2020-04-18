// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_TRAY_DETAILED_VIEW_DELEGATE_H_
#define ASH_SYSTEM_TRAY_DETAILED_VIEW_DELEGATE_H_

namespace ash {

// A delegate of TrayDetailedView that handles bubble related actions e.g.
// transition to the main view, closing the bubble, etc.
class DetailedViewDelegate {
 public:
  virtual ~DetailedViewDelegate() {}

  // Transition to the main view from the detailed view. |restore_focus| is true
  // if the title row has keyboard focus before transition. If so, the main view
  // should focus on the corresponding element of the detailed view.
  virtual void TransitionToMainView(bool restore_focus) = 0;

  // Close the bubble that contains the detailed view.
  virtual void CloseBubble() = 0;
};

}  // namespace ash

#endif  // ASH_SYSTEM_TRAY_DETAILED_VIEW_DELEGATE_H_
