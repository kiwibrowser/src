// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/browser/shell_javascript_dialog.h"

#import <Cocoa/Cocoa.h>

#import "base/mac/scoped_nsobject.h"
#include "base/strings/sys_string_conversions.h"
#include "content/shell/browser/shell_javascript_dialog_manager.h"

// Helper object that receives the notification that the dialog/sheet is
// going away. Is responsible for cleaning itself up.
@interface ShellJavaScriptDialogHelper : NSObject<NSAlertDelegate> {
 @private
  base::scoped_nsobject<NSAlert> alert_;
  NSTextField* textField_;  // WEAK; owned by alert_

  // Copies of the fields in ShellJavaScriptDialog because they're private.
  content::ShellJavaScriptDialogManager* manager_;
  content::JavaScriptDialogManager::DialogClosedCallback callback_;
}

- (id)initHelperWithManager:(content::ShellJavaScriptDialogManager*)manager
   andCallback:(content::JavaScriptDialogManager::DialogClosedCallback)callback;
- (NSAlert*)alert;
- (NSTextField*)textField;
- (void)alertDidEnd:(NSAlert*)alert
         returnCode:(int)returnCode
        contextInfo:(void*)contextInfo;
- (void)cancel;

@end

@implementation ShellJavaScriptDialogHelper

- (id)initHelperWithManager:(content::ShellJavaScriptDialogManager*)manager
  andCallback:(content::JavaScriptDialogManager::DialogClosedCallback)callback {
  if (self = [super init]) {
    manager_ = manager;
    callback_ = std::move(callback);
  }

  return self;
}

- (NSAlert*)alert {
  alert_.reset([[NSAlert alloc] init]);
  return alert_;
}

- (NSTextField*)textField {
  textField_ = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 300, 22)];
  [[textField_ cell] setLineBreakMode:NSLineBreakByTruncatingTail];
  [alert_ setAccessoryView:textField_];
  [[alert_ window] setInitialFirstResponder:textField_];
  [textField_ release];

  return textField_;
}

- (void)alertDidEnd:(NSAlert*)alert
         returnCode:(int)returnCode
        contextInfo:(void*)contextInfo {
  if (returnCode == NSRunStoppedResponse)
    return;

  bool success = returnCode == NSAlertFirstButtonReturn;
  base::string16 input;
  if (textField_)
    input = base::SysNSStringToUTF16([textField_ stringValue]);

  content::ShellJavaScriptDialog* native_dialog =
      reinterpret_cast<content::ShellJavaScriptDialog*>(contextInfo);
  std::move(callback_).Run(success, input);
  manager_->DialogClosed(native_dialog);
}

- (void)cancel {
  [NSApp endSheet:[alert_ window]];
  alert_.reset();
}

@end

namespace content {

ShellJavaScriptDialog::ShellJavaScriptDialog(
    ShellJavaScriptDialogManager* manager,
    gfx::NativeWindow parent_window,
    JavaScriptDialogType dialog_type,
    const base::string16& message_text,
    const base::string16& default_prompt_text,
    JavaScriptDialogManager::DialogClosedCallback callback)
    : callback_(std::move(callback)) {
  bool text_field = dialog_type == JAVASCRIPT_DIALOG_TYPE_PROMPT;
  bool one_button = dialog_type == JAVASCRIPT_DIALOG_TYPE_ALERT;

  helper_ = [[ShellJavaScriptDialogHelper alloc]
      initHelperWithManager:manager
                andCallback:std::move(callback)];

  // Show the modal dialog.
  NSAlert* alert = [helper_ alert];
  NSTextField* field = nil;
  if (text_field) {
    field = [helper_ textField];
    [field setStringValue:base::SysUTF16ToNSString(default_prompt_text)];
  }
  [alert setDelegate:helper_];
  [alert setInformativeText:base::SysUTF16ToNSString(message_text)];
  [alert setMessageText:@"Javascript alert"];
  [alert addButtonWithTitle:@"OK"];
  if (!one_button) {
    NSButton* other = [alert addButtonWithTitle:@"Cancel"];
    [other setKeyEquivalent:@"\e"];
  }

  [alert
      beginSheetModalForWindow:nil  // nil here makes it app-modal
                 modalDelegate:helper_
                didEndSelector:@selector(alertDidEnd:returnCode:contextInfo:)
                   contextInfo:this];
}

ShellJavaScriptDialog::~ShellJavaScriptDialog() {
  [helper_ release];
}

void ShellJavaScriptDialog::Cancel() {
  [helper_ cancel];
}

}  // namespace content
