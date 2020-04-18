// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/tab_contents/favicon_util_mac.h"

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/favicon/favicon_utils.h"
#include "skia/ext/skia_utils_mac.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia_util_mac.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/resources/grit/ui_resources.h"

namespace {

const CGFloat kVectorIconSize = 16;

bool HasDefaultFavicon(content::WebContents* contents) {
  gfx::Image favicon = favicon::TabFaviconFromWebContents(contents);
  if (favicon.IsEmpty())
    return false;
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  const gfx::ImageSkia* default_favicon =
      rb.GetImageSkiaNamed(IDR_DEFAULT_FAVICON);

  return favicon.ToImageSkia()->BackedBySameObjectAs(*default_favicon);
}

}  // namespace

namespace mac {

NSImage* FaviconForWebContents(content::WebContents* contents, SkColor color) {
  // If |contents| is using the default favicon, it needs to be drawn in
  // |color|, which means this function can't necessarily reuse the existing
  // favicon.
  if (contents && !HasDefaultFavicon(contents)) {
    NSImage* image = favicon::TabFaviconFromWebContents(contents).AsNSImage();

    // The |image| could be nil if the bitmap is null. In that case, fallback
    // to the default image.
    if (image)
      return image;
  }

  return NSImageFromImageSkia(
      gfx::CreateVectorIcon(kDefaultFaviconIcon, kVectorIconSize, color));
}

}  // namespace mac
