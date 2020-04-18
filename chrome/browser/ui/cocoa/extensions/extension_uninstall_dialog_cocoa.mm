// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_uninstall_dialog.h"

#import <Cocoa/Cocoa.h>

#include <string>

#import "base/mac/scoped_nsobject.h"
#include "base/message_loop/message_loop_current.h"
#import "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/cocoa/browser_dialogs_views_mac.h"
#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#include "extensions/common/extension.h"
#include "skia/ext/skia_utils_mac.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/gfx/image/image_skia_util_mac.h"

namespace {

// The Cocoa implementation of ExtensionUninstallDialog. This has a less
// complex life cycle than the Views and GTK implementations because the
// dialog blocks the page from navigating away and destroying the dialog,
// so there's no way for the dialog to outlive its delegate.
class ExtensionUninstallDialogCocoa
    : public extensions::ExtensionUninstallDialog {
 public:
  ExtensionUninstallDialogCocoa(Profile* profile, Delegate* delegate);
  ~ExtensionUninstallDialogCocoa() override;

 private:
  void Show() override;
};

ExtensionUninstallDialogCocoa::ExtensionUninstallDialogCocoa(
    Profile* profile,
    extensions::ExtensionUninstallDialog::Delegate* delegate)
    : extensions::ExtensionUninstallDialog(profile, nullptr, delegate) {}

ExtensionUninstallDialogCocoa::~ExtensionUninstallDialogCocoa() {}

void ExtensionUninstallDialogCocoa::Show() {
  NSAlert* alert = [[[NSAlert alloc] init] autorelease];

  NSButton* continueButton = [alert addButtonWithTitle:l10n_util::GetNSString(
      IDS_EXTENSION_PROMPT_UNINSTALL_BUTTON)];
  NSButton* cancelButton = [alert addButtonWithTitle:l10n_util::GetNSString(
      IDS_CANCEL)];
  // Default to accept when triggered via chrome://extensions page.
  if (triggering_extension()) {
    [continueButton setKeyEquivalent:@""];
    [cancelButton setKeyEquivalent:@"\r"];
  }

  [alert setMessageText:base::SysUTF8ToNSString(GetHeadingText())];
  [alert setAlertStyle:NSWarningAlertStyle];
  [alert setIcon:gfx::NSImageFromImageSkia(icon())];

  base::scoped_nsobject<NSButton> reportAbuseCheckbox;
  if (ShouldShowReportAbuseCheckbox()) {
    reportAbuseCheckbox.reset([[NSButton alloc] initWithFrame:NSZeroRect]);
    [reportAbuseCheckbox setButtonType:NSSwitchButton];
    [reportAbuseCheckbox setTitle:l10n_util::GetNSString(
        IDS_EXTENSION_PROMPT_UNINSTALL_REPORT_ABUSE)];
    [reportAbuseCheckbox sizeToFit];
    [alert setAccessoryView:reportAbuseCheckbox];
  }

  NSModalResponse response;
  {
    base::MessageLoopCurrent::ScopedNestableTaskAllower allow_nested;
    response = [alert runModal];
  }
  if (response == NSAlertFirstButtonReturn) {
    bool report_abuse_checked =
        reportAbuseCheckbox.get() && [reportAbuseCheckbox state] == NSOnState;
    OnDialogClosed(report_abuse_checked ?
        CLOSE_ACTION_UNINSTALL_AND_REPORT_ABUSE : CLOSE_ACTION_UNINSTALL);
  } else {
    OnDialogClosed(CLOSE_ACTION_CANCELED);
  }
}

}  // namespace

// static
extensions::ExtensionUninstallDialog*
extensions::ExtensionUninstallDialog::Create(Profile* profile,
                                             gfx::NativeWindow parent,
                                             Delegate* delegate) {
  if (chrome::ShowAllDialogsWithViewsToolkit()) {
    return extensions::ExtensionUninstallDialog::CreateViews(profile, parent,
                                                             delegate);
  }
  return new ExtensionUninstallDialogCocoa(profile, delegate);
}
