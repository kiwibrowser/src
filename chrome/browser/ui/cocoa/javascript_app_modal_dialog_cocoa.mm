// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/javascript_app_modal_dialog_cocoa.h"

#import <Cocoa/Cocoa.h>
#include <stddef.h>

#include "base/i18n/rtl.h"
#include "base/logging.h"
#import "base/mac/foundation_util.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/sys_string_conversions.h"
#import "chrome/browser/chrome_browser_application_mac.h"
#include "chrome/browser/ui/blocked_content/popunder_preventer.h"
#include "chrome/browser/ui/javascript_dialogs/chrome_javascript_native_dialog_factory.h"
#include "components/app_modal/javascript_app_modal_dialog.h"
#include "components/app_modal/javascript_dialog_manager.h"
#include "components/app_modal/javascript_native_dialog_factory.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/ui_base_types.h"
#include "ui/gfx/text_elider.h"
#include "ui/strings/grit/ui_strings.h"

namespace {

const int kSlotsPerLine = 50;
const int kMessageTextMaxSlots = 2000;

}  // namespace

// Helper object that receives the notification that the dialog/sheet is
// going away. Is responsible for cleaning itself up.
@interface JavaScriptAppModalDialogHelper : NSObject<NSAlertDelegate> {
 @private
  base::scoped_nsobject<NSAlert> alert_;
  JavaScriptAppModalDialogCocoa* nativeDialog_;  // Weak.
  base::scoped_nsobject<NSTextField> textField_;
  BOOL alertShown_;
}

// Creates an NSAlert if one does not already exist. Otherwise returns the
// existing NSAlert.
- (NSAlert*)alert;
- (void)addTextFieldWithPrompt:(NSString*)prompt;

// Presents an AppKit blocking dialog.
- (void)showAlert;

// Selects the first button of the alert, which should accept it.
- (void)acceptAlert;

// Selects the second button of the alert, which should cancel it.
- (void)cancelAlert;

// Closes the window, and the alert along with it.
- (void)closeWindow;

// Designated initializer.
- (instancetype)initWithNativeDialog:(JavaScriptAppModalDialogCocoa*)dialog;

@end

@implementation JavaScriptAppModalDialogHelper

- (instancetype)init {
  NOTREACHED();
  return nil;
}

- (instancetype)initWithNativeDialog:(JavaScriptAppModalDialogCocoa*)dialog {
  DCHECK(dialog);
  self = [super init];
  if (self)
    nativeDialog_ = dialog;
  return self;
}

- (NSAlert*)alert {
  if (!alert_) {
    alert_.reset([[NSAlert alloc] init]);
    if (!nativeDialog_->dialog()->is_before_unload_dialog()) {
      // Set a blank icon for dialogs with text provided by the page.
      // "onbeforeunload" dialogs don't have text provided by the page, so it's
      // OK to use the app icon.
      NSImage* image =
          [[[NSImage alloc] initWithSize:NSMakeSize(1, 1)] autorelease];
      [alert_ setIcon:image];
    }
  }
  return alert_;
}

- (void)addTextFieldWithPrompt:(NSString*)prompt {
  DCHECK(!textField_);
  textField_.reset(
      [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 300, 22)]);
  [[textField_ cell] setLineBreakMode:NSLineBreakByTruncatingTail];
  [[self alert] setAccessoryView:textField_];
  [[alert_ window] setInitialFirstResponder:textField_];

  [textField_ setStringValue:prompt];
}

// |contextInfo| is the JavaScriptAppModalDialogCocoa that owns us.
- (void)alertDidEnd:(NSAlert*)alert
         returnCode:(int)returnCode
        contextInfo:(void*)contextInfo {
  switch (returnCode) {
    case NSAlertFirstButtonReturn:  {  // OK
      [self sendAcceptToNativeDialog];
      break;
    }
    case NSAlertSecondButtonReturn:  {  // Cancel
      // If the user wants to stay on this page, stop quitting (if a quit is in
      // progress).
      [self sendCancelToNativeDialog];
      break;
    }
    case NSRunStoppedResponse: {  // Window was closed underneath us
      // Need to call OnClose() because there is some cleanup that needs
      // to be done.  It won't call back to the javascript since the
      // JavaScriptAppModalDialog knows that the WebContents was destroyed.
      [self sendCloseToNativeDialog];
      break;
    }
    default:  {
      NOTREACHED();
    }
  }
}

- (void)showAlert {
  DCHECK(nativeDialog_);
  DCHECK(!alertShown_);
  alertShown_ = YES;
  NSAlert* alert = [self alert];

  [alert layout];
  [[alert window] recalculateKeyViewLoop];

  [alert beginSheetModalForWindow:nil  // nil here makes it app-modal
                    modalDelegate:self
                   didEndSelector:@selector(alertDidEnd:returnCode:contextInfo:)
                      contextInfo:NULL];
}

- (void)acceptAlert {
  DCHECK(nativeDialog_);
  if (!alertShown_) {
    [self sendAcceptToNativeDialog];
    return;
  }
  NSButton* first = [[[self alert] buttons] objectAtIndex:0];
  [first performClick:nil];
}

- (void)cancelAlert {
  DCHECK(nativeDialog_);
  if (!alertShown_) {
    [self sendCancelToNativeDialog];
    return;
  }
  DCHECK_GE([[[self alert] buttons] count], 2U);
  NSButton* second = [[[self alert] buttons] objectAtIndex:1];
  [second performClick:nil];
}

- (void)closeWindow {
  DCHECK(nativeDialog_);
  if (!alertShown_) {
    [self sendCloseToNativeDialog];
    return;
  }
  [NSApp endSheet:[[self alert] window]];
}

- (void)sendAcceptToNativeDialog {
  DCHECK(nativeDialog_);
  nativeDialog_->dialog()->OnAccept([self input], [self shouldSuppress]);
  [self destroyNativeDialog];
}

- (void)sendCancelToNativeDialog {
  DCHECK(nativeDialog_);
  // If the user wants to stay on this page, stop quitting (if a quit is in
  // progress).
  if (nativeDialog_->dialog()->is_before_unload_dialog())
    chrome_browser_application_mac::CancelTerminate();
  nativeDialog_->dialog()->OnCancel([self shouldSuppress]);
  [self destroyNativeDialog];
}

- (void)sendCloseToNativeDialog {
  DCHECK(nativeDialog_);
  nativeDialog_->dialog()->OnClose();
  [self destroyNativeDialog];
}

- (void)destroyNativeDialog {
  DCHECK(nativeDialog_);
  JavaScriptAppModalDialogCocoa* nativeDialog = nativeDialog_;
  nativeDialog_ = nil;  // Need to fail on DCHECK if something wrong happens.
  delete nativeDialog;  // Careful, this will delete us.
}

- (base::string16)input {
  if (textField_)
    return base::SysNSStringToUTF16([textField_ stringValue]);
  return base::string16();
}

- (bool)shouldSuppress {
  if ([[self alert] showsSuppressionButton])
    return [[[self alert] suppressionButton] state] == NSOnState;
  return false;
}

@end

////////////////////////////////////////////////////////////////////////////////
// JavaScriptAppModalDialogCocoa, public:

JavaScriptAppModalDialogCocoa::JavaScriptAppModalDialogCocoa(
    app_modal::JavaScriptAppModalDialog* dialog)
    : dialog_(dialog),
      popunder_preventer_(new PopunderPreventer(dialog->web_contents())),
      is_showing_(false) {
  // Determine the names of the dialog buttons based on the flags. "Default"
  // is the OK button. "Other" is the cancel button. We don't use the
  // "Alternate" button in NSRunAlertPanel.
  NSString* default_button = l10n_util::GetNSStringWithFixup(IDS_APP_OK);
  NSString* other_button = l10n_util::GetNSStringWithFixup(IDS_APP_CANCEL);
  bool text_field = false;
  bool one_button = false;
  switch (dialog_->javascript_dialog_type()) {
    case content::JAVASCRIPT_DIALOG_TYPE_ALERT:
      one_button = true;
      break;
    case content::JAVASCRIPT_DIALOG_TYPE_CONFIRM:
      if (dialog_->is_before_unload_dialog()) {
        if (dialog_->is_reload()) {
          default_button = l10n_util::GetNSStringWithFixup(
              IDS_BEFORERELOAD_MESSAGEBOX_OK_BUTTON_LABEL);
        } else {
          default_button = l10n_util::GetNSStringWithFixup(
              IDS_BEFOREUNLOAD_MESSAGEBOX_OK_BUTTON_LABEL);
        }
      }
      break;
    case content::JAVASCRIPT_DIALOG_TYPE_PROMPT:
      text_field = true;
      break;

    default:
      NOTREACHED();
  }

  // Create a helper which will receive the sheet ended selector. It will
  // delete itself when done.
  helper_.reset(
      [[JavaScriptAppModalDialogHelper alloc] initWithNativeDialog:this]);

  // Show the modal dialog.
  if (text_field) {
    [helper_ addTextFieldWithPrompt:base::SysUTF16ToNSString(
        dialog_->default_prompt_text())];
  }
  [GetAlert() setDelegate:helper_];
  NSString* informative_text =
      base::SysUTF16ToNSString(dialog_->message_text());

  // Truncate long JS alerts - crbug.com/331219
  NSCharacterSet* newline_char_set = [NSCharacterSet newlineCharacterSet];
  for (size_t index = 0, slots_count = 0; index < informative_text.length;
      ++index) {
    unichar current_char = [informative_text characterAtIndex:index];
    if ([newline_char_set characterIsMember:current_char])
      slots_count += kSlotsPerLine;
    else
      slots_count++;
    if (slots_count > kMessageTextMaxSlots) {
      base::string16 info_text = base::SysNSStringToUTF16(informative_text);
      informative_text = base::SysUTF16ToNSString(
          gfx::TruncateString(info_text, index, gfx::WORD_BREAK));
      break;
    }
  }

  [GetAlert() setInformativeText:informative_text];
  NSString* message_text =
      base::SysUTF16ToNSString(dialog_->title());
  [GetAlert() setMessageText:message_text];
  [GetAlert() addButtonWithTitle:default_button];
  if (!one_button) {
    NSButton* other = [GetAlert() addButtonWithTitle:other_button];
    [other setKeyEquivalent:@"\e"];
  }
  if (dialog_->display_suppress_checkbox()) {
    [GetAlert() setShowsSuppressionButton:YES];
    NSString* suppression_title = l10n_util::GetNSStringWithFixup(
        IDS_JAVASCRIPT_MESSAGEBOX_SUPPRESS_OPTION);
    [[GetAlert() suppressionButton] setTitle:suppression_title];
  }

  // Fix RTL dialogs.
  //
  // Mac OS X will always display NSAlert strings as LTR. A workaround is to
  // manually set the text as attributed strings in the implementing
  // NSTextFields. This is a basic correctness issue.
  //
  // In addition, for readability, the overall alignment is set based on the
  // directionality of the first strongly-directional character.
  //
  // If the dialog fields are selectable then they will scramble when clicked.
  // Therefore, selectability is disabled.
  //
  // See http://crbug.com/70806 for more details.

  bool message_has_rtl =
      base::i18n::StringContainsStrongRTLChars(dialog_->title());
  bool informative_has_rtl =
      base::i18n::StringContainsStrongRTLChars(dialog_->message_text());

  NSTextField* message_text_field = nil;
  NSTextField* informative_text_field = nil;
  if (message_has_rtl || informative_has_rtl) {
    // Force layout of the dialog. NSAlert leaves its dialog alone once laid
    // out; if this is not done then all the modifications that are to come will
    // be un-done when the dialog is finally displayed.
    [GetAlert() layout];

    // Locate the NSTextFields that implement the text display. These are
    // actually available as the ivars |_messageField| and |_informationField|
    // of the NSAlert, but it is safer (and more forward-compatible) to search
    // for them in the subviews.
    for (NSView* view in [[[GetAlert() window] contentView] subviews]) {
      NSTextField* text_field = base::mac::ObjCCast<NSTextField>(view);
      if ([[text_field stringValue] isEqualTo:message_text])
        message_text_field = text_field;
      else if ([[text_field stringValue] isEqualTo:informative_text])
        informative_text_field = text_field;
    }

    // This may fail in future OS releases, but it will still work for shipped
    // versions of Chromium.
    DCHECK(message_text_field);
    DCHECK(informative_text_field);
  }

  if (message_has_rtl && message_text_field) {
    base::scoped_nsobject<NSMutableParagraphStyle> alignment(
        [[NSParagraphStyle defaultParagraphStyle] mutableCopy]);
    [alignment setAlignment:NSRightTextAlignment];

    NSDictionary* alignment_attributes =
        @{ NSParagraphStyleAttributeName : alignment };
    base::scoped_nsobject<NSAttributedString> attr_string(
        [[NSAttributedString alloc] initWithString:message_text
                                        attributes:alignment_attributes]);

    [message_text_field setAttributedStringValue:attr_string];
    [message_text_field setSelectable:NO];
  }

  if (informative_has_rtl && informative_text_field) {
    base::i18n::TextDirection direction =
        base::i18n::GetFirstStrongCharacterDirection(dialog_->message_text());
    base::scoped_nsobject<NSMutableParagraphStyle> alignment(
        [[NSParagraphStyle defaultParagraphStyle] mutableCopy]);
    [alignment setAlignment:
        (direction == base::i18n::RIGHT_TO_LEFT) ? NSRightTextAlignment
                                                 : NSLeftTextAlignment];

    NSDictionary* alignment_attributes =
        @{ NSParagraphStyleAttributeName : alignment };
    base::scoped_nsobject<NSAttributedString> attr_string(
        [[NSAttributedString alloc] initWithString:informative_text
                                        attributes:alignment_attributes]);

    [informative_text_field setAttributedStringValue:attr_string];
    [informative_text_field setSelectable:NO];
  }
}

JavaScriptAppModalDialogCocoa::~JavaScriptAppModalDialogCocoa() {
  [NSObject cancelPreviousPerformRequestsWithTarget:helper_.get()];
}

////////////////////////////////////////////////////////////////////////////////
// JavaScriptAppModalDialogCocoa, private:

NSAlert* JavaScriptAppModalDialogCocoa::GetAlert() const {
  return [helper_ alert];
}

////////////////////////////////////////////////////////////////////////////////
// JavaScriptAppModalDialogCocoa, NativeAppModalDialog implementation:

int JavaScriptAppModalDialogCocoa::GetAppModalDialogButtons() const {
  // From the above, it is the case that if there is 1 button, it is always the
  // OK button.  The second button, if it exists, is always the Cancel button.
  int num_buttons = [[GetAlert() buttons] count];
  switch (num_buttons) {
    case 1:
      return ui::DIALOG_BUTTON_OK;
    case 2:
      return ui::DIALOG_BUTTON_OK | ui::DIALOG_BUTTON_CANCEL;
    default:
      NOTREACHED();
      return 0;
  }
}

void JavaScriptAppModalDialogCocoa::ShowAppModalDialog() {
  is_showing_ = true;

  // Dispatch the method to show the alert back to the top of the CFRunLoop.
  // This fixes an interaction bug with NSSavePanel. http://crbug.com/375785
  // When this object is destroyed, outstanding performSelector: requests
  // should be cancelled.
  [helper_.get() performSelector:@selector(showAlert)
                      withObject:nil
                      afterDelay:0];
}

void JavaScriptAppModalDialogCocoa::ActivateAppModalDialog() {
}

void JavaScriptAppModalDialogCocoa::CloseAppModalDialog() {
  [helper_ closeWindow];
}

void JavaScriptAppModalDialogCocoa::AcceptAppModalDialog() {
  [helper_ acceptAlert];
}

void JavaScriptAppModalDialogCocoa::CancelAppModalDialog() {
  [helper_ cancelAlert];
}

bool JavaScriptAppModalDialogCocoa::IsShowing() const {
  return is_showing_;
}

namespace {

class ChromeJavaScriptNativeDialogCocoaFactory
    : public app_modal::JavaScriptNativeDialogFactory {
 public:
  ChromeJavaScriptNativeDialogCocoaFactory() {}
  ~ChromeJavaScriptNativeDialogCocoaFactory() override {}

 private:
  app_modal::NativeAppModalDialog* CreateNativeJavaScriptDialog(
      app_modal::JavaScriptAppModalDialog* dialog) override {
    app_modal::NativeAppModalDialog* d =
        new JavaScriptAppModalDialogCocoa(dialog);
    dialog->web_contents()->GetDelegate()->ActivateContents(
        dialog->web_contents());
    return d;
  }

  DISALLOW_COPY_AND_ASSIGN(ChromeJavaScriptNativeDialogCocoaFactory);
};

}  // namespace

void InstallChromeJavaScriptNativeDialogFactory() {
  app_modal::JavaScriptDialogManager::GetInstance()->SetNativeDialogFactory(
      base::WrapUnique(new ChromeJavaScriptNativeDialogCocoaFactory));
}
