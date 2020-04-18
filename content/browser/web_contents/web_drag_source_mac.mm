// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "content/browser/web_contents/web_drag_source_mac.h"

#include <sys/param.h>

#include <utility>

#include "base/bind.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/mac/foundation_util.h"
#include "base/pickle.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread.h"
#include "base/threading/thread_restrictions.h"
#include "content/browser/download/drag_download_file.h"
#include "content/browser/download/drag_download_util.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_view_base.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/content_client.h"
#include "content/public/common/drop_data.h"
#include "net/base/escape.h"
#include "net/base/filename_util.h"
#include "net/base/mime_util.h"
#include "ui/base/clipboard/custom_data_helper.h"
#include "ui/base/cocoa/cocoa_base_utils.h"
#include "ui/base/dragdrop/cocoa_dnd_util.h"
#include "ui/gfx/image/image.h"
#include "ui/resources/grit/ui_resources.h"
#include "url/url_constants.h"

using base::SysNSStringToUTF8;
using base::SysUTF8ToNSString;
using base::SysUTF16ToNSString;
using content::BrowserThread;
using content::DragDownloadFile;
using content::DropData;
using content::PromiseFileFinalizer;
using content::RenderViewHostImpl;

namespace {

// An unofficial standard pasteboard title type to be provided alongside the
// |NSURLPboardType|.
NSString* const kNSURLTitlePboardType = @"public.url-name";

// This helper's sole task is to write out data for a promised file; the caller
// is responsible for opening the file. It takes the drop data and an open file
// stream.
void PromiseWriterHelper(const DropData& drop_data,
                         base::File file) {
  DCHECK(file.IsValid());
  file.WriteAtCurrentPos(drop_data.file_contents.data(),
                         drop_data.file_contents.length());
}

}  // namespace


@interface WebDragSource(Private)

- (void)fillPasteboard;
- (NSImage*)dragImage;

@end  // @interface WebDragSource(Private)


@implementation WebDragSource

- (id)initWithContents:(content::WebContentsImpl*)contents
                  view:(NSView*)contentsView
              dropData:(const DropData*)dropData
             sourceRWH:(content::RenderWidgetHostImpl*)sourceRWH
                 image:(NSImage*)image
                offset:(NSPoint)offset
            pasteboard:(NSPasteboard*)pboard
     dragOperationMask:(NSDragOperation)dragOperationMask {
  if ((self = [super init])) {
    contents_ = contents;
    DCHECK(contents_);

    contentsView_ = contentsView;
    DCHECK(contentsView_);

    dropData_.reset(new DropData(*dropData));
    DCHECK(dropData_.get());

    dragStartRWH_ = sourceRWH->GetWeakPtr();
    dragImage_.reset([image retain]);
    imageOffset_ = offset;

    pasteboard_.reset([pboard retain]);
    DCHECK(pasteboard_.get());

    dragOperationMask_ = dragOperationMask;

    [self fillPasteboard];
  }

  return self;
}

- (void)clearWebContentsView {
  contents_ = nil;
  contentsView_ = nil;
}

- (NSDragOperation)draggingSourceOperationMaskForLocal:(BOOL)isLocal {
  return dragOperationMask_;
}

- (void)lazyWriteToPasteboard:(NSPasteboard*)pboard forType:(NSString*)type {
  // NSHTMLPboardType requires the character set to be declared. Otherwise, it
  // assumes US-ASCII. Awesome.
  const base::string16 kHtmlHeader = base::ASCIIToUTF16(
      "<meta http-equiv=\"Content-Type\" content=\"text/html;charset=UTF-8\">");

  // Be extra paranoid; avoid crashing.
  if (!dropData_) {
    NOTREACHED();
    return;
  }

  // HTML.
  if ([type isEqualToString:NSHTMLPboardType] ||
      [type isEqualToString:ui::kChromeDragImageHTMLPboardType]) {
    DCHECK(!dropData_->html.string().empty());
    // See comment on |kHtmlHeader| above.
    [pboard setString:SysUTF16ToNSString(kHtmlHeader + dropData_->html.string())
              forType:type];

  // URL.
  } else if ([type isEqualToString:NSURLPboardType]) {
    DCHECK(dropData_->url.is_valid());
    NSURL* url = [NSURL URLWithString:SysUTF8ToNSString(dropData_->url.spec())];
    // If NSURL creation failed, check for a badly-escaped JavaScript URL.
    // Strip out any existing escapes and then re-escape uniformly.
    if (!url && dropData_->url.SchemeIs(url::kJavaScriptScheme)) {
      std::string unescapedUrlString =
          net::UnescapeBinaryURLComponent(dropData_->url.spec());
      std::string escapedUrlString =
          net::EscapeUrlEncodedData(unescapedUrlString, false);
      url = [NSURL URLWithString:SysUTF8ToNSString(escapedUrlString)];
    }
    [url writeToPasteboard:pboard];
  // URL title.
  } else if ([type isEqualToString:kNSURLTitlePboardType]) {
    [pboard setString:SysUTF16ToNSString(dropData_->url_title)
              forType:kNSURLTitlePboardType];

  // File contents.
  } else if ([type isEqualToString:base::mac::CFToNSCast(fileUTI_)]) {
    [pboard setData:[NSData dataWithBytes:dropData_->file_contents.data()
                                   length:dropData_->file_contents.length()]
            forType:base::mac::CFToNSCast(fileUTI_.get())];

  // Plain text.
  } else if ([type isEqualToString:NSStringPboardType]) {
    DCHECK(!dropData_->text.string().empty());
    [pboard setString:SysUTF16ToNSString(dropData_->text.string())
              forType:NSStringPboardType];

  // Custom MIME data.
  } else if ([type isEqualToString:ui::kWebCustomDataPboardType]) {
    base::Pickle pickle;
    ui::WriteCustomDataToPickle(dropData_->custom_data, &pickle);
    [pboard setData:[NSData dataWithBytes:pickle.data() length:pickle.size()]
            forType:ui::kWebCustomDataPboardType];

  // Dummy type.
  } else if ([type isEqualToString:ui::kChromeDragDummyPboardType]) {
    // The dummy type _was_ promised and someone decided to call the bluff.
    [pboard setData:[NSData data]
            forType:ui::kChromeDragDummyPboardType];

  // Oops!
  } else {
    // Unknown drag pasteboard type.
    NOTREACHED();
  }
}

- (NSPoint)convertScreenPoint:(NSPoint)screenPoint {
  DCHECK([contentsView_ window]);
  NSPoint basePoint =
      ui::ConvertPointFromScreenToWindow([contentsView_ window], screenPoint);
  return [contentsView_ convertPoint:basePoint fromView:nil];
}

- (void)startDrag {
  if (!contentsView_)
    return;

  NSEvent* currentEvent = [NSApp currentEvent];

  // Synthesize an event for dragging, since we can't be sure that
  // [NSApp currentEvent] will return a valid dragging event.
  NSWindow* window = [contentsView_ window];
  NSPoint position = [window mouseLocationOutsideOfEventStream];
  NSTimeInterval eventTime = [currentEvent timestamp];
  NSEvent* dragEvent = [NSEvent mouseEventWithType:NSLeftMouseDragged
                                          location:position
                                     modifierFlags:NSLeftMouseDraggedMask
                                         timestamp:eventTime
                                      windowNumber:[window windowNumber]
                                           context:nil
                                       eventNumber:0
                                        clickCount:1
                                          pressure:1.0];

  if (dragImage_) {
    position.x -= imageOffset_.x;
    // Deal with Cocoa's flipped coordinate system.
    position.y -= [dragImage_.get() size].height - imageOffset_.y;
  }
  // Per kwebster, offset arg is ignored, see -_web_DragImageForElement: in
  // third_party/WebKit/Source/WebKit/mac/Misc/WebNSViewExtras.m.
  [window dragImage:[self dragImage]
                 at:position
             offset:NSZeroSize
              event:dragEvent
         pasteboard:pasteboard_
             source:contentsView_
          slideBack:YES];
}

- (void)endDragAt:(NSPoint)screenPoint
        operation:(NSDragOperation)operation {
  if (!contents_ || !contentsView_)
    return;

  contents_->SystemDragEnded(dragStartRWH_.get());

  if (dragImage_) {
    screenPoint.x += imageOffset_.x;
    // Deal with Cocoa's flipped coordinate system.
    screenPoint.y += [dragImage_.get() size].height - imageOffset_.y;
  }

  // Convert |screenPoint| to view coordinates and flip it.
  NSPoint localPoint = NSZeroPoint;
  if ([contentsView_ window])
    localPoint = [self convertScreenPoint:screenPoint];
  NSRect viewFrame = [contentsView_ frame];
  localPoint.y = viewFrame.size.height - localPoint.y;
  // Flip |screenPoint|.
  NSRect screenFrame = [[[contentsView_ window] screen] frame];
  screenPoint.y = screenFrame.size.height - screenPoint.y;

  // If AppKit returns a copy and move operation, mask off the move bit
  // because WebCore does not understand what it means to do both, which
  // results in an assertion failure/renderer crash.
  if (operation == (NSDragOperationMove | NSDragOperationCopy))
    operation &= ~NSDragOperationMove;

  // |localPoint| and |screenPoint| are in the root coordinate space, for
  // non-root RenderWidgetHosts they need to be transformed.
  gfx::PointF transformedPoint = gfx::PointF(localPoint.x, localPoint.y);
  gfx::PointF transformedScreenPoint =
      gfx::PointF(screenPoint.x, screenPoint.y);
  if (dragStartRWH_ && contents_->GetRenderWidgetHostView()) {
    content::RenderWidgetHostViewBase* contentsViewBase =
        static_cast<content::RenderWidgetHostViewBase*>(
            contents_->GetRenderWidgetHostView());
    content::RenderWidgetHostViewBase* dragStartViewBase =
        static_cast<content::RenderWidgetHostViewBase*>(
            dragStartRWH_->GetView());
    contentsViewBase->TransformPointToCoordSpaceForView(
        gfx::PointF(localPoint.x, localPoint.y), dragStartViewBase,
        &transformedPoint);
    contentsViewBase->TransformPointToCoordSpaceForView(
        gfx::PointF(screenPoint.x, screenPoint.y), dragStartViewBase,
        &transformedScreenPoint);
  }

  contents_->DragSourceEndedAt(
      transformedPoint.x(), transformedPoint.y(), transformedScreenPoint.x(),
      transformedScreenPoint.y(),
      static_cast<blink::WebDragOperation>(operation), dragStartRWH_.get());

  // Make sure the pasteboard owner isn't us.
  [pasteboard_ declareTypes:[NSArray array] owner:nil];
}

- (NSString*)dragPromisedFileTo:(NSString*)path {
  // Be extra paranoid; avoid crashing.
  if (!dropData_) {
    NOTREACHED() << "No drag-and-drop data available for promised file.";
    return nil;
  }

  base::FilePath filePath(SysNSStringToUTF8(path));
  filePath = filePath.Append(downloadFileName_);

  // CreateFileForDrop() will call base::PathExists(),
  // which is blocking.  Since this operation is already blocking the
  // UI thread on OSX, it should be reasonable to let it happen.
  base::ThreadRestrictions::ScopedAllowIO allowIO;
  base::File file(content::CreateFileForDrop(&filePath));
  if (!file.IsValid())
    return nil;

  if (downloadURL_.is_valid() && contents_) {
    scoped_refptr<DragDownloadFile> dragFileDownloader(
        new DragDownloadFile(filePath, std::move(file), downloadURL_,
                             content::Referrer(contents_->GetLastCommittedURL(),
                                               dropData_->referrer_policy),
                             contents_->GetEncoding(), contents_));

    // The finalizer will take care of closing and deletion.
    dragFileDownloader->Start(new PromiseFileFinalizer(
        dragFileDownloader.get()));
  } else {
    // The writer will take care of closing and deletion.
    base::PostTaskWithTraits(
        FROM_HERE, {base::TaskPriority::USER_VISIBLE, base::MayBlock()},
        base::Bind(&PromiseWriterHelper, *dropData_, base::Passed(&file)));
  }

  // The DragDownloadFile constructor may have altered the value of |filePath|
  // if, say, an existing file at the drop site has the same name. Return the
  // actual name that was used to write the file.
  return SysUTF8ToNSString(filePath.BaseName().value());
}

@end  // @implementation WebDragSource

@implementation WebDragSource (Private)

- (void)fillPasteboard {
  if (!contentsView_)
    return;

  DCHECK(pasteboard_.get());

  [pasteboard_ declareTypes:@[ ui::kChromeDragDummyPboardType ]
                      owner:contentsView_];

  // URL (and title).
  if (dropData_->url.is_valid()) {
    [pasteboard_ addTypes:@[ NSURLPboardType, kNSURLTitlePboardType ]
                    owner:contentsView_];
  }

  // MIME type.
  std::string mimeType;

  // File.
  if (!dropData_->file_contents.empty() ||
      !dropData_->download_metadata.empty()) {
    if (dropData_->download_metadata.empty()) {
      base::Optional<base::FilePath> suggestedFilename =
          dropData_->GetSafeFilenameForImageFileContents();
      if (suggestedFilename) {
        downloadFileName_ = std::move(*suggestedFilename);
        net::GetMimeTypeFromExtension(downloadFileName_.Extension(), &mimeType);
      }
    } else {
      base::string16 mimeType16;
      base::FilePath fileName;
      if (content::ParseDownloadMetadata(
              dropData_->download_metadata,
              &mimeType16,
              &fileName,
              &downloadURL_)) {
        // Generate the file name based on both mime type and proposed file
        // name.
        std::string defaultName =
            content::GetContentClient()->browser()->GetDefaultDownloadName();
        mimeType = base::UTF16ToUTF8(mimeType16);
        downloadFileName_ =
            net::GenerateFileName(downloadURL_,
                                  std::string(),
                                  std::string(),
                                  fileName.value(),
                                  mimeType,
                                  defaultName);
      }
    }

    if (!mimeType.empty()) {
      base::ScopedCFTypeRef<CFStringRef> mimeTypeCF(
          base::SysUTF8ToCFStringRef(mimeType));
      fileUTI_.reset(UTTypeCreatePreferredIdentifierForTag(
          kUTTagClassMIMEType, mimeTypeCF.get(), NULL));

      // File (HFS) promise.
      // There are two ways to drag/drop files. NSFilesPromisePboardType is the
      // deprecated way, and kPasteboardTypeFilePromiseContent is the way that
      // does not work. kPasteboardTypeFilePromiseContent is thoroughly broken:
      // * API: There is no good way to get the location for the drop.
      //   <http://lists.apple.com/archives/cocoa-dev/2012/Feb/msg00706.html>
      //   <rdar://14943849> <http://openradar.me/14943849>
      // * Behavior: A file dropped in the Finder is not selected. This can be
      //   worked around by selecting the file in the Finder using AppleEvents,
      //   but the drop target window will come to the front of the Finder's
      //   window list (unlike the previous behavior). <http://crbug.com/278515>
      //   <rdar://14943865> <http://openradar.me/14943865>
      // * Behavior: Files dragged over app icons in the dock do not highlight
      //   the dock icons, and the dock icons do not accept the drop.
      //   <http://crbug.com/282916> <rdar://14943872>
      //   <http://openradar.me/14943872>
      // * Behavior: A file dropped onto the desktop is positioned at the upper
      //   right of the desktop rather than at the position at which it was
      //   dropped. <http://crbug.com/284942> <rdar://14943881>
      //   <http://openradar.me/14943881>
      NSArray* fileUTIList = @[ base::mac::CFToNSCast(fileUTI_.get()) ];
      [pasteboard_ addTypes:@[ NSFilesPromisePboardType ] owner:contentsView_];
      [pasteboard_ setPropertyList:fileUTIList
                           forType:NSFilesPromisePboardType];

      if (!dropData_->file_contents.empty())
        [pasteboard_ addTypes:fileUTIList owner:contentsView_];
    }
  }

  // HTML.
  bool hasHTMLData = !dropData_->html.string().empty();
  // Mail.app and TextEdit accept drags that have both HTML and image flavors on
  // them, but don't process them correctly <http://crbug.com/55879>. Therefore,
  // if there is an image flavor, don't put the HTML data on as HTML, but rather
  // put it on as this Chrome-only flavor.
  //
  // (The only time that Blink fills in the DropData::file_contents is with
  // an image drop, but the MIME time is tested anyway for paranoia's sake.)
  bool hasImageData = !dropData_->file_contents.empty() &&
                      fileUTI_ &&
                      UTTypeConformsTo(fileUTI_.get(), kUTTypeImage);
  if (hasHTMLData) {
    if (hasImageData) {
      [pasteboard_ addTypes:@[ ui::kChromeDragImageHTMLPboardType ]
                      owner:contentsView_];
    } else {
      [pasteboard_ addTypes:@[ NSHTMLPboardType ] owner:contentsView_];
    }
  }

  // Plain text.
  if (!dropData_->text.string().empty()) {
    [pasteboard_ addTypes:@[ NSStringPboardType ]
                    owner:contentsView_];
  }

  if (!dropData_->custom_data.empty()) {
    [pasteboard_ addTypes:@[ ui::kWebCustomDataPboardType ]
                    owner:contentsView_];
  }
}

- (NSImage*)dragImage {
  if (dragImage_)
    return dragImage_;

  // Default to returning a generic image.
  return content::GetContentClient()->GetNativeImageNamed(
      IDR_DEFAULT_FAVICON).ToNSImage();
}

@end  // @implementation WebDragSource (Private)
