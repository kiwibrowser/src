// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_MEDIA_PICKER_DESKTOP_MEDIA_PICKER_BRIDGE_H_
#define CHROME_BROWSER_UI_COCOA_MEDIA_PICKER_DESKTOP_MEDIA_PICKER_BRIDGE_H_

#include "base/macros.h"
#include "chrome/browser/media/webrtc/desktop_media_list.h"
#include "chrome/browser/media/webrtc/desktop_media_list_observer.h"

// Protocol corresponding to |DesktopMediaListObserver|.
@protocol DesktopMediaPickerObserver
- (void)sourceAddedForList:(DesktopMediaList*)list atIndex:(int)index;
- (void)sourceRemovedForList:(DesktopMediaList*)list atIndex:(int)index;
- (void)sourceMovedForList:(DesktopMediaList*)list
                      from:(int)oldIndex
                        to:(int)newIndex;
- (void)sourceNameChangedForList:(DesktopMediaList*)list atIndex:(int)index;
- (void)sourceThumbnailChangedForList:(DesktopMediaList*)list
                              atIndex:(int)index;
@end

// Provides a |DesktopMediaListObserver| implementation that forwards
// notifications to a objective-c object implementing the
// |DesktopMediaPickerObserver| protocol.
class DesktopMediaPickerBridge : public DesktopMediaListObserver {
 public:
  DesktopMediaPickerBridge(id<DesktopMediaPickerObserver> observer);
  ~DesktopMediaPickerBridge() override;

  // DesktopMediaListObserver overrides.
  void OnSourceAdded(DesktopMediaList* list, int index) override;
  void OnSourceRemoved(DesktopMediaList* list, int index) override;
  void OnSourceMoved(DesktopMediaList* list,
                     int old_index,
                     int new_index) override;
  void OnSourceNameChanged(DesktopMediaList* list, int index) override;
  void OnSourceThumbnailChanged(DesktopMediaList* list, int index) override;

 private:
  id<DesktopMediaPickerObserver> observer_;  // weak; owns this

  DISALLOW_COPY_AND_ASSIGN(DesktopMediaPickerBridge);
};

#endif  // CHROME_BROWSER_UI_COCOA_MEDIA_PICKER_DESKTOP_MEDIA_PICKER_BRIDGE_H_
