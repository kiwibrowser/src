// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_DOWNLOAD_DOWNLOAD_ITEM_VIEW_PROTOCOL_H_
#define CHROME_BROWSER_UI_COCOA_DOWNLOAD_DOWNLOAD_ITEM_VIEW_PROTOCOL_H_

#import <AppKit/AppKit.h>

#include "base/files/file_path.h"

// TODO(sdy): This shouldn't know about DownloadItemController.
@class DownloadItemController;
class DownloadItemModel;

// NSView<DownloadItemView> displays a an individual download in the shelf.
// It's a common interface for the pre-Material Design view and the MD view,
// and it can probably be deleted when the pre-MD code is deleted.
@protocol DownloadItemView
@property(assign, nonatomic) DownloadItemController* controller;

// TODO(sdy): A delegate would be better once the pre-MD path is gone.
@property(assign) id target;
@property SEL action;

// Update the view based on the passed download item (including danger,
// progress, filename and status labels, clickability, and draggability).
- (void)setStateFromDownload:(DownloadItemModel*)downloadModel;

- (void)setImage:(NSImage*)image;

@optional
@property(readonly) CGFloat preferredWidth;
@end

#endif  // CHROME_BROWSER_UI_COCOA_DOWNLOAD_DOWNLOAD_ITEM_VIEW_PROTOCOL_H_
