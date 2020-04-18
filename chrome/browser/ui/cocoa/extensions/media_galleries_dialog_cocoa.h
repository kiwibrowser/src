// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_EXTENSIONS_MEDIA_GALLERIES_DIALOG_COCOA_H_
#define CHROME_BROWSER_UI_COCOA_EXTENSIONS_MEDIA_GALLERIES_DIALOG_COCOA_H_

#import <Cocoa/Cocoa.h>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "chrome/browser/media_galleries/media_galleries_dialog_controller.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_mac.h"
#import "chrome/browser/ui/cocoa/extensions/media_gallery_list_entry_view.h"

@class ConstrainedWindowAlert;
@class MediaGalleriesCocoaController;
@class NSString;

class MediaGalleriesDialogBrowserTest;
class MediaGalleriesDialogTest;

namespace ui {
class MenuModel;
}

// This class displays an alert that can be used to manage lists of media
// galleries.
class MediaGalleriesDialogCocoa : public ConstrainedWindowMacDelegate,
                                  public MediaGalleriesDialog,
                                  public MediaGalleryListEntryController {
 public:
  MediaGalleriesDialogCocoa(
      MediaGalleriesDialogController* controller,
      MediaGalleriesCocoaController* delegate);
  ~MediaGalleriesDialogCocoa() override;

  // Called when the user clicks the accept button.
  void OnAcceptClicked();
  // Called when the user clicks the cancel button.
  void OnCancelClicked();
  // Called when the user clicks the auxiliary button.
  void OnAuxiliaryButtonClicked();

  // MediaGalleriesDialog implementation:
  void UpdateGalleries() override;

  // ConstrainedWindowMacDelegate implementation.
  void OnConstrainedWindowClosed(ConstrainedWindowMac* window) override;

  // MediaGalleryListEntryController implementation.
  void OnCheckboxToggled(MediaGalleryPrefId pref_id, bool checked) override;
  ui::MenuModel* GetContextMenu(MediaGalleryPrefId pref_id) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(MediaGalleriesDialogBrowserTest, Close);
  FRIEND_TEST_ALL_PREFIXES(MediaGalleriesDialogTest, InitializeCheckboxes);
  FRIEND_TEST_ALL_PREFIXES(MediaGalleriesDialogTest, ToggleCheckboxes);
  FRIEND_TEST_ALL_PREFIXES(MediaGalleriesDialogTest, UpdateAdds);
  FRIEND_TEST_ALL_PREFIXES(MediaGalleriesDialogTest, ForgetDeletes);

  // MediaGalleriesDialog implementation:
  void AcceptDialogForTesting() override;

  void InitDialogControls();
  CGFloat CreateCheckboxes(
      CGFloat y_pos,
      const MediaGalleriesDialogController::Entries& entries);
  CGFloat CreateCheckboxSeparator(CGFloat y_pos, NSString* header);

  MediaGalleriesDialogController* controller_;  // weak
  std::unique_ptr<ConstrainedWindowMac> window_;

  // The alert that the dialog is being displayed as.
  base::scoped_nsobject<ConstrainedWindowAlert> alert_;

  // True if the user has pressed accept.
  bool accepted_;

  // Container view for checkboxes.
  base::scoped_nsobject<NSView> checkbox_container_;

  // Container view for the main dialog contents.
  base::scoped_nsobject<NSBox> main_container_;

  // An Objective-C class to route callbacks from Cocoa code.
  base::scoped_nsobject<MediaGalleriesCocoaController> cocoa_controller_;

  DISALLOW_COPY_AND_ASSIGN(MediaGalleriesDialogCocoa);
};

#endif  // CHROME_BROWSER_UI_COCOA_EXTENSIONS_MEDIA_GALLERIES_DIALOG_COCOA_H_
