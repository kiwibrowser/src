// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <cmath>

#import "chrome/browser/ui/cocoa/location_bar/keyword_hint_decoration.h"

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/cocoa/l10n_util.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/theme_resources.h"
#include "skia/ext/skia_utils_mac.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

namespace {

// How far to inset the hint image from sides.  Lines baseline of text
// in image with baseline of prefix and suffix.
const CGFloat kHintImageYInset = 4.0;

// Extra padding right and left of the image.
const CGFloat kHintImagePadding = 1.0;

// Maxmimum of the available space to allow the hint to take over.
// Should leave enough so that the user has space to edit things.
const CGFloat kHintAvailableRatio = 2.0 / 3.0;

// Extra padding at the right of the decoration.
const CGFloat kHintTrailingPadding = 5.0;

// Helper to convert |s| to an |NSString|, trimming whitespace at
// ends.
NSString* TrimAndConvert(const base::string16& s) {
  base::string16 output;
  base::TrimWhitespace(s, base::TRIM_ALL, &output);
  return base::SysUTF16ToNSString(output);
}

}  // namespace

KeywordHintDecoration::KeywordHintDecoration() {
  NSColor* text_color = [NSColor lightGrayColor];
  attributes_.reset([@{ NSFontAttributeName : GetFont(),
                        NSForegroundColorAttributeName : text_color
                      } retain]);
}

KeywordHintDecoration::~KeywordHintDecoration() {
}

NSImage* KeywordHintDecoration::GetHintImage() {
  if (!hint_image_) {
    hint_image_.reset(ui::ResourceBundle::GetSharedInstance()
                          .GetNativeImageNamed(IDR_OMNIBOX_KEYWORD_HINT_TAB)
                          .CopyNSImage());
  }
  return hint_image_;
}

void KeywordHintDecoration::SetKeyword(const base::string16& short_name,
                                       bool is_extension_keyword) {
  // KEYWORD_HINT is a message like "Press [tab] to search <site>".
  // [tab] is a parameter to be replaced by an image.  "<site>" is
  // derived from |short_name|.
  std::vector<size_t> content_param_offsets;
  int message_id = is_extension_keyword ?
      IDS_OMNIBOX_EXTENSION_KEYWORD_HINT : IDS_OMNIBOX_KEYWORD_HINT;
  const base::string16 keyword_hint(
      l10n_util::GetStringFUTF16(message_id,
                                 base::string16(), short_name,
                                 &content_param_offsets));
  accessibility_text_ = l10n_util::GetStringFUTF16(
      message_id, base::UTF8ToUTF16("Tab"), short_name);

  // Should always be 2 offsets, see the comment in
  // location_bar_view.cc after IDS_OMNIBOX_KEYWORD_HINT fetch.
  DCHECK_EQ(content_param_offsets.size(), 2U);

  // Where to put the [tab] image.
  const size_t split = content_param_offsets.front();

  // Trim the spaces from the edges (there is space in the image) and
  // convert to |NSString|.
  hint_prefix_.reset([TrimAndConvert(keyword_hint.substr(0, split)) retain]);
  hint_suffix_.reset([TrimAndConvert(keyword_hint.substr(split)) retain]);
}

CGFloat KeywordHintDecoration::GetWidthForSpace(CGFloat width) {
  NSImage* image = GetHintImage();
  const CGFloat image_width = image ? [image size].width : 0.0;

  // AFAICT, on Windows the choices are "everything" if it fits, then
  // "image only" if it fits.

  // Entirely too small to fit, omit.
  if (width < image_width)
    return kOmittedWidth;

  // Show the full hint if it won't take up too much space.  The image
  // needs to be placed at a pixel boundary, round the text widths so
  // that any partially-drawn pixels don't look too close (or too
  // far).
  CGFloat full_width =
      std::floor(GetLabelSize(hint_prefix_, attributes_).width + 0.5) +
      kHintImagePadding + image_width + kHintImagePadding +
      std::floor(GetLabelSize(hint_suffix_, attributes_).width + 0.5) +
      kHintTrailingPadding;
  if (full_width <= width * kHintAvailableRatio)
    return full_width;

  return image_width;
}

NSString* KeywordHintDecoration::GetAccessibilityLabel() {
  return base::SysUTF16ToNSString(accessibility_text_);
}

bool KeywordHintDecoration::IsAccessibilityIgnored() {
  return true;
}

void KeywordHintDecoration::DrawInFrame(NSRect frame, NSView* control_view) {
  NSImage* image = GetHintImage();
  const CGFloat image_width = image ? [image size].width : 0.0;

  const bool draw_full = NSWidth(frame) > image_width;

  BOOL is_rtl = cocoa_l10n_util::ShouldDoExperimentalRTLLayout();
  NSString* left_string = is_rtl ? hint_suffix_ : hint_prefix_;
  NSString* right_string = is_rtl ? hint_prefix_ : hint_suffix_;
  if (is_rtl) {
    frame.origin.x += kHintTrailingPadding;
    frame.size.width -= kHintTrailingPadding;
  }

  if (draw_full) {
    NSRect left_rect = frame;
    const CGFloat left_width = GetLabelSize(left_string, attributes_).width;
    left_rect.size.width = left_width;
    DrawLabel(left_string, attributes_, left_rect);
    // The image should be drawn at a pixel boundary, round the prefix
    // so that partial pixels aren't oddly close (or distant).
    frame.origin.x += std::floor(left_width + 0.5) + kHintImagePadding;
    frame.size.width -= std::floor(left_width + 0.5) + kHintImagePadding;
  }

  NSRect image_rect = NSInsetRect(frame, 0.0, kHintImageYInset);
  image_rect.size = [image size];
  [image drawInRect:image_rect
            fromRect:NSZeroRect  // Entire image
           operation:NSCompositeSourceOver
            fraction:1.0
      respectFlipped:YES
               hints:nil];
  frame.origin.x += NSWidth(image_rect);
  frame.size.width -= NSWidth(image_rect);

  if (draw_full) {
    NSRect right_rect = frame;
    const CGFloat right_width = GetLabelSize(right_string, attributes_).width;

    // Draw the text kHintImagePadding away from [tab] icon so that
    // equal amount of space is maintained on either side of the icon.
    // This also ensures that suffix text is at the same distance
    // from [tab] icon in different web pages.
    right_rect.origin.x += kHintImagePadding;
    DCHECK_GE(NSWidth(right_rect), right_width);
    DrawLabel(right_string, attributes_, right_rect);
  }
}
