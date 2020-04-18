// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/location_bar/autocomplete_text_field.h"

#include "base/logging.h"
#import "base/mac/mac_util.h"
#import "base/mac/sdk_forward_declarations.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/ui/cocoa/browser_dialogs_views_mac.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#include "chrome/browser/ui/cocoa/l10n_util.h"
#import "chrome/browser/ui/cocoa/location_bar/autocomplete_text_field_cell.h"
#import "chrome/browser/ui/cocoa/location_bar/autocomplete_text_field_editor.h"
#import "chrome/browser/ui/cocoa/location_bar/location_bar_decoration.h"
#include "chrome/browser/ui/cocoa/omnibox/omnibox_view_mac.h"
#import "chrome/browser/ui/cocoa/toolbar/toolbar_controller.h"
#import "chrome/browser/ui/cocoa/url_drop_target.h"
#import "chrome/browser/ui/cocoa/view_id_util.h"
#import "ui/base/cocoa/nsview_additions.h"
#include "ui/gfx/scoped_ns_graphics_context_save_gstate_mac.h"

namespace {
const CGFloat kAnimationDuration = 0.2;

}

@implementation AutocompleteTextField

@synthesize observer = observer_;

+ (Class)cellClass {
  return [AutocompleteTextFieldCell class];
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [super dealloc];
}

- (void)awakeFromNib {
  DCHECK([[self cell] isKindOfClass:[AutocompleteTextFieldCell class]]);
  [[self cell] setTruncatesLastVisibleLine:YES];
  [[self cell] setLineBreakMode:NSLineBreakByTruncatingTail];
  currentToolTips_.reset([[NSMutableArray alloc] init]);
  resizeAnimation_.reset([[NSViewAnimation alloc] init]);
  [resizeAnimation_ setDuration:kAnimationDuration];
  [resizeAnimation_ setAnimationBlockingMode:NSAnimationNonblocking];
  [self setAlignment:cocoa_l10n_util::ShouldDoExperimentalRTLLayout()
                         ? NSRightTextAlignment
                         : NSLeftTextAlignment];

  // Disable Force Touch in the Omnibox. Note that this API is documented as
  // being available in 10.11 or higher, but if the API is available in an older
  // version we still want to use it. That prevents us from guarding the call
  // with @available, so instead we use respondsToSelector and silence the
  // availability warning. Also, because NSPressureConfiguration is not in the
  // original 10.10 SDK, use NSClassFromString() to instantiate it (otherwise
  // there's a linker error).
  if (base::mac::IsAtLeastOS10_10() &&
      [self respondsToSelector:@selector(setPressureConfiguration:)]) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunguarded-availability"
    NSPressureConfiguration* pressureConfiguration =
        [[[NSClassFromString(@"NSPressureConfiguration") alloc]
            initWithPressureBehavior:NSPressureBehaviorPrimaryClick]
                autorelease];
    [self setPressureConfiguration:pressureConfiguration];
#pragma clang diagnostic pop
  }
}

- (void)flagsChanged:(NSEvent*)theEvent {
  if (observer_) {
    const bool controlFlag = ([theEvent modifierFlags]&NSControlKeyMask) != 0;
    observer_->OnControlKeyChanged(controlFlag);
  }
}

- (AutocompleteTextFieldCell*)cell {
  NSCell* cell = [super cell];
  if (!cell)
    return nil;

  DCHECK([cell isKindOfClass:[AutocompleteTextFieldCell class]]);
  return static_cast<AutocompleteTextFieldCell*>(cell);
}

// Reroute events for the decoration area to the field editor.  This
// will cause the cursor to be moved as close to the edge where the
// event was seen as possible.
//
// The reason for this code's existence is subtle.  NSTextField
// implements text selection and editing in terms of a "field editor".
// This is an NSTextView which is installed as a subview of the
// control when the field becomes first responder.  When the field
// editor is installed, it will get -mouseDown: events and handle
// them, rather than the text field - EXCEPT for the event which
// caused the change in first responder, or events which fall in the
// decorations outside the field editor's area.  In that case, the
// default NSTextField code will setup the field editor all over
// again, which has the side effect of doing "select all" on the text.
// This effect can be observed with a normal NSTextField if you click
// in the narrow border area, and is only really a problem because in
// our case the focus ring surrounds decorations which look clickable.
//
// When the user first clicks on the field, after installing the field
// editor the default NSTextField code detects if the hit is in the
// field editor area, and if so sets the selection to {0,0} to clear
// the selection before forwarding the event to the field editor for
// processing (it will set the cursor position).  This also starts the
// click-drag selection machinery.
//
// This code does the same thing for cases where the click was in the
// decoration area.  This allows the user to click-drag starting from
// a decoration area and get the expected selection behaviour,
// likewise for multiple clicks in those areas.
- (void)mouseDown:(NSEvent*)theEvent {
  // TODO(groby): Figure out if OnMouseDown needs to be postponed/skipped
  // for button decorations.
  if (observer_)
    observer_->OnMouseDown([theEvent buttonNumber]);

  // If the click was a Control-click, bring up the context menu.
  // |NSTextField| handles these cases inconsistently if the field is
  // not already first responder.
  if (([theEvent modifierFlags] & NSControlKeyMask) != 0) {
    NSText* editor = [self currentEditor];
    NSMenu* menu = [editor menuForEvent:theEvent];
    [NSMenu popUpContextMenu:menu withEvent:theEvent forView:editor];
    return;
  }

  const NSPoint location =
      [self convertPoint:[theEvent locationInWindow] fromView:nil];
  const NSRect bounds([self bounds]);

  AutocompleteTextFieldCell* cell = [self cell];
  const NSRect textFrame([cell textFrameForFrame:bounds]);

  // A version of the textFrame which extends across the field's
  // entire width.

  const NSRect fullFrame(NSMakeRect(bounds.origin.x, textFrame.origin.y,
                                    bounds.size.width, textFrame.size.height));

  // If the mouse is in the editing area, or above or below where the
  // editing area would be if we didn't add decorations, forward to
  // NSTextField -mouseDown: because it does the right thing.  The
  // above/below test is needed because NSTextView treats mouse events
  // above/below as select-to-end-in-that-direction, which makes
  // things janky.
  BOOL flipped = [self isFlipped];
  if (NSMouseInRect(location, textFrame, flipped) ||
      !NSMouseInRect(location, fullFrame, flipped)) {
    [super mouseDown:theEvent];

    // After the event has been handled, if the current event is a
    // mouse up and no selection was created (the mouse didn't move),
    // select the entire field.
    // NOTE(shess): This does not interfere with single-clicking to
    // place caret after a selection is made.  An NSTextField only has
    // a selection when it has a field editor.  The field editor is an
    // NSText subview, which will receive the -mouseDown: in that
    // case, and this code will never fire.
    NSText* editor = [self currentEditor];
    if (editor) {
      NSEvent* currentEvent = [NSApp currentEvent];
      if ([currentEvent type] == NSLeftMouseUp &&
          ![editor selectedRange].length) {
        [editor selectAll:nil];
      }
    }

    return;
  }

  // Give the cell a chance to intercept clicks in page-actions and
  // other decorative items.
  if ([cell mouseDown:theEvent inRect:bounds ofView:self]) {
    return;
  }

  NSText* editor = [self currentEditor];

  // We should only be here if we accepted first-responder status and
  // have a field editor.  If one of these fires, it means some
  // assumptions are being broken.
  DCHECK(editor != nil);
  DCHECK([editor isDescendantOf:self]);

  // -becomeFirstResponder does a select-all, which we don't want
  // because it can lead to a dragged-text situation.  Clear the
  // selection (any valid empty selection will do).
  [editor setSelectedRange:NSMakeRange(0, 0)];

  // If the event is to the right of the editing area, scroll the
  // field editor to the end of the content so that the selection
  // doesn't initiate from somewhere in the middle of the text.
  if (location.x > NSMaxX(textFrame)) {
    [editor scrollRangeToVisible:NSMakeRange([[self stringValue] length], 0)];
  }

  [editor mouseDown:theEvent];
}

- (void)mouseUp:(NSEvent*)theEvent {
  const NSRect bounds([self bounds]);
  AutocompleteTextFieldCell* cell = [self cell];
  [cell mouseUp:theEvent inRect:bounds ofView:self];
}

- (void)rightMouseDown:(NSEvent*)event {
  if (observer_)
    observer_->OnMouseDown([event buttonNumber]);
  [super rightMouseDown:event];
}

- (void)otherMouseDown:(NSEvent *)event {
  if (observer_)
    observer_->OnMouseDown([event buttonNumber]);
  [super otherMouseDown:event];
}

// Overridden so that cursor and tooltip rects can be updated.
- (void)setFrame:(NSRect)frameRect {
  [super setFrame:frameRect];
  if (observer_) {
    observer_->OnFrameChanged();
  }
  [self updateMouseTracking];
}

- (void)setAttributedStringValue:(NSAttributedString*)aString {
  AutocompleteTextFieldEditor* editor =
      static_cast<AutocompleteTextFieldEditor*>([self currentEditor]);

  if (!editor) {
    [super setAttributedStringValue:aString];
  } else {
    // The type of the field editor must be AutocompleteTextFieldEditor,
    // otherwise things won't work.
    DCHECK([editor isKindOfClass:[AutocompleteTextFieldEditor class]]);

    [editor setAttributedString:aString];
  }
}

- (NSUndoManager*)undoManagerForTextView:(NSTextView*)textView {
  if (!undoManager_.get())
    undoManager_.reset([[NSUndoManager alloc] init]);
  return undoManager_.get();
}

- (void)animateToFrame:(NSRect)frame {
  [self stopAnimation];
  NSDictionary* animationDictionary = @{
    NSViewAnimationTargetKey : self,
    NSViewAnimationStartFrameKey : [NSValue valueWithRect:[self frame]],
    NSViewAnimationEndFrameKey : [NSValue valueWithRect:frame]
  };
  [resizeAnimation_ setViewAnimations:@[ animationDictionary ]];
  [resizeAnimation_ startAnimation];
}

- (void)stopAnimation {
  if ([resizeAnimation_ isAnimating]) {
    // [NSViewAnimation stopAnimation] results in advancing the animation to
    // the end. Since this is almost certainly not the behavior we want, reset
    // the frame to the current frame.
    NSRect frame = [self frame];
    [resizeAnimation_ stopAnimation];
    [self setFrame:frame];
  }
}

- (void)clearUndoChain {
  [undoManager_ removeAllActions];
}

- (NSRange)textView:(NSTextView *)aTextView
    willChangeSelectionFromCharacterRange:(NSRange)oldRange
    toCharacterRange:(NSRange)newRange {
  if (observer_)
    return observer_->SelectionRangeForProposedRange(newRange);
  return newRange;
}

- (void)addToolTip:(NSString*)tooltip forRect:(NSRect)aRect {
  [currentToolTips_ addObject:tooltip];
  [self addToolTipRect:aRect owner:tooltip userData:nil];
}

- (NSPoint)bubblePointForDecoration:(LocationBarDecoration*)decoration {
  // Use MD-style anchoring, even if only pilot dialogs are enabled. MD dialogs
  // have no arrow and align corners. Cocoa dialogs will always have an arrow.
  // This causes the arrows on Cocoa dialogs to align to the omnibox corner.
  if (!chrome::ShowPilotDialogsWithViewsToolkit()) {
    const NSRect frame =
        [[self cell] frameForDecoration:decoration inFrame:[self bounds]];
    NSPoint point = decoration->GetBubblePointInFrame(frame);
    return [self convertPoint:point toView:nil];
  }

  // Under MD, dialogs have no arrow and anchor to corner of the location bar
  // frame, not a specific point within it. See http://crbug.com/566115.

  // Inset the omnibox frame by 2 real pixels. This is done because the border
  // stroke of the omnibox is inside its frame, but bubbles have no border
  // stroke. The bubble border is part of the shadow drawn by the window server;
  // outside the bubble frame. In the Y direction, some of that same "gap" must
  // be kept, otherwise the "border" stroke from the window server shadow would
  // be drawn inside the omnibox. But since these insets round to integers when
  // positioning the window, retina needs to use a zero vertical offset to avoid
  // insetting an entire DIP. This looks OK, since the bubble border and omnibox
  // border are still drawn flush. TODO(tapted): Convince the borders to overlap
  // on retina somehow.
  const CGFloat kStrokeInsetX = 2 * [self cr_lineWidth];
  const CGFloat kStrokeInsetY = [self cr_lineWidth] == 1.0 ? 1 : 0;
  const NSRect frame = NSInsetRect([self bounds], kStrokeInsetX, kStrokeInsetY);

  BOOL isLeftDecoration = [[self cell] isLeftDecoration:decoration];
  NSPoint point = NSMakePoint(isLeftDecoration ? NSMinX(frame) : NSMaxX(frame),
                              NSMaxY(frame));
  return [self convertPoint:point toView:nil];
}

// TODO(shess): -resetFieldEditorFrameIfNeeded is the place where
// changes to the cell layout should be flushed.  LocationBarViewMac
// and ToolbarController are calling this routine directly, and I
// think they are probably wrong.
// http://crbug.com/40053
- (void)updateMouseTracking {
  // This will force |resetCursorRects| to be called, as it is not to be called
  // directly.
  [[self window] invalidateCursorRectsForView:self];

  // |removeAllToolTips| only removes those set on the current NSView, not any
  // subviews. Unless more tooltips are added to this view, this should suffice
  // in place of managing a set of NSToolTipTag objects.
  [self removeAllToolTips];

  // Reload the decoration tooltips.
  [currentToolTips_ removeAllObjects];
  [[self cell] updateMouseTrackingAndToolTipsInRect:[self bounds] ofView:self];
}

// NOTE(shess): http://crbug.com/19116 describes a weird bug which
// happens when the user runs a Print panel on Leopard.  After that,
// spurious -controlTextDidBeginEditing notifications are sent when an
// NSTextField is firstResponder, even though -currentEditor on that
// field returns nil.  That notification caused significant problems
// in OmniboxViewMac.  -textDidBeginEditing: was NOT being
// sent in those cases, so this approach doesn't have the problem.
- (void)textDidBeginEditing:(NSNotification*)aNotification {
  [super textDidBeginEditing:aNotification];
  if (observer_) {
    observer_->OnDidBeginEditing();
  }
}

- (void)textDidEndEditing:(NSNotification *)aNotification {
  [super textDidEndEditing:aNotification];
  if (observer_) {
    observer_->OnDidEndEditing();
  }
}

// When the window resigns, make sure the autocomplete popup is no
// longer visible, since the user's focus is elsewhere.
- (void)windowDidResignKey:(NSNotification*)notification {
  DCHECK_EQ([self window], [notification object]);
  if (observer_)
    observer_->ClosePopup();
}

- (void)windowDidResize:(NSNotification*)notification {
  DCHECK_EQ([self window], [notification object]);
  if (observer_)
    observer_->OnFrameChanged();
}

- (void)viewWillMoveToWindow:(NSWindow*)newWindow {
  if ([self window]) {
    NSNotificationCenter* nc = [NSNotificationCenter defaultCenter];
    [nc removeObserver:self
                  name:NSWindowDidResignKeyNotification
                object:[self window]];
    [nc removeObserver:self
                  name:NSWindowDidResizeNotification
                object:[self window]];
    [nc removeObserver:self
                  name:NSWindowDidChangeScreenNotification
                object:[self window]];
  }
}

- (void)windowDidChangeScreen {
  // Inform the AutocompleteTextFieldCell's of the coordinate system line
  // width needed to draw a single-pixel line. This value changes as we move
  // between Retina and non-Retina displays.
  [[self cell] setSinglePixelLineWidth:[self cr_lineWidth]];
  [self setNeedsDisplay];
}

- (void)updateColorsToMatchTheme {
  if (![[self window] inIncognitoMode]) {
    return;
  }

  // Invert the textfield's colors when Material Design and Incognito and not
  // a custom theme.
  bool inDarkMode = [[self window] inIncognitoModeWithSystemTheme];
  const CGFloat kDarkModeGray = 97 / 255.;
  [self setBackgroundColor:
      inDarkMode ? [NSColor colorWithGenericGamma22White:kDarkModeGray
                                                   alpha:1]
                 : [NSColor whiteColor]];
  [self setTextColor:OmniboxViewMac::BaseTextColor(inDarkMode)];
}

- (void)viewDidMoveToWindow {
  if (![self window]) {
    return;
  }

  // Allow the ToolbarController to take action upon the
  // AutocompleteTextField being added to the window.
  BrowserWindowController* browserWindowController =
      [BrowserWindowController browserWindowControllerForView:self];
  [[browserWindowController toolbarController] locationBarWasAddedToWindow];

  [self updateColorsToMatchTheme];

  NSNotificationCenter* nc = [NSNotificationCenter defaultCenter];
  [nc addObserver:self
         selector:@selector(windowDidResignKey:)
             name:NSWindowDidResignKeyNotification
           object:[self window]];
  [nc addObserver:self
         selector:@selector(windowDidResize:)
             name:NSWindowDidResizeNotification
           object:[self window]];
  [nc addObserver:self
         selector:@selector(windowDidChangeScreen)
             name:NSWindowDidChangeScreenNotification
           object:[self window]];

  // Make sure the cell has the current line width.
  [[self cell] setSinglePixelLineWidth:[self cr_lineWidth]];

  // Only register for drops if not in a popup window. Lazily create the
  // drop handler when the type of window is known.
  BrowserWindowController* windowController =
      [BrowserWindowController browserWindowControllerForView:self];
  if ([windowController isTabbedWindow])
      dropHandler_.reset([[URLDropTargetHandler alloc] initWithView:self]);
}

// NSTextField becomes first responder by installing a "field editor"
// subview.  Clicks outside the field editor (such as a decoration)
// will attempt to make the field the first-responder again, which
// causes a select-all, even if the decoration handles the click.  If
// the field editor is already in place, don't accept first responder
// again.  This allows the selection to be unmodified if the click is
// handled by a decoration or context menu (|-mouseDown:| will still
// change it if appropriate).
- (BOOL)acceptsFirstResponder {
  if ([self currentEditor]) {
    DCHECK_EQ([self currentEditor], [[self window] firstResponder]);
    return NO;
  }

  // If the event is a left-mouse click, and it lands on a decoration, then the
  // event should not cause the text field to become first responder.
  NSEvent* event = [NSApp currentEvent];
  if ([event type] == NSLeftMouseDown) {
    LocationBarDecoration* decoration =
        [[self cell] decorationForEvent:event inRect:[self bounds] ofView:self];
    if (decoration && decoration->AcceptsMousePress() != AcceptsPress::NEVER)
      return NO;
  }

  return [super acceptsFirstResponder];
}

// (Overridden from NSResponder)
- (BOOL)becomeFirstResponder {
  BOOL doAccept = [super becomeFirstResponder];
  if (doAccept) {
    [[BrowserWindowController browserWindowControllerForView:self]
        lockToolbarVisibilityForOwner:self
                        withAnimation:YES];

    // Tells the observer that we get the focus.
    // But we can't call observer_->OnKillFocus() in resignFirstResponder:,
    // because the first responder will be immediately set to the field editor
    // when calling [super becomeFirstResponder], thus we won't receive
    // resignFirstResponder: anymore when losing focus.
    [[self cell] handleFocusEvent:[NSApp currentEvent] ofView:self];
  }
  return doAccept;
}

// (Overridden from NSResponder)
- (BOOL)resignFirstResponder {
  BOOL doResign = [super resignFirstResponder];
  if (doResign) {
    [[BrowserWindowController browserWindowControllerForView:self]
        releaseToolbarVisibilityForOwner:self
                           withAnimation:YES];
  }
  return doResign;
}

// (URLDropTarget protocol)
- (id<URLDropTargetController>)urlDropController {
  BrowserWindowController* windowController =
      [BrowserWindowController browserWindowControllerForView:self];
  return [windowController toolbarController];
}

// (URLDropTarget protocol)
- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender {
  bool canDropAtLocation =
      [[self cell] canDropAtLocationInWindow:[sender draggingLocation]
                                      ofView:self];
  return canDropAtLocation ? [dropHandler_ draggingEntered:sender]
                           : NSDragOperationNone;
}

// (URLDropTarget protocol)
- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender {
  bool canDropAtLocation =
      [[self cell] canDropAtLocationInWindow:[sender draggingLocation]
                                      ofView:self];
  return canDropAtLocation ? [dropHandler_ draggingUpdated:sender]
                           : NSDragOperationNone;
}

// (URLDropTarget protocol)
- (void)draggingExited:(id<NSDraggingInfo>)sender {
  return [dropHandler_ draggingExited:sender];
}

// (URLDropTarget protocol)
- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender {
  return [dropHandler_ performDragOperation:sender];
}

- (NSMenu*)decorationMenuForEvent:(NSEvent*)event {
  AutocompleteTextFieldCell* cell = [self cell];
  return [cell decorationMenuForEvent:event inRect:[self bounds] ofView:self];
}

- (ViewID)viewID {
  return VIEW_ID_OMNIBOX;
}

// ThemedWindowDrawing implementation.

- (void)windowDidChangeTheme {
  [self updateColorsToMatchTheme];
}

- (void)windowDidChangeActive {
}

@end
