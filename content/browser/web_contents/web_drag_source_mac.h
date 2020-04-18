// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEB_CONTENTS_WEB_DRAG_SOURCE_MAC_H_
#define CONTENT_BROWSER_WEB_CONTENTS_WEB_DRAG_SOURCE_MAC_H_

#import <Cocoa/Cocoa.h>

#include <memory>

#include "base/files/file_path.h"
#include "base/mac/scoped_cftyperef.h"
#include "base/mac/scoped_nsobject.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"
#include "url/gurl.h"

namespace content {
class RenderWidgetHostImpl;
class WebContentsImpl;
struct DropData;
}

// A class that handles tracking and event processing for a drag and drop
// originating from the content area.
CONTENT_EXPORT
@interface WebDragSource : NSObject {
 @private
  // Our contents. Weak reference (owns or co-owns us).
  // An instance of this class may outlive |contents_|. The destructor of
  // |contents_| must set this ivar to |nullptr|.
  content::WebContentsImpl* contents_;

  // The view from which the drag was initiated. Weak reference.
  // An instance of this class may outlive |contentsView_|. The destructor of
  // |contentsView_| must set this ivar to |nullptr|.
  NSView* contentsView_;

  // Our drop data. Should only be initialized once.
  std::unique_ptr<content::DropData> dropData_;

  // The image to show as drag image. Can be nil.
  base::scoped_nsobject<NSImage> dragImage_;

  // The offset to draw |dragImage_| at.
  NSPoint imageOffset_;

  // Our pasteboard.
  base::scoped_nsobject<NSPasteboard> pasteboard_;

  // A mask of the allowed drag operations.
  NSDragOperation dragOperationMask_;

  // The file name to be saved to for a drag-out download.
  base::FilePath downloadFileName_;

  // The URL to download from for a drag-out download.
  GURL downloadURL_;

  // The file UTI associated with the file drag, if any.
  base::ScopedCFTypeRef<CFStringRef> fileUTI_;

  // Tracks the RenderWidgetHost where the current drag started.
  base::WeakPtr<content::RenderWidgetHostImpl> dragStartRWH_;
}

// Initialize a WebDragSource object for a drag (originating on the given
// contentsView and with the given dropData and pboard). Fill the pasteboard
// with data types appropriate for dropData.
- (id)initWithContents:(content::WebContentsImpl*)contents
                  view:(NSView*)contentsView
              dropData:(const content::DropData*)dropData
             sourceRWH:(content::RenderWidgetHostImpl*)sourceRWH
                 image:(NSImage*)image
                offset:(NSPoint)offset
            pasteboard:(NSPasteboard*)pboard
     dragOperationMask:(NSDragOperation)dragOperationMask;

// Call when the web contents is gone.
- (void)clearWebContentsView;

// Returns a mask of the allowed drag operations.
- (NSDragOperation)draggingSourceOperationMaskForLocal:(BOOL)isLocal;

// Call when asked to do a lazy write to the pasteboard; hook up to
// -pasteboard:provideDataForType: (on the contentsView).
- (void)lazyWriteToPasteboard:(NSPasteboard*)pboard
                      forType:(NSString*)type;

// Start the drag (on the originally provided contentsView); can do this right
// after -initWithContentsView:....
- (void)startDrag;

// End the drag and clear the pasteboard; hook up to
// -draggedImage:endedAt:operation:.
- (void)endDragAt:(NSPoint)screenPoint
        operation:(NSDragOperation)operation;

// Call to drag a promised file to the given path (should be called before
// -endDragAt:...); hook up to -namesOfPromisedFilesDroppedAtDestination:.
// Returns the file name (not including path) of the file deposited (or which
// will be deposited).
- (NSString*)dragPromisedFileTo:(NSString*)path;

@end

#endif  // CONTENT_BROWSER_WEB_CONTENTS_WEB_DRAG_SOURCE_MAC_H_
