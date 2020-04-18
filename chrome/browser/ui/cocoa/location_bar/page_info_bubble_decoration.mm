// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/location_bar/page_info_bubble_decoration.h"

#include <cmath>

#import "base/mac/mac_util.h"
#include "base/strings/sys_string_conversions.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_controller.h"
#import "chrome/browser/ui/cocoa/drag_util.h"
#include "chrome/browser/ui/cocoa/l10n_util.h"
#import "chrome/browser/ui/cocoa/location_bar/location_bar_view_mac.h"
#import "chrome/browser/ui/cocoa/themed_window.h"
#include "chrome/browser/ui/page_info/page_info_dialog.h"
#include "chrome/grit/generated_resources.h"
#include "components/favicon/content/content_favicon_driver.h"
#include "components/strings/grit/components_strings.h"
#include "components/toolbar/toolbar_model.h"
#include "skia/ext/skia_utils_mac.h"
#import "third_party/mozilla/NSPasteboard+Utils.h"
#import "ui/base/cocoa/nsview_additions.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/gfx/animation/tween.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/image/image_skia_util_mac.h"
#include "ui/gfx/scoped_ns_graphics_context_save_gstate_mac.h"
#include "ui/gfx/text_elider.h"

// TODO(spqchan): Decorations that don't fit in the available space are
// omitted. See crbug.com/638427.

namespace {

// Padding between the label and icon/divider.
CGFloat kLabelPadding = 4.0;

// Inset for the background.
const CGFloat kBackgroundYInset = 4.0;

// The offset of the text's baseline on a retina screen.
const CGFloat kRetinaBaselineOffset = 0.5;

// The info-bubble point should look like it points to the bottom of the lock
// icon. Determined with Pixie.app.
const CGFloat kPageInfoBubblePointYOffset = 2.0;

// Minimum acceptable width for the ev bubble.
const CGFloat kMinElidedBubbleWidth = 150.0;

// Maximum amount of available space to make the bubble, subject to
// |kMinElidedBubbleWidth|.
const float kMaxBubbleFraction = 0.5;

// Duration of animation in ms.
const NSTimeInterval kInAnimationDuration = 330;
const NSTimeInterval kOutAnimationDuration = 250;

// Transformation values at the beginning of the animation.
const CGFloat kStartScale = 0.25;
const CGFloat kStartx_offset = 15.0;

}  // namespace

//////////////////////////////////////////////////////////////////
// PageInfoBubbleDecoration, public:

PageInfoBubbleDecoration::PageInfoBubbleDecoration(LocationBarViewMac* owner)
    : label_color_(gfx::kGoogleGreen700),
      image_fade_(true),
      animation_(this),
      owner_(owner),
      disable_animations_during_testing_(false) {
  // On Retina the text label is 1px above the Omnibox textfield's text
  // baseline. If the Omnibox textfield also drew the label the baselines
  // would align.
  SetRetinaBaselineOffset(kRetinaBaselineOffset);

  base::scoped_nsobject<NSMutableParagraphStyle> style(
      [[NSMutableParagraphStyle alloc] init]);
  [style setLineBreakMode:NSLineBreakByClipping];
  if (cocoa_l10n_util::ShouldDoExperimentalRTLLayout())
    [style setAlignment:NSRightTextAlignment];
  [attributes_ setObject:style forKey:NSParagraphStyleAttributeName];
  animation_.SetTweenType(gfx::Tween::FAST_OUT_SLOW_IN);
}

PageInfoBubbleDecoration::~PageInfoBubbleDecoration() {
  // Just in case the timer is still holding onto the animation object, force
  // cleanup so it can't get back to |this|.
}

void PageInfoBubbleDecoration::SetFullLabel(NSString* label) {
  full_label_.reset([label copy]);
  SetLabel(full_label_);
}

void PageInfoBubbleDecoration::SetLabelColor(SkColor color) {
  label_color_ = color;
}

void PageInfoBubbleDecoration::AnimateIn(bool image_fade) {
  image_fade_ = image_fade;
  if (HasAnimatedIn())
    animation_.Reset();

  animation_.SetSlideDuration(kInAnimationDuration);
  animation_.Show();
}

void PageInfoBubbleDecoration::AnimateOut() {
  if (!HasAnimatedIn())
    return;

  animation_.SetSlideDuration(kOutAnimationDuration);
  animation_.Hide();
}

void PageInfoBubbleDecoration::ShowWithoutAnimation() {
  animation_.Reset(1.0);
}

bool PageInfoBubbleDecoration::HasAnimatedIn() const {
  return animation_.IsShowing() && animation_.GetCurrentValue() == 1.0;
}

bool PageInfoBubbleDecoration::HasAnimatedOut() const {
  return !animation_.IsShowing() && animation_.GetCurrentValue() == 0.0;
}

bool PageInfoBubbleDecoration::AnimatingOut() const {
  return !animation_.IsShowing() && animation_.GetCurrentValue() != 0.0;
}

void PageInfoBubbleDecoration::ResetAnimation() {
  animation_.Reset();
}

//////////////////////////////////////////////////////////////////
// PageInfoBubbleDecoration::LocationBarDecoration:

CGFloat PageInfoBubbleDecoration::GetWidthForSpace(CGFloat width) {
  CGFloat icon_width = GetWidthForImageAndLabel(image_, nil);
  CGFloat text_width = GetWidthForText(width) - icon_width;
  return (text_width * GetAnimationProgress()) + icon_width;
}

void PageInfoBubbleDecoration::DrawInFrame(NSRect frame, NSView* control_view) {
  const NSRect decoration_frame = NSInsetRect(frame, 0.0, kBackgroundYInset);
  CGFloat text_left_offset = NSMinX(decoration_frame);
  CGFloat text_right_offset = NSMaxX(decoration_frame);
  const BOOL is_rtl = cocoa_l10n_util::ShouldDoExperimentalRTLLayout();
  focus_ring_right_inset_ = 0;
  focus_ring_left_inset_ = 0;
  if (image_) {
    // The image should fade in if we're animating in.
    CGFloat image_alpha =
        image_fade_ && animation_.IsShowing() ? GetAnimationProgress() : 1.0;

    NSRect image_rect = GetImageRectInFrame(decoration_frame);
    [image_ drawInRect:image_rect
              fromRect:NSZeroRect  // Entire image
             operation:NSCompositeSourceOver
              fraction:image_alpha
        respectFlipped:YES
                 hints:nil];
    if (is_rtl) {
      text_left_offset += DividerPadding();
      text_right_offset = NSMinX(image_rect);
    } else {
      text_right_offset -= DividerPadding();
      text_left_offset = NSMaxX(image_rect);
    }
  }

  // Set the text color and draw the text.
  if (HasLabel()) {
    bool in_dark_mode = [[control_view window] inIncognitoModeWithSystemTheme];
    NSColor* text_color =
        in_dark_mode ? skia::SkColorToSRGBNSColor(kMaterialDarkModeTextColor)
                     : GetBackgroundBorderColor();
    SetTextColor(text_color);

    // Transform the coordinate system to adjust the baseline on Retina.
    // This is the only way to get fractional adjustments.
    gfx::ScopedNSGraphicsContextSaveGState save_graphics_state;
    CGFloat line_width = [control_view cr_lineWidth];
    if (line_width < 1) {
      NSAffineTransform* transform = [NSAffineTransform transform];
      [transform translateXBy:0 yBy:kRetinaBaselineOffset];
      [transform concat];
    }

    base::scoped_nsobject<NSAttributedString> text([[NSAttributedString alloc]
        initWithString:label_
            attributes:attributes_]);

    // Calculate the text frame based on the text height and offsets.
    NSRect text_rect = frame;
    CGFloat textHeight = [text size].height;

    text_rect.origin.x = text_left_offset;
    text_rect.origin.y = std::round(NSMidY(text_rect) - textHeight / 2.0) - 1;
    text_rect.size.width = text_right_offset - text_left_offset;
    text_rect.size.height = textHeight;
    text_rect = NSInsetRect(text_rect, kLabelPadding, 0);

    NSAffineTransform* transform = [NSAffineTransform transform];
    CGFloat progress = GetAnimationProgress();

    // Apply transformations so that the text animation:
    // - Scales from 0.75 to 1.
    // - Translates the X position to its origin after it got scaled, and
    //   before moving in a position from from -15 to 0
    // - Translates the Y position so that the text is centered vertically.
    double scale = gfx::Tween::DoubleValueBetween(progress, kStartScale, 1.0);

    double x_origin_offset = NSMinX(text_rect) * (1 - scale);
    double y_origin_offset = NSMinY(text_rect) * (1 - scale);
    double start_x_offset = is_rtl ? -kStartx_offset : kStartx_offset;
    double x_offset =
        gfx::Tween::DoubleValueBetween(progress, start_x_offset, 0);
    double y_offset = NSHeight(text_rect) * (1 - scale) / 2.0;

    [transform translateXBy:x_offset + x_origin_offset
                        yBy:y_offset + y_origin_offset];
    [transform scaleBy:scale];
    [transform concat];

    // Draw the label.
    [text drawInRect:text_rect];

    // Draw the divider.
    if (state() == DecorationMouseState::NONE && !active()) {
      DrawDivider(control_view, decoration_frame, GetAnimationProgress());
      focus_ring_right_inset_ = DividerPadding() + line_width;
    } else {
      // When mouse-hovered, the divider isn't drawn, but the padding for it is
      // still present to separate the button from the location bar text.
      focus_ring_right_inset_ = DividerPadding();
    }
  } else {
    // When there's no label, the page info bubble decoration is just an icon.
    // In this case, this decoration is hard up against the location bar text,
    // so the focus ring needs to be inset to avoid overlapping the location bar
    // text. The focus ring also has to be inset symmetrically, since this
    // decoration is symmetric. This causes the focus ring to overlap the
    // location bar's border by different amounts depending on the page info
    // decoration's style, which is a bummer.
    focus_ring_right_inset_ = 2;
    focus_ring_left_inset_ = 2;
  }
}

bool PageInfoBubbleDecoration::IsDraggable() {
  // Without a tab it will be impossible to get the information needed
  // to perform a drag.
  if (!owner_->GetWebContents())
    return false;

  // Do not drag if the user has been editing the location bar, or the
  // location bar is at the NTP.
  return (!owner_->GetOmniboxView()->IsEditingOrEmpty());
}

NSPasteboard* PageInfoBubbleDecoration::GetDragPasteboard() {
  content::WebContents* tab = owner_->GetWebContents();
  DCHECK(tab);  // See |IsDraggable()|.

  NSString* url = base::SysUTF8ToNSString(tab->GetURL().spec());
  NSString* title = base::SysUTF16ToNSString(tab->GetTitle());

  NSPasteboard* pboard = [NSPasteboard pasteboardWithName:NSDragPboard];
  [pboard declareURLPasteboardWithAdditionalTypes:@[ NSFilesPromisePboardType ]
                                            owner:nil];
  [pboard setDataForURL:url title:title];

  [pboard setPropertyList:@[ @"webloc" ] forType:NSFilesPromisePboardType];

  return pboard;
}

NSImage* PageInfoBubbleDecoration::GetDragImage() {
  content::WebContents* web_contents = owner_->GetWebContents();
  NSImage* favicon =
      favicon::ContentFaviconDriver::FromWebContents(web_contents)
          ->GetFavicon()
          .AsNSImage();
  NSImage* icon_image = favicon ? favicon : GetImage();

  NSImage* image = drag_util::DragImageForBookmark(
      icon_image, web_contents->GetTitle(), bookmarks::kDefaultBookmarkWidth);
  NSSize image_size = [image size];
  drag_frame_ = NSMakeRect(0, 0, image_size.width, image_size.height);
  return image;
}

NSRect PageInfoBubbleDecoration::GetDragImageFrame(NSRect frame) {
  // If GetDragImage has never been called, drag_frame_ has not been calculated.
  if (NSIsEmptyRect(drag_frame_))
    GetDragImage();
  return drag_frame_;
}

bool PageInfoBubbleDecoration::OnMousePressed(NSRect frame, NSPoint location) {
  // Do not show page info if the user has been editing the location
  // bar, or the location bar is at the NTP.
  if (owner_->GetOmniboxView()->IsEditingOrEmpty())
    return true;

  return ShowPageInfoDialog(owner_->GetWebContents());
}

AcceptsPress PageInfoBubbleDecoration::AcceptsMousePress() {
  return owner_->GetOmniboxView()->IsEditingOrEmpty()
             ? AcceptsPress::NEVER
             : AcceptsPress::WHEN_ACTIVATED;
}

NSPoint PageInfoBubbleDecoration::GetBubblePointInFrame(NSRect frame) {
  NSRect image_rect = GetImageRectInFrame(frame);
  return NSMakePoint(NSMidX(image_rect),
                     NSMaxY(image_rect) - kPageInfoBubblePointYOffset);
}

NSString* PageInfoBubbleDecoration::GetToolTip() {
  return l10n_util::GetNSStringWithFixup(IDS_TOOLTIP_LOCATION_ICON);
}

NSString* PageInfoBubbleDecoration::GetAccessibilityLabel() {
  NSString* tooltip_icon_text =
      l10n_util::GetNSStringWithFixup(IDS_TOOLTIP_LOCATION_ICON);

  NSString* full_label = full_label_.get();
  security_state::SecurityLevel security_level =
      owner_->GetToolbarModel()->GetSecurityLevel(false);
  if ([full_label length] == 0 &&
      (security_level == security_state::EV_SECURE ||
       security_level == security_state::SECURE)) {
    full_label = l10n_util::GetNSStringWithFixup(IDS_SECURE_VERBOSE_STATE);
  }

  if ([full_label length] == 0)
    return tooltip_icon_text;
  return [NSString stringWithFormat:@"%@. %@", full_label, tooltip_icon_text];
}

NSRect PageInfoBubbleDecoration::GetRealFocusRingBounds(NSRect bounds) const {
  bounds.size.width -= (focus_ring_right_inset_ + focus_ring_left_inset_);
  bounds.origin.x += focus_ring_left_inset_;
  return bounds;
}

//////////////////////////////////////////////////////////////////
// PageInfoBubbleDecoration::BubbleDecoration:

NSColor* PageInfoBubbleDecoration::GetBackgroundBorderColor() {
  return skia::SkColorToSRGBNSColor(
      SkColorSetA(label_color_, 255.0 * GetAnimationProgress()));
}

NSColor* PageInfoBubbleDecoration::GetDarkModeTextColor() {
  return [NSColor whiteColor];
}

//////////////////////////////////////////////////////////////////
// PageInfoBubbleDecoration::AnimationDelegate:

void PageInfoBubbleDecoration::AnimationProgressed(
    const gfx::Animation* animation) {
  owner_->Layout();
}

//////////////////////////////////////////////////////////////////
// PageInfoBubbleDecoration, private:

CGFloat PageInfoBubbleDecoration::GetAnimationProgress() const {
  if (disable_animations_during_testing_)
    return 1.0;

  return animation_.GetCurrentValue();
}

CGFloat PageInfoBubbleDecoration::GetWidthForText(CGFloat width) {
  // Limit with to not take up too much of the available width, but
  // also don't let it shrink too much.
  width = std::max(width * kMaxBubbleFraction, kMinElidedBubbleWidth);

  // Use the full label if it fits.
  NSImage* image = GetImage();
  const CGFloat all_width = GetWidthForImageAndLabel(image, full_label_);
  if (all_width <= width) {
    SetLabel(full_label_);
    return all_width;
  }

  // Width left for laying out the label.
  const CGFloat width_left = width - GetWidthForImageAndLabel(image, @"");

  // Middle-elide the label to fit |width_left|.  This leaves the
  // prefix and the trailing country code in place.
  NSString* elided_label = base::SysUTF16ToNSString(
      gfx::ElideText(base::SysNSStringToUTF16(full_label_),
                     gfx::FontList(gfx::Font(GetFont())), width_left,
                     gfx::ELIDE_MIDDLE, gfx::Typesetter::BROWSER));

  // Use the elided label.
  SetLabel(elided_label);
  return GetWidthForImageAndLabel(image, elided_label);
}
