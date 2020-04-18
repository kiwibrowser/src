// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_MEDIA_PICKER_DESKTOP_MEDIA_PICKER_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_MEDIA_PICKER_DESKTOP_MEDIA_PICKER_CONTROLLER_H_

#import <Cocoa/Cocoa.h>
#import <Quartz/Quartz.h>

#include <memory>

#include "base/callback.h"
#import "base/mac/scoped_nsobject.h"
#include "base/strings/string16.h"
#include "chrome/browser/media/webrtc/desktop_media_list.h"
#include "chrome/browser/media/webrtc/desktop_media_picker.h"
#import "chrome/browser/ui/cocoa/media_picker/desktop_media_picker_bridge.h"

// A controller for the Desktop Media Picker. Presents the user with a list of
// media sources for screen-capturing, and reports the result.
@interface DesktopMediaPickerController
    : NSWindowController<NSWindowDelegate,
                         DesktopMediaPickerObserver,
                         NSTableViewDataSource,
                         NSTableViewDelegate> {
 @private
  // The image browser view or table view to present sources to the user
  // (thumbnails and names).
  base::scoped_nsobject<IKImageBrowserView> screenBrowser_;
  base::scoped_nsobject<IKImageBrowserView> windowBrowser_;
  base::scoped_nsobject<NSTableView> tabBrowser_;

  base::scoped_nsobject<NSScrollView> imageBrowserScroll_;

  base::scoped_nsobject<NSSegmentedControl> sourceTypeControl_;

  // The button used to confirm the selection.
  NSButton* shareButton_;  // weak; owned by contentView

  // The button used to cancel and close the dialog.
  NSButton* cancelButton_;  // weak; owned by contentView

  // The checkbox for audio share.
  base::scoped_nsobject<NSButton> audioShareCheckbox_;

  // Provides source information (including thumbnails) to fill up the array of
  // |screenItems_|, |windowItems_| and |tabItems_|, and to render in
  // |screenBrowser_|, |windowBrowser_| and |tabBrowser_|.
  std::vector<std::unique_ptr<DesktopMediaList>> sourceLists_;

  // To be called with the user selection.
  DesktopMediaPicker::DoneCallback doneCallback_;

  // Arrays of |DesktopMediaPickerItem| used as data for |screenBrowser_|,
  // |windowBrowser_| and |tabBrowser_|.
  base::scoped_nsobject<NSMutableArray> screenItems_;
  base::scoped_nsobject<NSMutableArray> windowItems_;
  base::scoped_nsobject<NSMutableArray> tabItems_;

  // C++ bridge to use as an observer to |screenList_|, |windowList_| and
  // |tabList_|, that forwards obj-c notifications to this object.
  std::unique_ptr<DesktopMediaPickerBridge> bridge_;

  // Used to create |DesktopMediaPickerItem|s with unique IDs.
  int lastImageUID_;
}

// Designated initializer.
// To show the dialog, use |NSWindowController|'s |showWindow:|.
// |callback| will be called to report the user's selection.
// |appName| will be used to format the dialog's title and the label, where it
// appears as the initiator of the request.
// |targetName| will be used to format the dialog's label and appear as the
// consumer of the requested stream.
- (id)initWithSourceLists:
          (std::vector<std::unique_ptr<DesktopMediaList>>)sourceLists
                   parent:(NSWindow*)parent
                 callback:(const DesktopMediaPicker::DoneCallback&)callback
                  appName:(const base::string16&)appName
               targetName:(const base::string16&)targetName
             requestAudio:(bool)requestAudio;

@end

#endif  // CHROME_BROWSER_UI_COCOA_MEDIA_PICKER_DESKTOP_MEDIA_PICKER_CONTROLLER_H_
