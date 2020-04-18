// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/omnibox/omnibox_view_mac.h"

#include <Carbon/Carbon.h>  // kVK_Return

#include "base/auto_reset.h"
#include "base/mac/foundation_util.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/autocomplete/chrome_autocomplete_scheme_classifier.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/search/search.h"
#include "chrome/browser/themes/theme_service.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/l10n_util.h"
#import "chrome/browser/ui/cocoa/location_bar/autocomplete_text_field_cell.h"
#import "chrome/browser/ui/cocoa/location_bar/autocomplete_text_field_editor.h"
#include "chrome/browser/ui/cocoa/omnibox/omnibox_popup_view_mac.h"
#include "chrome/browser/ui/omnibox/chrome_omnibox_client.h"
#include "chrome/browser/ui/omnibox/clipboard_utils.h"
#include "chrome/grit/generated_resources.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/omnibox_edit_controller.h"
#include "components/omnibox/browser/omnibox_field_trial.h"
#include "components/omnibox/browser/omnibox_popup_model.h"
#include "components/toolbar/toolbar_model.h"
#include "content/public/browser/web_contents.h"
#include "extensions/common/constants.h"
#import "skia/ext/skia_utils_mac.h"
#import "third_party/mozilla/NSPasteboard+Utils.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/clipboard_util_mac.h"
#import "ui/base/cocoa/cocoa_base_utils.h"
#import "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/font.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/rect.h"

using content::WebContents;

// Focus-handling between |field_| and model() is a bit subtle.
// Other platforms detect change of focus, which is inconvenient
// without subclassing NSTextField (even with a subclass, the use of a
// field editor may complicate things).
//
// model() doesn't actually do anything when it gains focus, it just
// initializes.  Visible activity happens only after the user edits.
// NSTextField delegate receives messages around starting and ending
// edits, so that suffices to catch focus changes.  Since all calls
// into model() start from OmniboxViewMac, in the worst case
// we can add code to sync up the sense of focus as needed.
//
// I've added DCHECK(IsFirstResponder()) in the places which I believe
// should only be reachable when |field_| is being edited.  If these
// fire, it probably means someone unexpected is calling into
// model().
//
// Other platforms don't appear to have the sense of "key window" that
// Mac does (I believe their fields lose focus when the window loses
// focus).  Rather than modifying focus outside the control's edit
// scope, when the window resigns key the autocomplete popup is
// closed.  model() still believes it has focus, and the popup will
// be regenerated on the user's next edit.  That seems to match how
// things work on other platforms.

namespace {
const int kOmniboxLargeFontSizeDelta = 9;
const int kOmniboxNormalFontSizeDelta = 1;
const int kOmniboxSmallMaterialFontSizeDelta = -1;

NSColor* HostTextColor(bool in_dark_mode) {
  return in_dark_mode ? [NSColor whiteColor] : [NSColor blackColor];
}
NSColor* SecureSchemeColor(bool in_dark_mode) {
  return in_dark_mode ? skia::SkColorToSRGBNSColor(SK_ColorWHITE)
                      : skia::SkColorToSRGBNSColor(gfx::kGoogleGreen700);
}
NSColor* SecurityErrorSchemeColor(bool in_dark_mode) {
  return in_dark_mode
      ? skia::SkColorToSRGBNSColor(SkColorSetA(SK_ColorWHITE, 0x7F))
      : skia::SkColorToSRGBNSColor(gfx::kGoogleRed700);
}

const char kOmniboxViewMacStateKey[] = "OmniboxViewMacState";

// Store's the model and view state across tab switches.
struct OmniboxViewMacState : public base::SupportsUserData::Data {
  OmniboxViewMacState(const OmniboxEditModel::State model_state,
                      const bool has_focus,
                      const NSRange& selection)
      : model_state(model_state),
        has_focus(has_focus),
        selection(selection) {
  }
  ~OmniboxViewMacState() override {}

  const OmniboxEditModel::State model_state;
  const bool has_focus;
  const NSRange selection;
};

// Accessors for storing and getting the state from the tab.
void StoreStateToTab(WebContents* tab,
                     std::unique_ptr<OmniboxViewMacState> state) {
  tab->SetUserData(kOmniboxViewMacStateKey, std::move(state));
}

const OmniboxViewMacState* GetStateFromTab(const WebContents* tab) {
  return static_cast<OmniboxViewMacState*>(
      tab->GetUserData(&kOmniboxViewMacStateKey));
}

}  // namespace

// static
NSImage* OmniboxViewMac::ImageForResource(int resource_id) {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  return rb.GetNativeImageNamed(resource_id).ToNSImage();
}

// static
NSColor* OmniboxViewMac::SuggestTextColor() {
  return [NSColor colorWithCalibratedWhite:0.0 alpha:0.5];
}

//static
SkColor OmniboxViewMac::BaseTextColorSkia(bool in_dark_mode) {
  return in_dark_mode ? SkColorSetA(SK_ColorWHITE, 0x7F)
                      : SkColorSetA(SK_ColorBLACK, 0x7F);
}

// static
NSColor* OmniboxViewMac::BaseTextColor(bool in_dark_mode) {
  return skia::SkColorToSRGBNSColor(BaseTextColorSkia(in_dark_mode));
}

// static
NSColor* OmniboxViewMac::GetSecureTextColor(
    security_state::SecurityLevel security_level,
    bool in_dark_mode) {
  if (security_level == security_state::EV_SECURE ||
      security_level == security_state::SECURE) {
    return SecureSchemeColor(in_dark_mode);
  }

  DCHECK_EQ(security_state::DANGEROUS, security_level);
  return SecurityErrorSchemeColor(in_dark_mode);
}

OmniboxViewMac::OmniboxViewMac(OmniboxEditController* controller,
                               Profile* profile,
                               CommandUpdater* command_updater,
                               AutocompleteTextField* field)
    : OmniboxView(
          controller,
          base::WrapUnique(new ChromeOmniboxClient(controller, profile))),
      profile_(profile),
      popup_view_(new OmniboxPopupViewMac(this, model(), field)),
      field_(field),
      saved_temporary_selection_(NSMakeRange(0, 0)),
      marked_range_before_change_(NSMakeRange(0, 0)),
      delete_was_pressed_(false),
      in_coalesced_update_block_(false),
      do_coalesced_text_update_(false),
      do_coalesced_range_update_(false),
      attributing_display_string_(nil) {
  [field_ setObserver:this];

  // Needed so that editing doesn't lose the styling.
  [field_ setAllowsEditingTextAttributes:YES];

  // Get the appropriate line height for the font that we use.
  base::scoped_nsobject<NSLayoutManager> layoutManager(
      [[NSLayoutManager alloc] init]);
  [layoutManager setUsesScreenFonts:YES];
}

OmniboxViewMac::~OmniboxViewMac() {
  // Destroy popup view before this object in case it tries to call us
  // back in the destructor.
  popup_view_.reset();

  // Disconnect from |field_|, it outlives this object.
  [field_ setObserver:NULL];
}

void OmniboxViewMac::SaveStateToTab(WebContents* tab) {
  DCHECK(tab);

  const bool hasFocus = [field_ currentEditor] ? true : false;

  NSRange range;
  if (hasFocus) {
    range = GetSelectedRange();
  } else {
    // If we are not focused, there is no selection.  Manufacture
    // something reasonable in case it starts to matter in the future.
    range = NSMakeRange(0, GetTextLength());
  }

  StoreStateToTab(tab, std::make_unique<OmniboxViewMacState>(
                           model()->GetStateForTabSwitch(), hasFocus, range));
}

void OmniboxViewMac::OnTabChanged(const WebContents* web_contents) {
  const OmniboxViewMacState* state = GetStateFromTab(web_contents);
  model()->RestoreState(state ? &state->model_state : NULL);
  // Restore focus and selection if they were present when the tab
  // was switched away.
  if (state && state->has_focus) {
    // TODO(shess): Unfortunately, there is no safe way to update
    // this because TabStripController -selectTabWithContents:* is
    // also messing with focus.  Both parties need to agree to
    // store existing state before anyone tries to setup the new
    // state.  Anyhow, it would look something like this.
#if 0
    [[field_ window] makeFirstResponder:field_];
    [[field_ currentEditor] setSelectedRange:state->selection];
#endif
  }
}

void OmniboxViewMac::ResetTabState(WebContents* web_contents) {
  StoreStateToTab(web_contents, nullptr);
}

void OmniboxViewMac::Update() {
  if (model()->ResetDisplayUrls()) {
    // Restore everything to the baseline look.
    RevertAll();

    // Only select all when we have focus. It's incorrect to have the
    // Omnibox text selected while unfocused, and we'll re-select it
    // when focus returns.
    if (model()->has_focus())
      SelectAll(true);
  } else {
    // TODO(shess): This corresponds to _win and _gtk, except those
    // guard it with a test for whether the security level changed.
    // But AFAICT, that can only change if the text changed, and that
    // code compares the toolbar model security level with the local
    // security level.  Dig in and figure out why this isn't a no-op
    // that should go away.
    EmphasizeURLComponents();
  }
}

void OmniboxViewMac::OpenMatch(const AutocompleteMatch& match,
                               WindowOpenDisposition disposition,
                               const GURL& alternate_nav_url,
                               const base::string16& pasted_text,
                               size_t selected_line) {
  // Coalesce text and selection updates from the following function. If we
  // don't do this, the user may see intermediate states as brief flickers.
  in_coalesced_update_block_ = true;
  OmniboxView::OpenMatch(
      match, disposition, alternate_nav_url, pasted_text, selected_line);
  in_coalesced_update_block_ = false;
  if (do_coalesced_text_update_) {
    SetText(coalesced_text_update_);
    // Ensure location bar icon is updated to reflect text.
    controller()->OnChanged();
  }
  do_coalesced_text_update_ = false;
  if (do_coalesced_range_update_)
    SetSelectedRange(coalesced_range_update_);
  do_coalesced_range_update_ = false;
}

base::string16 OmniboxViewMac::GetText() const {
  return base::SysNSStringToUTF16([field_ stringValue]);
}

NSRange OmniboxViewMac::GetSelectedRange() const {
  return [[field_ currentEditor] selectedRange];
}

NSRange OmniboxViewMac::GetMarkedRange() const {
  DCHECK([field_ currentEditor]);
  return [(NSTextView*)[field_ currentEditor] markedRange];
}

void OmniboxViewMac::SetSelectedRange(const NSRange range) {
  if (in_coalesced_update_block_) {
    do_coalesced_range_update_ = true;
    coalesced_range_update_ = range;
    return;
  }

  // This can be called when we don't have focus.  For instance, when
  // the user clicks the "Go" button.
  if (model()->has_focus()) {
    // TODO(shess): If model() thinks we have focus, this should not
    // be necessary.  Try to convert to DCHECK(IsFirstResponder()).
    if (![field_ currentEditor]) {
      [[field_ window] makeFirstResponder:field_];
    }

    // TODO(shess): What if it didn't get first responder, and there is
    // no field editor?  This will do nothing.  Well, at least it won't
    // crash.  Think of something more productive to do, or prove that
    // it cannot occur and DCHECK appropriately.
    [[field_ currentEditor] setSelectedRange:range];
  }
}

void OmniboxViewMac::SetWindowTextAndCaretPos(const base::string16& text,
                                              size_t caret_pos,
                                              bool update_popup,
                                              bool notify_text_changed) {
  DCHECK_LE(caret_pos, text.size());
  SetTextAndSelectedRange(text, NSMakeRange(caret_pos, 0));

  if (update_popup)
    UpdatePopup();

  if (notify_text_changed)
    TextChanged();
}

void OmniboxViewMac::SetCaretPos(size_t caret_pos) {
  size_t pos = std::min(caret_pos, GetTextLength());
  SetSelectedRange(NSMakeRange(pos, 0));
}

void OmniboxViewMac::EnterKeywordModeForDefaultSearchProvider() {
  // We need to do this first, else |SetSelectedRange()| won't work.
  FocusLocation(true);

  // Transition the user into keyword mode using their default search provider.
  model()->EnterKeywordModeForDefaultSearchProvider(
      KeywordModeEntryMethod::KEYBOARD_SHORTCUT);
}

bool OmniboxViewMac::IsSelectAll() const {
  if (![field_ currentEditor])
    return true;
  const NSRange all_range = NSMakeRange(0, GetTextLength());
  if (all_range.length == 0)
    return false;
  return NSEqualRanges(all_range, GetSelectedRange());
}

void OmniboxViewMac::GetSelectionBounds(base::string16::size_type* start,
                                        base::string16::size_type* end) const {
  if (![field_ currentEditor]) {
    *start = *end = 0;
    return;
  }

  const NSRange selected_range = GetSelectedRange();
  *start = static_cast<size_t>(selected_range.location);
  *end = static_cast<size_t>(NSMaxRange(selected_range));
}

void OmniboxViewMac::SelectAll(bool reversed) {
  if (!model()->has_focus())
    return;

  NSRange full_range = NSMakeRange(0, GetTextLength());

  // When coalescing updates, just set the range and not the direction. It's
  // unlikely that the direction will matter after OpenMatch() applies updates.
  if (in_coalesced_update_block_) {
    SetSelectedRange(full_range);
    return;
  }

  NSTextView* text_view =
      base::mac::ObjCCastStrict<NSTextView>([field_ currentEditor]);
  NSSelectionAffinity affinity =
      reversed ? NSSelectionAffinityUpstream : NSSelectionAffinityDownstream;

  [text_view setSelectedRange:full_range affinity:affinity stillSelecting:NO];
}

void OmniboxViewMac::RevertAll() {
  OmniboxView::RevertAll();
  [field_ clearUndoChain];
}

void OmniboxViewMac::UpdatePopup() {
  // Comment copied from OmniboxViewWin::UpdatePopup():
  // Don't inline autocomplete when:
  //   * The user is deleting text
  //   * The caret/selection isn't at the end of the text
  //   * The user has just pasted in something that replaced all the text
  //   * The user is trying to compose something in an IME
  bool prevent_inline_autocomplete = IsImeComposing();
  NSTextView* editor = (NSTextView*)[field_ currentEditor];
  if (editor) {
    if (NSMaxRange([editor selectedRange]) < [[editor textStorage] length])
      prevent_inline_autocomplete = true;
  }

  model()->UpdateInput([editor selectedRange].length != 0,
                       prevent_inline_autocomplete);
}

void OmniboxViewMac::CloseOmniboxPopup() {
  // Call both base class methods.
  ClosePopup();
  OmniboxView::CloseOmniboxPopup();
}

void OmniboxViewMac::SetFocus() {
  FocusLocation(false);
  model()->SetCaretVisibility(true);
}

void OmniboxViewMac::ApplyCaretVisibility() {
  [[field_ cell] setHideFocusState:!model()->is_caret_visible()
                            ofView:field_];
}

void OmniboxViewMac::SetText(const base::string16& display_text) {
  SetTextInternal(display_text);
}

void OmniboxViewMac::SetTextInternal(const base::string16& display_text) {
  if (in_coalesced_update_block_) {
    do_coalesced_text_update_ = true;
    coalesced_text_update_ = display_text;
    // Don't do any selection changes, since they apply to the previous text.
    do_coalesced_range_update_ = false;
    return;
  }

  NSString* ss = base::SysUTF16ToNSString(display_text);
  NSMutableAttributedString* attributedString =
      [[[NSMutableAttributedString alloc] initWithString:ss] autorelease];

  ApplyTextAttributes(display_text, attributedString);
  [field_ setAttributedStringValue:attributedString];

  // TODO(shess): This may be an appropriate place to call:
  //   model()->OnChanged();
  // In the current implementation, this tells LocationBarViewMac to
  // mess around with model() and update |field_|.  Unfortunately,
  // when I look at our peer implementations, it's not entirely clear
  // to me if this is safe.  SetTextInternal() is sort of an utility method,
  // and different callers sometimes have different needs.  Research
  // this issue so that it can be added safely.

  // TODO(shess): Also, consider whether this code couldn't just
  // manage things directly.  Windows uses a series of overlaid view
  // objects to accomplish the hinting stuff that OnChanged() does, so
  // it makes sense to have it in the controller that lays those
  // things out.  Mac instead pushes the support into a custom
  // text-field implementation.
}

void OmniboxViewMac::SetTextAndSelectedRange(const base::string16& display_text,
                                             const NSRange range) {
  SetText(display_text);
  SetSelectedRange(range);
}

void OmniboxViewMac::EmphasizeURLComponents() {
  NSTextView* editor = (NSTextView*)[field_ currentEditor];
  // If the autocomplete text field is in editing mode, then we can just change
  // its attributes through its editor. Otherwise, we simply reset its content.
  if (editor) {
    NSTextStorage* storage = [editor textStorage];
    [storage beginEditing];

    // Clear the existing attributes from the text storage, then
    // overlay the appropriate Omnibox attributes.
    [storage setAttributes:[NSDictionary dictionary]
                     range:NSMakeRange(0, [storage length])];
    ApplyTextAttributes(GetText(), storage);

    [storage endEditing];

    // This function can be called during the editor's -resignFirstResponder. If
    // that happens, |storage| and |field_| will not be synced automatically any
    // more. Calling -stringValue ensures that |field_| reflects the changes to
    // |storage|.
    [field_ stringValue];
  } else if (!in_coalesced_update_block_) {
    // Skip this if we're in a coalesced update block. Otherwise, the user text
    // entered can get set in a new tab because we haven't yet set the URL text.
    SetText(GetText());
  }
}

void OmniboxViewMac::ApplyTextStyle(
    NSMutableAttributedString* attributedString) {
  [attributedString addAttribute:NSFontAttributeName
                           value:GetNormalFieldFont()
                           range:NSMakeRange(0, [attributedString length])];

  // Make a paragraph style locking in the standard line height as the maximum,
  // otherwise the baseline may shift "downwards".
  base::scoped_nsobject<NSMutableParagraphStyle> paragraph_style(
      [[NSMutableParagraphStyle alloc] init]);
  CGFloat line_height = [[field_ cell] lineHeight];
  [paragraph_style setMaximumLineHeight:line_height];
  [paragraph_style setMinimumLineHeight:line_height];
  [paragraph_style setLineBreakMode:NSLineBreakByTruncatingTail];
  // Set an explicit alignment so it isn't implied from writing direction.
  [paragraph_style setAlignment:cocoa_l10n_util::ShouldDoExperimentalRTLLayout()
                                    ? NSRightTextAlignment
                                    : NSLeftTextAlignment];
  if (@available(macOS 10.11, *))
    [paragraph_style setAllowsDefaultTighteningForTruncation:NO];
  // If this is a URL, set the top-level paragraph direction to LTR (avoids RTL
  // characters from making the URL render from right to left, as per RFC 3987
  // Section 4.1).
  if (model()->CurrentTextIsURL())
    [paragraph_style setBaseWritingDirection:NSWritingDirectionLeftToRight];
  [attributedString addAttribute:NSParagraphStyleAttributeName
                           value:paragraph_style
                           range:NSMakeRange(0, [attributedString length])];
}

void OmniboxViewMac::SetEmphasis(bool emphasize, const gfx::Range& range) {
  bool in_dark_mode = [[field_ window] inIncognitoModeWithSystemTheme];

  NSRange ns_range = range.IsValid()
                         ? range.ToNSRange()
                         : NSMakeRange(0, [attributing_display_string_ length]);

  [attributing_display_string_
      addAttribute:NSForegroundColorAttributeName
             value:(emphasize) ? HostTextColor(in_dark_mode)
                               : BaseTextColor(in_dark_mode)
             range:ns_range];
}

void OmniboxViewMac::UpdateSchemeStyle(const gfx::Range& range) {
  if (!range.IsValid())
    return;

  const security_state::SecurityLevel security_level =
      controller()->GetToolbarModel()->GetSecurityLevel(false);

  if ((security_level == security_state::NONE) ||
      (security_level == security_state::HTTP_SHOW_WARNING))
    return;

  if (security_level == security_state::DANGEROUS) {
    // Add a strikethrough through the scheme.
    [attributing_display_string_
        addAttribute:NSStrikethroughStyleAttributeName
               value:[NSNumber numberWithInt:NSUnderlineStyleSingle]
               range:range.ToNSRange()];
  }

  bool in_dark_mode = [[field_ window] inIncognitoModeWithSystemTheme];

  [attributing_display_string_
      addAttribute:NSForegroundColorAttributeName
             value:GetSecureTextColor(security_level, in_dark_mode)
             range:range.ToNSRange()];
}

void OmniboxViewMac::ApplyTextAttributes(
    const base::string16& display_text,
    NSMutableAttributedString* attributed_string) {
  NSUInteger as_length = [attributed_string length];
  if (as_length == 0) {
    return;
  }

  ApplyTextStyle(attributed_string);

  // A kinda hacky way to add breaking at periods. This is what Safari does.
  // This works for IDNs too, despite the "en_US".
  [attributed_string addAttribute:@"NSLanguage"
                            value:@"en_US_POSIX"
                            range:NSMakeRange(0, as_length)];

  // Cache a pointer to the attributed string to allow the superclass'
  // virtual method invocations to add attributes.
  DCHECK(attributing_display_string_ == nil);
  base::AutoReset<NSMutableAttributedString*> resetter(
      &attributing_display_string_, attributed_string);
  UpdateTextStyle(display_text, model()->CurrentTextIsURL(),
                  ChromeAutocompleteSchemeClassifier(profile_));
}

void OmniboxViewMac::OnTemporaryTextMaybeChanged(
    const base::string16& display_text,
    const AutocompleteMatch& match,
    bool save_original_selection,
    bool notify_text_changed) {
  if (save_original_selection)
    saved_temporary_selection_ = GetSelectedRange();

  SetWindowTextAndCaretPos(display_text, display_text.size(), false, false);
  if (notify_text_changed)
    model()->OnChanged();
  [field_ clearUndoChain];

  // Get friendly accessibility label.
  AnnounceAutocompleteForScreenReader(
      AutocompleteMatchType::ToAccessibilityLabel(
          match, display_text, model()->popup_model()->selected_line(),
          model()->result().size()));
}

bool OmniboxViewMac::OnInlineAutocompleteTextMaybeChanged(
    const base::string16& display_text,
    size_t user_text_length) {
  // TODO(shess): Make sure that this actually works.  The round trip
  // to native form and back may mean that it's the same but not the
  // same.
  if (display_text == GetText())
    return false;

  DCHECK_LE(user_text_length, display_text.size());
  const NSRange range =
      NSMakeRange(user_text_length, display_text.size() - user_text_length);
  SetTextAndSelectedRange(display_text, range);
  model()->OnChanged();
  [field_ clearUndoChain];

  AnnounceAutocompleteForScreenReader(display_text);

  return true;
}

void OmniboxViewMac::OnInlineAutocompleteTextCleared() {
}

void OmniboxViewMac::OnRevertTemporaryText() {
  SetSelectedRange(saved_temporary_selection_);
  // We got here because the user hit the Escape key. We explicitly don't call
  // TextChanged(), since OmniboxPopupModel::ResetToDefaultMatch() has already
  // been called by now, and it would've called TextChanged() if it was
  // warranted.
}

bool OmniboxViewMac::IsFirstResponder() const {
  return [field_ currentEditor] != nil ? true : false;
}

void OmniboxViewMac::OnBeforePossibleChange() {
  // We should only arrive here when the field is focused.
  DCHECK(IsFirstResponder());

  GetState(&state_before_change_);
  marked_range_before_change_ = GetMarkedRange();
}

bool OmniboxViewMac::OnAfterPossibleChange(bool allow_keyword_ui_change) {
  // We should only arrive here when the field is focused.
  DCHECK(IsFirstResponder());

  State new_state;
  GetState(&new_state);
  OmniboxView::StateChanges state_changes =
      GetStateChanges(state_before_change_, new_state);

  const bool something_changed = model()->OnAfterPossibleChange(
      state_changes, allow_keyword_ui_change && !IsImeComposing());

  // Restyle in case the user changed something.
  // TODO(shess): I believe there are multiple-redraw cases, here.
  // Linux watches for something_changed && text_differs, but that
  // fails for us in case you copy the URL and paste the identical URL
  // back (we'll lose the styling).
  TextChanged();

  delete_was_pressed_ = false;

  return something_changed;
}

gfx::NativeView OmniboxViewMac::GetNativeView() const {
  return field_;
}

gfx::NativeView OmniboxViewMac::GetRelativeWindowForPopup() const {
  // Not used on mac.
  NOTREACHED();
  return NULL;
}

int OmniboxViewMac::GetTextWidth() const {
  // Not used on mac.
  NOTREACHED();
  return 0;
}

int OmniboxViewMac::GetWidth() const {
  return ceil([field_ bounds].size.width);
}

bool OmniboxViewMac::IsImeComposing() const {
  return [(NSTextView*)[field_ currentEditor] hasMarkedText];
}

void OmniboxViewMac::OnDidBeginEditing() {
  // We should only arrive here when the field is focused.
  DCHECK([field_ currentEditor]);
}

void OmniboxViewMac::OnBeforeChange() {
  // Capture the current state.
  OnBeforePossibleChange();
}

void OmniboxViewMac::OnDidChange() {
  // Figure out what changed and notify the model.
  OnAfterPossibleChange(true);
}

void OmniboxViewMac::OnDidEndEditing() {
  ClosePopup();
}

void OmniboxViewMac::OnInsertText() {
  // If |insert_char_time_| is not null, there's a pending insert char operation
  // that hasn't been painted yet. Keep the earlier time.
  if (insert_char_time_.is_null())
    insert_char_time_ = base::TimeTicks::Now();
}

void OmniboxViewMac::OnBeforeDrawRect() {
  if (!insert_char_time_.is_null()) {
    UMA_HISTOGRAM_TIMES("Omnibox.CharTypedToRepaintLatency.ToPaint",
                        base::TimeTicks::Now() - insert_char_time_);
  }
  draw_rect_start_time_ = base::TimeTicks::Now();
}

void OmniboxViewMac::OnDidDrawRect() {
  base::TimeTicks now = base::TimeTicks::Now();
  UMA_HISTOGRAM_TIMES("Omnibox.PaintTime", now - draw_rect_start_time_);
  if (!insert_char_time_.is_null()) {
    UMA_HISTOGRAM_TIMES("Omnibox.CharTypedToRepaintLatency",
                        now - insert_char_time_);
    insert_char_time_ = base::TimeTicks();
  }
}

bool OmniboxViewMac::OnDoCommandBySelector(SEL cmd) {
  if (cmd == @selector(deleteForward:))
    delete_was_pressed_ = true;

  if (cmd == @selector(moveDown:)) {
    model()->OnUpOrDownKeyPressed(1);
    return true;
  }

  if (cmd == @selector(moveUp:)) {
    model()->OnUpOrDownKeyPressed(-1);
    return true;
  }

  if (model()->popup_model()->IsOpen()) {
    if (cmd == @selector(insertBacktab:)) {
      if (model()->popup_model()->selected_line_state() ==
            OmniboxPopupModel::KEYWORD) {
        model()->ClearKeyword();
        return true;
      } else {
        model()->OnUpOrDownKeyPressed(-1);
        return true;
      }
    }

    if ((cmd == @selector(insertTab:) ||
        cmd == @selector(insertTabIgnoringFieldEditor:)) &&
        !model()->is_keyword_hint()) {
      model()->OnUpOrDownKeyPressed(1);
      return true;
    }
  }

  if (cmd == @selector(scrollPageDown:)) {
    model()->OnUpOrDownKeyPressed(model()->result().size());
    return true;
  }

  if (cmd == @selector(scrollPageUp:)) {
    model()->OnUpOrDownKeyPressed(-model()->result().size());
    return true;
  }

  if (cmd == @selector(cancelOperation:)) {
    return model()->OnEscapeKeyPressed();
  }

  if ((cmd == @selector(insertTab:) ||
      cmd == @selector(insertTabIgnoringFieldEditor:)) &&
      model()->is_keyword_hint()) {
    return model()->AcceptKeyword(KeywordModeEntryMethod::TAB);
  }

  // |-noop:| is sent when the user presses Cmd+Return. Override the no-op
  // behavior with the proper WindowOpenDisposition.
  NSEvent* event = [NSApp currentEvent];
  if (([event type] == NSKeyDown || [event type] == NSKeyUp) &&
      [event keyCode] == kVK_Shift) {
    OnShiftKeyChanged([event type] == NSKeyDown);
    return true;
  }
  if (cmd == @selector(insertNewline:) ||
     (cmd == @selector(noop:) &&
      ([event type] == NSKeyDown || [event type] == NSKeyUp) &&
      [event keyCode] == kVK_Return)) {
    // If the user hasn't entered any text in keyword search mode, we need to
    // return early in order to avoid cancelling the search.
    if (GetTextLength() == 0)
      return true;

    WindowOpenDisposition disposition =
        ui::WindowOpenDispositionFromNSEvent(event);
    model()->AcceptInput(disposition, false);
    // Opening a URL in a background tab should also revert the omnibox contents
    // to their original state.  We cannot do a blanket revert in OpenURL()
    // because middle-clicks also open in a new background tab, but those should
    // not revert the omnibox text.
    RevertAll();
    return true;
  }

  // Option-Return
  if (cmd == @selector(insertNewlineIgnoringFieldEditor:)) {
    model()->AcceptInput(WindowOpenDisposition::NEW_FOREGROUND_TAB, false);
    return true;
  }

  // When the user does Control-Enter, the existing content has "www."
  // prepended and ".com" appended.  model() should already have
  // received notification when the Control key was depressed, but it
  // is safe to tell it twice.
  if (cmd == @selector(insertLineBreak:)) {
    OnControlKeyChanged(true);
    WindowOpenDisposition disposition =
        ui::WindowOpenDispositionFromNSEvent([NSApp currentEvent]);
    model()->AcceptInput(disposition, false);
    return true;
  }

  if (cmd == @selector(deleteBackward:)) {
    if (OnBackspacePressed()) {
      return true;
    }
  }

  if (cmd == @selector(deleteForward:)) {
    const NSUInteger modifiers = [[NSApp currentEvent] modifierFlags];
    if ((modifiers & NSShiftKeyMask) != 0) {
      if (model()->popup_model()->IsOpen()) {
        model()->popup_model()->TryDeletingCurrentItem();
        return true;
      }
    }
  }

  return false;
}

void OmniboxViewMac::OnSetFocus(bool control_down) {
  model()->OnSetFocus(control_down);
  // Update the keyword and search hint states.
  controller()->OnChanged();
}

void OmniboxViewMac::OnKillFocus() {
  OnShiftKeyChanged(false);
  // Tell the model to reset itself.
  model()->OnWillKillFocus();
  model()->OnKillFocus();
}

void OmniboxViewMac::OnMouseDown(NSInteger button_number) {
  // Restore caret visibility whenever the user clicks in the the omnibox. This
  // is not always covered by OnSetFocus() because when clicking while the
  // omnibox has invisible focus does not trigger a new OnSetFocus() call.
  if (button_number == 0 || button_number == 1)
    model()->SetCaretVisibility(true);
}

bool OmniboxViewMac::CanCopy() {
  const NSRange selection = GetSelectedRange();
  return selection.length > 0;
}

base::scoped_nsobject<NSPasteboardItem> OmniboxViewMac::CreatePasteboardItem() {
  DCHECK(CanCopy());

  const NSRange selection = GetSelectedRange();
  base::string16 text = base::SysNSStringToUTF16(
      [[field_ stringValue] substringWithRange:selection]);

  // Copy the URL.
  GURL url;
  bool write_url = false;
  model()->AdjustTextForCopy(selection.location, &text, &url, &write_url);

  if (IsSelectAll())
    UMA_HISTOGRAM_COUNTS(OmniboxEditModel::kCutOrCopyAllTextHistogram, 1);

  NSString* nstext = base::SysUTF16ToNSString(text);
  if (write_url) {
    return ui::ClipboardUtil::PasteboardItemFromUrl(
        base::SysUTF8ToNSString(url.spec()), nstext);
  } else {
    return ui::ClipboardUtil::PasteboardItemFromString(nstext);
  }
}

void OmniboxViewMac::CopyToPasteboard(NSPasteboard* pboard) {
  [pboard clearContents];
  base::scoped_nsobject<NSPasteboardItem> item(CreatePasteboardItem());
  [pboard writeObjects:@[ item.get() ]];
}

void OmniboxViewMac::OnPaste() {
  // This code currently expects |field_| to be focused.
  DCHECK([field_ currentEditor]);

  base::string16 text = GetClipboardText();
  if (text.empty()) {
    return;
  }
  NSString* s = base::SysUTF16ToNSString(text);

  // -shouldChangeTextInRange:* and -didChangeText are documented in
  // NSTextView as things you need to do if you write additional
  // user-initiated editing functions.  They cause the appropriate
  // delegate methods to be called.
  // TODO(shess): It would be nice to separate the Cocoa-specific code
  // from the Chrome-specific code.
  NSTextView* editor = static_cast<NSTextView*>([field_ currentEditor]);
  const NSRange selectedRange = GetSelectedRange();
  if ([editor shouldChangeTextInRange:selectedRange replacementString:s]) {
    // Record this paste, so we can do different behavior.
    model()->OnPaste();

    // Force a Paste operation to trigger the text_changed code in
    // OnAfterPossibleChange(), even if identical contents are pasted
    // into the text box.
    state_before_change_.text.clear();

    [editor replaceCharactersInRange:selectedRange withString:s];
    [editor didChangeText];
  }
}

bool OmniboxViewMac::CanPasteAndGo() {
  return model()->CanPasteAndGo(GetClipboardText());
}

int OmniboxViewMac::GetPasteActionStringId() {
  base::string16 text(GetClipboardText());
  DCHECK(model()->CanPasteAndGo(text));
  return model()->ClassifiesAsSearch(text) ? IDS_PASTE_AND_SEARCH
                                           : IDS_PASTE_AND_GO;
}

void OmniboxViewMac::OnPasteAndGo() {
  base::string16 text(GetClipboardText());
  if (model()->CanPasteAndGo(text))
    model()->PasteAndGo(text);
}

void OmniboxViewMac::OnFrameChanged() {
  // TODO(shess): UpdatePopupAppearance() is called frequently, so it
  // should be really cheap, but in this case we could probably make
  // things even cheaper by refactoring between the popup-placement
  // code and the matrix-population code.
  popup_view_->UpdatePopupAppearance();

  // Give controller a chance to rearrange decorations.
  model()->OnChanged();
}

void OmniboxViewMac::ClosePopup() {
  OmniboxView::CloseOmniboxPopup();
}

bool OmniboxViewMac::OnBackspacePressed() {
  // Don't intercept if not in keyword search mode.
  if (model()->is_keyword_hint() || model()->keyword().empty()) {
    return false;
  }

  // Don't intercept if there is a selection, or the cursor isn't at
  // the leftmost position.
  const NSRange selection = GetSelectedRange();
  if (selection.length > 0 || selection.location > 0) {
    return false;
  }

  // We're showing a keyword and the user pressed backspace at the
  // beginning of the text.  Delete the selected keyword.
  model()->ClearKeyword();
  return true;
}

NSRange OmniboxViewMac::SelectionRangeForProposedRange(NSRange proposed_range) {
  return proposed_range;
}

void OmniboxViewMac::OnControlKeyChanged(bool pressed) {
  model()->OnControlKeyChanged(pressed);
}

void OmniboxViewMac::FocusLocation(bool select_all) {
  if ([field_ isEditable]) {
    // If the text field has a field editor, it's the first responder, meaning
    // that it's already focused. makeFirstResponder: will select all, so only
    // call it if this behavior is desired.
    if (select_all || ![field_ currentEditor])
      [[field_ window] makeFirstResponder:field_];
    DCHECK_EQ([field_ currentEditor], [[field_ window] firstResponder]);
  }
}

// static
NSFont* OmniboxViewMac::GetNormalFieldFont() {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  return rb
      .GetFontWithDelta(kOmniboxNormalFontSizeDelta, gfx::Font::NORMAL,
                        gfx::Font::Weight::NORMAL)
      .GetNativeFont();
}

NSFont* OmniboxViewMac::GetBoldFieldFont() {
  // Request a bold font, then make it larger. ResourceBundle will do the
  // opposite which makes a large system normal font a non-system bold font.
  // That gives a different baseline to making the non-system bold font larger.
  // And while the omnibox locks the baseline in ApplyTextStyle(),
  // OmniboxPopupCellData does not.
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  return rb
      .GetFontWithDelta(0, gfx::Font::NORMAL, gfx::Font::Weight::BOLD)
      .Derive(kOmniboxNormalFontSizeDelta, gfx::Font::NORMAL,
              gfx::Font::Weight::BOLD)
      .GetNativeFont();
}

NSFont* OmniboxViewMac::GetLargeFont() {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  return rb
      .GetFontWithDelta(kOmniboxLargeFontSizeDelta, gfx::Font::NORMAL,
                        gfx::Font::Weight::NORMAL)
      .GetNativeFont();
}

NSFont* OmniboxViewMac::GetSmallFont() {
  return ui::ResourceBundle::GetSharedInstance()
      .GetFontWithDelta(kOmniboxSmallMaterialFontSizeDelta, gfx::Font::NORMAL,
                        gfx::Font::Weight::NORMAL)
      .GetNativeFont();
}

int OmniboxViewMac::GetOmniboxTextLength() const {
  return static_cast<int>(GetTextLength());
}

NSUInteger OmniboxViewMac::GetTextLength() const {
  return [field_ currentEditor] ?  [[[field_ currentEditor] string] length] :
                                   [[field_ stringValue] length];
}

bool OmniboxViewMac::IsCaretAtEnd() const {
  const NSRange selection = GetSelectedRange();
  return NSMaxRange(selection) == GetTextLength();
}

void OmniboxViewMac::AnnounceAutocompleteForScreenReader(
    const base::string16& display_text) {
  NSDictionary* notification_info = @{
    NSAccessibilityAnnouncementKey : base::SysUTF16ToNSString(display_text),
    NSAccessibilityPriorityKey : @(NSAccessibilityPriorityHigh)
  };
  // We direct the screen reader to announce the friendly text.
  NSAccessibilityPostNotificationWithUserInfo(
      [field_ window],
      NSAccessibilityAnnouncementRequestedNotification,
      notification_info);
}
