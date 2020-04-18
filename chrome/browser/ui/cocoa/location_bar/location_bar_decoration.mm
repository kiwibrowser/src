// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/location_bar/location_bar_decoration.h"

#include "base/logging.h"
#include "base/mac/scoped_nsobject.h"
#include "chrome/browser/ui/cocoa/l10n_util.h"
#include "chrome/browser/ui/cocoa/omnibox/omnibox_view_mac.h"
#include "skia/ext/skia_utils_mac.h"
#import "ui/base/cocoa/nsview_additions.h"
#import "ui/base/cocoa/tracking_area.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/font.h"
#include "ui/gfx/image/image_skia_util_mac.h"
#include "ui/gfx/paint_vector_icon.h"

namespace {

// Layout and color values for the vertical divider.
const CGFloat kDividerHeight = 16.0;
const CGFloat kDividerPadding = 6.0;  // Between the divider and label.
const CGFloat kDividerGray = 0xFFA6A6A6;
const CGFloat kDividerGrayIncognito = 0xFFCCCCCC;

// Color values for the hover and pressed background.
const SkColor kHoverBackgroundColor = 0x14000000;
const SkColor kHoverDarkBackgroundColor = 0x1EFFFFFF;
const SkColor kPressedBackgroundColor = 0x1E000000;
const SkColor kPressedDarkBackgroundColor = 0x3DFFFFFF;

// Amount of inset for the background frame.
const CGFloat kBackgroundFrameYInset = 2.0;

}  // namespace

// A DecorationAccessibilityView is a focusable, pressable button that is fully
// transparent and cannot be hit by mouse clicks. This is overlaid over a drawn
// decoration, to fake keyboard focus on the decoration and make it visible to
// VoiceOver.
@interface DecorationAccessibilityView : NSButton {
  LocationBarDecoration* owner_;  // weak
  NSRect apparentFrame_;
}

// NSView:
- (id)initWithOwner:(LocationBarDecoration*)owner;
- (BOOL)acceptsFirstResponder;
- (void)drawRect:(NSRect)dirtyRect;
- (NSView*)hitTest:(NSPoint)aPoint;
- (NSString*)accessibilityLabel;

// This method is called when this DecorationAccessibilityView is activated.
- (void)actionDidHappen;

- (void)setApparentFrame:(NSRect)r;
@end

@implementation DecorationAccessibilityView

- (id)initWithOwner:(LocationBarDecoration*)owner {
  if ((self = [super initWithFrame:NSZeroRect])) {
    self.bordered = NO;
    self.focusRingType = NSFocusRingTypeExterior;
    self->owner_ = owner;
    [self setAction:@selector(actionDidHappen)];
    [self setTarget:self];
    self->apparentFrame_ = NSZeroRect;
  }
  return self;
}

- (BOOL)acceptsFirstResponder {
  // This NSView is only focusable if the owning LocationBarDecoration can
  // accept mouse presses.
  return owner_->AcceptsMousePress() == AcceptsPress::NEVER ? NO : YES;
}

- (void)drawRect:(NSRect)dirtyRect {
  // Draw nothing. This NSView is fully transparent.
}

- (NSView*)hitTest:(NSPoint)aPoint {
  // Mouse clicks cannot hit this NSView.
  return nil;
}

- (void)actionDidHappen {
  owner_->OnAccessibilityViewAction();
}

- (NSString*)accessibilityLabel {
  NSString* label = owner_->GetAccessibilityLabel();
  return label ? label : owner_->GetToolTip();
}

- (void)setApparentFrame:(NSRect)r {
  apparentFrame_ = r;
}

// The focus ring (and all other visuals) should be positioned using the
// apparent frame, not the real frame, because of the hack in
// LocationBarViewMac::UpdateAccessibilityView().
- (void)drawFocusRingMask {
  NSRectFill([self focusRingMaskBounds]);
}

- (NSRect)focusRingMaskBounds {
  return owner_->GetRealFocusRingBounds(
      [[self superview] convertRect:apparentFrame_ toView:self]);
}

@end

@interface DecorationMouseTrackingDelegate : NSObject {
  LocationBarDecoration* owner_;  // weak
}

- (id)initWithOwner:(LocationBarDecoration*)owner;
- (void)mouseEntered:(NSEvent*)event;
- (void)mouseExited:(NSEvent*)event;

@end

@implementation DecorationMouseTrackingDelegate

- (id)initWithOwner:(LocationBarDecoration*)owner {
  if ((self = [super init])) {
    owner_ = owner;
  }

  return self;
}

- (void)mouseEntered:(NSEvent*)event {
  owner_->OnMouseEntered();
}

- (void)mouseExited:(NSEvent*)event {
  owner_->OnMouseExited();
}

@end

const CGFloat LocationBarDecoration::kOmittedWidth = 0.0;
const SkColor LocationBarDecoration::kMaterialDarkModeTextColor = SK_ColorWHITE;

LocationBarDecoration::LocationBarDecoration() {
  accessibility_view_.reset(
      [[DecorationAccessibilityView alloc] initWithOwner:this]);
  [accessibility_view_.get() setHidden:YES];
}

void LocationBarDecoration::OnAccessibilityViewAction() {
  DCHECK(!IsAccessibilityIgnored());
  // Turn the action into a synthesized mouse click at the center of |this|.
  NSRect frame = [accessibility_view_.get() frame];
  NSPoint mousePoint = NSMakePoint(NSMidX(frame), NSMidY(frame));
  HandleMousePressed(frame, mousePoint);
}

LocationBarDecoration::~LocationBarDecoration() {
  [accessibility_view_.get() removeFromSuperview];
}

CGFloat LocationBarDecoration::DividerPadding() const {
  return kDividerPadding;
}

bool LocationBarDecoration::IsVisible() const {
  return visible_;
}

void LocationBarDecoration::SetVisible(bool visible) {
  visible_ = visible;
  bool a11y_hidden = !visible || IsAccessibilityIgnored();
  [accessibility_view_.get() setHidden:a11y_hidden];
}


CGFloat LocationBarDecoration::GetWidthForSpace(CGFloat width) {
  NOTREACHED();
  return kOmittedWidth;
}

NSRect LocationBarDecoration::GetBackgroundFrame(NSRect frame) {
  return NSInsetRect(frame, 0.0, kBackgroundFrameYInset);
}

void LocationBarDecoration::UpdateAccessibilityView(NSRect apparent_frame) {
  auto v = static_cast<DecorationAccessibilityView*>(accessibility_view_);
  [accessibility_view_ setEnabled:AcceptsMousePress() != AcceptsPress::NEVER];
  [v setApparentFrame:apparent_frame];
}

NSRect LocationBarDecoration::GetRealFocusRingBounds(NSRect bounds) const {
  return bounds;
}

void LocationBarDecoration::DrawInFrame(NSRect frame, NSView* control_view) {
  NOTREACHED();
}

void LocationBarDecoration::DrawWithBackgroundInFrame(NSRect frame,
                                                      NSView* control_view) {
  // Draw the background if available.
  if ((active_ || state_ != DecorationMouseState::NONE) &&
      HasHoverAndPressEffect()) {
    bool in_dark_mode = [[control_view window] inIncognitoModeWithSystemTheme];

    SkColor background_color;
    if (active_ || state_ == DecorationMouseState::PRESSED) {
      background_color =
          in_dark_mode ? kPressedDarkBackgroundColor : kPressedBackgroundColor;
    } else {
      background_color =
          in_dark_mode ? kHoverDarkBackgroundColor : kHoverBackgroundColor;
    }

    [skia::SkColorToSRGBNSColor(background_color) setFill];

    NSBezierPath* path = [NSBezierPath bezierPath];
    [path appendBezierPathWithRoundedRect:GetBackgroundFrame(frame)
                                  xRadius:2
                                  yRadius:2];
    [path fill];
  }
  DrawInFrame(frame, control_view);
}

NSString* LocationBarDecoration::GetToolTip() {
  return nil;
}

NSString* LocationBarDecoration::GetAccessibilityLabel() {
  return nil;
}

bool LocationBarDecoration::IsAccessibilityIgnored() {
  return false;
}

NSRect LocationBarDecoration::GetTrackingFrame(NSRect frame) {
  return frame;
}

CrTrackingArea* LocationBarDecoration::SetupTrackingArea(NSRect frame,
                                                         NSView* control_view) {
  if (!HasHoverAndPressEffect() || !control_view)
    return nil;

  NSRect tracking_frame = GetTrackingFrame(frame);
  if (control_view == tracking_area_owner_ && tracking_area_.get() &&
      NSEqualRects([tracking_area_ rect], tracking_frame)) {
    return tracking_area_.get();
  }

  if (tracking_area_owner_)
    [tracking_area_owner_ removeTrackingArea:tracking_area_.get()];

  if (!tracking_delegate_) {
    tracking_delegate_.reset(
        [[DecorationMouseTrackingDelegate alloc] initWithOwner:this]);
  }

  tracking_area_.reset([[CrTrackingArea alloc]
      initWithRect:tracking_frame
           options:NSTrackingMouseEnteredAndExited | NSTrackingActiveInKeyWindow
             owner:tracking_delegate_.get()
          userInfo:nil]);

  [control_view addTrackingArea:tracking_area_.get()];
  tracking_area_owner_ = control_view;

  state_ = [tracking_area_ mouseInsideTrackingAreaForView:control_view]
               ? DecorationMouseState::HOVER
               : DecorationMouseState::NONE;

  return tracking_area_.get();
}

void LocationBarDecoration::RemoveTrackingArea() {
  DCHECK(tracking_area_owner_);
  DCHECK(tracking_area_);
  [tracking_area_owner_ removeTrackingArea:tracking_area_.get()];
  tracking_area_owner_ = nullptr;
  tracking_area_.reset();
}

AcceptsPress LocationBarDecoration::AcceptsMousePress() {
  return AcceptsPress::NEVER;
}

bool LocationBarDecoration::HasHoverAndPressEffect() {
  return AcceptsMousePress() != AcceptsPress::NEVER;
}

bool LocationBarDecoration::IsDraggable() {
  return false;
}

NSImage* LocationBarDecoration::GetDragImage() {
  return nil;
}

NSRect LocationBarDecoration::GetDragImageFrame(NSRect frame) {
  return NSZeroRect;
}

NSPasteboard* LocationBarDecoration::GetDragPasteboard() {
  return nil;
}

bool LocationBarDecoration::HandleMousePressed(NSRect frame, NSPoint location) {
  if (was_active_in_last_mouse_down_) {
    was_active_in_last_mouse_down_ = false;
    if (AcceptsMousePress() == AcceptsPress::WHEN_ACTIVATED) {
      // Clear the active state. This is usually the responsibility of the popup
      // code (i.e. whatever called SetActive(true)). However, there exist some
      // tricky lifetimes and corner cases where a client may "forget", or be
      // unable to clear the active state. In that case, we risk getting "stuck"
      // in a state where client code may never receive a signal. Clearing the
      // active state here ensures a subsequent click will always send a signal.
      SetActive(false);
      return true;
    }
  }

  return OnMousePressed(frame, location);
}

bool LocationBarDecoration::OnMousePressed(NSRect frame, NSPoint location) {
  return false;
}

void LocationBarDecoration::OnMouseDown() {
  was_active_in_last_mouse_down_ = active_;
  state_ = DecorationMouseState::PRESSED;
  UpdateDecorationState();
}

void LocationBarDecoration::OnMouseUp() {
  // Ignore the mouse up if it's not associated with a mouse down event.
  if (state_ != DecorationMouseState::PRESSED)
    return;

  DCHECK(tracking_area_owner_);
  state_ = [tracking_area_ mouseInsideTrackingAreaForView:tracking_area_owner_]
               ? DecorationMouseState::HOVER
               : DecorationMouseState::NONE;
  UpdateDecorationState();
}

void LocationBarDecoration::OnMouseEntered() {
  state_ = DecorationMouseState::HOVER;
  UpdateDecorationState();
}

void LocationBarDecoration::OnMouseExited() {
  state_ = DecorationMouseState::NONE;
  UpdateDecorationState();
}

void LocationBarDecoration::SetActive(bool active) {
  if (active_ == active)
    return;

  active_ = active;
  state_ = [tracking_area_ mouseInsideTrackingAreaForView:tracking_area_owner_]
               ? DecorationMouseState::HOVER
               : DecorationMouseState::NONE;
  UpdateDecorationState();
}

void LocationBarDecoration::UpdateDecorationState() {
  if (tracking_area_owner_)
    [tracking_area_owner_ setNeedsDisplay:YES];
}

NSMenu* LocationBarDecoration::GetMenu() {
  return nil;
}

NSFont* LocationBarDecoration::GetFont() const {
  return OmniboxViewMac::GetNormalFieldFont();
}

NSView* LocationBarDecoration::GetAccessibilityView() {
  return accessibility_view_.get();
}

NSPoint LocationBarDecoration::GetBubblePointInFrame(NSRect frame) {
  // Clients that use a bubble should implement this. Can't be abstract
  // because too many LocationBarDecoration subclasses don't use a bubble.
  // Can't live on subclasses only because it needs to be on a shared API.
  NOTREACHED();
  return frame.origin;
}

NSImage* LocationBarDecoration::GetMaterialIcon(
    bool location_bar_is_dark) const {
  const int kIconSize = 16;
  const gfx::VectorIcon* vector_icon = GetMaterialVectorIcon();
  if (!vector_icon) {
    // Return an empty image when the decoration specifies no vector icon, so
    // that its bubble is positioned correctly (the position is based on the
    // width of the image; returning nil will mess up the positioning).
    NSSize icon_size = NSMakeSize(kIconSize, kIconSize);
    return [[[NSImage alloc] initWithSize:icon_size] autorelease];
  }

  SkColor vector_icon_color = GetMaterialIconColor(location_bar_is_dark);
  return NSImageFromImageSkia(
      gfx::CreateVectorIcon(*vector_icon, kIconSize, vector_icon_color));
}

// static
void LocationBarDecoration::DrawLabel(NSString* label,
                                      NSDictionary* attributes,
                                      const NSRect& frame) {
  base::scoped_nsobject<NSAttributedString> str(
      [[NSAttributedString alloc] initWithString:label attributes:attributes]);
  DrawAttributedString(str, frame);
}

// static
void LocationBarDecoration::DrawAttributedString(NSAttributedString* str,
                                                 const NSRect& frame) {
  NSRect text_rect = frame;
  text_rect.size.height = [str size].height;
  text_rect.origin.y = roundf(NSMidY(frame) - NSHeight(text_rect) / 2.0) - 1;
  [str drawInRect:text_rect];
}

// static
NSSize LocationBarDecoration::GetLabelSize(NSString* label,
                                           NSDictionary* attributes) {
  return [label sizeWithAttributes:attributes];
}

SkColor LocationBarDecoration::GetMaterialIconColor(
    bool location_bar_is_dark) const {
  return location_bar_is_dark ? SK_ColorWHITE : gfx::kChromeIconGrey;
}

void LocationBarDecoration::DrawDivider(NSView* control_view,
                                        NSRect decoration_frame,
                                        CGFloat alpha) const {
  if (alpha == 0.0) {
    return;
  }

  NSBezierPath* line = [NSBezierPath bezierPath];

  CGFloat line_width = [control_view cr_lineWidth];
  [line setLineWidth:line_width];

  const BOOL is_rtl = cocoa_l10n_util::ShouldDoExperimentalRTLLayout();
  NSPoint divider_origin = NSZeroPoint;
  divider_origin.x = is_rtl ? NSMinX(decoration_frame) + DividerPadding()
                            : NSMaxX(decoration_frame) - DividerPadding();
  // Screen pixels lay between integral coordinates in user space. If you
  // draw a line from (16, 16) to (16, 32), Core Graphics maps that line to
  // the pixels that lay along x=16.5. In order to achieve a line that
  // appears to lay along x=16, CG will perform dithering. To get a crisp
  // line, you have to specify x=16.5, so translating by 1/2 the 1-pixel
  // line width creates the crisp line we want. We subtract to better center
  // the line between the label and URL.
  divider_origin.x -= line_width / 2.;
  CGFloat divider_y_frame_offset =
      (NSHeight(decoration_frame) - kDividerHeight) / 2.0;
  divider_origin.y = NSMinY(decoration_frame) + divider_y_frame_offset;
  // Adjust the divider origin by 1/2 a point to visually center the
  // divider vertically on Retina.
  if (line_width < 1) {
    divider_origin.y -= 0.5;
  }
  [line moveToPoint:divider_origin];
  [line relativeLineToPoint:NSMakePoint(0, kDividerHeight)];

  NSColor* divider_color = nil;
  bool location_bar_is_dark =
      [[control_view window] inIncognitoModeWithSystemTheme];
  if (location_bar_is_dark) {
    divider_color = skia::SkColorToSRGBNSColor(kDividerGrayIncognito);
  } else {
    divider_color = skia::SkColorToSRGBNSColor(kDividerGray);
  }

  if (alpha < 1) {
    divider_color = [divider_color colorWithAlphaComponent:alpha];
  }
  [divider_color set];
  [line stroke];
}

const gfx::VectorIcon* LocationBarDecoration::GetMaterialVectorIcon() const {
  NOTREACHED();
  return nullptr;
}
