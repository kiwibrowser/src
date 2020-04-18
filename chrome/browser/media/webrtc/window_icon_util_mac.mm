// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/webrtc/window_icon_util.h"

#include <ApplicationServices/ApplicationServices.h>
#include <Cocoa/Cocoa.h>
#include <CoreFoundation/CoreFoundation.h>

#include "third_party/libyuv/include/libyuv/convert_argb.h"

gfx::ImageSkia GetWindowIcon(content::DesktopMediaID id) {
  DCHECK(id.type == content::DesktopMediaID::TYPE_WINDOW);

  CGWindowID ids[1];
  ids[0] = id.id;
  CFArrayRef window_id_array =
      CFArrayCreate(nullptr, reinterpret_cast<const void**>(&ids), 1, nullptr);
  CFArrayRef window_array =
      CGWindowListCreateDescriptionFromArray(window_id_array);
  if (!window_array || 0 == CFArrayGetCount(window_array)) {
    return gfx::ImageSkia();
  }

  CFDictionaryRef window = reinterpret_cast<CFDictionaryRef>(
      CFArrayGetValueAtIndex(window_array, 0));
  CFNumberRef pid_ref = reinterpret_cast<CFNumberRef>(
      CFDictionaryGetValue(window, kCGWindowOwnerPID));

  int pid;
  CFNumberGetValue(pid_ref, kCFNumberIntType, &pid);

  NSImage* icon_image =
      [[NSRunningApplication runningApplicationWithProcessIdentifier:pid] icon];

  int width = [icon_image size].width;
  int height = [icon_image size].height;

  CGImageRef cg_icon_image =
      [icon_image CGImageForProposedRect:nil context:nil hints:nil];

  int bits_per_pixel = CGImageGetBitsPerPixel(cg_icon_image);
  if (bits_per_pixel != 32) {
    return gfx::ImageSkia();
  }

  CGDataProviderRef provider = CGImageGetDataProvider(cg_icon_image);
  CFDataRef cf_data = CGDataProviderCopyData(provider);

  int src_stride = CGImageGetBytesPerRow(cg_icon_image);
  const uint8_t* src_data = CFDataGetBytePtr(cf_data);

  SkBitmap result;
  result.allocN32Pixels(width, height, false);

  uint8_t* pixels_data = reinterpret_cast<uint8_t*>(result.getPixels());

  libyuv::ABGRToARGB(src_data, src_stride, pixels_data, result.rowBytes(),
                     width, height);

  CFRelease(cf_data);

  return gfx::ImageSkia::CreateFrom1xBitmap(result);
}