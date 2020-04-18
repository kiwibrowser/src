// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/extensions/media_galleries_dialog_cocoa.h"

#include <stddef.h>

#include "base/mac/scoped_nsobject.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/browser/ui/cocoa/chrome_style.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_alert.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_button.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_control_utils.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_custom_sheet.h"
#import "chrome/browser/ui/cocoa/key_equivalent_constants.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/web_contents.h"
#import "ui/base/cocoa/flipped_view.h"
#import "ui/base/cocoa/menu_controller.h"
#include "ui/base/l10n/l10n_util.h"
#import "ui/base/models/menu_model.h"
#include "ui/base/ui_features.h"

// Controller for UI events on items in the media galleries dialog.
@interface MediaGalleriesCocoaController : NSObject {
 @private
  MediaGalleriesDialogCocoa* dialog_;
}

@property(assign, nonatomic) MediaGalleriesDialogCocoa* dialog;

- (void)onAcceptButton:(id)sender;
- (void)onCancelButton:(id)sender;
- (void)onAuxiliaryButton:(id)sender;

@end

@implementation MediaGalleriesCocoaController

@synthesize dialog = dialog_;

- (void)onAcceptButton:(id)sender {
  dialog_->OnAcceptClicked();
}

- (void)onCancelButton:(id)sender {
  dialog_->OnCancelClicked();
}

- (void)onAuxiliaryButton:(id)sender {
  DCHECK(dialog_);
  dialog_->OnAuxiliaryButtonClicked();
}

@end

namespace {

const CGFloat kCheckboxMargin = 10;
const CGFloat kCheckboxMaxWidth = 440;
const CGFloat kScrollAreaHeight = 220;

}  // namespace

MediaGalleriesDialogCocoa::MediaGalleriesDialogCocoa(
    MediaGalleriesDialogController* controller,
    MediaGalleriesCocoaController* cocoa_controller)
    : controller_(controller),
      accepted_(false),
      cocoa_controller_([cocoa_controller retain]) {
  [cocoa_controller_ setDialog:this];

  alert_.reset([[ConstrainedWindowAlert alloc] init]);

  [alert_ setMessageText:base::SysUTF16ToNSString(controller_->GetHeader())];
  [alert_ setInformativeText:
      base::SysUTF16ToNSString(controller_->GetSubtext())];
  [alert_ addButtonWithTitle:
      base::SysUTF16ToNSString(controller_->GetAcceptButtonText())
               keyEquivalent:kKeyEquivalentReturn
                      target:cocoa_controller_
                      action:@selector(onAcceptButton:)];
  [alert_ addButtonWithTitle:
      l10n_util::GetNSString(IDS_MEDIA_GALLERIES_DIALOG_CANCEL)
               keyEquivalent:kKeyEquivalentEscape
                      target:cocoa_controller_
                      action:@selector(onCancelButton:)];
  base::string16 auxiliaryButtonLabel = controller_->GetAuxiliaryButtonText();
  if (!auxiliaryButtonLabel.empty()) {
    [alert_ addButtonWithTitle:base::SysUTF16ToNSString(auxiliaryButtonLabel)
                 keyEquivalent:kKeyEquivalentNone
                        target:cocoa_controller_
                        action:@selector(onAuxiliaryButton:)];
  }
  [[alert_ closeButton] setTarget:cocoa_controller_];
  [[alert_ closeButton] setAction:@selector(onCancelButton:)];

  InitDialogControls();

  // May be NULL during tests.
  if (controller->WebContents()) {
    base::scoped_nsobject<CustomConstrainedWindowSheet> sheet(
        [[CustomConstrainedWindowSheet alloc]
            initWithCustomWindow:[alert_ window]]);
    window_ = CreateAndShowWebModalDialogMac(
        this, controller->WebContents(), sheet);
  }
}

MediaGalleriesDialogCocoa::~MediaGalleriesDialogCocoa() {
}

void MediaGalleriesDialogCocoa::AcceptDialogForTesting() {
  OnAcceptClicked();
}

void MediaGalleriesDialogCocoa::InitDialogControls() {
  main_container_.reset([[NSBox alloc] init]);
  [main_container_ setBoxType:NSBoxCustom];
  [main_container_ setBorderType:NSLineBorder];
  [main_container_ setBorderWidth:1];
  [main_container_ setCornerRadius:0];
  [main_container_ setContentViewMargins:NSZeroSize];
  [main_container_ setTitlePosition:NSNoTitle];
  [main_container_ setBorderColor:[NSColor disabledControlTextColor]];

  base::scoped_nsobject<NSScrollView> scroll_view(
      [[NSScrollView alloc] initWithFrame:
          NSMakeRect(0, 0, kCheckboxMaxWidth, kScrollAreaHeight)]);
  [scroll_view setHasVerticalScroller:YES];
  [scroll_view setHasHorizontalScroller:NO];
  [scroll_view setBorderType:NSNoBorder];
  [scroll_view setAutohidesScrollers:YES];
  [[main_container_ contentView] addSubview:scroll_view];

  // Add gallery checkboxes inside the scrolling view.
  checkbox_container_.reset([[FlippedView alloc] initWithFrame:NSZeroRect]);

  std::vector<base::string16> headers = controller_->GetSectionHeaders();
  CGFloat y_pos = 0;
  for (size_t i = 0; i < headers.size(); i++) {
    MediaGalleriesDialogController::Entries entries =
        controller_->GetSectionEntries(i);
    if (!entries.empty()) {
      if (!headers[i].empty()) {
        y_pos = CreateCheckboxSeparator(y_pos,
                                        base::SysUTF16ToNSString(headers[i]));
      }
      y_pos = CreateCheckboxes(y_pos, entries);
    }
  }

  // Give the container a reasonable initial size so that the scroll_view can
  // figure out the content size.
  [checkbox_container_ setFrameSize:NSMakeSize(kCheckboxMaxWidth, y_pos)];
  [scroll_view setDocumentView:checkbox_container_];
  [checkbox_container_ setFrameSize:NSMakeSize([scroll_view contentSize].width,
                                               y_pos)];

  // Resize to pack the scroll view if possible.
  NSRect scroll_frame = [scroll_view frame];
  if (NSHeight(scroll_frame) > NSHeight([checkbox_container_ frame])) {
    scroll_frame.size.height = NSHeight([checkbox_container_ frame]);
    [scroll_view setFrameSize:scroll_frame.size];
  }

  [main_container_ setFrameFromContentFrame:scroll_frame];
  [main_container_ setFrameOrigin:NSZeroPoint];
  [alert_ setAccessoryView:main_container_];

  // As a safeguard against the user skipping reading over the dialog and just
  // confirming, the button will be unavailable for dialogs without any checks
  // until the user toggles something.
  [[[alert_ buttons] objectAtIndex:0] setEnabled:
      controller_->IsAcceptAllowed()];

  [alert_ layout];
}

CGFloat MediaGalleriesDialogCocoa::CreateCheckboxes(
    CGFloat y_pos,
    const MediaGalleriesDialogController::Entries& entries) {
  for (MediaGalleriesDialogController::Entries::const_iterator iter =
       entries.begin(); iter != entries.end(); ++iter) {
    const MediaGalleriesDialogController::Entry& entry = *iter;
    base::scoped_nsobject<MediaGalleryListEntry> checkbox_entry(
        [[MediaGalleryListEntry alloc]
            initWithFrame:NSZeroRect
               controller:this
                 prefInfo:entry.pref_info]);

    [checkbox_entry setState:entry.selected];

    [checkbox_entry setFrameOrigin:NSMakePoint(0, y_pos)];
    y_pos = NSMaxY([checkbox_entry frame]) + kCheckboxMargin;

    [checkbox_container_ addSubview:checkbox_entry];
  }

  return y_pos;
}

CGFloat MediaGalleriesDialogCocoa::CreateCheckboxSeparator(CGFloat y_pos,
                                                           NSString* header) {
  base::scoped_nsobject<NSBox> separator(
      [[NSBox alloc] initWithFrame:NSMakeRect(
          0, y_pos + kCheckboxMargin * 0.5, kCheckboxMaxWidth, 1.0)]);
  [separator setBoxType:NSBoxSeparator];
  [separator setBorderType:NSLineBorder];
  [separator setAlphaValue:0.2];
  [checkbox_container_ addSubview:separator];
  y_pos += kCheckboxMargin * 0.5 + 4;

  base::scoped_nsobject<NSTextField> unattached_label(
      [[NSTextField alloc] initWithFrame:NSZeroRect]);
  [unattached_label setEditable:NO];
  [unattached_label setSelectable:NO];
  [unattached_label setBezeled:NO];
  [unattached_label setAttributedStringValue:
      constrained_window::GetAttributedLabelString(
          header,
          chrome_style::kTextFontStyle,
          NSNaturalTextAlignment,
          NSLineBreakByClipping
      )];
  [unattached_label sizeToFit];
  NSSize unattached_label_size = [unattached_label frame].size;
  [unattached_label setFrame:NSMakeRect(
      kCheckboxMargin, y_pos + kCheckboxMargin,
      kCheckboxMaxWidth, unattached_label_size.height)];
  [checkbox_container_ addSubview:unattached_label];
  y_pos = NSMaxY([unattached_label frame]) + kCheckboxMargin;

  return y_pos;
}

void MediaGalleriesDialogCocoa::OnAcceptClicked() {
  accepted_ = true;

  if (window_)
    window_->CloseWebContentsModalDialog();
}

void MediaGalleriesDialogCocoa::OnCancelClicked() {
  if (window_)
    window_->CloseWebContentsModalDialog();
}

void MediaGalleriesDialogCocoa::OnAuxiliaryButtonClicked() {
  controller_->DidClickAuxiliaryButton();
}

void MediaGalleriesDialogCocoa::UpdateGalleries() {
  InitDialogControls();
}

void MediaGalleriesDialogCocoa::OnConstrainedWindowClosed(
    ConstrainedWindowMac* window) {
  controller_->DialogFinished(accepted_);
}

void MediaGalleriesDialogCocoa::OnCheckboxToggled(MediaGalleryPrefId pref_id,
                                                  bool checked) {
  controller_->DidToggleEntry(pref_id, checked);

  [[[alert_ buttons] objectAtIndex:0] setEnabled:
      controller_->IsAcceptAllowed()];
}

ui::MenuModel* MediaGalleriesDialogCocoa::GetContextMenu(
    MediaGalleryPrefId pref_id) {
  return controller_->GetContextMenu(pref_id);
}

// static
MediaGalleriesDialog* MediaGalleriesDialog::CreateCocoa(
    MediaGalleriesDialogController* controller) {
  base::scoped_nsobject<MediaGalleriesCocoaController> cocoa_controller(
      [[MediaGalleriesCocoaController alloc] init]);
  return new MediaGalleriesDialogCocoa(controller, cocoa_controller);
}

#if !BUILDFLAG(MAC_VIEWS_BROWSER)
MediaGalleriesDialog* MediaGalleriesDialog::Create(
    MediaGalleriesDialogController* controller) {
  return CreateCocoa(controller);
}
#endif
