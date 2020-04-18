// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/url_drop_target.h"

#import <Cocoa/Cocoa.h>

#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#import "third_party/ocmock/ocmock_extensions.h"
#include "ui/base/clipboard/clipboard_util_mac.h"

namespace {
constexpr NSPoint kDropPoint = {10, 10};
}  // namespace

@interface TestViewDropTarget : NSView<URLDropTarget> {
 @private
  id<URLDropTargetController> controller_;
  base::scoped_nsobject<URLDropTargetHandler> dropHandler_;
}

@end

@implementation TestViewDropTarget

- (id)initWithController:(id<URLDropTargetController>)controller {
  if (self = [super init]) {
    controller_ = controller;
    dropHandler_.reset([[URLDropTargetHandler alloc] initWithView:self]);
  }
  return self;
}

// URLDropTarget protocol.
- (id<URLDropTargetController>)urlDropController {
  return controller_;
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender {
  return [dropHandler_ draggingEntered:sender];
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender {
  return [dropHandler_ draggingUpdated:sender];
}

- (void)draggingExited:(id<NSDraggingInfo>)sender {
  return [dropHandler_ draggingExited:sender];
}

- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender {
  return [dropHandler_ performDragOperation:sender];
}

@end

class UrlDropControllerTest : public CocoaTest {
 protected:
  UrlDropControllerTest();

  id GetFakeDragInfoForPasteboardItem(NSPasteboardItem* item);

  URLDropTargetHandler* DropHandler() { return drop_handler_.get(); }
  id ControllerMock() { return controller_mock_; }
  TestViewDropTarget* View() { return view_; }

 private:
  scoped_refptr<ui::UniquePasteboard> pasteboard_;
  base::scoped_nsobject<TestViewDropTarget> view_;
  base::scoped_nsobject<id> controller_mock_;
  base::scoped_nsobject<URLDropTargetHandler> drop_handler_;
};

UrlDropControllerTest::UrlDropControllerTest()
    : pasteboard_(new ui::UniquePasteboard()) {
  controller_mock_.reset([[OCMockObject
      mockForProtocol:@protocol(URLDropTargetController)] retain]);

  view_.reset([[TestViewDropTarget alloc] initWithController:controller_mock_]);

  [[controller_mock_ stub] hideDropURLsIndicatorInView:view_.get()];

  drop_handler_.reset([[URLDropTargetHandler alloc] initWithView:view_]);
}

id UrlDropControllerTest::GetFakeDragInfoForPasteboardItem(
    NSPasteboardItem* item) {
  // initializing pasteboard
  [pasteboard_->get() clearContents];
  [pasteboard_->get() writeObjects:@[ item ]];

  // creating dragInfo
  id source = [OCMockObject mockForClass:[NSObject class]];
  id dragInfo = [OCMockObject mockForProtocol:@protocol(NSDraggingInfo)];
  [[[dragInfo stub] andReturn:pasteboard_->get()] draggingPasteboard];
  [[[dragInfo stub] andReturnNSPoint:kDropPoint] draggingLocation];
  [[[dragInfo stub] andReturn:source] draggingSource];
  [[[dragInfo stub]
      andReturnUnsignedInteger:NSDragOperationCopy | NSDragOperationMove]
      draggingSourceOperationMask];
  return dragInfo;
}

TEST_F(UrlDropControllerTest, DragAndDropText) {
  constexpr NSString* text = @"query";
  [[ControllerMock() expect] dropText:text inView:View() at:kDropPoint];

  base::scoped_nsobject<NSPasteboardItem> item =
      ui::ClipboardUtil::PasteboardItemFromString(text);

  [View() performDragOperation:GetFakeDragInfoForPasteboardItem(item.get())];
  [ControllerMock() verify];
}

TEST_F(UrlDropControllerTest, DragAndDropTextParsableAsURL) {
  // Different drop targets might choose different behaviors
  // for processing text parsable as URL. For example: tabstrip runs
  // Autocomplete on such text and bookmarks always try to parse it.
  {
    constexpr NSString* text = @"http://edition.cnn.com/";
    [[ControllerMock() expect] dropText:text inView:View() at:kDropPoint];

    base::scoped_nsobject<NSPasteboardItem> item =
        ui::ClipboardUtil::PasteboardItemFromString(text);

    [View() performDragOperation:GetFakeDragInfoForPasteboardItem(item.get())];
    [ControllerMock() verify];
  }
  // Motivational example for not always attempting interpreting strings as
  // URLs.
  {
    constexpr NSString* text = @"query: query";
    [[ControllerMock() expect] dropText:text inView:View() at:kDropPoint];

    base::scoped_nsobject<NSPasteboardItem> item =
        ui::ClipboardUtil::PasteboardItemFromString(text);

    [View() performDragOperation:GetFakeDragInfoForPasteboardItem(item.get())];
    [ControllerMock() verify];
  }
}

TEST_F(UrlDropControllerTest, DragAndDropURL) {
  NSArray* urls = [NSArray arrayWithObject:@"http://abc.xyz"];
  NSArray* titles = [NSArray arrayWithObject:@"abc"];

  [[ControllerMock() expect] dropURLs:urls inView:View() at:kDropPoint];

  base::scoped_nsobject<NSPasteboardItem> item =
      ui::ClipboardUtil::PasteboardItemFromUrls(urls, titles);

  [View() performDragOperation:GetFakeDragInfoForPasteboardItem(item.get())];
  [ControllerMock() verify];
}
