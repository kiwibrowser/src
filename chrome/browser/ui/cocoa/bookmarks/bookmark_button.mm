// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/bookmarks/bookmark_button.h"

#include <cmath>

#include "base/logging.h"
#include "base/mac/foundation_util.h"
#import "base/mac/scoped_nsobject.h"
#include "base/mac/sdk_forward_declarations.h"
#include "base/metrics/user_metrics.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_controller.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_folder_window.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_view_cocoa.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_button_cell.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_folder_target.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/view_id_util.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "ui/base/clipboard/clipboard_util_mac.h"
#include "ui/base/cocoa/cocoa_base_utils.h"
#import "ui/base/cocoa/nsview_additions.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/gfx/scoped_ns_graphics_context_save_gstate_mac.h"

using base::UserMetricsAction;
using bookmarks::BookmarkNode;

// The opacity of the bookmark button drag image.
static const CGFloat kDragImageOpacity = 0.7;

namespace {
// We need a class variable to track the current dragged button to enable
// proper live animated dragging behavior, and can't do it in the
// delegate/controller since you can drag a button from one domain to the
// other (from a "folder" menu, to the main bar, or vice versa).
BookmarkButton* gDraggedButton = nil; // Weak
};

@interface BookmarkButton()

// NSDraggingSource:
- (void)draggingSession:(NSDraggingSession*)session
           endedAtPoint:(NSPoint)aPoint
              operation:(NSDragOperation)operation;
- (NSDragOperation)draggingSession:(NSDraggingSession*)session
    sourceOperationMaskForDraggingContext:(NSDraggingContext)context;

// Make a drag image for the button.
- (NSImage*)dragImage;

- (void)installCustomTrackingArea;

@end  // @interface BookmarkButton(Private)

@implementation BookmarkButton

@synthesize delegate = delegate_;
@synthesize acceptsTrackIn = acceptsTrackIn_;
@synthesize backgroundColor = backgroundColor_;

- (id)initWithFrame:(NSRect)frameRect {
  // BookmarkButton's ViewID may be changed to VIEW_ID_OTHER_BOOKMARKS in
  // BookmarkBarController, so we can't just override -viewID method to return
  // it.
  if ((self = [super initWithFrame:frameRect])) {
    view_id_util::SetID(self, VIEW_ID_BOOKMARK_BAR_ELEMENT);
    [self installCustomTrackingArea];
  }
  return self;
}

- (void)dealloc {
  if ([[self cell] respondsToSelector:@selector(safelyStopPulsing)])
    [[self cell] safelyStopPulsing];
  view_id_util::UnsetID(self);

  if (area_) {
    [self removeTrackingArea:area_];
    [area_ release];
  }

  [backgroundColor_ release];

  [super dealloc];
}

- (const BookmarkNode*)bookmarkNode {
  return [[self cell] bookmarkNode];
}

- (BOOL)isFolder {
  const BookmarkNode* node = [self bookmarkNode];
  return (node && node->is_folder());
}

- (BOOL)isEmpty {
  return [self bookmarkNode] ? NO : YES;
}

- (void)setPulseIsStuckOn:(BOOL)flag {
  [[self cell] setPulseIsStuckOn:flag];
}

- (BOOL)isPulseStuckOn {
  return [[self cell] isPulseStuckOn];
}

- (NSPoint)screenLocationForRemoveAnimation {
  NSPoint point;

  if (dragPending_) {
    // Use the position of the mouse in the drag image as the location.
    point = dragEndScreenLocation_;
    point.x += dragMouseOffset_.x;
    if ([self isFlipped]) {
      point.y += [self bounds].size.height - dragMouseOffset_.y;
    } else {
      point.y += dragMouseOffset_.y;
    }
  } else {
    // Use the middle of this button as the location.
    NSRect bounds = [self bounds];
    point = NSMakePoint(NSMidX(bounds), NSMidY(bounds));
    point = [self convertPoint:point toView:nil];
    point = ui::ConvertPointFromWindowToScreen([self window], point);
  }

  return point;
}


- (void)updateTrackingAreas {
  [self installCustomTrackingArea];
  [super updateTrackingAreas];
}

- (DraggableButtonResult)deltaIndicatesDragStartWithXDelta:(float)xDelta
                                                    yDelta:(float)yDelta
                                               xHysteresis:(float)xHysteresis
                                               yHysteresis:(float)yHysteresis
                                                 indicates:(BOOL*)result {
  const float kDownProportion = 1.4142135f; // Square root of 2.

  // We want to show a folder menu when you drag down on folder buttons,
  // so don't classify this as a drag for that case.
  if ([self isFolder] &&
      (yDelta <= -yHysteresis) &&  // Bottom of hysteresis box was hit.
      (std::abs(yDelta) / std::abs(xDelta)) >= kDownProportion) {
    *result = NO;
    return kDraggableButtonMixinDidWork;
  }

  return kDraggableButtonImplUseBase;
}


// By default, NSButton ignores middle-clicks.
// But we want them.
- (void)otherMouseUp:(NSEvent*)event {
  [self performClick:self];
}

- (BOOL)acceptsTrackInFrom:(id)sender {
  return  [self isFolder] || [self acceptsTrackIn];
}


// Overridden from DraggableButton.
- (void)beginDrag:(NSEvent*)event {
  // Don't allow a drag of the empty node.
  // The empty node is a placeholder for "(empty)", to be revisited.
  if ([self isEmpty])
    return;

  if (![self delegate]) {
    NOTREACHED();
    return;
  }

  if ([self isFolder]) {
    // Close the folder's drop-down menu if it's visible.
    [[self target] closeBookmarkFolder:self];
  }

  // At the moment, moving bookmarks causes their buttons (like me!)
  // to be destroyed and rebuilt.  Make sure we don't go away while on
  // the stack.
  [self retain];

  // Lock bar visibility, forcing the overlay to stay visible if we are in
  // fullscreen mode.
  if ([[self delegate] dragShouldLockBarVisibility]) {
    DCHECK(!visibilityDelegate_);
    NSWindow* window = [[self delegate] browserWindow];
    visibilityDelegate_ =
        [BrowserWindowController browserWindowControllerForWindow:window];
    [visibilityDelegate_ lockToolbarVisibilityForOwner:self withAnimation:NO];
  }
  const BookmarkNode* node = [self bookmarkNode];
  const BookmarkNode* parent = node->parent();
  if (parent && parent->type() == BookmarkNode::FOLDER) {
    base::RecordAction(UserMetricsAction("BookmarkBarFolder_DragStart"));
  } else {
    base::RecordAction(UserMetricsAction("BookmarkBar_DragStart"));
  }

  dragMouseOffset_ = [self convertPoint:[event locationInWindow] fromView:nil];
  dragPending_ = YES;
  gDraggedButton = self;

  NSImage* image = [self dragImage];
  [self setHidden:YES];

  NSPasteboardItem* item = [[self delegate] pasteboardItemForDragOfButton:self];
  if ([[self delegate] respondsToSelector:@selector(willBeginPasteboardDrag)])
    [[self delegate] willBeginPasteboardDrag];

  base::scoped_nsobject<NSDraggingItem> dragItem(
      [[NSDraggingItem alloc] initWithPasteboardWriter:item]);
  [dragItem setDraggingFrame:[self bounds] contents:image];

  [self beginDraggingSessionWithItems:@[ dragItem.get() ]
                                event:event
                               source:self];
  while (gDraggedButton != nil) {
    [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode
                             beforeDate:[NSDate distantFuture]];
  }
  [self setHidden:NO];

  // And we're done.
  dragPending_ = NO;
  gDraggedButton = nil;

  [self autorelease];
}

- (NSDragOperation)draggingSession:(NSDraggingSession*)session
    sourceOperationMaskForDraggingContext:(NSDraggingContext)context {
  NSDragOperation operation = NSDragOperationCopy;

  if (context == NSDraggingContextWithinApplication)
    operation |= NSDragOperationMove;

  if ([delegate_ canDragBookmarkButtonToTrash:self])
    operation |= NSDragOperationDelete;

  return operation;
}

// Overridden to release bar visibility.
- (DraggableButtonResult)endDrag {
  gDraggedButton = nil;

  // visibilityDelegate_ can be nil if we're detached, and that's fine.
  [visibilityDelegate_ releaseToolbarVisibilityForOwner:self withAnimation:YES];
  visibilityDelegate_ = nil;

  return kDraggableButtonImplUseBase;
}

- (void)draggingSession:(NSDraggingSession*)session
           endedAtPoint:(NSPoint)aPoint
              operation:(NSDragOperation)operation {
  gDraggedButton = nil;
  // Inform delegate of drag source that we're finished dragging,
  // so it can close auto-opened bookmark folders etc.
  [delegate_ bookmarkDragDidEnd:self
                      operation:operation];
  // Tell delegate if it should delete us.
  if (operation & NSDragOperationDelete) {
    dragEndScreenLocation_ = aPoint;
    [delegate_ didDragBookmarkToTrash:self];
  }
}

- (DraggableButtonResult)performMouseDownAction:(NSEvent*)theEvent {
  int eventMask = NSLeftMouseUpMask | NSMouseEnteredMask | NSMouseExitedMask |
      NSLeftMouseDraggedMask;

  BOOL keepGoing = YES;
  [[self target] performSelector:[self action] withObject:self];
  self.draggableButton.actionHasFired = YES;

  DraggableButton* insideBtn = nil;

  while (keepGoing) {
    theEvent = [[self window] nextEventMatchingMask:eventMask];
    if (!theEvent)
      continue;

    NSPoint mouseLoc = [self convertPoint:[theEvent locationInWindow]
                                 fromView:nil];
    BOOL isInside = [self mouse:mouseLoc inRect:[self bounds]];

    switch ([theEvent type]) {
      case NSMouseEntered:
      case NSMouseExited: {
        NSView* trackedView = (NSView*)[[theEvent trackingArea] owner];
        if (trackedView && [trackedView isKindOfClass:[self class]]) {
          BookmarkButton* btn = static_cast<BookmarkButton*>(trackedView);
          if (![btn acceptsTrackInFrom:self])
            break;
          if ([theEvent type] == NSMouseEntered) {
            [[NSCursor arrowCursor] set];
            [[btn cell] mouseEntered:theEvent];
            insideBtn = btn;
          } else {
            [[btn cell] mouseExited:theEvent];
            if (insideBtn == btn)
              insideBtn = nil;
          }
        }
        break;
      }
      case NSLeftMouseDragged: {
        if (insideBtn)
          [insideBtn mouseDragged:theEvent];
        break;
      }
      case NSLeftMouseUp: {
        self.draggableButton.durationMouseWasDown =
            [theEvent timestamp] - self.draggableButton.whenMouseDown;
        if (!isInside && insideBtn && insideBtn != self) {
          // Has tracked onto another BookmarkButton menu item, and released,
          // so fire its action.
          [[insideBtn target] performSelector:[insideBtn action]
                                   withObject:insideBtn];

        } else {
          [self secondaryMouseUpAction:isInside];
          [[self cell] mouseExited:theEvent];
          [[insideBtn cell] mouseExited:theEvent];
        }
        keepGoing = NO;
        break;
      }
      default:
        /* Ignore any other kind of event. */
        break;
    }
  }
  return kDraggableButtonMixinDidWork;
}

// mouseEntered: and mouseExited: are called from our
// BookmarkButtonCell.  We redirect this information to our delegate.
// The controller can then perform menu-like actions (e.g. "hover over
// to open menu").
- (void)mouseEntered:(NSEvent*)event {
  [delegate_ mouseEnteredButton:self event:event];
}

// See comments above mouseEntered:.
- (void)mouseExited:(NSEvent*)event {
  [delegate_ mouseExitedButton:self event:event];
}

- (void)mouseMoved:(NSEvent*)theEvent {
  if ([delegate_ respondsToSelector:@selector(mouseMoved:)])
    [id(delegate_) mouseMoved:theEvent];
}

- (void)mouseDragged:(NSEvent*)theEvent {
  if ([delegate_ respondsToSelector:@selector(mouseDragged:)])
    [id(delegate_) mouseDragged:theEvent];
}

- (void)mouseUp:(NSEvent*)theEvent {
  [super mouseUp:theEvent];

  // Update the highlight on mouse up.
  GradientButtonCell* cell =
      base::mac::ObjCCastStrict<GradientButtonCell>([self cell]);
  [cell setMouseInside:[cell isMouseReallyInside] animate:NO];
}

- (void)willOpenMenu:(NSMenu *)menu withEvent:(NSEvent *)event {
  // Ensure that right-clicking on a button while a context menu is already open
  // highlights the new button.
  [delegate_ mouseEnteredButton:self event:event];

  GradientButtonCell* cell =
      base::mac::ObjCCastStrict<GradientButtonCell>([self cell]);
  // Opt for animate:NO, otherwise the upcoming contextual menu's modal loop
  // will block the animation and the button's state will visually never change
  // ( https://crbug.com/649256 ).
  [cell setMouseInside:YES animate:NO];
}

- (void)didCloseMenu:(NSMenu *)menu withEvent:(NSEvent *)event {
  // Update the highlight after the contextual menu closes.
  GradientButtonCell* cell =
      base::mac::ObjCCastStrict<GradientButtonCell>([self cell]);

  if (![cell isMouseReallyInside]) {
    [cell setMouseInside:NO animate:YES];
    [delegate_ mouseExitedButton:self event:event];
  }
}

+ (BookmarkButton*)draggedButton {
  return gDraggedButton;
}

- (BOOL)canBecomeKeyView {
  if (![super canBecomeKeyView])
    return NO;

  // If button is an item in a folder menu, don't become key.
  return ![[self cell] isFolderButtonCell];
}

// This only gets called after a click that wasn't a drag, and only on folders.
- (DraggableButtonResult)secondaryMouseUpAction:(BOOL)wasInside {
  const NSTimeInterval kShortClickLength = 0.5;
  // Long clicks that end over the folder button result in the menu hiding.
  if (wasInside &&
      self.draggableButton.durationMouseWasDown > kShortClickLength) {
    [[self target] performSelector:[self action] withObject:self];
  } else {
    // Mouse tracked out of button during menu track. Hide menus.
    if (!wasInside)
      [delegate_ bookmarkDragDidEnd:self
                          operation:NSDragOperationNone];
  }
  return kDraggableButtonMixinDidWork;
}

- (BOOL)isOpaque {
  // Make this control opaque so that sub-pixel anti-aliasing works when
  // CoreAnimation is enabled.
  return YES;
}

- (void)drawRect:(NSRect)rect {
  NSView* bookmarkBarToolbarView = [[self superview] superview];
  if (backgroundColor_) {
    [backgroundColor_ set];
    NSRectFill(rect);
  } else {
    [self cr_drawUsingAncestor:bookmarkBarToolbarView inRect:(NSRect)rect];
  }
  [super drawRect:rect];
}

- (void)updateIconToMatchTheme {
  // During testing, the window might not be a browser window, and the
  // superview might not be a BookmarkBarView.
  if (![[self window] respondsToSelector:@selector(hasDarkTheme)] ||
      ![[self superview] isKindOfClass:[BookmarkBarView class]]) {
    return;
  }

  BookmarkBarView* bookmarkBarView =
      base::mac::ObjCCastStrict<BookmarkBarView>([self superview]);
  BookmarkBarController* bookmarkBarController = [bookmarkBarView controller];

  // The apps page shortcut button does not need to be updated.
  if (self == [bookmarkBarController appsPageShortcutButton]) {
    return;
  }

  BOOL darkTheme = [[self window] hasDarkTheme];
  NSImage* theImage = nil;
  // Make sure the "off the side" button gets the chevron icon.
  if ([bookmarkBarController offTheSideButton] == self) {
    theImage = [bookmarkBarController offTheSideButtonImage:darkTheme];
  } else {
    theImage = [bookmarkBarController faviconForNode:[self bookmarkNode]
                                       forADarkTheme:darkTheme];
  }

  [[self cell] setImage:theImage];
}

- (void)viewDidMoveToWindow {
  [super viewDidMoveToWindow];
  if ([self window]) {
    // The new window may have different main window status.
    // This happens when the view is moved into a TabWindowOverlayWindow for
    // tab dragging.
    [self updateIconToMatchTheme];
    [self windowDidChangeActive];
  }
}

// ThemedWindowDrawing implementation.

- (void)windowDidChangeTheme {
  [self updateIconToMatchTheme];
  [self setNeedsDisplay:YES];
}

- (void)windowDidChangeActive {
  [self setNeedsDisplay:YES];
}

- (void)installCustomTrackingArea {
  const NSTrackingAreaOptions options =
      NSTrackingActiveAlways |
      NSTrackingMouseEnteredAndExited |
      NSTrackingEnabledDuringMouseDrag;

  if (area_) {
    [self removeTrackingArea:area_];
    [area_ release];
  }

  area_ = [[NSTrackingArea alloc] initWithRect:[self bounds]
                                       options:options
                                         owner:self
                                      userInfo:nil];
  [self addTrackingArea:area_];
}

- (NSImage*)dragImage {
  NSRect bounds = [self bounds];
  base::scoped_nsobject<NSImage> image(
      [[NSImage alloc] initWithSize:bounds.size]);
  [image lockFocusFlipped:[self isFlipped]];

  NSGraphicsContext* context = [NSGraphicsContext currentContext];
  CGContextRef cgContext = static_cast<CGContextRef>([context graphicsPort]);
  CGContextBeginTransparencyLayer(cgContext, 0);
  CGContextSetAlpha(cgContext, kDragImageOpacity);

  GradientButtonCell* cell =
      base::mac::ObjCCastStrict<GradientButtonCell>([self cell]);
  [[cell clipPathForFrame:bounds inView:self] setClip];
  [cell drawWithFrame:bounds inView:self];

  CGContextEndTransparencyLayer(cgContext);
  [image unlockFocus];

  return image.autorelease();
}

@end
