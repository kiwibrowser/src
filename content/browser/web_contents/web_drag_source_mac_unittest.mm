// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "content/browser/web_contents/web_drag_source_mac.h"

#include "base/memory/ref_counted.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/common/drop_data.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "ui/base/clipboard/clipboard_util_mac.h"
#include "url/gurl.h"

namespace content {

typedef RenderViewHostTestHarness WebDragSourceMacTest;

TEST_F(WebDragSourceMacTest, DragInvalidlyEscapedBookmarklet) {
  std::unique_ptr<WebContents> contents(CreateTestWebContents());
  base::scoped_nsobject<NSView> view(
      [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 10, 10)]);

  std::unique_ptr<DropData> dropData(new DropData);
  dropData->url = GURL("javascript:%");

  WebContentsImpl* contentsImpl = static_cast<WebContentsImpl*>(contents.get());
  scoped_refptr<ui::UniquePasteboard> pasteboard1 = new ui::UniquePasteboard;
  base::scoped_nsobject<WebDragSource> source([[WebDragSource alloc]
       initWithContents:contentsImpl
                   view:view
               dropData:dropData.get()
              sourceRWH:contentsImpl->GetRenderViewHost()->GetWidget()
                  image:nil
                 offset:NSZeroPoint
             pasteboard:pasteboard1->get()
      dragOperationMask:NSDragOperationCopy]);

  // Test that this call doesn't throw any exceptions: http://crbug.com/128371
  scoped_refptr<ui::UniquePasteboard> pasteboard2 = new ui::UniquePasteboard;
  [source lazyWriteToPasteboard:pasteboard2->get() forType:NSURLPboardType];
}

}  // namespace content
