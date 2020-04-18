// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/javascript_dialogs/javascript_dialog_cocoa.h"

#import "base/mac/scoped_nsobject.h"
#include "base/strings/sys_string_conversions.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_alert.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_custom_sheet.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_mac.h"
#import "chrome/browser/ui/cocoa/key_equivalent_constants.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/strings/grit/ui_strings.h"

@class JavaScriptDialogCocoaBridge;

class JavaScriptDialogCocoa::JavaScriptDialogCocoaImpl
    : public ConstrainedWindowMacDelegate {
 public:
  JavaScriptDialogCocoaImpl(
      JavaScriptDialogCocoa* parent,
      content::WebContents* parent_web_contents,
      content::WebContents* alerting_web_contents,
      const base::string16& title,
      content::JavaScriptDialogType dialog_type,
      const base::string16& message_text,
      const base::string16& default_prompt_text,
      content::JavaScriptDialogManager::DialogClosedCallback dialog_callback);
  virtual ~JavaScriptDialogCocoaImpl() = default;

  // Callbacks from the bridge when buttons are clicked.
  void Accept();
  void Cancel();

  // ConstrainedWindowMacDelegate:
  void OnConstrainedWindowClosed(ConstrainedWindowMac* window) override;

 private:
  friend class JavaScriptDialogCocoa;

  content::JavaScriptDialogManager::DialogClosedCallback dialog_callback_;

  std::unique_ptr<ConstrainedWindowMac> window_;
  base::scoped_nsobject<ConstrainedWindowAlert> alert_;
  base::scoped_nsobject<JavaScriptDialogCocoaBridge> bridge_;

  base::scoped_nsobject<NSTextField> textField_;

  JavaScriptDialogCocoa* parent_;
};

@interface JavaScriptDialogCocoaBridge : NSObject {
  JavaScriptDialogCocoa::JavaScriptDialogCocoaImpl* dialogImpl_;  // weak
}
@end

@implementation JavaScriptDialogCocoaBridge

- (instancetype)initWithDialogImpl:
    (JavaScriptDialogCocoa::JavaScriptDialogCocoaImpl*)dialogImpl {
  if ((self = [super init])) {
    dialogImpl_ = dialogImpl;
  }
  return self;
}

- (void)onAcceptButton:(id)sender {
  dialogImpl_->Accept();
}

- (void)onCancelButton:(id)sender {
  dialogImpl_->Cancel();
}

@end

JavaScriptDialogCocoa::JavaScriptDialogCocoaImpl::JavaScriptDialogCocoaImpl(
    JavaScriptDialogCocoa* parent,
    content::WebContents* parent_web_contents,
    content::WebContents* alerting_web_contents,
    const base::string16& title,
    content::JavaScriptDialogType dialog_type,
    const base::string16& message_text,
    const base::string16& default_prompt_text,
    content::JavaScriptDialogManager::DialogClosedCallback dialog_callback)
    : dialog_callback_(std::move(dialog_callback)), parent_(parent) {
  bridge_.reset([[JavaScriptDialogCocoaBridge alloc] initWithDialogImpl:this]);

  alert_.reset([[ConstrainedWindowAlert alloc] init]);
  [alert_ setMessageText:base::SysUTF16ToNSString(title)];
  [alert_ setInformativeText:base::SysUTF16ToNSString(message_text)];
  [alert_ addButtonWithTitle:l10n_util::GetNSStringWithFixup(IDS_APP_OK)
               keyEquivalent:kKeyEquivalentReturn
                      target:bridge_
                      action:@selector(onAcceptButton:)];
  if (dialog_type != content::JAVASCRIPT_DIALOG_TYPE_ALERT) {
    [alert_ addButtonWithTitle:l10n_util::GetNSStringWithFixup(IDS_APP_CANCEL)
                 keyEquivalent:kKeyEquivalentEscape
                        target:bridge_
                        action:@selector(onCancelButton:)];
  }
  if (dialog_type == content::JAVASCRIPT_DIALOG_TYPE_PROMPT) {
    textField_.reset(
        [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 460, 22)]);
    [[textField_ cell] setLineBreakMode:NSLineBreakByTruncatingTail];
    [textField_ setStringValue:base::SysUTF16ToNSString(default_prompt_text)];
    [alert_ setAccessoryView:textField_];
  }
  [[alert_ closeButton] removeFromSuperview];
  [alert_ layout];

  base::scoped_nsobject<CustomConstrainedWindowSheet> sheet(
      [[CustomConstrainedWindowSheet alloc]
          initWithCustomWindow:[alert_ window]]);
  window_ = CreateAndShowWebModalDialogMac(this, parent_web_contents, sheet);
}

void JavaScriptDialogCocoa::JavaScriptDialogCocoaImpl::
    OnConstrainedWindowClosed(ConstrainedWindowMac* window) {
  if (dialog_callback_)
    std::move(dialog_callback_).Run(false, base::string16());
  delete parent_;
  // parent_ owns this object, so return immediately.
}

void JavaScriptDialogCocoa::JavaScriptDialogCocoaImpl::Cancel() {
  if (dialog_callback_) {
    std::move(dialog_callback_).Run(false, base::string16());
  }
  window_->CloseWebContentsModalDialog();
}

void JavaScriptDialogCocoa::JavaScriptDialogCocoaImpl::Accept() {
  if (dialog_callback_) {
    base::string16 input;
    if (textField_)
      input = base::SysNSStringToUTF16([textField_ stringValue]);
    std::move(dialog_callback_).Run(true, input);
  }
  window_->CloseWebContentsModalDialog();
}

JavaScriptDialogCocoa::~JavaScriptDialogCocoa() = default;

// static
base::WeakPtr<JavaScriptDialogCocoa> JavaScriptDialogCocoa::Create(
    content::WebContents* parent_web_contents,
    content::WebContents* alerting_web_contents,
    const base::string16& title,
    content::JavaScriptDialogType dialog_type,
    const base::string16& message_text,
    const base::string16& default_prompt_text,
    content::JavaScriptDialogManager::DialogClosedCallback dialog_callback) {
  return (new JavaScriptDialogCocoa(
              parent_web_contents, alerting_web_contents, title, dialog_type,
              message_text, default_prompt_text, std::move(dialog_callback)))
      ->weak_factory_.GetWeakPtr();
}

void JavaScriptDialogCocoa::CloseDialogWithoutCallback() {
  impl_->dialog_callback_.Reset();
  impl_->window_->CloseWebContentsModalDialog();
}

base::string16 JavaScriptDialogCocoa::GetUserInput() {
  if (!impl_->textField_)
    return base::string16();

  return base::SysNSStringToUTF16([impl_->textField_ stringValue]);
}

JavaScriptDialogCocoa::JavaScriptDialogCocoa(
    content::WebContents* parent_web_contents,
    content::WebContents* alerting_web_contents,
    const base::string16& title,
    content::JavaScriptDialogType dialog_type,
    const base::string16& message_text,
    const base::string16& default_prompt_text,
    content::JavaScriptDialogManager::DialogClosedCallback dialog_callback)
    : JavaScriptDialog(parent_web_contents),
      impl_(std::make_unique<JavaScriptDialogCocoaImpl>(
          this,
          parent_web_contents,
          alerting_web_contents,
          title,
          dialog_type,
          message_text,
          default_prompt_text,
          std::move(dialog_callback))),
      weak_factory_(this) {}
