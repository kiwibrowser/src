// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/drag_util.h"

#include <cmath>

#include "base/files/file_path.h"
#include "base/i18n/rtl.h"
#include "base/mac/scoped_nsobject.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/cocoa/l10n_util.h"
#include "content/public/browser/plugin_service.h"
#include "content/public/common/webplugininfo.h"
#include "ipc/ipc_message.h"
#include "net/base/filename_util.h"
#include "net/base/mime_util.h"
#include "third_party/blink/public/common/mime_util/mime_util.h"
#import "third_party/mozilla/NSPasteboard+Utils.h"
#import "ui/base/dragdrop/cocoa_dnd_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/scoped_ns_graphics_context_save_gstate_mac.h"
#include "ui/resources/grit/ui_resources.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "url/url_constants.h"

using content::PluginService;

namespace drag_util {

namespace {

BOOL IsSupportedFileURL(Profile* profile, const GURL& url) {
  base::FilePath full_path;
  net::FileURLToFilePath(url, &full_path);

  std::string mime_type;
  net::GetMimeTypeFromFile(full_path, &mime_type);

  if (blink::IsSupportedMimeType(mime_type))
    return YES;

  // Check whether there is a plugin that supports the mime type. (e.g. PDF)
  // TODO(bauerb): This possibly uses stale information, but it's guaranteed not
  // to do disk access.
  bool allow_wildcard = false;
  content::WebPluginInfo plugin;
  return PluginService::GetInstance()->GetPluginInfo(
      -1,                // process ID
      MSG_ROUTING_NONE,  // routing ID
      profile->GetResourceContext(), url, url::Origin(), mime_type,
      allow_wildcard, NULL, &plugin, NULL);
}

// Draws string |title| within box |frame|, positioning it at the origin.
// Truncates text with fading if it is too long to fit horizontally.
// Based on code from GradientButtonCell but simplified where possible.
void DrawTruncatedTitle(NSAttributedString* title,
                        NSRect frame,
                        bool is_title_rtl) {
  NSSize size = [title size];
  if (std::floor(size.width) <= NSWidth(frame)) {
    [title drawAtPoint:frame.origin];
    return;
  }

  // The gradient is about twice the line height long.
  NSRectEdge gradient_edge;
  if (is_title_rtl)
    gradient_edge = NSMinXEdge;
  else
    gradient_edge = NSMaxXEdge;

  CGFloat gradient_width = std::min(size.height * 2, NSWidth(frame) / 4);
  NSRect solid_part, gradient_part;
  NSDivideRect(frame, &gradient_part, &solid_part, gradient_width,
               gradient_edge);
  CGContextRef context = static_cast<CGContextRef>(
      [[NSGraphicsContext currentContext] graphicsPort]);
  CGContextBeginTransparencyLayerWithRect(context, NSRectToCGRect(frame), 0);
  { // Draw text clipped to frame.
    gfx::ScopedNSGraphicsContextSaveGState scoped_state;
    [NSBezierPath clipRect:frame];
    [title drawInRect:frame];
  }

  NSColor* color = [NSColor blackColor];
  NSColor* alpha_color = [color colorWithAlphaComponent:0.0];
  base::scoped_nsobject<NSGradient> mask(
      [[NSGradient alloc] initWithStartingColor:color endingColor:alpha_color]);
  // Draw the gradient mask.
  CGContextSetBlendMode(context, kCGBlendModeDestinationIn);
  CGFloat gradient_x_start, gradient_x_end;
  if (is_title_rtl) {
    gradient_x_start = NSMaxX(gradient_part);
    gradient_x_end = NSMinX(gradient_part);
  } else {
    gradient_x_start = NSMinX(gradient_part);
    gradient_x_end = NSMaxX(gradient_part);
  }
  [mask drawFromPoint:NSMakePoint(gradient_x_start, NSMinY(frame))
              toPoint:NSMakePoint(gradient_x_end, NSMinY(frame))
              options:NSGradientDrawsBeforeStartingLocation];
  CGContextEndTransparencyLayer(context);
}

}  // namespace

GURL GetFileURLFromDropData(id<NSDraggingInfo> info) {
  if ([[info draggingPasteboard] containsURLDataConvertingTextToURL:YES]) {
    GURL url;
    ui::PopulateURLAndTitleFromPasteboard(&url,
                                          NULL,
                                          [info draggingPasteboard],
                                          YES);

    if (url.SchemeIs(url::kFileScheme))
      return url;
  }
  return GURL();
}

BOOL IsUnsupportedDropData(Profile* profile, id<NSDraggingInfo> info) {
  GURL url = GetFileURLFromDropData(info);
  if (!url.is_empty()) {
    // If dragging a file, only allow dropping supported file types (that the
    // web view can display).
    return !IsSupportedFileURL(profile, url);
  }
  return NO;
}

NSImage* DragImageForBookmark(NSImage* favicon,
                              const base::string16& title,
                              CGFloat drag_image_width) {
  // If no favicon, use a default.
  if (!favicon) {
    ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
    favicon = rb.GetNativeImageNamed(IDR_DEFAULT_FAVICON).ToNSImage();
  }

  // If no title, just use icon.
  if (title.empty())
    return favicon;
  NSString* ns_title = base::SysUTF16ToNSString(title);

  // Set the look of the title.
  base::scoped_nsobject<NSMutableParagraphStyle> paragraph_style(
      [[NSMutableParagraphStyle alloc] init]);
  [paragraph_style setLineBreakMode:NSLineBreakByClipping];
  NSDictionary* attrs = @{
    NSFontAttributeName :
        [NSFont systemFontOfSize:[NSFont smallSystemFontSize]],
    NSParagraphStyleAttributeName : paragraph_style
  };
  base::scoped_nsobject<NSAttributedString> rich_title(
      [[NSAttributedString alloc] initWithString:ns_title attributes:attrs]);

  // Set up sizes and locations for rendering.
  const CGFloat kIconPadding = 2.0;  // Gap between icon and text.
  NSRect favicon_rect = {NSZeroPoint, [favicon size]};
  CGFloat icon_plus_padding_width = NSWidth(favicon_rect) + kIconPadding;
  CGFloat full_text_width = [rich_title size].width;
  CGFloat allowed_text_width = drag_image_width - icon_plus_padding_width;
  CGFloat used_text_width = std::min(full_text_width, allowed_text_width);
  NSRect full_drag_image_rect = NSMakeRect(
      0, 0, icon_plus_padding_width + used_text_width, NSHeight(favicon_rect));

  NSRectEdge icon_edge;
  if (cocoa_l10n_util::ShouldDoExperimentalRTLLayout())
    icon_edge = NSMaxXEdge;
  else
    icon_edge = NSMinXEdge;

  NSRect icon_rect;
  NSRect text_plus_padding_rect;
  NSRect padding_rect;
  NSRect text_rect;

  // Slice off the icon.
  NSDivideRect(full_drag_image_rect, &icon_rect, &text_plus_padding_rect,
               NSWidth(favicon_rect), icon_edge);

  // Slice off the padding.
  NSDivideRect(text_plus_padding_rect, &padding_rect, &text_rect, kIconPadding,
               icon_edge);

  // Render the drag image.
  NSImage* drag_image =
      [[[NSImage alloc] initWithSize:full_drag_image_rect.size] autorelease];
  [drag_image lockFocus];
  [favicon drawAtPoint:icon_rect.origin
              fromRect:NSZeroRect
             operation:NSCompositeSourceOver
              fraction:0.7];

  bool is_title_rtl = base::i18n::GetFirstStrongCharacterDirection(title) ==
                      base::i18n::RIGHT_TO_LEFT;
  DrawTruncatedTitle(rich_title, text_rect, is_title_rtl);
  [drag_image unlockFocus];

  return drag_image;
}

}  // namespace drag_util
