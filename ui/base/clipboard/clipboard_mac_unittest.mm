// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/base/clipboard/clipboard_mac.h"

#import <AppKit/AppKit.h>

#include "base/mac/scoped_cftyperef.h"
#include "base/mac/scoped_nsobject.h"
#include "base/memory/free_deleter.h"
#include "base/memory/ref_counted.h"
#include "testing/platform_test.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/clipboard/clipboard_types.h"
#include "ui/base/clipboard/clipboard_util_mac.h"

@interface RedView : NSView
@end
@implementation RedView
- (void)drawRect:(NSRect)dirtyRect {
  [[NSColor redColor] setFill];
  NSRectFill(dirtyRect);
  [super drawRect:dirtyRect];
}
@end

namespace ui {

class ClipboardMacTest : public PlatformTest {
 public:
  ClipboardMacTest() { }

  base::scoped_nsobject<NSImage> CreateImage(int32_t width,
                                             int32_t height,
                                             bool retina) {
    int32_t pixel_width = retina ? width * 2 : width;
    int32_t pixel_height = retina ? height * 2 : height;

    // It seems more natural to create an NSBitmapImageRep and set it as the
    // representation for an NSImage. This doesn't work, because when the
    // result is written, and then read from an NSPasteboard, the new NSImage
    // loses its "retina-ness".
    std::unique_ptr<uint8_t, base::FreeDeleter> buffer(
        static_cast<uint8_t*>(calloc(pixel_width * pixel_height, 4)));
    base::ScopedCFTypeRef<CGDataProviderRef> provider(
        CGDataProviderCreateWithData(
            nullptr, buffer.get(), (pixel_width * pixel_height * 4), nullptr));
    base::ScopedCFTypeRef<CGColorSpaceRef> color_space(
        CGColorSpaceCreateWithName(kCGColorSpaceSRGB));
    base::ScopedCFTypeRef<CGImageRef> image_ref(
        CGImageCreate(pixel_width, pixel_height, 8, 32, 4 * pixel_width,
                      color_space.get(), kCGBitmapByteOrderDefault,
                      provider.get(), nullptr, NO, kCGRenderingIntentDefault));
    return base::scoped_nsobject<NSImage>([[NSImage alloc]
        initWithCGImage:image_ref.get()
                   size:NSMakeSize(width, height)]);
  }
};

#if defined(MEMORY_SANITIZER) || defined(ADDRESS_SANITIZER)
#define MAYBE_ReadImageRetina DISABLED_ReadImageRetina
#else
#define MAYBE_ReadImageRetina ReadImageRetina
#endif
TEST_F(ClipboardMacTest, MAYBE_ReadImageRetina) {
  int32_t width = 99;
  int32_t height = 101;
  scoped_refptr<ui::UniquePasteboard> pasteboard = new ui::UniquePasteboard;
  base::scoped_nsobject<NSImage> image = CreateImage(width, height, true);
  [pasteboard->get() writeObjects:@[ image.get() ]];

  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  ui::ClipboardMac* clipboard_mac = static_cast<ui::ClipboardMac*>(clipboard);

  SkBitmap bitmap = clipboard_mac->ReadImage(ui::CLIPBOARD_TYPE_COPY_PASTE,
                                             pasteboard->get());
  EXPECT_EQ(2 * width, bitmap.width());
  EXPECT_EQ(2 * height, bitmap.height());
}

#if defined(MEMORY_SANITIZER) || defined(ADDRESS_SANITIZER)
#define MAYBE_ReadImageNonRetina DISABLED_ReadImageNonRetina
#else
#define MAYBE_ReadImageNonRetina ReadImageNonRetina
#endif
TEST_F(ClipboardMacTest, MAYBE_ReadImageNonRetina) {
  int32_t width = 99;
  int32_t height = 101;
  scoped_refptr<ui::UniquePasteboard> pasteboard = new ui::UniquePasteboard;
  base::scoped_nsobject<NSImage> image = CreateImage(width, height, false);
  [pasteboard->get() writeObjects:@[ image.get() ]];

  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  ui::ClipboardMac* clipboard_mac = static_cast<ui::ClipboardMac*>(clipboard);

  SkBitmap bitmap = clipboard_mac->ReadImage(ui::CLIPBOARD_TYPE_COPY_PASTE,
                                             pasteboard->get());
  EXPECT_EQ(width, bitmap.width());
  EXPECT_EQ(height, bitmap.height());
}

TEST_F(ClipboardMacTest, EmptyImage) {
  base::scoped_nsobject<NSImage> image([[NSImage alloc] init]);
  scoped_refptr<ui::UniquePasteboard> pasteboard = new ui::UniquePasteboard;
  [pasteboard->get() writeObjects:@[ image.get() ]];

  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  ui::ClipboardMac* clipboard_mac = static_cast<ui::ClipboardMac*>(clipboard);

  SkBitmap bitmap = clipboard_mac->ReadImage(ui::CLIPBOARD_TYPE_COPY_PASTE,
                                             pasteboard->get());
  EXPECT_EQ(0, bitmap.width());
  EXPECT_EQ(0, bitmap.height());
}

TEST_F(ClipboardMacTest, PDFImage) {
  int32_t width = 99;
  int32_t height = 101;
  NSRect frame = NSMakeRect(0, 0, width, height);

  // This seems like a round-about way of getting a NSPDFImageRep to shove into
  // an NSPasteboard. However, I haven't found any other way of generating a
  // "PDF" image that makes NSPasteboard happy.
  base::scoped_nsobject<NSView> v([[RedView alloc] initWithFrame:frame]);
  NSData* data = [v dataWithPDFInsideRect:frame];

  scoped_refptr<ui::UniquePasteboard> pasteboard = new ui::UniquePasteboard;
  [pasteboard->get() setData:data forType:NSPasteboardTypePDF];

  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  ui::ClipboardMac* clipboard_mac = static_cast<ui::ClipboardMac*>(clipboard);

  SkBitmap bitmap = clipboard_mac->ReadImage(ui::CLIPBOARD_TYPE_COPY_PASTE,
                                             pasteboard->get());
  EXPECT_EQ(width, bitmap.width());
  EXPECT_EQ(height, bitmap.height());
}

}  // namespace ui
