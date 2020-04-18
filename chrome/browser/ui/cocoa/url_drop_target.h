// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_URL_DROP_TARGET_H_
#define CHROME_BROWSER_UI_COCOA_URL_DROP_TARGET_H_

#import <Cocoa/Cocoa.h>

@protocol URLDropTarget;
@protocol URLDropTargetController;

// Object which coordinates the dropping of URLs on a given view, sending data
// and updates to a controller.
@interface URLDropTargetHandler : NSObject {
 @private
  NSView<URLDropTarget>* view_;  // weak
}

// Returns an array of drag types that can be handled.
+ (NSArray*)handledDragTypes;

// Initialize the given view, which must implement the |URLDropTarget| (below),
// to accept drops of URLs.
- (id)initWithView:(NSView<URLDropTarget>*)view;

// The owner view should implement the following methods by calling the
// |URLDropTargetHandler|'s version, and leave the others to the default
// implementation provided by |NSView|/|NSWindow|.
- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender;
- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender;
- (void)draggingExited:(id<NSDraggingInfo>)sender;
- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender;

@end  // @interface URLDropTargetHandler

// Protocol which views that are URL drop targets and use |URLDropTargetHandler|
// must implement.
@protocol URLDropTarget

// Returns the controller which handles the drop.
- (id<URLDropTargetController>)urlDropController;

// The following, which come from |NSDraggingDestination|, must be implemented
// by calling the |URLDropTargetHandler|'s implementations.
- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender;
- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender;
- (void)draggingExited:(id<NSDraggingInfo>)sender;
- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender;

@end  // @protocol URLDropTarget

// Protocol for the controller which handles the actual drop data/drop updates.
@protocol URLDropTargetController

// Determines whether the given drag and drop operation contains content that
// is supported by the web view. In particular, if the content is a local file
// URL, this checks if it is of a type that can be shown in the tab contents.
- (BOOL)isUnsupportedDropData:(id<NSDraggingInfo>)info;

// The given URLs (an |NSArray| of |NSString|s) were dropped in the given view
// at the given point (in that view's coordinates).
- (void)dropURLs:(NSArray*)urls inView:(NSView*)view at:(NSPoint)point;

// The given text was dropped in the given view at the given point (in that
// view's coordinates).
- (void)dropText:(NSString*)text inView:(NSView*)view at:(NSPoint)point;

// Dragging is in progress over the owner view (at the given point, in view
// coordinates) and any indicator of location -- e.g., an arrow -- should be
// updated/shown.
- (void)indicateDropURLsInView:(NSView*)view at:(NSPoint)point;

// Dragging is over, and any indicator should be hidden.
- (void)hideDropURLsIndicatorInView:(NSView*)view;

@end  // @protocol URLDropTargetController

#endif  // CHROME_BROWSER_UI_COCOA_URL_DROP_TARGET_H_
