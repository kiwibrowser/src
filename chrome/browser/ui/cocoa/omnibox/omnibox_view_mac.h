// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_OMNIBOX_OMNIBOX_VIEW_MAC_H_
#define CHROME_BROWSER_UI_COCOA_OMNIBOX_OMNIBOX_VIEW_MAC_H_

#import <Cocoa/Cocoa.h>
#include <stddef.h>

#include <memory>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "chrome/browser/ui/cocoa/location_bar/autocomplete_text_field.h"
#include "components/omnibox/browser/omnibox_view.h"
#include "components/security_state/core/security_state.h"
#include "third_party/skia/include/core/SkColor.h"

class CommandUpdater;
class OmniboxPopupView;
class Profile;

namespace content {
class WebContents;
}

// Implements OmniboxView on an AutocompleteTextField.
class OmniboxViewMac : public OmniboxView,
                       public AutocompleteTextFieldObserver {
 public:
  static SkColor BaseTextColorSkia(bool in_dark_mode);
  static NSColor* BaseTextColor(bool in_dark_mode);
  // Returns a color representing |security_level|, adjusted based on whether
  // the browser is in Incognito mode.
  static NSColor* GetSecureTextColor(
      security_state::SecurityLevel security_level,
      bool in_dark_mode);

  OmniboxViewMac(OmniboxEditController* controller,
                 Profile* profile,
                 CommandUpdater* command_updater,
                 AutocompleteTextField* field);
  ~OmniboxViewMac() override;

  // For use when switching tabs, this saves the current state onto the tab so
  // that it can be restored during a later call to Update().
  void SaveStateToTab(content::WebContents* tab);

  // Called when the window's active tab changes.
  void OnTabChanged(const content::WebContents* web_contents);

  // Called to clear the saved state for |web_contents|.
  void ResetTabState(content::WebContents* web_contents);

  // OmniboxView:
  void Update() override;
  void OpenMatch(const AutocompleteMatch& match,
                 WindowOpenDisposition disposition,
                 const GURL& alternate_nav_url,
                 const base::string16& pasted_text,
                 size_t selected_line) override;
  base::string16 GetText() const override;
  void SetWindowTextAndCaretPos(const base::string16& text,
                                size_t caret_pos,
                                bool update_popup,
                                bool notify_text_changed) override;
  void SetCaretPos(size_t caret_pos) override;
  void EnterKeywordModeForDefaultSearchProvider() override;
  bool IsSelectAll() const override;
  void GetSelectionBounds(base::string16::size_type* start,
                          base::string16::size_type* end) const override;
  void SelectAll(bool reversed) override;
  void RevertAll() override;
  void UpdatePopup() override;
  void CloseOmniboxPopup() override;
  void SetFocus() override;
  void ApplyCaretVisibility() override;
  void OnTemporaryTextMaybeChanged(const base::string16& display_text,
                                   const AutocompleteMatch& match,
                                   bool save_original_selection,
                                   bool notify_text_changed) override;
  bool OnInlineAutocompleteTextMaybeChanged(const base::string16& display_text,
                                            size_t user_text_length) override;
  void OnInlineAutocompleteTextCleared() override;
  void OnRevertTemporaryText() override;
  void OnBeforePossibleChange() override;
  bool OnAfterPossibleChange(bool allow_keyword_ui_change) override;
  gfx::NativeView GetNativeView() const override;
  gfx::NativeView GetRelativeWindowForPopup() const override;
  int GetTextWidth() const override;
  int GetWidth() const override;
  bool IsImeComposing() const override;

  // Implement the AutocompleteTextFieldObserver interface.
  NSRange SelectionRangeForProposedRange(NSRange proposed_range) override;
  void OnControlKeyChanged(bool pressed) override;
  bool CanCopy() override;
  base::scoped_nsobject<NSPasteboardItem> CreatePasteboardItem() override;
  void CopyToPasteboard(NSPasteboard* pboard) override;
  void OnPaste() override;
  bool CanPasteAndGo() override;
  int GetPasteActionStringId() override;
  void OnPasteAndGo() override;
  void OnFrameChanged() override;
  void ClosePopup() override;
  void OnDidBeginEditing() override;
  void OnBeforeChange() override;
  void OnDidChange() override;
  void OnDidEndEditing() override;
  void OnInsertText() override;
  void OnBeforeDrawRect() override;
  void OnDidDrawRect() override;
  bool OnDoCommandBySelector(SEL cmd) override;
  void OnSetFocus(bool control_down) override;
  void OnKillFocus() override;
  void OnMouseDown(NSInteger button_number) override;

  // Helper for LocationBarViewMac.  Optionally selects all in |field_|.
  void FocusLocation(bool select_all);

  // Helper to get the font to use in the field, exposed for the
  // popup.
  static NSFont* GetNormalFieldFont();
  static NSFont* GetBoldFieldFont();
  static NSFont* GetLargeFont();
  static NSFont* GetSmallFont();

  // If |resource_id| has a PDF image which can be used, return it.
  // Otherwise return the PNG image from the resource bundle.
  static NSImage* ImageForResource(int resource_id);

  // Color used to draw suggest text.
  static NSColor* SuggestTextColor();

  AutocompleteTextField* field() const { return field_; }

 private:
  FRIEND_TEST_ALL_PREFIXES(OmniboxViewMacTest, WritingDirectionLTR);
  FRIEND_TEST_ALL_PREFIXES(OmniboxViewMacTest, WritingDirectionRTL);
  // Called when the user hits backspace in |field_|.  Checks whether
  // keyword search is being terminated.  Returns true if the
  // backspace should be intercepted (not forwarded on to the standard
  // machinery).
  bool OnBackspacePressed();

  // Returns the field's currently selected range.  Only valid if the
  // field has focus.
  NSRange GetSelectedRange() const;

  // Returns the field's currently marked range. Only valid if the field has
  // focus.
  NSRange GetMarkedRange() const;

  // Returns true if |field_| is first-responder in the window.  Used
  // in various DCHECKS to make sure code is running in appropriate
  // situations.
  bool IsFirstResponder() const;

  // If |model_| believes it has focus, grab focus if needed and set
  // the selection to |range|.  Otherwise does nothing.
  void SetSelectedRange(const NSRange range);

  // Update the field with |display_text| and highlight the host and scheme (if
  // it's an URL or URL-fragment).  Resets any suggest text that may be present.
  void SetText(const base::string16& display_text);

  // Internal implementation of SetText.  Does not reset the suggest text before
  // setting the display text.  Most callers should use |SetText()| instead.
  void SetTextInternal(const base::string16& display_text);

  // Update the field with |display_text| and set the selection.
  void SetTextAndSelectedRange(const base::string16& display_text,
                               const NSRange range);

  // Pass the current content of |field_| to SetText(), maintaining
  // any selection.  Named to be consistent with GTK and Windows,
  // though here we cannot really do the in-place operation they do.
  void EmphasizeURLComponents() override;

  // Apply our font and paragraph style to |attributedString|.
  void ApplyTextStyle(NSMutableAttributedString* attributedString);

  // Calculates text attributes according to |display_text| and applies them
  // to the given |attributed_string| object.
  void ApplyTextAttributes(const base::string16& display_text,
                           NSMutableAttributedString* attributed_string);

  // OmniboxView:
  void SetEmphasis(bool emphasize, const gfx::Range& range) override;
  void UpdateSchemeStyle(const gfx::Range& scheme_range) override;

  // Return the number of UTF-16 units in the current buffer, excluding the
  // suggested text.
  int GetOmniboxTextLength() const override;
  NSUInteger GetTextLength() const;

  // Returns true if the caret is at the end of the content.
  bool IsCaretAtEnd() const;

  // Announce that an inline autocomplete is available for screenreaders.
  void AnnounceAutocompleteForScreenReader(const base::string16& text);

  Profile* profile_;

  std::unique_ptr<OmniboxPopupView> popup_view_;

  AutocompleteTextField* field_;  // owned by tab controller

  // Selection at the point where the user started using the
  // arrows to move around in the popup.
  NSRange saved_temporary_selection_;

  // Tracking state before and after a possible change for reporting
  // to model_.
  State state_before_change_;
  NSRange marked_range_before_change_;

  // Was delete pressed?
  bool delete_was_pressed_;

  // State used to coalesce changes to text and selection to avoid drawing
  // transient state.
  bool in_coalesced_update_block_;
  bool do_coalesced_text_update_;
  base::string16 coalesced_text_update_;
  bool do_coalesced_range_update_;
  NSRange coalesced_range_update_;

  // The time of the first character insert operation that has not yet been
  // painted. Used to measure omnibox responsiveness with a histogram.
  base::TimeTicks insert_char_time_;

  // The time when OnBeforeDrawRect() was called.
  base::TimeTicks draw_rect_start_time_;

  // Temporary pointer to the attributed display string, stored as color and
  // other emphasis attributes are applied by the superclass.
  NSMutableAttributedString* attributing_display_string_;  // weak

  DISALLOW_COPY_AND_ASSIGN(OmniboxViewMac);
};

#endif  // CHROME_BROWSER_UI_COCOA_OMNIBOX_OMNIBOX_VIEW_MAC_H_
