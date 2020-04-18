// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_WEB_MODAL_MODAL_DIALOG_HOST_H_
#define COMPONENTS_WEB_MODAL_MODAL_DIALOG_HOST_H_

#include "ui/gfx/native_widget_types.h"

namespace gfx {
class Point;
class Size;
}  // namespace gfx

namespace web_modal {

// Observer to be implemented to update modal dialogs when the host indicates
// their position needs to be changed.
class ModalDialogHostObserver {
 public:
  virtual ~ModalDialogHostObserver();

  virtual void OnPositionRequiresUpdate() = 0;
  virtual void OnHostDestroying() = 0;
};

// Interface for supporting positioning of modal dialogs over a window/widget.
class ModalDialogHost {
 public:
  virtual ~ModalDialogHost();

  // Returns the view against which the dialog is positioned and parented.
  virtual gfx::NativeView GetHostView() const = 0;
  // Gets the position for the dialog in coordinates relative to the host view.
  virtual gfx::Point GetDialogPosition(const gfx::Size& size) = 0;
  // Returns whether a dialog currently about to be shown should be activated.
  virtual bool ShouldActivateDialog() const;

  // Add/remove observer.
  virtual void AddObserver(ModalDialogHostObserver* observer) = 0;
  virtual void RemoveObserver(ModalDialogHostObserver* observer) = 0;
};

}  // namespace web_modal

#endif  // COMPONENTS_WEB_MODAL_MODAL_DIALOG_HOST_H_
