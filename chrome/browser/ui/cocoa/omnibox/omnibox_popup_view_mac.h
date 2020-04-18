// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_OMNIBOX_OMNIBOX_POPUP_VIEW_MAC_H_
#define CHROME_BROWSER_UI_COCOA_OMNIBOX_OMNIBOX_POPUP_VIEW_MAC_H_

#import <Cocoa/Cocoa.h>
#include <stddef.h>

#include <memory>

#include "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#import "chrome/browser/ui/cocoa/omnibox/omnibox_popup_matrix.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/omnibox_popup_view.h"
#include "ui/gfx/font.h"

class AutocompleteResult;
class OmniboxEditModel;
class OmniboxPopupModel;
class OmniboxView;

// Implements OmniboxPopupView using a raw NSWindow containing an
// NSTableView.
class OmniboxPopupViewMac : public OmniboxPopupView,
                            public OmniboxPopupMatrixObserver {
 public:
  OmniboxPopupViewMac(OmniboxView* omnibox_view,
                      OmniboxEditModel* edit_model,
                      NSTextField* field);
  ~OmniboxPopupViewMac() override;

  // Return the OmniboxPopupViewMac background color.
  static NSColor* BackgroundColor(bool is_dark_theme);

  // Overridden from OmniboxPopupView:
  bool IsOpen() const override;
  void InvalidateLine(size_t line) override {}
  void OnLineSelected(size_t line) override {}
  void UpdatePopupAppearance() override;
  void OnMatchIconUpdated(size_t match_index) override;
  // This is only called by model in SetSelectedLine() after updating
  // everything.  Popup should already be visible.
  void PaintUpdatesNow() override;
  void OnDragCanceled() override {}

  // Overridden from OmniboxPopupMatrixDelegate:
  void OnMatrixRowSelected(OmniboxPopupMatrix* matrix, size_t row) override;
  void OnMatrixRowClicked(OmniboxPopupMatrix* matrix, size_t row) override;
  void OnMatrixRowMiddleClicked(OmniboxPopupMatrix* matrix,
                                size_t row) override;

  // Returns the NSImage that should be used as an icon for the given match.
  NSImage* ImageForMatch(const AutocompleteMatch& match) const;

  OmniboxPopupMatrix* matrix() { return matrix_; }

 protected:
  // Gets the autocomplete results. This is virtual so that it can be overridden
  // by tests.
  virtual const AutocompleteResult& GetResult() const;

 private:
  // Create the popup_ instance if needed.
  void CreatePopupIfNeeded();

  // Calculate the appropriate position for the popup based on the
  // field's screen position and the given target for the matrix
  // height, and makes the popup visible.  Animates to the new frame
  // if the popup shrinks, snaps to the new frame if the popup grows,
  // allows existing animations to continue if the size doesn't
  // change.
  void PositionPopup(const CGFloat matrixHeight);

  // Opens the URL at the given row.
  void OpenURLForRow(size_t row, WindowOpenDisposition disposition);

  OmniboxView* omnibox_view_;
  std::unique_ptr<OmniboxPopupModel> model_;
  NSTextField* field_;  // owned by tab controller

  // Child window containing a matrix which implements the popup.
  base::scoped_nsobject<NSWindow> popup_;

  base::scoped_nsobject<OmniboxPopupMatrix> matrix_;
  base::scoped_nsobject<NSView> top_separator_view_;
  base::scoped_nsobject<NSView> bottom_separator_view_;
  base::scoped_nsobject<NSBox> background_view_;

  DISALLOW_COPY_AND_ASSIGN(OmniboxPopupViewMac);
};

#endif  // CHROME_BROWSER_UI_COCOA_OMNIBOX_OMNIBOX_POPUP_VIEW_MAC_H_
