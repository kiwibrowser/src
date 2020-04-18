// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_LOCATION_BAR_AUTOCOMPLETE_TEXT_FIELD_H_
#define CHROME_BROWSER_UI_COCOA_LOCATION_BAR_AUTOCOMPLETE_TEXT_FIELD_H_

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/styled_text_field.h"
#import "chrome/browser/ui/cocoa/themed_window.h"
#import "chrome/browser/ui/cocoa/url_drop_target.h"

@class AutocompleteTextFieldCell;
class LocationBarDecoration;

// AutocompleteTextField intercepts UI actions for forwarding to
// OmniboxViewMac (*), and provides a custom look.  It works
// together with AutocompleteTextFieldEditor (mostly for intercepting
// user actions) and AutocompleteTextFieldCell (mostly for custom
// drawing).
//
// For historical reasons, chrome/browser/autocomplete is the core
// implementation of the Omnibox.  Chrome code seems to vary between
// autocomplete and Omnibox in describing this.
//
// (*) OmniboxViewMac is a view in the MVC sense for the
// Chrome internals, though it's really more of a mish-mash of model,
// view, and controller.

// Provides a hook so that we can call directly down to
// OmniboxViewMac rather than traversing the delegate chain.
class AutocompleteTextFieldObserver {
 public:
  // Called before changing the selected range of the field.
  virtual NSRange SelectionRangeForProposedRange(NSRange proposed_range) = 0;

  // Called when the control-key state changes while the field is
  // first responder.
  virtual void OnControlKeyChanged(bool pressed) = 0;

  // Called when the user pastes into the field.
  virtual void OnPaste() = 0;

  // Return |true| if there is a selection to copy.
  virtual bool CanCopy() = 0;

  // Creates a pasteboard item from the field's current selection.
  virtual base::scoped_nsobject<NSPasteboardItem> CreatePasteboardItem() = 0;

  // Copies the pasteboard item returned from |CreatePasteboardItem()| to
  // |pboard|.
  virtual void CopyToPasteboard(NSPasteboard* pboard) = 0;

  // Returns true if the current clipboard text supports paste and go
  // (or paste and search).
  virtual bool CanPasteAndGo() = 0;

  // Returns the appropriate "Paste and Go" or "Paste and Search"
  // context menu string, depending on what is currently in the
  // clipboard.  Must not be called unless CanPasteAndGo() returns
  // true.
  virtual int GetPasteActionStringId() = 0;

  // Called when the user initiates a "paste and go" or "paste and
  // search" into the field.
  virtual void OnPasteAndGo() = 0;

  // Called when the field's frame changes.
  virtual void OnFrameChanged() = 0;

  // Called when the popup is no longer appropriate, such as when the
  // field's window loses focus or a page action is clicked.
  virtual void ClosePopup() = 0;

  // Called when the user begins editing the field, for every edit,
  // and when the user is done editing the field.
  virtual void OnDidBeginEditing() = 0;
  virtual void OnBeforeChange() = 0;
  virtual void OnDidChange() = 0;
  virtual void OnDidEndEditing() = 0;

  // Called when -insertText: is invoked on the editor.
  virtual void OnInsertText() = 0;

  // Called before a -drawRect: operation.
  virtual void OnBeforeDrawRect() = 0;

  // Called after the completion of a -drawRect: operation.
  virtual void OnDidDrawRect() = 0;

  // NSResponder translates certain keyboard actions into selectors
  // passed to -doCommandBySelector:.  The selector is forwarded here,
  // return true if |cmd| is handled, false if the caller should
  // handle it.
  // TODO(shess): For now, I think having the code which makes these
  // decisions closer to the other autocomplete code is worthwhile,
  // since it calls a wide variety of methods which otherwise aren't
  // clearly relevent to expose here.  But consider pulling more of
  // the OmniboxViewMac calls up to here.
  virtual bool OnDoCommandBySelector(SEL cmd) = 0;

  // Called whenever the autocomplete text field gets focused.
  virtual void OnSetFocus(bool control_down) = 0;

  // Called whenever the autocomplete text field is losing focus.
  virtual void OnKillFocus() = 0;

  // Called before the text field handles a mouse down event.
  virtual void OnMouseDown(NSInteger button_number) = 0;

 protected:
  virtual ~AutocompleteTextFieldObserver() {}
};

@interface AutocompleteTextField : StyledTextField<NSTextViewDelegate,
                                                   URLDropTarget,
                                                   ThemedWindowDrawing> {
 @private
  // Undo manager for this text field.  We use a specific instance rather than
  // the standard undo manager in order to let us clear the undo stack at will.
  base::scoped_nsobject<NSUndoManager> undoManager_;

  AutocompleteTextFieldObserver* observer_;  // weak, owned by location bar.

  // Handles being a drag-and-drop target.
  base::scoped_nsobject<URLDropTargetHandler> dropHandler_;

  // Holds current tooltip strings, to keep them from being dealloced.
  base::scoped_nsobject<NSMutableArray> currentToolTips_;

  // Animation object used for resizing the autocomplete field.
  base::scoped_nsobject<NSViewAnimation> resizeAnimation_;
}

@property(nonatomic) AutocompleteTextFieldObserver* observer;

// Convenience method to return the cell, casted appropriately.
- (AutocompleteTextFieldCell*)cell;

// Superclass aborts editing before changing the string, which causes
// problems for undo.  This version modifies the field editor's
// contents if the control is already being edited.
- (void)setAttributedStringValue:(NSAttributedString*)aString;

// Clears the undo chain for this text field.
- (void)clearUndoChain;

// Animates the text field to the given |frame|.
- (void)animateToFrame:(NSRect)frame;

// Stops the current animation, if any. The frame will be set to the current
// (mid-animation) frame.
- (void)stopAnimation;

// Updates cursor and tooltip rects depending on the contents of the text field
// e.g. the security icon should have a default pointer shown on hover instead
// of an I-beam.
- (void)updateMouseTracking;

// Return the appropriate menu for any decoration under |event|.
- (NSMenu*)decorationMenuForEvent:(NSEvent*)event;

// Retains |tooltip| (in |currentToolTips_|) and adds this tooltip
// via -[NSView addToolTipRect:owner:userData:].
- (void)addToolTip:(NSString*)tooltip forRect:(NSRect)aRect;

// Obtain the bubble anchor point for |decoration| in window coordinates.
- (NSPoint)bubblePointForDecoration:(LocationBarDecoration*)decoration;

@end

#endif  // CHROME_BROWSER_UI_COCOA_LOCATION_BAR_AUTOCOMPLETE_TEXT_FIELD_H_
