// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_MESSAGE_CENTER_VIEWS_POPUP_ALIGNMENT_DELEGATE_H_
#define UI_MESSAGE_CENTER_VIEWS_POPUP_ALIGNMENT_DELEGATE_H_

#include "ui/message_center/message_center_export.h"
#include "ui/views/widget/widget.h"

namespace gfx {
class Rect;
}

namespace display {
class Display;
}

namespace message_center {

class MessagePopupCollection;

class MESSAGE_CENTER_EXPORT PopupAlignmentDelegate {
 public:
  PopupAlignmentDelegate();

  void set_collection(MessagePopupCollection* collection) {
    collection_ = collection;
  }

  // Returns the x-origin for the given toast bounds in the current work area.
  virtual int GetToastOriginX(const gfx::Rect& toast_bounds) const = 0;

  // Returns the baseline height of the current work area. That is the starting
  // point if there are no other toasts.
  virtual int GetBaseline() const = 0;

  // Returns the rect of the current work area.
  virtual gfx::Rect GetWorkArea() const = 0;

  // Returns true if the toast should be aligned top down.
  virtual bool IsTopDown() const = 0;

  // Returns true if the toasts are positioned at the left side of the desktop
  // so that their reveal animation should happen from left side.
  virtual bool IsFromLeft() const = 0;

  // Called when a new toast appears or toasts are rearranged in the |display|.
  // The subclass may override this method to check the current desktop status
  // so that the toasts are arranged at the correct place.
  virtual void RecomputeAlignment(const display::Display& display) = 0;

  // Sets the parent container for popups. If it does not set a parent a
  // default parent will be used (e.g. the native desktop on Windows).
  virtual void ConfigureWidgetInitParamsForContainer(
      views::Widget* widget,
      views::Widget::InitParams* init_params) = 0;

  // Returns true if the display which notifications show on is the primary
  // display.
  virtual bool IsPrimaryDisplayForNotification() const = 0;

 protected:
  virtual ~PopupAlignmentDelegate();

  void DoUpdateIfPossible();

 private:
  MessagePopupCollection* collection_;
};

}  // namespace message_center

#endif  // UI_MESSAGE_CENTER_VIEWS_POPUP_ALIGNMENT_DELEGATE_H_
