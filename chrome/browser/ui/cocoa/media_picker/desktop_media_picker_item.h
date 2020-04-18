// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_MEDIA_PICKER_DESKTOP_MEDIA_PICKER_ITEM_H_
#define CHROME_BROWSER_UI_COCOA_MEDIA_PICKER_DESKTOP_MEDIA_PICKER_ITEM_H_

#import <AppKit/AppKit.h>

#import "base/mac/scoped_nsobject.h"
#include "chrome/browser/media/webrtc/desktop_media_list.h"

// Stores the data representing a |DesktopMediaPicker| source for displaying in
// a |IKImageBrowserView|. Implements the |IKImageBrowserItem| informal
// protocol.
@interface DesktopMediaPickerItem : NSObject {
 @private
  content::DesktopMediaID sourceID_;
  base::scoped_nsobject<NSString> imageUID_;
  base::scoped_nsobject<NSString> imageTitle_;
  base::scoped_nsobject<NSImage> image_;
  NSUInteger imageVersion_;
  BOOL titleHidden_;
}

@property(assign, nonatomic) BOOL titleHidden;

// Designated initializer.
// |sourceID| is the corresponding source's ID as provided by the media list.
// |imageUID| is a unique number in the context of the |IKImageBrowserView|
// instance.
// |imageTitle| is the source's name to be used as the label in
// |IKImageBrowserView|.
- (id)initWithSourceId:(content::DesktopMediaID)sourceID
              imageUID:(int)imageUID
            imageTitle:(NSString*)imageTitle;

// Returns the source's ID.
- (content::DesktopMediaID)sourceID;

// Sets the image of the item to be displayed in |IKImageBrowserView|.
- (void)setImageRepresentation:(NSImage*)image;

// Sets the label of the item to be displayed in |IKImageBrowserView|.
- (void)setImageTitle:(NSString*)imageTitle;

@end

#endif  // CHROME_BROWSER_UI_COCOA_MEDIA_PICKER_DESKTOP_MEDIA_PICKER_ITEM_H_
