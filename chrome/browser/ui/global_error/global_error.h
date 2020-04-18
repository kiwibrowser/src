// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_GLOBAL_ERROR_GLOBAL_ERROR_H_
#define CHROME_BROWSER_UI_GLOBAL_ERROR_GLOBAL_ERROR_H_

#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"

class Browser;
class GlobalErrorBubbleViewBase;

namespace gfx {
class Image;
}

// This object describes a single global error.
class GlobalError {
 public:
  enum Severity {
    SEVERITY_LOW,
    SEVERITY_MEDIUM,
    SEVERITY_HIGH,
  };

  GlobalError();
  virtual ~GlobalError();

  // Returns the error's severity level. If there are multiple errors,
  // the error with the highest severity will display in the menu. If not
  // overridden, this is based on the badge resource ID.
  virtual Severity GetSeverity();

  // Returns true if a menu item should be added to the app menu.
  virtual bool HasMenuItem() = 0;
  // Returns the command ID for the menu item.
  virtual int MenuItemCommandID() = 0;
  // Returns the label for the menu item.
  virtual base::string16 MenuItemLabel() = 0;
  // Returns the menu item icon.
  virtual gfx::Image MenuItemIcon();
  // Called when the user clicks on the menu item.
  virtual void ExecuteMenuItem(Browser* browser) = 0;

  // Returns true if a bubble view should be shown.
  virtual bool HasBubbleView() = 0;
  // Returns true if the bubble view has been shown.
  virtual bool HasShownBubbleView() = 0;
  // Called to show the bubble view.
  virtual void ShowBubbleView(Browser* browser) = 0;
  // Returns the bubble view.
  virtual GlobalErrorBubbleViewBase* GetBubbleView() = 0;
};

// This object describes a single global error that already comes with support
// for showing a standard Bubble UI. Derived classes just need to supply the
// content to be displayed in the bubble.
class GlobalErrorWithStandardBubble
    : public GlobalError,
      public base::SupportsWeakPtr<GlobalErrorWithStandardBubble> {
 public:
  GlobalErrorWithStandardBubble();
  ~GlobalErrorWithStandardBubble() override;

  // Returns an icon to use for the bubble view.
  virtual gfx::Image GetBubbleViewIcon();

  // Returns the title for the bubble view.
  virtual base::string16 GetBubbleViewTitle() = 0;
  // Returns the messages for the bubble view, one per line. Multiple messages
  // are only supported on Views.
  // TODO(yoz): Add multi-line support for GTK and Cocoa.
  virtual std::vector<base::string16> GetBubbleViewMessages() = 0;
  // Returns the accept button label for the bubble view.
  virtual base::string16 GetBubbleViewAcceptButtonLabel() = 0;
  // Returns true if the bubble needs a close(x) button.
  virtual bool ShouldShowCloseButton() const;
  // Returns true if the accept button needs elevation icon (only effective
  // on Windows platform).
  virtual bool ShouldAddElevationIconToAcceptButton();
  // Returns the cancel button label for the bubble view. To hide the cancel
  // button return an empty string.
  virtual base::string16 GetBubbleViewCancelButtonLabel() = 0;
  // Returns the default dialog button. Default behavior is to return
  // ui::DIALOG_BUTTON_OK. Do not return ui::DIALOG_BUTTON_CANCEL if hiding the
  // cancel button.
  virtual int GetDefaultDialogButton() const;
  // Called when the bubble view is closed. |browser| is the Browser that the
  // bubble view was shown on.
  virtual void BubbleViewDidClose(Browser* browser);
  // Notifies subclasses that the bubble view is closed. |browser| is the
  // Browser that the bubble view was shown on.
  virtual void OnBubbleViewDidClose(Browser* browser) = 0;
  // Called when the user clicks on the accept button. |browser| is the
  // Browser that the bubble view was shown on.
  virtual void BubbleViewAcceptButtonPressed(Browser* browser) = 0;
  // Called when the user clicks the cancel button. |browser| is the
  // Browser that the bubble view was shown on.
  virtual void BubbleViewCancelButtonPressed(Browser* browser) = 0;
  // Returns true if the bubble should close when focus is lost. If false, the
  // bubble will stick around until the user explicitly acknowledges it.
  // Defaults to true.
  virtual bool ShouldCloseOnDeactivate() const;
  // Return true if 'cancel' button should be created as an extra view placed on
  // the left edge across from the 'ok' button.
  virtual bool ShouldUseExtraView() const;

  // GlobalError overrides:
  bool HasBubbleView() override;
  bool HasShownBubbleView() override;
  void ShowBubbleView(Browser* browser) override;
  GlobalErrorBubbleViewBase* GetBubbleView() override;

 private:
  bool has_shown_bubble_view_;
  GlobalErrorBubbleViewBase* bubble_view_;

  DISALLOW_COPY_AND_ASSIGN(GlobalErrorWithStandardBubble);
};

#endif  // CHROME_BROWSER_UI_GLOBAL_ERROR_GLOBAL_ERROR_H_
