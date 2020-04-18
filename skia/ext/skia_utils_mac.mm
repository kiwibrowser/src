// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "skia/ext/skia_utils_mac.h"

#import <AppKit/AppKit.h>
#include <stdint.h>

#include <memory>

#include "base/logging.h"
#include "base/mac/mac_util.h"
#include "base/mac/scoped_cftyperef.h"
#include "base/mac/scoped_nsobject.h"
#include "skia/ext/platform_canvas.h"
#include "third_party/skia/include/utils/mac/SkCGUtils.h"

namespace {

// Draws an NSImage or an NSImageRep with a given size into a SkBitmap.
SkBitmap NSImageOrNSImageRepToSkBitmapWithColorSpace(
    NSImage* image,
    NSImageRep* image_rep,
    NSSize size,
    bool is_opaque,
    CGColorSpaceRef color_space) {
  // Only image or image_rep should be provided, not both.
  DCHECK((image != 0) ^ (image_rep != 0));

  SkBitmap bitmap;
  if (!bitmap.tryAllocN32Pixels(size.width, size.height, is_opaque))
    return bitmap;  // Return |bitmap| which should respond true to isNull().


  void* data = bitmap.getPixels();

  // Allocate a bitmap context with 4 components per pixel (BGRA). Apple
  // recommends these flags for improved CG performance.
#define HAS_ARGB_SHIFTS(a, r, g, b) \
            (SK_A32_SHIFT == (a) && SK_R32_SHIFT == (r) \
             && SK_G32_SHIFT == (g) && SK_B32_SHIFT == (b))
#if defined(SK_CPU_LENDIAN) && HAS_ARGB_SHIFTS(24, 16, 8, 0)
  base::ScopedCFTypeRef<CGContextRef> context(CGBitmapContextCreate(
      data,
      size.width,
      size.height,
      8,
      size.width * 4,
      color_space,
      kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host));
#else
#error We require that Skia's and CoreGraphics's recommended \
       image memory layout match.
#endif
#undef HAS_ARGB_SHIFTS

  // Something went really wrong. Best guess is that the bitmap data is invalid.
  DCHECK(context);

  [NSGraphicsContext saveGraphicsState];

  NSGraphicsContext* context_cocoa =
      [NSGraphicsContext graphicsContextWithGraphicsPort:context flipped:NO];
  [NSGraphicsContext setCurrentContext:context_cocoa];

  NSRect drawRect = NSMakeRect(0, 0, size.width, size.height);
  if (image) {
    [image drawInRect:drawRect
             fromRect:NSZeroRect
            operation:NSCompositeCopy
             fraction:1.0];
  } else {
    [image_rep drawInRect:drawRect
                 fromRect:NSZeroRect
                operation:NSCompositeCopy
                 fraction:1.0
           respectFlipped:NO
                    hints:nil];
  }

  [NSGraphicsContext restoreGraphicsState];

  return bitmap;
}

} // namespace

namespace skia {

CGAffineTransform SkMatrixToCGAffineTransform(const SkMatrix& matrix) {
  // CGAffineTransforms don't support perspective transforms, so make sure
  // we don't get those.
  DCHECK(matrix[SkMatrix::kMPersp0] == 0.0f);
  DCHECK(matrix[SkMatrix::kMPersp1] == 0.0f);
  DCHECK(matrix[SkMatrix::kMPersp2] == 1.0f);

  return CGAffineTransformMake(matrix[SkMatrix::kMScaleX],
                               matrix[SkMatrix::kMSkewY],
                               matrix[SkMatrix::kMSkewX],
                               matrix[SkMatrix::kMScaleY],
                               matrix[SkMatrix::kMTransX],
                               matrix[SkMatrix::kMTransY]);
}

SkRect CGRectToSkRect(const CGRect& rect) {
  SkRect sk_rect = {
    rect.origin.x, rect.origin.y, CGRectGetMaxX(rect), CGRectGetMaxY(rect)
  };
  return sk_rect;
}

CGRect SkIRectToCGRect(const SkIRect& rect) {
  CGRect cg_rect = {
    { rect.fLeft, rect.fTop },
    { rect.fRight - rect.fLeft, rect.fBottom - rect.fTop }
  };
  return cg_rect;
}

CGRect SkRectToCGRect(const SkRect& rect) {
  CGRect cg_rect = {
    { rect.fLeft, rect.fTop },
    { rect.fRight - rect.fLeft, rect.fBottom - rect.fTop }
  };
  return cg_rect;
}

SkColor CGColorRefToSkColor(CGColorRef color) {
  // TODO(ccameron): This assumes that |color| is already in sRGB. Ideally we'd
  // use something like CGColorCreateCopyByMatchingToColorSpace, but that's
  // only available in macOS 10.11.
  DCHECK(CGColorGetNumberOfComponents(color) == 4);
  const CGFloat* components = CGColorGetComponents(color);
  return SkColorSetARGB(SkScalarRoundToInt(255.0 * components[3]), // alpha
                        SkScalarRoundToInt(255.0 * components[0]), // red
                        SkScalarRoundToInt(255.0 * components[1]), // green
                        SkScalarRoundToInt(255.0 * components[2])); // blue
}

CGColorRef CGColorCreateFromSkColor(SkColor color) {
  double components[] = {SkColorGetR(color) / 255.0,
                         SkColorGetG(color) / 255.0,
                         SkColorGetB(color) / 255.0,
                         SkColorGetA(color) / 255.0};
  return CGColorCreate(base::mac::GetSRGBColorSpace(), components);
}

// Converts NSColor to ARGB
SkColor NSDeviceColorToSkColor(NSColor* color) {
  DCHECK([color colorSpace] == [NSColorSpace genericRGBColorSpace] ||
         [color colorSpace] == [NSColorSpace deviceRGBColorSpace]);
  CGFloat red, green, blue, alpha;
  color = [color colorUsingColorSpace:[NSColorSpace deviceRGBColorSpace]];
  [color getRed:&red green:&green blue:&blue alpha:&alpha];
  return SkColorSetARGB(SkScalarRoundToInt(255.0 * alpha),
                        SkScalarRoundToInt(255.0 * red),
                        SkScalarRoundToInt(255.0 * green),
                        SkScalarRoundToInt(255.0 * blue));
}

// Converts ARGB to NSColor.
NSColor* SkColorToCalibratedNSColor(SkColor color) {
  return [NSColor colorWithCalibratedRed:SkColorGetR(color) / 255.0
                                   green:SkColorGetG(color) / 255.0
                                    blue:SkColorGetB(color) / 255.0
                                   alpha:SkColorGetA(color) / 255.0];
}

NSColor* SkColorToDeviceNSColor(SkColor color) {
  return [NSColor colorWithDeviceRed:SkColorGetR(color) / 255.0
                               green:SkColorGetG(color) / 255.0
                                blue:SkColorGetB(color) / 255.0
                               alpha:SkColorGetA(color) / 255.0];
}

NSColor* SkColorToSRGBNSColor(SkColor color) {
  const CGFloat components[] = {
    SkColorGetR(color) / 255.0,
    SkColorGetG(color) / 255.0,
    SkColorGetB(color) / 255.0,
    SkColorGetA(color) / 255.0
  };
  return [NSColor colorWithColorSpace:[NSColorSpace sRGBColorSpace]
                           components:components
                                count:4];
}

SkBitmap CGImageToSkBitmap(CGImageRef image) {
  SkBitmap bitmap;
  if (image && SkCreateBitmapFromCGImage(&bitmap, image))
    return bitmap;
  return SkBitmap();
}

SkBitmap NSImageToSkBitmapWithColorSpace(
    NSImage* image, bool is_opaque, CGColorSpaceRef color_space) {
  return NSImageOrNSImageRepToSkBitmapWithColorSpace(
      image, nil, [image size], is_opaque, color_space);
}

SkBitmap NSImageRepToSkBitmapWithColorSpace(NSImageRep* image_rep,
                                            NSSize size,
                                            bool is_opaque,
                                            CGColorSpaceRef color_space) {
  return NSImageOrNSImageRepToSkBitmapWithColorSpace(
      nil, image_rep, size, is_opaque, color_space);
}

NSBitmapImageRep* SkBitmapToNSBitmapImageRep(const SkBitmap& skiaBitmap) {
  base::ScopedCFTypeRef<CGColorSpaceRef> color_space(
      CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB));
  return SkBitmapToNSBitmapImageRepWithColorSpace(skiaBitmap, color_space);
}

NSBitmapImageRep* SkBitmapToNSBitmapImageRepWithColorSpace(
    const SkBitmap& skiaBitmap,
    CGColorSpaceRef colorSpace) {
  // First convert SkBitmap to CGImageRef.
  base::ScopedCFTypeRef<CGImageRef> cgimage(
      SkCreateCGImageRefWithColorspace(skiaBitmap, colorSpace));

  // Now convert to NSBitmapImageRep.
  base::scoped_nsobject<NSBitmapImageRep> bitmap(
      [[NSBitmapImageRep alloc] initWithCGImage:cgimage]);
  return [bitmap.release() autorelease];
}

NSImage* SkBitmapToNSImageWithColorSpace(const SkBitmap& skiaBitmap,
                                         CGColorSpaceRef colorSpace) {
  if (skiaBitmap.isNull())
    return nil;

  base::scoped_nsobject<NSImage> image([[NSImage alloc] init]);
  [image addRepresentation:
      SkBitmapToNSBitmapImageRepWithColorSpace(skiaBitmap, colorSpace)];
  [image setSize:NSMakeSize(skiaBitmap.width(), skiaBitmap.height())];
  return [image.release() autorelease];
}

NSImage* SkBitmapToNSImage(const SkBitmap& skiaBitmap) {
  base::ScopedCFTypeRef<CGColorSpaceRef> colorSpace(
      CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB));
  return SkBitmapToNSImageWithColorSpace(skiaBitmap, colorSpace.get());
}

}  // namespace skia
