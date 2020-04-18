// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/auto_reset.h"
#include "base/mac/bundle_locations.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/find_bar/find_bar_bridge.h"
#import "chrome/browser/ui/cocoa/find_bar/find_bar_cocoa_controller.h"
#import "chrome/browser/ui/cocoa/find_bar/find_bar_text_field.h"
#import "chrome/browser/ui/cocoa/find_bar/find_bar_text_field_cell.h"
#import "chrome/browser/ui/cocoa/image_button_cell.h"
#import "chrome/browser/ui/cocoa/tabs/tab_strip_controller.h"
#include "chrome/browser/ui/find_bar/find_bar_controller.h"
#include "chrome/browser/ui/find_bar/find_tab_helper.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/theme_resources.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/web_contents.h"
#import "third_party/google_toolbox_for_mac/src/AppKit/GTMNSAnimation+Duration.h"
#import "ui/base/cocoa/find_pasteboard.h"
#import "ui/base/cocoa/focus_tracker.h"
#import "ui/base/cocoa/nsview_additions.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image_skia_util_mac.h"
#include "ui/gfx/paint_vector_icon.h"

using content::NativeWebKeyboardEvent;

const float kFindBarOpenDuration = 0.2;
const float kFindBarCloseDuration = 0.15;
const float kFindBarMoveDuration = 0.15;
const float kRightEdgeOffset = 25;
const int kMaxCharacters = 4000;
const int kUndefinedResultCount = -1;

@interface FindBarCocoaController (PrivateMethods) <NSAnimationDelegate>
// Returns the appropriate frame for a hidden find bar.
- (NSRect)hiddenFindBarFrame;

// Animates the given |view| to the given |endFrame| within |duration| seconds.
// Returns a new NSViewAnimation.
- (NSViewAnimation*)createAnimationForView:(NSView*)view
                                   toFrame:(NSRect)endFrame
                                  duration:(float)duration;

// Sets the frame of |findBarView_|.  |duration| is ignored if |animate| is NO.
- (void)setFindBarFrame:(NSRect)endFrame
                animate:(BOOL)animate
               duration:(float)duration;

// Returns the horizontal position the FindBar should use in order to avoid
// overlapping with the current find result, if there's one.
- (float)findBarHorizontalPosition;

// Adjusts the horizontal position if necessary to avoid overlapping with the
// current find result.
- (void)moveFindBarIfNecessary:(BOOL)animate;

// Puts |text| into the find bar and enables the buttons, but doesn't start a
// new search for |text|.
- (void)prepopulateText:(NSString*)text;

// Clears the find results for all tabs in browser associated with this find
// bar. If |suppressPboardUpdateActions_| is true then the current tab is not
// cleared.
- (void)clearFindResultsForCurrentBrowser;

- (BrowserWindowController*)browserWindowController;

// Returns the number of matches from the last find results of the active
// web contents. Returns kUndefinedResultCount if unable to determine the count.
- (int)lastNumberOfMatchesForActiveWebContents;
@end

@implementation FindBarCocoaController

@synthesize findBarView = findBarView_;

- (id)initWithBrowser:(Browser*)browser {
  if ((self = [super initWithNibName:@"FindBar"
                              bundle:base::mac::FrameworkBundle()])) {
    browser_ = browser;
  }
  return self;
}

- (void)dealloc {
  [self browserWillBeDestroyed];
  [super dealloc];
}

- (void)browserWillBeDestroyed {
  // All animations should have been explicitly stopped before a tab is closed.
  DCHECK(!showHideAnimation_.get());
  DCHECK(!moveAnimation_.get());
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  browser_ = nullptr;
}

- (void)setFindBarBridge:(FindBarBridge*)findBarBridge {
  DCHECK(!findBarBridge_);  // should only be called once.
  findBarBridge_ = findBarBridge;
}

- (void)awakeFromNib {
  [[closeButton_ cell] setImageID:IDR_CLOSE_1
                   forButtonState:image_button_cell::kDefaultState];
  [[closeButton_ cell] setImageID:IDR_CLOSE_1_H
                   forButtonState:image_button_cell::kHoverState];
  [[closeButton_ cell] setImageID:IDR_CLOSE_1_P
                   forButtonState:image_button_cell::kPressedState];
  [[closeButton_ cell] setImageID:IDR_CLOSE_1
                   forButtonState:image_button_cell::kDisabledState];

  [closeButton_ setToolTip:l10n_util::GetNSString(
       IDS_FIND_IN_PAGE_CLOSE_TOOLTIP)];
  [previousButton_ setToolTip:l10n_util::GetNSString(
       IDS_FIND_IN_PAGE_PREVIOUS_TOOLTIP)];
  [nextButton_ setToolTip:l10n_util::GetNSString(
       IDS_FIND_IN_PAGE_NEXT_TOOLTIP)];

  [closeButton_ setTitle:l10n_util::GetNSString(IDS_ACCNAME_CLOSE)];
  [previousButton_ setTitle:l10n_util::GetNSString(IDS_ACCNAME_PREVIOUS)];
  [nextButton_ setTitle:l10n_util::GetNSString(IDS_ACCNAME_NEXT)];

  NSImage* image = NSImageFromImageSkia(
      gfx::CreateVectorIcon(kCaretDownIcon, SK_ColorBLACK));
  [image setTemplate:YES];
  [nextButton_ setImage:image];

  image =
      NSImageFromImageSkia(gfx::CreateVectorIcon(kCaretUpIcon, SK_ColorBLACK));
  [image setTemplate:YES];
  [previousButton_ setImage:image];

  [findBarView_ setFrame:[self hiddenFindBarFrame]];
  defaultWidth_ = NSWidth([findBarView_ frame]);
  [[self view] setHidden:YES];

  [self prepopulateText:[[FindPasteboard sharedInstance] findText]];
}

- (IBAction)close:(id)sender {
  if (findBarBridge_) {
    findBarBridge_->GetFindBarController()->EndFindSession(
        FindBarController::kKeepSelectionOnPage,
        FindBarController::kKeepResultsInFindBox);
  }
}

- (IBAction)previousResult:(id)sender {
  if (findBarBridge_) {
    FindTabHelper* findTabHelper = FindTabHelper::FromWebContents(
        findBarBridge_->GetFindBarController()->web_contents());
    findTabHelper->StartFinding(
        base::SysNSStringToUTF16([findText_ stringValue]),
        false, false);
  }
}

- (IBAction)nextResult:(id)sender {
  if (findBarBridge_) {
    FindTabHelper* findTabHelper = FindTabHelper::FromWebContents(
        findBarBridge_->GetFindBarController()->web_contents());
    findTabHelper->StartFinding(
        base::SysNSStringToUTF16([findText_ stringValue]),
        true, false);
  }
}

- (void)findPboardUpdated:(NSNotification*)notification {
  [self clearFindResultsForCurrentBrowser];
  if (!suppressPboardUpdateActions_)
    [self prepopulateText:[[FindPasteboard sharedInstance] findText]];
}

- (void)positionFindBarViewAtMaxY:(CGFloat)maxY maxWidth:(CGFloat)maxWidth {
  NSView* containerView = [self view];
  CGFloat containerHeight = NSHeight([containerView frame]);
  CGFloat containerWidth = std::min(maxWidth, defaultWidth_);

  // Adjust where we'll actually place the find bar.
  maxY += [containerView cr_lineWidth];
  maxY_ = maxY;
  CGFloat x = [self findBarHorizontalPosition];
  NSRect newFrame = NSMakeRect(x, maxY - containerHeight,
                               containerWidth, containerHeight);

  if (moveAnimation_.get() != nil) {
    NSRect frame = [containerView frame];
    [moveAnimation_ stopAnimation];
    // Restore to the X position before the animation was stopped. The Y
    // position is immediately adjusted.
    frame.origin.y = newFrame.origin.y;
    [containerView setFrame:frame];
    moveAnimation_.reset([self createAnimationForView:containerView
                                              toFrame:newFrame
                                             duration:kFindBarMoveDuration]);
  } else {
    [containerView setFrame:newFrame];
  }
}

- (BOOL)isOffTheRecordProfile {
  return browser_ && browser_->profile() &&
         browser_->profile()->IsOffTheRecord();
}

// NSControl delegate method.
- (void)controlTextDidChange:(NSNotification*)aNotification {
  if (!findBarBridge_)
    return;

  content::WebContents* webContents =
      findBarBridge_->GetFindBarController()->web_contents();
  if (!webContents)
    return;
  FindTabHelper* findTabHelper = FindTabHelper::FromWebContents(webContents);

  // The find bar stops functioning if too many characters are used.
  if ([[findText_ stringValue] length] > kMaxCharacters)
    [findText_ setStringValue:[[findText_ stringValue]
                                  substringToIndex:kMaxCharacters]];
  NSString* findText = [findText_ stringValue];
  if (![self isOffTheRecordProfile]) {
    base::AutoReset<BOOL> suppressReset(&suppressPboardUpdateActions_, YES);
    [[FindPasteboard sharedInstance] setFindText:findText];
  }

  if ([findText length] > 0) {
    findTabHelper->
        StartFinding(base::SysNSStringToUTF16(findText), true, false);
  } else {
    // The textbox is empty so we reset.
    findTabHelper->StopFinding(FindBarController::kClearSelectionOnPage);
    [self updateUIForFindResult:findTabHelper->find_result()
                       withText:base::string16()];
  }
}

// NSControl delegate method
- (BOOL)control:(NSControl*)control
    textView:(NSTextView*)textView
    doCommandBySelector:(SEL)command {
  if (command == @selector(insertNewline:)) {
    // Pressing Return
    NSEvent* event = [NSApp currentEvent];

    if ([event modifierFlags] & NSShiftKeyMask)
      [previousButton_ performClick:nil];
    else
      [nextButton_ performClick:nil];

    return YES;
  } else if (command == @selector(insertLineBreak:)) {
    // Pressing Ctrl-Return
    if (findBarBridge_) {
      findBarBridge_->GetFindBarController()->EndFindSession(
          FindBarController::kActivateSelectionOnPage,
          FindBarController::kClearResultsInFindBox);
    }
    return YES;
  } else if (command == @selector(cancelOperation:)) {
    // Pressing ESC.
    [closeButton_ performClick:nil];
    return YES;
  } else if (command == @selector(pageUp:) ||
             command == @selector(pageUpAndModifySelection:) ||
             command == @selector(scrollPageUp:) ||
             command == @selector(pageDown:) ||
             command == @selector(pageDownAndModifySelection:) ||
             command == @selector(scrollPageDown:) ||
             command == @selector(scrollToBeginningOfDocument:) ||
             command == @selector(scrollToEndOfDocument:) ||
             command == @selector(moveUp:) ||
             command == @selector(moveDown:)) {
    content::WebContents* web_contents =
        findBarBridge_->GetFindBarController()->web_contents();
    if (!web_contents)
      return NO;

    // Sanity-check to make sure we got a keyboard event.
    NSEvent* event = [NSApp currentEvent];
    if ([event type] != NSKeyDown && [event type] != NSKeyUp)
      return NO;

    // Forward the event to the renderer.
    // TODO(rohitrao): Should this call -[BaseView keyEvent:]?  Is there code in
    // that function that we want to keep or avoid? Calling
    // |ForwardKeyboardEvent()| directly ignores edit commands, which breaks
    // cmd-up/down if we ever decide to include |moveToBeginningOfDocument:| in
    // the list above.
    content::RenderViewHost* render_view_host =
        web_contents->GetRenderViewHost();

    // TODO(tdresser): get the hardware timestamp from the NSEvent.
    render_view_host->GetWidget()->ForwardKeyboardEvent(
        NativeWebKeyboardEvent(event));
    return YES;
  }

  return NO;
}

// Methods from FindBar
- (void)showFindBar:(BOOL)animate {
  // Save the currently-focused view.  |findBarView_| is in the view
  // hierarchy by now.  showFindBar can be called even when the
  // findbar is already open, so do not overwrite an already saved
  // view.
  if (!focusTracker_.get())
    focusTracker_.reset(
        [[FocusTracker alloc] initWithWindow:[findBarView_ window]]);

  // The browser window might have changed while the FindBar was hidden.
  // Update its position now.
  [[self browserWindowController] layoutSubviews];

  // Move to the correct horizontal position first, to prevent the FindBar
  // from jumping around when switching tabs.
  // Prevent jumping while the FindBar is animating (hiding, then showing) too.
  if (![self isFindBarVisible])
    [self moveFindBarIfNecessary:NO];

  // Animate the view into place.
  NSRect frame = [findBarView_ frame];
  frame.origin = NSZeroPoint;
  [self setFindBarFrame:frame animate:animate duration:kFindBarOpenDuration];

  // Clear the "mouse inside" state on the close button cell, so that the close
  // button isn't shown highlighted if previously the mouse was inside it. Done
  // here instead of in -close:, as it's possible for the cell to receive a
  // -mouseEntered: right after -close: is called.
  [[closeButton_ cell] setIsMouseInside:NO];
}

- (void)hideFindBar:(BOOL)animate {
  NSRect frame = [self hiddenFindBarFrame];
  [self setFindBarFrame:frame animate:animate duration:kFindBarCloseDuration];
}

- (void)stopAnimation {
  if (showHideAnimation_.get()) {
    [showHideAnimation_ stopAnimation];
    showHideAnimation_.reset(nil);
  }
  if (moveAnimation_.get()) {
    [moveAnimation_ stopAnimation];
    moveAnimation_.reset(nil);
  }
}

- (void)setFocusAndSelection {
  [[findText_ window] makeFirstResponder:findText_];
  BOOL buttonsEnabled = [self lastNumberOfMatchesForActiveWebContents] != 0 &&
                        [[findText_ stringValue] length] > 0;

  [previousButton_ setEnabled:buttonsEnabled];
  [nextButton_ setEnabled:buttonsEnabled];
}

- (void)restoreSavedFocus {
  if (!(focusTracker_.get() &&
        [focusTracker_ restoreFocusInWindow:[findBarView_ window]])) {
    // Fall back to giving focus to the tab contents.
    findBarBridge_->GetFindBarController()->web_contents()->Focus();
  }
  focusTracker_.reset(nil);
}

- (void)setFindText:(NSString*)findText
      selectedRange:(const NSRange&)selectedRange {
  [findText_ setStringValue:findText];

  if (![self isOffTheRecordProfile]) {
    // Make sure the text in the find bar always ends up in the find pasteboard
    // (and, via notifications, in the other find bars too).
    base::AutoReset<BOOL> suppressReset(&suppressPboardUpdateActions_, YES);
    [[FindPasteboard sharedInstance] setFindText:findText];
  }

  NSText* editor = [findText_ currentEditor];
  if (selectedRange.location != NSNotFound)
    [editor setSelectedRange:selectedRange];
}

- (NSString*)findText {
  return [findText_ stringValue];
}

- (NSRange)selectedRange {
  NSText* editor = [findText_ currentEditor];
  return (editor != nil) ? [editor selectedRange] : NSMakeRange(NSNotFound, 0);
}

- (NSString*)matchCountText {
  return [[findText_ findBarTextFieldCell] resultsString];
}

- (void)updateFindBarForChangedWebContents {
  content::WebContents* contents =
      findBarBridge_->GetFindBarController()->web_contents();
  if (!contents)
    return;
  FindTabHelper* findTabHelper = FindTabHelper::FromWebContents(contents);

  // If the find UI is visible but the results are cleared then also clear
  // the results label and update the buttons.
  if (findTabHelper->find_ui_active() &&
      findTabHelper->previous_find_text().empty()) {
    BOOL buttonsEnabled = [[findText_ stringValue] length] > 0 ? YES : NO;
    [previousButton_ setEnabled:buttonsEnabled];
    [nextButton_ setEnabled:buttonsEnabled];
    [[findText_ findBarTextFieldCell] clearResults];
  }
}

- (void)clearResults:(const FindNotificationDetails&)results {
  // Just call updateUIForFindResult, which will take care of clearing
  // the search text and the results label.
  [self updateUIForFindResult:results withText:base::string16()];
}

- (void)updateUIForFindResult:(const FindNotificationDetails&)result
                     withText:(const base::string16&)findText {
  // If we don't have any results and something was passed in, then
  // that means someone pressed Cmd-G while the Find box was
  // closed. In that case we need to repopulate the Find box with what
  // was passed in.
  if ([[findText_ stringValue] length] == 0 && !findText.empty()) {
    [findText_ setStringValue:base::SysUTF16ToNSString(findText)];
    [findText_ selectText:self];
  }

  // Make sure Find Next and Find Previous are enabled if we found any matches.
  BOOL buttonsEnabled = result.number_of_matches() > 0;
  [previousButton_ setEnabled:buttonsEnabled];
  [nextButton_ setEnabled:buttonsEnabled];

  // Update the results label.
  BOOL validRange = result.active_match_ordinal() != -1 &&
                    result.number_of_matches() != -1;
  NSString* searchString = [findText_ stringValue];
  if ([searchString length] > 0 && validRange) {
    [[findText_ findBarTextFieldCell]
        setActiveMatch:result.active_match_ordinal()
                    of:result.number_of_matches()];
  } else {
    // If there was no text entered, we don't show anything in the results area.
    [[findText_ findBarTextFieldCell] clearResults];
  }

  [findText_ resetFieldEditorFrameIfNeeded];
  [findText_ setNeedsDisplay:YES];

  // If we found any results, reset the focus tracker, so we always
  // restore focus to the tab contents.
  if (result.number_of_matches() > 0)
    focusTracker_.reset(nil);

  // Adjust the FindBar position, even when there are no matches (so that it
  // goes back to the default position, if required).
  [self moveFindBarIfNecessary:[self isFindBarVisible]];
}

- (BOOL)isFindBarVisible {
  // Find bar is visible if any part of it is on the screen.
  return NSIntersectsRect([[self view] bounds], [findBarView_ frame]);
}

- (BOOL)isFindBarAnimating {
  return (showHideAnimation_.get() != nil) || (moveAnimation_.get() != nil);
}

// NSAnimation delegate methods.
- (void)animationDidEnd:(NSAnimation*)animation {
  // Autorelease the animations (cannot use release because the animation object
  // is still on the stack.
  if (animation == showHideAnimation_.get()) {
    [showHideAnimation_.release() autorelease];
  } else if (animation == moveAnimation_.get()) {
    [moveAnimation_.release() autorelease];
  } else {
    NOTREACHED();
  }

  // If the find bar is not visible, make it actually hidden, so it'll no longer
  // respond to key events.
  [[self view] setHidden:![self isFindBarVisible]];
  // Notify the FindBarController that the visibility animation has completed in
  // order to show or hide a decoration in the location bar.
  if (findBarBridge_)
    findBarBridge_->GetFindBarController()->FindBarVisibilityChanged();
}

- (gfx::Point)findBarWindowPosition {
  gfx::Rect viewRect(NSRectToCGRect([[self view] frame]));
  // Convert Cocoa coordinates (Y growing up) to Y growing down.
  // Offset from |maxY_|, which represents the content view's top, instead
  // of from the superview, which represents the whole browser window.
  viewRect.set_y(maxY_ - viewRect.bottom());
  return viewRect.origin();
}

- (int)findBarWidth {
  return NSWidth([[self view] frame]);
}

@end

@implementation FindBarCocoaController (PrivateMethods)

- (NSRect)hiddenFindBarFrame {
  NSRect frame = [findBarView_ frame];
  NSRect containerBounds = [[self view] bounds];
  frame.origin = NSMakePoint(NSMinX(containerBounds), NSMaxY(containerBounds));
  return frame;
}

- (NSViewAnimation*)createAnimationForView:(NSView*)view
                                   toFrame:(NSRect)endFrame
                                  duration:(float)duration {
  NSDictionary* dict = [NSDictionary dictionaryWithObjectsAndKeys:
      view, NSViewAnimationTargetKey,
      [NSValue valueWithRect:endFrame], NSViewAnimationEndFrameKey, nil];

  NSViewAnimation* animation =
      [[NSViewAnimation alloc]
        initWithViewAnimations:[NSArray arrayWithObjects:dict, nil]];
  [animation gtm_setDuration:duration
                   eventMask:NSLeftMouseUpMask];
  [animation setDelegate:self];
  [animation startAnimation];
  return animation;
}

- (void)setFindBarFrame:(NSRect)endFrame
                animate:(BOOL)animate
               duration:(float)duration {
  // Save the current frame.
  NSRect startFrame = [findBarView_ frame];

  // Stop any existing animations.
  [showHideAnimation_ stopAnimation];

  if (!animate) {
    [findBarView_ setFrame:endFrame];
    [[self view] setHidden:![self isFindBarVisible]];
    showHideAnimation_.reset(nil);
    return;
  }

  // If animating, ensure that the find bar is not hidden. Hidden status will be
  // updated at the end of the animation.
  [[self view] setHidden:NO];

  // Reset the frame to what was saved above.
  [findBarView_ setFrame:startFrame];

  showHideAnimation_.reset([self createAnimationForView:findBarView_
                                                toFrame:endFrame
                                               duration:duration]);
}

- (float)findBarHorizontalPosition {
  // Get the rect of the FindBar.
  NSView* view = [self view];
  NSRect frame = [view frame];
  gfx::Rect viewRect(NSRectToCGRect(frame));

  if (!findBarBridge_ || !findBarBridge_->GetFindBarController())
    return frame.origin.x;
  content::WebContents* contents =
      findBarBridge_->GetFindBarController()->web_contents();
  if (!contents)
    return frame.origin.x;

  // Get the size of the container.
  gfx::Rect containerRect(contents->GetContainerBounds().size());

  // Position the FindBar on the top right corner.
  viewRect.set_x(
      containerRect.width() - viewRect.width() - kRightEdgeOffset);
  // Convert from Cocoa coordinates (Y growing up) to Y growing down.
  // Notice that the view frame's Y offset is relative to the whole window,
  // while GetLocationForFindbarView() expects it relative to the
  // content's boundaries. |maxY_| has the correct placement in Cocoa coords,
  // so we just have to invert the Y coordinate.
  viewRect.set_y(maxY_ - viewRect.bottom());

  // Get the rect of the current find result, if there is one.
  const FindNotificationDetails& findResult =
      FindTabHelper::FromWebContents(contents)->find_result();
  if (findResult.number_of_matches() == 0)
    return viewRect.x();
  gfx::Rect selectionRect(findResult.selection_rect());

  // Adjust |view_rect| to avoid the |selection_rect| within |container_rect|.
  gfx::Rect newPos = FindBarController::GetLocationForFindbarView(
      viewRect, containerRect, selectionRect);

  return newPos.x();
}

- (void)moveFindBarIfNecessary:(BOOL)animate {
  // Don't animate during tests.
  if (FindBarBridge::disable_animations_during_testing_)
    animate = NO;

  NSView* view = [self view];
  NSRect frame = [view frame];
  float x = [self findBarHorizontalPosition];
  if (frame.origin.x == x)
    return;

  if (animate) {
    [moveAnimation_ stopAnimation];
    // Restore to the position before the animation was stopped.
    [view setFrame:frame];
    frame.origin.x = x;
    moveAnimation_.reset([self createAnimationForView:view
                                              toFrame:frame
                                             duration:kFindBarMoveDuration]);
  } else {
    frame.origin.x = x;
    [view setFrame:frame];
  }
}

- (void)prepopulateText:(NSString*)text {
  [self setFindText:text selectedRange:NSMakeRange(NSNotFound, 0)];

  // Has to happen after |ClearResults()| above.
  BOOL buttonsEnabled = [text length] > 0 ? YES : NO;
  [previousButton_ setEnabled:buttonsEnabled];
  [nextButton_ setEnabled:buttonsEnabled];
}

- (void)clearFindResultsForCurrentBrowser {
  if (!browser_)
    return;

  content::WebContents* activeWebContents =
      findBarBridge_->GetFindBarController()->web_contents();

  TabStripModel* tabStripModel = browser_->tab_strip_model();
  for (int i = 0; i < tabStripModel->count(); ++i) {
    content::WebContents* webContents = tabStripModel->GetWebContentsAt(i);
    if (suppressPboardUpdateActions_ && activeWebContents == webContents)
      continue;
    FindTabHelper* findTabHelper =
        FindTabHelper::FromWebContents(webContents);
    findTabHelper->StopFinding(FindBarController::kClearSelectionOnPage);
    findBarBridge_->ClearResults(findTabHelper->find_result());
  }
}

- (BrowserWindowController*)browserWindowController {
  if (!browser_)
    return nil;
  return [BrowserWindowController
      browserWindowControllerForWindow:browser_->window()->GetNativeWindow()];
}

- (int)lastNumberOfMatchesForActiveWebContents {
  if (!browser_)
    return kUndefinedResultCount;

  content::WebContents* contents =
      findBarBridge_->GetFindBarController()->web_contents();
  FindTabHelper* findTabHelper = FindTabHelper::FromWebContents(contents);
  return findTabHelper->find_result().number_of_matches();
}
@end
