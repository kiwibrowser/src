// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/media_picker/desktop_media_picker_item.h"

#import <Quartz/Quartz.h>

#include "chrome/browser/media/webrtc/desktop_media_list.h"

@implementation DesktopMediaPickerItem

@synthesize titleHidden = titleHidden_;

- (id)initWithSourceId:(content::DesktopMediaID)sourceID
              imageUID:(int)imageUID
            imageTitle:(NSString*)imageTitle {
  if ((self = [super init])) {
    sourceID_ = sourceID;
    imageUID_.reset([[NSString stringWithFormat:@"%d", imageUID] retain]);
    imageTitle_.reset([imageTitle retain]);
  }
  return self;
}

- (content::DesktopMediaID)sourceID {
  return sourceID_;
}

- (void)setImageRepresentation:(NSImage*)image {
  image_.reset([image retain]);
  ++imageVersion_;
}

- (void)setImageTitle:(NSString*)imageTitle {
  imageTitle_.reset([imageTitle copy]);
}

#pragma mark IKImageBrowserItem

- (NSString*)imageUID {
  return imageUID_;
}

- (NSString*)imageRepresentationType {
  return IKImageBrowserNSImageRepresentationType;
}

- (NSString*)imageTitle {
  return titleHidden_ ? nil : imageTitle_;
}

- (NSUInteger)imageVersion {
  return imageVersion_;
}

- (id)imageRepresentation {
  return image_.get();
}

@end  // @interface DesktopMediaPickerItem
