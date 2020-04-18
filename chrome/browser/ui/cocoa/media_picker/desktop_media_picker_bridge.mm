// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/media_picker/desktop_media_picker_bridge.h"

DesktopMediaPickerBridge::DesktopMediaPickerBridge(
    id<DesktopMediaPickerObserver> observer)
    : observer_(observer) {
}

DesktopMediaPickerBridge::~DesktopMediaPickerBridge() {
}

void DesktopMediaPickerBridge::OnSourceAdded(DesktopMediaList* list,
                                             int index) {
  [observer_ sourceAddedForList:list atIndex:index];
}

void DesktopMediaPickerBridge::OnSourceRemoved(DesktopMediaList* list,
                                               int index) {
  [observer_ sourceRemovedForList:list atIndex:index];
}

void DesktopMediaPickerBridge::OnSourceMoved(DesktopMediaList* list,
                                             int old_index,
                                             int new_index) {
  [observer_ sourceMovedForList:list from:old_index to:new_index];
}

void DesktopMediaPickerBridge::OnSourceNameChanged(DesktopMediaList* list,
                                                   int index) {
  [observer_ sourceNameChangedForList:list atIndex:index];
}

void DesktopMediaPickerBridge::OnSourceThumbnailChanged(DesktopMediaList* list,
                                                        int index) {
  [observer_ sourceThumbnailChangedForList:list atIndex:index];
}
