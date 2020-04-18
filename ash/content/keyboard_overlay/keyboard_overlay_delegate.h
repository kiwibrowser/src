// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_KEYBOARD_OVERLAY_KEYBOARD_OVERLAY_DELEGATE_H_
#define ASH_KEYBOARD_OVERLAY_KEYBOARD_OVERLAY_DELEGATE_H_

#include "ash/content/ash_with_content_export.h"
#include "base/compiler_specific.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "ui/web_dialogs/web_dialog_delegate.h"
#include "url/gurl.h"

namespace views {
class WebDialogView;
class Widget;
}

namespace ash {

// Delegate to handle showing the keyboard overlay drawing. Exported for test.
class ASH_WITH_CONTENT_EXPORT KeyboardOverlayDelegate
    : public ui::WebDialogDelegate {
 public:
  KeyboardOverlayDelegate(const base::string16& title, const GURL& url);

  // Shows the keyboard overlay widget. Returns the widget for testing.
  views::Widget* Show(views::WebDialogView* view);

  // Overridden from ui::WebDialogDelegate:
  void GetDialogSize(gfx::Size* size) const override;

 private:
  FRIEND_TEST_ALL_PREFIXES(KeyboardOverlayDelegateTest, ShowAndClose);

  ~KeyboardOverlayDelegate() override;

  // Overridden from ui::WebDialogDelegate:
  ui::ModalType GetDialogModalType() const override;
  base::string16 GetDialogTitle() const override;
  GURL GetDialogContentURL() const override;
  void GetWebUIMessageHandlers(
      std::vector<content::WebUIMessageHandler*>* handlers) const override;
  std::string GetDialogArgs() const override;
  void OnDialogClosed(const std::string& json_retval) override;
  void OnCloseContents(content::WebContents* source,
                       bool* out_close_dialog) override;
  bool ShouldShowDialogTitle() const override;
  bool HandleContextMenu(const content::ContextMenuParams& params) override;

  // The dialog title.
  base::string16 title_;

  // The URL of the keyboard overlay.
  GURL url_;

  // The widget associated with this delegate. Not owned.
  views::Widget* widget_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardOverlayDelegate);
};

}  // namespace ash

#endif  // ASH_KEYBOARD_OVERLAY_KEYBOARD_OVERLAY_DELEGATE_H_
