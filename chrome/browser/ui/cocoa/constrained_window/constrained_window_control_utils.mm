// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_control_utils.h"

#include "base/mac/scoped_nsobject.h"
#include "base/strings/sys_string_conversions.h"
#include "skia/ext/skia_utils_mac.h"
#import "third_party/google_toolbox_for_mac/src/AppKit/GTMUILocalizerAndLayoutTweaker.h"
#include "ui/base/l10n/l10n_util_mac.h"

namespace constrained_window {

NSTextField* CreateLabel() {
  NSTextField* label =
      [[[NSTextField alloc] initWithFrame:NSZeroRect] autorelease];
  [label setEditable:NO];
  [label setSelectable:NO];
  [label setBezeled:NO];
  [label setDrawsBackground:NO];
  return label;
}

NSAttributedString* GetAttributedLabelString(
    NSString* string,
    ui::ResourceBundle::FontStyle fontStyle,
    NSTextAlignment alignment,
    NSLineBreakMode lineBreakMode) {
  if (!string)
    return nil;

  const gfx::Font& font =
      ui::ResourceBundle::GetSharedInstance().GetFont(fontStyle);
  base::scoped_nsobject<NSMutableParagraphStyle> paragraphStyle(
      [[NSMutableParagraphStyle alloc] init]);
  [paragraphStyle setAlignment:alignment];
  [paragraphStyle setLineBreakMode:lineBreakMode];

  NSDictionary* attributes = @{
      NSFontAttributeName:            font.GetNativeFont(),
      NSParagraphStyleAttributeName:  paragraphStyle.get()
  };
  return [[[NSAttributedString alloc] initWithString:string
                                          attributes:attributes] autorelease];
}

NSTextField* AddTextField(NSView* parent,
                          const base::string16& message,
                          const ui::ResourceBundle::FontStyle& font_style) {
  NSTextField* textField = constrained_window::CreateLabel();
  [textField
      setAttributedStringValue:constrained_window::GetAttributedLabelString(
                                   base::SysUTF16ToNSString(message),
                                   font_style, NSNaturalTextAlignment,
                                   NSLineBreakByWordWrapping)];
  [parent addSubview:textField];
  return textField;
}

void AddButton(NSView* parent,
               NSButton* button,
               int title_id,
               id target,
               SEL action,
               BOOL should_auto_size) {
  if (title_id)
    [button setTitle:l10n_util::GetNSString(title_id)];
  [button setTarget:target];
  [button setAction:action];
  [parent addSubview:button];
  if (should_auto_size)
    [GTMUILocalizerAndLayoutTweaker sizeToFitView:button];
}

NSRect ComputeFrame(NSAttributedString* text, CGFloat width, CGFloat height) {
  NSRect frame =
      [text boundingRectWithSize:NSMakeSize(width, height)
                         options:NSStringDrawingUsesLineFragmentOrigin];

  // boundingRectWithSize: is known to underestimate the width, so
  // additional padding needs to be added.
  static const CGFloat kTextViewPadding = 10;
  frame.size.width += kTextViewPadding;
  return frame;
}

}  // namespace constrained_window
