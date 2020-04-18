// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/location_bar/selected_keyword_decoration.h"

#include <stddef.h>

#include "base/i18n/rtl.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#import "chrome/browser/ui/cocoa/omnibox/omnibox_view_mac.h"
#include "chrome/grit/generated_resources.h"
#include "skia/ext/skia_utils_mac.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/text_elider.h"

namespace {

// Build a short string to use in keyword-search when the field isn't very big.
base::string16 CalculateMinString(const base::string16& description) {
  // Chop at the first '.' or whitespace.
  const size_t chop_index = description.find_first_of(base::kWhitespaceUTF16 +
                                                      base::ASCIIToUTF16("."));
  base::string16 min_string(
      (chop_index == base::string16::npos)
          ? gfx::TruncateString(description, 3, gfx::WORD_BREAK)
          : description.substr(0, chop_index));
  base::i18n::AdjustStringForLocaleDirection(&min_string);
  return min_string;
}

}  // namespace

SelectedKeywordDecoration::SelectedKeywordDecoration() {
  // Note: the unit test
  // SelectedKeywordDecorationTest.UsesPartialKeywordIfNarrow expects to work
  // with a fully-initialized SelectedKeywordDecoration (i.e. one that has a
  // text color and image). During ordinary operation,
  // LocationBarViewMac::Layout() sets the image before the decoration is
  // actually used, which the unit test does as well. If
  // SelectedKeywordDecoration's initialization process changes, the unit test
  // should also be updated.
  SetTextColor(GetBackgroundBorderColor());
}

SelectedKeywordDecoration::~SelectedKeywordDecoration() {}

NSColor* SelectedKeywordDecoration::GetBackgroundBorderColor() {
  return skia::SkColorToSRGBNSColor(gfx::kGoogleBlue700);
}

CGFloat SelectedKeywordDecoration::GetWidthForSpace(CGFloat width) {
  const CGFloat full_width =
      GetWidthForImageAndLabel(search_image_, full_string_);
  if (full_width <= width) {
    BubbleDecoration::SetImage(search_image_);
    SetLabel(full_string_);
    return full_width;
  }

  BubbleDecoration::SetImage(nil);
  const CGFloat no_image_width = GetWidthForImageAndLabel(nil, full_string_);
  if (no_image_width <= width || !partial_string_) {
    SetLabel(full_string_);
    return no_image_width;
  }

  SetLabel(partial_string_);
  return GetWidthForImageAndLabel(nil, partial_string_);
}

void SelectedKeywordDecoration::SetKeyword(const base::string16& short_name,
                                           bool is_extension_keyword) {
  const base::string16 min_name(CalculateMinString(short_name));
  const int keyword_text_id = IDS_OMNIBOX_KEYWORD_TEXT_MD;

  NSString* full_string =
      is_extension_keyword
          ? base::SysUTF16ToNSString(short_name)
          : l10n_util::GetNSStringF(keyword_text_id, short_name);

  // The text will be like "Search <name>:".  "<name>" is a parameter
  // derived from |short_name|. If we're using Material Design, the text will
  // be like "Search <name>" instead.
  full_string_.reset([full_string copy]);

  if (min_name.empty()) {
    partial_string_.reset();
  } else {
    NSString* partial_string =
        is_extension_keyword
            ? base::SysUTF16ToNSString(min_name)
            : l10n_util::GetNSStringF(keyword_text_id, min_name);
    partial_string_.reset([partial_string copy]);
  }
}

void SelectedKeywordDecoration::SetImage(NSImage* image) {
  if (image != search_image_)
    search_image_.reset([image retain]);
  BubbleDecoration::SetImage(image);
}

NSString* SelectedKeywordDecoration::GetAccessibilityLabel() {
  return full_string_.get();
}

bool SelectedKeywordDecoration::IsAccessibilityIgnored() {
  return true;
}
