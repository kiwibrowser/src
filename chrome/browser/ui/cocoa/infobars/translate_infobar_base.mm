// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/infobars/translate_infobar_base.h"

#include <stddef.h>

#include <utility>

#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/translate/chrome_translate_client.h"
#import "chrome/browser/ui/cocoa/hover_close_button.h"
#include "chrome/browser/ui/cocoa/infobars/after_translate_infobar_controller.h"
#import "chrome/browser/ui/cocoa/infobars/before_translate_infobar_controller.h"
#include "chrome/browser/ui/cocoa/infobars/infobar_cocoa.h"
#import "chrome/browser/ui/cocoa/infobars/infobar_container_controller.h"
#import "chrome/browser/ui/cocoa/infobars/infobar_controller.h"
#import "chrome/browser/ui/cocoa/infobars/infobar_utilities.h"
#include "chrome/browser/ui/cocoa/infobars/translate_message_infobar_controller.h"
#include "chrome/browser/ui/cocoa/l10n_util.h"
#include "components/strings/grit/components_strings.h"
#include "components/translate/core/browser/translate_infobar_delegate.h"
#include "third_party/google_toolbox_for_mac/src/AppKit/GTMUILocalizerAndLayoutTweaker.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/ui_features.h"

using InfoBarUtilities::MoveControl;
using InfoBarUtilities::CreateLabel;
using InfoBarUtilities::AddMenuItem;

namespace {

// Vertically center |toMove| in its container.
void VerticallyCenterView(NSView* toMove) {
  NSRect superViewFrame = [[toMove superview] frame];
  NSRect viewFrame = [toMove frame];
  viewFrame.origin.y =
      floor((NSHeight(superViewFrame) - NSHeight(viewFrame)) / 2.0);
  [toMove setFrame:viewFrame];
}

// Check that the control |before| is ordered visually before the |after|
// control. Also, check that there is space between them. Is RTL-aware.
bool VerifyControlOrderAndSpacing(id before, id after) {
  NSRect beforeFrame = [before frame];
  NSRect afterFrame = [after frame];
  if (cocoa_l10n_util::ShouldDoExperimentalRTLLayout())
    std::swap(beforeFrame, afterFrame);
  return NSMinX(afterFrame) >= NSMaxX(beforeFrame);
}

}  // namespace

std::unique_ptr<infobars::InfoBar> ChromeTranslateClient::CreateInfoBarCocoa(
    std::unique_ptr<translate::TranslateInfoBarDelegate> delegate) const {
  std::unique_ptr<InfoBarCocoa> infobar(new InfoBarCocoa(std::move(delegate)));
  base::scoped_nsobject<TranslateInfoBarControllerBase> infobar_controller;
  switch (infobar->delegate()->AsTranslateInfoBarDelegate()->translate_step()) {
    case translate::TRANSLATE_STEP_BEFORE_TRANSLATE:
      infobar_controller.reset([[BeforeTranslateInfobarController alloc]
          initWithInfoBar:infobar.get()]);
      break;
    case translate::TRANSLATE_STEP_AFTER_TRANSLATE:
      infobar_controller.reset([[AfterTranslateInfobarController alloc]
          initWithInfoBar:infobar.get()]);
      break;
    case translate::TRANSLATE_STEP_TRANSLATING:
    case translate::TRANSLATE_STEP_TRANSLATE_ERROR:
      infobar_controller.reset([[TranslateMessageInfobarController alloc]
          initWithInfoBar:infobar.get()]);
      break;
    default:
      NOTREACHED();
  }
  infobar->set_controller(infobar_controller);
  return std::move(infobar);
}

#if !BUILDFLAG(MAC_VIEWS_BROWSER)
std::unique_ptr<infobars::InfoBar> ChromeTranslateClient::CreateInfoBar(
    std::unique_ptr<translate::TranslateInfoBarDelegate> delegate) const {
  return CreateInfoBarCocoa(std::move(delegate));
}
#endif

@implementation TranslateInfoBarControllerBase (FrameChangeObserver)

// Triggered when the frame changes.  This will figure out what size and
// visibility the options popup should be.
- (void)didChangeFrame:(NSNotification*)notification {
  [self adjustOptionsButtonSizeAndVisibilityForView:
      [[self visibleControls] lastObject]];
}

@end


@interface TranslateInfoBarControllerBase (Private)

// Removes all controls so that layout can add in only the controls
// required.
- (void)clearAllControls;

// Create all the various controls we need for the toolbar.
- (void)constructViews;

// Reloads text for all labels for the current state.
- (void)loadLabelText:(translate::TranslateErrors::Type)error;

// Main function to update the toolbar graphic state and data model after
// the state has changed.
// Controls are moved around as needed and visibility changed to match the
// current state.
- (void)updateState;

// Called when the source or target language selection changes in a menu.
// |newLanguageCode| is the ISO language of the newly selected item.
// |newLanguageIdx| is the index of the newly selected item in the appropriate
// menu.
- (void)sourceLanguageModified:(NSString*)newLanguageCode
             withLanguageIndex:(NSInteger)newLanguageIdx;
- (void)targetLanguageModified:(NSString*)newLanguageCode
             withLanguageIndex:(NSInteger)newLanguageIdx;

// Completely rebuild "from" and "to" language menus from the data model.
- (void)populateLanguageMenus;

@end

#pragma mark TranslateInfoBarController class

@implementation TranslateInfoBarControllerBase

- (translate::TranslateInfoBarDelegate*)delegate {
  return reinterpret_cast<translate::TranslateInfoBarDelegate*>(
      [super delegate]);
}

- (void)constructViews {
  // Using a zero or very large frame causes GTMUILocalizerAndLayoutTweaker
  // to not resize the view properly so we take the bounds of the first label
  // which is contained in the nib.
  NSRect bogusFrame = [label_ frame];
  label1_.reset(CreateLabel(bogusFrame));
  label2_.reset(CreateLabel(bogusFrame));
  label3_.reset(CreateLabel(bogusFrame));

  optionsPopUp_.reset([[NSPopUpButton alloc] initWithFrame:bogusFrame
                                                 pullsDown:YES]);
  fromLanguagePopUp_.reset([[NSPopUpButton alloc] initWithFrame:bogusFrame
                                                      pullsDown:NO]);
  toLanguagePopUp_.reset([[NSPopUpButton alloc] initWithFrame:bogusFrame
                                                    pullsDown:NO]);
  showOriginalButton_.reset([[NSButton alloc] init]);
  translateMessageButton_.reset([[NSButton alloc] init]);
}

- (void)sourceLanguageModified:(NSString*)newLanguageCode
             withLanguageIndex:(NSInteger)newLanguageIdx {
  std::string newLanguageCodeS = base::SysNSStringToUTF8(newLanguageCode);
  if (newLanguageCodeS.compare([self delegate]->original_language_code()) == 0)
    return;
  [self delegate]->UpdateOriginalLanguage(newLanguageCodeS);
  if ([self delegate]->translate_step() ==
      translate::TRANSLATE_STEP_AFTER_TRANSLATE)
    [self delegate]->Translate();
  int commandId = IDC_TRANSLATE_ORIGINAL_LANGUAGE_BASE + newLanguageIdx;
  int newMenuIdx = [fromLanguagePopUp_ indexOfItemWithTag:commandId];
  [fromLanguagePopUp_ selectItemAtIndex:newMenuIdx];
}

- (void)targetLanguageModified:(NSString*)newLanguageCode
             withLanguageIndex:(NSInteger)newLanguageIdx {
  std::string newLanguageCodeS = base::SysNSStringToUTF8(newLanguageCode);
  if (newLanguageCodeS.compare([self delegate]->target_language_code()) == 0)
    return;
  [self delegate]->UpdateTargetLanguage(newLanguageCodeS);
  if ([self delegate]->translate_step() ==
      translate::TRANSLATE_STEP_AFTER_TRANSLATE)
    [self delegate]->Translate();
  int commandId = IDC_TRANSLATE_TARGET_LANGUAGE_BASE + newLanguageIdx;
  int newMenuIdx = [toLanguagePopUp_ indexOfItemWithTag:commandId];
  [toLanguagePopUp_ selectItemAtIndex:newMenuIdx];
}

- (void)loadLabelText {
  // Do nothing by default, should be implemented by subclasses.
}

- (void)updateState {
  [self loadLabelText];
  [self clearAllControls];
  [self showVisibleControls:[self visibleControls]];
  [optionsPopUp_ setHidden:![self shouldShowOptionsPopUp]];
  [self layout];
  [self adjustOptionsButtonSizeAndVisibilityForView:
      [[self visibleControls] lastObject]];
}

- (void)removeOkCancelButtons {
  // Removing okButton_ & cancelButton_ from the view may cause them
  // to be released and since we can still access them from other areas
  // in the code later, we need them to be nil when this happens.
  [okButton_ removeFromSuperview];
  okButton_ = nil;
  [cancelButton_ removeFromSuperview];
  cancelButton_ = nil;
}

- (void)clearAllControls {
  // Step 1: remove all controls from the infobar so we have a clean slate.
  NSArray *allControls = [self allControls];

  for (NSControl* control in allControls) {
    if ([control superview])
      [control removeFromSuperview];
  }
}

- (void)showVisibleControls:(NSArray*)visibleControls {
  NSRect optionsFrame = [optionsPopUp_ frame];
  for (NSControl* control in visibleControls) {
    [GTMUILocalizerAndLayoutTweaker sizeToFitView:control];
    [control setAutoresizingMask:NSViewMaxXMargin];

    // Need to check if a view is already attached since |label1_| is always
    // parented and we don't want to add it again.
    if (![control superview])
      [infoBarView_ addSubview:control];

    if ([control isKindOfClass:[NSButton class]])
      VerticallyCenterView(control);

    // Make "from" and "to" language popup menus the same size as the options
    // menu.
    // We don't autosize since some languages names are really long causing
    // the toolbar to overflow.
    if ([control isKindOfClass:[NSPopUpButton class]])
      [control setFrame:optionsFrame];
  }
}

- (void)layout {

}

- (NSArray*)visibleControls {
  return [NSArray array];
}

- (void)rebuildOptionsMenu:(BOOL)hideTitle {
  if (![self shouldShowOptionsPopUp])
     return;

  // The options model doesn't know how to handle state transitions, so rebuild
  // it each time through here.
  optionsMenuModel_.reset(new translate::OptionsMenuModel([self delegate]));

  [optionsPopUp_ removeAllItems];
  // Set title.
  NSString* optionsLabel = hideTitle ? @"" :
      l10n_util::GetNSString(IDS_TRANSLATE_INFOBAR_OPTIONS);
  [optionsPopUp_ addItemWithTitle:optionsLabel];

   // Populate options menu.
  NSMenu* optionsMenu = [optionsPopUp_ menu];
  [optionsMenu setAutoenablesItems:NO];
  for (int i = 0; i < optionsMenuModel_->GetItemCount(); ++i) {
    AddMenuItem(optionsMenu, self, @selector(optionsMenuChanged:),
                base::SysUTF16ToNSString(optionsMenuModel_->GetLabelAt(i)),
                optionsMenuModel_->GetCommandIdAt(i),
                optionsMenuModel_->IsEnabledAt(i),
                optionsMenuModel_->IsItemCheckedAt(i), nil);
  }
}

- (BOOL)shouldShowOptionsPopUp {
  return YES;
}

- (void)populateLanguageMenus {
  NSMenu* originalLanguageMenu = [fromLanguagePopUp_ menu];
  [originalLanguageMenu setAutoenablesItems:NO];
  NSMenu* targetLanguageMenu = [toLanguagePopUp_ menu];
  [targetLanguageMenu setAutoenablesItems:NO];
  size_t source_index = translate::TranslateInfoBarDelegate::kNoIndex;
  size_t target_index = translate::TranslateInfoBarDelegate::kNoIndex;
  for (size_t i = 0; i < [self delegate]->num_languages(); ++i) {
    NSString* title =
        base::SysUTF16ToNSString([self delegate]->language_name_at(i));
    std::string language_code = [self delegate]->language_code_at(i);
    if (language_code == [self delegate]->original_language_code()) {
      source_index = i;
    }
    if (language_code == [self delegate]->target_language_code()) {
      target_index = i;
    }
    AddMenuItem(originalLanguageMenu, self, @selector(languageMenuChanged:),
                title, IDC_TRANSLATE_ORIGINAL_LANGUAGE_BASE + i,
                language_code != [self delegate]->target_language_code(),
                language_code == [self delegate]->original_language_code(),
                base::SysUTF8ToNSString(language_code));
    AddMenuItem(targetLanguageMenu, self, @selector(languageMenuChanged:),
                title, IDC_TRANSLATE_TARGET_LANGUAGE_BASE + i,
                language_code != [self delegate]->original_language_code(),
                language_code == [self delegate]->target_language_code(),
                base::SysUTF8ToNSString(language_code));
  }
  if (source_index != translate::TranslateInfoBarDelegate::kNoIndex) {
    [fromLanguagePopUp_ selectItemAtIndex:(source_index)];
  }
  [toLanguagePopUp_ selectItemAtIndex:(target_index)];
}

- (void)addAdditionalControls {
  using l10n_util::GetNSString;
  using l10n_util::GetNSStringWithFixup;

  // Get layout information from the NIB.
  NSRect okButtonFrame = [okButton_ frame];
  NSRect cancelButtonFrame = [cancelButton_ frame];

  DCHECK(NSMaxX(cancelButtonFrame) < NSMinX(okButtonFrame))
      << "Ok button expected to be on the right of the Cancel button in nib";

  spaceBetweenControls_ = NSMinX(okButtonFrame) - NSMaxX(cancelButtonFrame);

  // Instantiate additional controls.
  [self constructViews];

  // Set ourselves as the delegate for the options menu so we can populate it
  // dynamically.
  [[optionsPopUp_ menu] setDelegate:self];

  // Replace label_ with label1_ so we get a consistent look between all the
  // labels we display in the translate view.
  [[label_ superview] replaceSubview:label_ with:label1_.get()];
  label_.reset(); // Now released.

  // Populate contextual menus.
  [self rebuildOptionsMenu:NO];
  [self populateLanguageMenus];

  // Set OK & Cancel text.
  [okButton_ setTitle:GetNSStringWithFixup(IDS_TRANSLATE_INFOBAR_ACCEPT)];
  [cancelButton_ setTitle:GetNSStringWithFixup(IDS_TRANSLATE_INFOBAR_DENY)];

  // Set up "Show original" and "Try again" buttons.
  [showOriginalButton_ setFrame:okButtonFrame];

  // Set each of the buttons and popups to the NSTexturedRoundedBezelStyle
  // (metal-looking) style.
  NSArray* allControls = [self allControls];
  for (NSControl* control in allControls) {
    if (![control isKindOfClass:[NSButton class]])
      continue;
    NSButton* button = (NSButton*)control;
    [button setBezelStyle:NSTexturedRoundedBezelStyle];
    if ([button isKindOfClass:[NSPopUpButton class]]) {
      [[button cell] setArrowPosition:NSPopUpArrowAtBottom];
    }
  }
  // The options button is handled differently than the rest as it floats
  // to the right.
  [optionsPopUp_ setBezelStyle:NSTexturedRoundedBezelStyle];
  [[optionsPopUp_ cell] setArrowPosition:NSPopUpArrowAtBottom];

  [showOriginalButton_ setTarget:self];
  [showOriginalButton_ setAction:@selector(showOriginal:)];
  [translateMessageButton_ setTarget:self];
  [translateMessageButton_ setAction:@selector(messageButtonPressed:)];

  [showOriginalButton_
      setTitle:GetNSStringWithFixup(IDS_TRANSLATE_INFOBAR_REVERT)];

  // Add and configure controls that are visible in all modes.
  [optionsPopUp_ setAutoresizingMask:NSViewMinXMargin];
  // Add "options" popup z-ordered below all other controls so when we
  // resize the toolbar it doesn't hide them.
  [infoBarView_ addSubview:optionsPopUp_
                positioned:NSWindowBelow
                relativeTo:nil];
  [GTMUILocalizerAndLayoutTweaker sizeToFitView:optionsPopUp_];
  MoveControl(closeButton_, optionsPopUp_, spaceBetweenControls_, false);
  VerticallyCenterView(optionsPopUp_);

  [infoBarView_ setPostsFrameChangedNotifications:YES];
  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(didChangeFrame:)
             name:NSViewFrameDidChangeNotification
           object:infoBarView_];
  // Show and place GUI elements.
  [self updateState];
}

- (void)infobarWillHide {
  [[fromLanguagePopUp_ menu] cancelTracking];
  [[toLanguagePopUp_ menu] cancelTracking];
  [[optionsPopUp_ menu] cancelTracking];
  [super infobarWillHide];
}

- (void)infobarWillClose {
  [self disablePopUpMenu:[fromLanguagePopUp_ menu]];
  [self disablePopUpMenu:[toLanguagePopUp_ menu]];
  [self disablePopUpMenu:[optionsPopUp_ menu]];
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [super infobarWillClose];
}

- (void)adjustOptionsButtonSizeAndVisibilityForView:(NSView*)lastView {
  [optionsPopUp_ setHidden:NO];
  [self rebuildOptionsMenu:NO];
  [[optionsPopUp_ cell] setArrowPosition:NSPopUpArrowAtBottom];
  [optionsPopUp_ sizeToFit];
  // Typically, infobars are mirrored after |addAdditionalControls| in RTL.
  // Since the options menu is being moved after that code runs, an RTL
  // check is necessary here as well.
  MoveControl(closeButton_, optionsPopUp_, spaceBetweenControls_,
              cocoa_l10n_util::ShouldDoExperimentalRTLLayout());
  if (!VerifyControlOrderAndSpacing(lastView, optionsPopUp_)) {
    [self rebuildOptionsMenu:YES];
    NSRect oldFrame = [optionsPopUp_ frame];
    oldFrame.size.width = NSHeight(oldFrame);
    [optionsPopUp_ setFrame:oldFrame];
    [[optionsPopUp_ cell] setArrowPosition:NSPopUpArrowAtCenter];
    MoveControl(closeButton_, optionsPopUp_, spaceBetweenControls_, false);
    if (!VerifyControlOrderAndSpacing(lastView, optionsPopUp_)) {
      [optionsPopUp_ setHidden:YES];
    }
  }
}

// Called when "Translate" button is clicked.
- (void)ok:(id)sender {
  if (![self isOwned])
    return;
  translate::TranslateInfoBarDelegate* delegate = [self delegate];
  translate::TranslateStep state = delegate->translate_step();
  DCHECK(state == translate::TRANSLATE_STEP_BEFORE_TRANSLATE ||
         state == translate::TRANSLATE_STEP_TRANSLATE_ERROR);
  delegate->Translate();
}

// Called when someone clicks on the "Nope" button.
- (void)cancel:(id)sender {
  if (![self isOwned])
    return;
  translate::TranslateInfoBarDelegate* delegate = [self delegate];
  DCHECK_EQ(translate::TRANSLATE_STEP_BEFORE_TRANSLATE,
            delegate->translate_step());
  delegate->TranslationDeclined();
  [super removeSelf];
}

- (void)messageButtonPressed:(id)sender {
  if (![self isOwned])
    return;
  [self delegate]->MessageInfoBarButtonPressed();
}

- (IBAction)showOriginal:(id)sender {
  if (![self isOwned])
    return;
  [self delegate]->RevertTranslation();
}

// Called when any of the language drop down menus are changed.
- (void)languageMenuChanged:(id)item {
  if (![self isOwned])
    return;
  if ([item respondsToSelector:@selector(tag)]) {
    int cmd = [item tag];
    NSString* language_code = [item representedObject];
    if (cmd >= IDC_TRANSLATE_TARGET_LANGUAGE_BASE) {
      cmd -= IDC_TRANSLATE_TARGET_LANGUAGE_BASE;
      [self targetLanguageModified:language_code withLanguageIndex:cmd];
      return;
    } else if (cmd >= IDC_TRANSLATE_ORIGINAL_LANGUAGE_BASE) {
      cmd -= IDC_TRANSLATE_ORIGINAL_LANGUAGE_BASE;
      [self sourceLanguageModified:language_code withLanguageIndex:cmd];
      return;
    }
  }
  NOTREACHED() << "Language menu was changed with a bad language ID";
}

// Called when the options menu is changed.
- (void)optionsMenuChanged:(id)item {
  if (![self isOwned])
    return;
  if ([item respondsToSelector:@selector(tag)]) {
    int cmd = [item tag];
    // Danger Will Robinson! : This call can release the infobar (e.g. invoking
    // "About Translate" can open a new tab).
    // Do not access member variables after this line!
    optionsMenuModel_->ExecuteCommand(cmd, 0);
  } else {
    NOTREACHED();
  }
}

- (void)dealloc {
  [showOriginalButton_ setTarget:nil];
  [translateMessageButton_ setTarget:nil];
  [super dealloc];
}

#pragma mark NSMenuDelegate

// Invoked by virtue of us being set as the delegate for the options menu.
- (void)menuNeedsUpdate:(NSMenu *)menu {
  [self adjustOptionsButtonSizeAndVisibilityForView:
      [[self visibleControls] lastObject]];
}

@end

@implementation TranslateInfoBarControllerBase (TestingAPI)

- (NSArray*)allControls {
  return [NSArray arrayWithObjects:label1_.get(),fromLanguagePopUp_.get(),
      label2_.get(), toLanguagePopUp_.get(), label3_.get(), okButton_,
      cancelButton_, showOriginalButton_.get(), translateMessageButton_.get(),
      nil];
}

- (NSMenu*)optionsMenu {
  return [optionsPopUp_ menu];
}

- (NSButton*)translateMessageButton {
  return translateMessageButton_.get();
}

- (bool)verifyLayout {
  // All the controls available to translate infobars, except the options popup.
  // The options popup is shown/hidden instead of actually removed.  This gets
  // checked in the subclasses.
  NSArray* allControls = [self allControls];
  NSArray* visibleControls = [self visibleControls];

  // Step 1: Make sure control visibility is what we expect.
  for (NSUInteger i = 0; i < [allControls count]; ++i) {
    id control = [allControls objectAtIndex:i];
    bool hasSuperView = [control superview];
    bool expectedVisibility = [visibleControls containsObject:control];

    if (expectedVisibility != hasSuperView) {
      NSString *title = @"";
      if ([control isKindOfClass:[NSPopUpButton class]]) {
        title = [[[control menu] itemAtIndex:0] title];
      }

      LOG(ERROR) <<
          "State: " << [self description] <<
          " Control @" << i << (hasSuperView ? " has" : " doesn't have") <<
          " a superview" << [[control description] UTF8String] <<
          " Title=" << [title UTF8String];
      return false;
    }
  }

  // Step 2: Check that controls are ordered correctly with no overlap.
  id previousControl = nil;
  for (NSUInteger i = 0; i < [visibleControls count]; ++i) {
    id control = [visibleControls objectAtIndex:i];
    // The options pop up doesn't lay out like the rest of the controls as
    // it floats to the right.  It has some known issues shown in
    // http://crbug.com/47941.
    if (control == optionsPopUp_.get())
      continue;
    if (previousControl &&
        !VerifyControlOrderAndSpacing(previousControl, control)) {
      NSString *title = @"";
      if ([control isKindOfClass:[NSPopUpButton class]]) {
        title = [[[control menu] itemAtIndex:0] title];
      }
      LOG(ERROR) <<
          "State: " << [self description] <<
          " Control @" << i << " not ordered correctly: " <<
          [[control description] UTF8String] <<[title UTF8String];
      return false;
    }
    previousControl = control;
  }

  return true;
}

@end // TranslateInfoBarControllerBase (TestingAPI)
