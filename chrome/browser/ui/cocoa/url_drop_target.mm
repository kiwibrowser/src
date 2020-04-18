// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/url_drop_target.h"

#include "chrome/browser/ui/cocoa/drag_util.h"
#import "third_party/mozilla/NSPasteboard+Utils.h"
#include "ui/base/clipboard/clipboard_util_mac.h"
#include "url/gurl.h"

@interface URLDropTargetHandler(Private)

// Gets the appropriate drag operation given the |NSDraggingInfo|.
- (NSDragOperation)getDragOperation:(id<NSDraggingInfo>)sender;

// Tell the window controller to hide the drop indicator.
- (void)hideIndicator;

@end  // @interface URLDropTargetHandler(Private)

@implementation URLDropTargetHandler

+ (NSArray*)handledDragTypes {
  return @[
    ui::ClipboardUtil::UTIForWebURLsAndTitles(), NSURLPboardType,
    NSStringPboardType, NSFilenamesPboardType
  ];
}

- (id)initWithView:(NSView<URLDropTarget>*)view {
  if ((self = [super init])) {
    view_ = view;
    [view_ registerForDraggedTypes:[URLDropTargetHandler handledDragTypes]];
  }
  return self;
}

// The following four methods implement parts of the |NSDraggingDestination|
// protocol, which the owner should "forward" to its |URLDropTargetHandler|
// (us).

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender {
  if ([[view_ urlDropController] isUnsupportedDropData:sender])
    return NSDragOperationNone;

  return [self getDragOperation:sender];
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender {
  NSDragOperation dragOp = NSDragOperationNone;
  BOOL showIndicator = NO;
  // Show indicator for drag data supported for tab contents as well as for
  // local file drags that may not be viewable in tab contents, but should
  // still trigger hover tab selection.
  if (![[view_ urlDropController] isUnsupportedDropData:sender]) {
    dragOp = [self getDragOperation:sender];
    if (dragOp == NSDragOperationCopy)
      showIndicator = YES;
  } else if (!drag_util::GetFileURLFromDropData(sender).is_empty()) {
    showIndicator = YES;
  }

  if (showIndicator) {
    // Just tell the window controller to update the indicator.
    NSPoint hoverPoint = [view_ convertPoint:[sender draggingLocation]
                                    fromView:nil];
    [[view_ urlDropController] indicateDropURLsInView:view_ at:hoverPoint];
  }
  return dragOp;
}

- (void)draggingExited:(id<NSDraggingInfo>)sender {
  [self hideIndicator];
}

- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender {
  [self hideIndicator];

  NSPasteboard* pboard = [sender draggingPasteboard];
  NSArray* supportedTypes = [NSArray arrayWithObjects:NSStringPboardType, nil];
  NSString* bestType = [pboard availableTypeFromArray:supportedTypes];

  NSPoint dropPoint =
      [view_ convertPoint:[sender draggingLocation] fromView:nil];
  // Tell the window controller about the dropped URL(s).
  if ([pboard containsURLDataConvertingTextToURL:NO]) {
    NSArray* urls = nil;
    NSArray* titles;  // discarded
    [pboard getURLs:&urls
                  andTitles:&titles
        convertingFilenames:YES
        convertingTextToURL:NO];

    if ([urls count]) {
      [[view_ urlDropController] dropURLs:urls inView:view_ at:dropPoint];
      return YES;
    }
  } else if (NSString* text = [pboard stringForType:bestType]) {
    // This does not include any URLs, so treat it as plain text if we can
    // get NSString.
    [[view_ urlDropController] dropText:text inView:view_ at:dropPoint];
    return YES;
  }

  return NO;
}

@end  // @implementation URLDropTargetHandler

@implementation URLDropTargetHandler(Private)

- (NSDragOperation)getDragOperation:(id<NSDraggingInfo>)sender {
  NSPasteboard* pboard = [sender draggingPasteboard];
  NSArray *supportedTypes = [NSArray arrayWithObjects:NSStringPboardType, nil];
  NSString *bestType = [pboard availableTypeFromArray:supportedTypes];
  if (![pboard containsURLDataConvertingTextToURL:YES] &&
      ![pboard stringForType:bestType])
    return NSDragOperationNone;

  // Only allow the copy operation.
  return [sender draggingSourceOperationMask] & NSDragOperationCopy;
}

- (void)hideIndicator {
  [[view_ urlDropController] hideDropURLsIndicatorInView:view_];
}

@end  // @implementation URLDropTargetHandler(Private)
