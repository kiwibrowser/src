// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/translate/translate_bubble_controller.h"

#include <utility>

#include "base/mac/foundation_util.h"
#include "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/browser/ui/chrome_pages.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/bubble_combobox.h"
#include "chrome/browser/ui/cocoa/chrome_style.h"
#import "chrome/browser/ui/cocoa/hover_close_button.h"
#import "chrome/browser/ui/cocoa/info_bubble_view.h"
#import "chrome/browser/ui/cocoa/info_bubble_window.h"
#import "chrome/browser/ui/cocoa/location_bar/location_bar_view_mac.h"
#import "chrome/browser/ui/cocoa/location_bar/translate_decoration.h"
#include "chrome/browser/ui/translate/language_combobox_model.h"
#include "chrome/browser/ui/translate/translate_bubble_model.h"
#include "chrome/browser/ui/translate/translate_bubble_view_state_transition.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/theme_resources.h"
#include "components/strings/grit/components_strings.h"
#include "components/translate/core/browser/translate_ui_delegate.h"
#include "content/public/browser/browser_context.h"
#include "content/public/common/referrer.h"
#include "skia/ext/skia_utils_mac.h"
#include "ui/base/cocoa/cocoa_base_utils.h"
#import "ui/base/cocoa/controls/hyperlink_button_cell.h"
#include "ui/base/cocoa/controls/hyperlink_text_view.h"
#import "ui/base/cocoa/window_size_constants.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/combobox_model.h"

// TODO(hajimehoshi): This class is almost same as that of views. Refactor them.
class TranslateDenialComboboxModel : public ui::ComboboxModel {
 public:
  explicit TranslateDenialComboboxModel(
      const base::string16& original_language_name) {
    // Dummy menu item, which is shown on the top of a NSPopUpButton. The top
    // text of the denial pop up menu should be IDS_TRANSLATE_BUBBLE_DENY, while
    // it is impossible to use it here because NSPopUpButtons' addItemWithTitle
    // removes a duplicated menu item. Instead, the title will be set later by
    // NSMenuItem's setTitle.
    items_.push_back(base::string16());

    // Menu items in the drop down menu.
    items_.push_back(l10n_util::GetStringFUTF16(
        IDS_TRANSLATE_BUBBLE_NEVER_TRANSLATE_LANG,
        original_language_name));
    items_.push_back(l10n_util::GetStringUTF16(
        IDS_TRANSLATE_BUBBLE_NEVER_TRANSLATE_SITE));
  }
  ~TranslateDenialComboboxModel() override {}

 private:
  // ComboboxModel:
  int GetItemCount() const override { return items_.size(); }
  base::string16 GetItemAt(int index) override { return items_[index]; }
  bool IsItemSeparatorAt(int index) override { return false; }
  int GetDefaultIndex() const override { return 0; }

  std::vector<base::string16> items_;

  DISALLOW_COPY_AND_ASSIGN(TranslateDenialComboboxModel);
};

const CGFloat kIconWidth = 30;
const CGFloat kIconHeight = 30;
const CGFloat kIconPadding = 12;

const CGFloat kWindowWidth = 320;

// Padding between the window frame and content.
const CGFloat kFramePadding = 16;

const CGFloat kRelatedControlHorizontalSpacing = -2;

const CGFloat kRelatedControlVerticalSpacing = 8;
const CGFloat kUnrelatedControlVerticalSpacing = 20;

const CGFloat kContentWidth = kWindowWidth - 2 * kFramePadding;

@interface TranslateBubbleController()

- (void)performLayout;
- (NSView*)newBeforeTranslateView;
- (NSView*)newTranslatingView;
- (NSView*)newAfterTranslateView;
- (NSView*)newErrorView;
- (NSView*)newAdvancedView;
- (void)updateAdvancedView;
- (void)updateAlwaysCheckboxes;
- (NSImageView*)addIcon:(NSView*)view;
- (NSTextView*)addStyledTextView:(NSString*)message
                          toView:(NSView*)view
                      withRanges:(std::vector<NSRange>)ranges
                        delegate:(id<NSTextViewDelegate>)delegate;
- (NSTextField*)addText:(NSString*)text
                 toView:(NSView*)view;
- (NSButton*)addLinkButtonWithText:(NSString*)text
                            action:(SEL)action
                            toView:(NSView*)view;
- (NSButton*)addButton:(NSString*)title
                action:(SEL)action
                toView:(NSView*)view;
- (NSButton*)addCheckbox:(NSString*)title
                  action:(SEL)action
                  toView:(NSView*)view;
- (NSButton*)addCloseButton:(NSView*)view action:(SEL)action;
- (NSPopUpButton*)addPopUpButton:(ui::ComboboxModel*)model
                          action:(SEL)action
                          toView:(NSView*)view;
- (IBAction)handleAlwaysTranslateCheckboxPressed:(id)sender;
- (IBAction)handleDoneButtonPressed:(id)sender;
- (IBAction)handleCancelButtonPressed:(id)sender;
- (IBAction)handleCloseButtonPressed:(id)sender;
- (IBAction)handleShowOriginalButtonPressed:(id)sender;
- (IBAction)handleTryAgainButtonPressed:(id)sender;
- (IBAction)handleAdvancedLinkButtonPressed:(id)sender;
- (IBAction)handleLanguageSettingsLinkButtonPressed:(id)sender;
- (IBAction)handleDenialPopUpButtonNeverTranslateLanguageSelected:(id)sender;
- (IBAction)handleDenialPopUpButtonNeverTranslateSiteSelected:(id)sender;
- (IBAction)handleSourceLanguagePopUpButtonSelectedItemChanged:(id)sender;
- (IBAction)handleTargetLanguagePopUpButtonSelectedItemChanged:(id)sender;
- (BOOL)textView:(NSTextView*)aTextView
    clickedOnLink:(id)link
          atIndex:(NSUInteger)charIndex;
@end

@implementation TranslateBubbleController {
 @private
  content::WebContents* webContents_;
  std::unique_ptr<TranslateBubbleModel> model_;

  // The 'Done' or 'Translate' button on the advanced (option) panel.
  NSButton* advancedDoneButton_;

  // The 'Cancel' button on the advanced (option) panel.
  NSButton* advancedCancelButton_;

  // The 'Always translate' checkbox on the before panel.
  // This is nil when the current WebContents is in an incognito window.
  NSButton* beforeAlwaysTranslateCheckbox_;

  // The 'Always translate' checkbox on the advanced (option) panel.
  // This is nil when the current WebContents is in an incognito window.
  NSButton* advancedAlwaysTranslateCheckbox_;

  // The '[x]' close button on the upper right side of the before panel.
  NSButton* closeButton_;

  // The combobox model which is used to deny translation at the view before
  // translate.
  std::unique_ptr<TranslateDenialComboboxModel> translateDenialComboboxModel_;

  // The combobox model for source languages on the advanced (option) panel.
  std::unique_ptr<LanguageComboboxModel> sourceLanguageComboboxModel_;

  // The combobox model for target languages on the advanced (option) panel.
  std::unique_ptr<LanguageComboboxModel> targetLanguageComboboxModel_;

  // Whether the translation is actually executed once at least.
  BOOL translateExecuted_;

  // The state of the 'Always ...' checkboxes.
  BOOL shouldAlwaysTranslate_;
}

@synthesize webContents = webContents_;

- (id)initWithParentWindow:(BrowserWindowController*)controller
                     model:(std::unique_ptr<TranslateBubbleModel>)model
               webContents:(content::WebContents*)webContents {
  NSWindow* parentWindow = [controller window];

  // Use an arbitrary size; it will be changed in performLayout.
  NSRect contentRect = ui::kWindowSizeDeterminedLater;
  base::scoped_nsobject<InfoBubbleWindow> window(
      [[InfoBubbleWindow alloc] initWithContentRect:contentRect
                                          styleMask:NSBorderlessWindowMask
                                            backing:NSBackingStoreBuffered
                                              defer:NO]);

  // Disable animations - otherwise, the window/controller will outlive the web
  // contents it's associated with.
  [window setAllowedAnimations:info_bubble::kAnimateNone];

  if ((self = [super initWithWindow:window
                       parentWindow:parentWindow
                         anchoredAt:NSZeroPoint])) {
    webContents_ = webContents;
    model_ = std::move(model);

    shouldAlwaysTranslate_ = model_->ShouldAlwaysTranslate();
    if (!webContents_->GetBrowserContext()->IsOffTheRecord()) {
      shouldAlwaysTranslate_ =
          model_->ShouldAlwaysTranslateBeCheckedByDefault();
    }

    if (model_->GetViewState() !=
        TranslateBubbleModel::VIEW_STATE_BEFORE_TRANSLATE) {
      translateExecuted_ = YES;
    }

    views_.reset([@{
        @(TranslateBubbleModel::VIEW_STATE_BEFORE_TRANSLATE):
            [self newBeforeTranslateView],
        @(TranslateBubbleModel::VIEW_STATE_TRANSLATING):
            [self newTranslatingView],
        @(TranslateBubbleModel::VIEW_STATE_AFTER_TRANSLATE):
            [self newAfterTranslateView],
        @(TranslateBubbleModel::VIEW_STATE_ERROR):
            [self newErrorView],
        @(TranslateBubbleModel::VIEW_STATE_ADVANCED):
            [self newAdvancedView],
    } retain]);

    // The [X] Close Button.
    closeButton_ = [self addCloseButton:[[self window] contentView]
                                 action:@selector(handleCloseButtonPressed:)];

    [self performLayout];
    translate::ReportUiAction(translate::BUBBLE_SHOWN);
  }
  return self;
}

- (void)windowWillClose:(NSNotification*)notification {
  model_->OnBubbleClosing();
  [super windowWillClose:notification];
}

- (NSView*)currentView {
  NSNumber* key = @(model_->GetViewState());
  NSView* view = [views_ objectForKey:key];
  DCHECK(view);
  return view;
}

- (const TranslateBubbleModel*)model {
  return model_.get();
}

- (void)showWindow:(id)sender {
  BrowserWindowController* browserWindowController = [BrowserWindowController
      browserWindowControllerForWindow:self.parentWindow];
  LocationBarViewMac* locationBar = [browserWindowController locationBarBridge];
  if (locationBar) {
    NSPoint anchorPoint =
        locationBar->GetBubblePointForDecoration([self decorationForBubble]);
    anchorPoint =
        ui::ConvertPointFromWindowToScreen([self parentWindow], anchorPoint);
    [self setAnchorPoint:anchorPoint];
  }
  [super showWindow:sender];
}

- (LocationBarDecoration*)decorationForBubble {
  BrowserWindowController* browserWindowController = [BrowserWindowController
      browserWindowControllerForWindow:self.parentWindow];
  LocationBarViewMac* locationBar = [browserWindowController locationBarBridge];
  return locationBar ? locationBar->translate_decoration() : nullptr;
}

- (void)switchView:(TranslateBubbleModel::ViewState)viewState {
  if (model_->GetViewState() == viewState)
    return;

  model_->SetViewState(viewState);
  [self performLayout];
}

- (void)switchToErrorView:(translate::TranslateErrors::Type)errorType {
  [self switchView:TranslateBubbleModel::VIEW_STATE_ERROR];
  model_->ShowError(errorType);
}

- (void)performLayout {
  [self updateAlwaysCheckboxes];
  NSWindow* window = [self window];
  [[window contentView] setSubviews:@[ [self currentView], closeButton_ ]];

  CGFloat height = NSHeight([[self currentView] frame]) +
      2 * kFramePadding + info_bubble::kBubbleArrowHeight;

  NSRect windowFrame = [window contentRectForFrameRect:[[self window] frame]];
  NSRect newWindowFrame = [window frameRectForContentRect:NSMakeRect(
      NSMinX(windowFrame), NSMaxY(windowFrame) - height, kWindowWidth, height)];

  // Adjust the origin of closeButton.
  CGFloat closeX = kWindowWidth - chrome_style::kCloseButtonPadding -
                   chrome_style::GetCloseButtonSize();
  CGFloat closeY = height - kFramePadding - chrome_style::kCloseButtonPadding -
                   info_bubble::kBubbleArrowHeight;
  [closeButton_ setFrameOrigin:NSMakePoint(closeX, closeY)];

  [window setFrame:newWindowFrame
           display:YES
           animate:[[self window] isVisible]];
}

- (NSView*)newBeforeTranslateView {
  NSRect contentFrame = NSMakeRect(
      kFramePadding,
      kFramePadding,
      kContentWidth,
      0);
  NSView* view = [[NSView alloc] initWithFrame:contentFrame];

  base::string16 originalLanguageName =
      model_->GetLanguageNameAt(model_->GetOriginalLanguageIndex());
  base::string16 targetLanguageName =
      model_->GetLanguageNameAt(model_->GetTargetLanguageIndex());

  std::vector<size_t> offsets;
  NSString* message = l10n_util::GetNSStringF(
      IDS_TRANSLATE_BUBBLE_BEFORE_TRANSLATE_NEW, originalLanguageName,
      targetLanguageName, &offsets);
  std::vector<NSRange> ranges;
  ranges.push_back(NSMakeRange(offsets[0], originalLanguageName.length()));
  ranges.push_back(NSMakeRange(offsets[1], targetLanguageName.length()));

  NSTextView* textLabel = [self addStyledTextView:message
                                           toView:view
                                       withRanges:ranges
                                         delegate:self];

  NSString* title =
      l10n_util::GetNSStringWithFixup(IDS_TRANSLATE_BUBBLE_ACCEPT);
  NSButton* translateButton =
      [self addButton:title
               action:@selector(handleTranslateButtonPressed:)
               toView:view];
  [translateButton setKeyEquivalent:@"\r"];

  // TODO(hajimehoshi): When TranslateDenialComboboxModel is factored out as a
  // common model, ui::MenuModel will be used here.
  translateDenialComboboxModel_.reset(
      new TranslateDenialComboboxModel(originalLanguageName));
  NSPopUpButton* denyPopUpButton =
      [self addPopUpButton:translateDenialComboboxModel_.get()
                    action:nil
                    toView:view];
  [denyPopUpButton setPullsDown:YES];
  [[denyPopUpButton itemAtIndex:1] setTarget:self];
  [[denyPopUpButton itemAtIndex:1]
      setAction:@selector(
                    handleDenialPopUpButtonNeverTranslateLanguageSelected:)];
  [[denyPopUpButton itemAtIndex:2] setTarget:self];
  [[denyPopUpButton itemAtIndex:2]
      setAction:@selector(handleDenialPopUpButtonNeverTranslateSiteSelected:)];

  title = base::SysUTF16ToNSString(
      l10n_util::GetStringUTF16(IDS_TRANSLATE_BUBBLE_OPTIONS_MENU_BUTTON));
  [[denyPopUpButton itemAtIndex:0] setTitle:title];

  // Adjust width for the first item.
  base::scoped_nsobject<NSMenu> originalMenu([[denyPopUpButton menu] copy]);
  [denyPopUpButton removeAllItems];
  [denyPopUpButton addItemWithTitle:[[originalMenu itemAtIndex:0] title]];
  [denyPopUpButton sizeToFit];
  [denyPopUpButton setMenu:originalMenu];

  // 'Always translate' checkbox
  if (!webContents_->GetBrowserContext()->IsOffTheRecord()) {
    title =
        l10n_util::GetNSStringWithFixup(IDS_TRANSLATE_BUBBLE_ALWAYS_DO_THIS);
    SEL action = @selector(handleAlwaysTranslateCheckboxPressed:);
    beforeAlwaysTranslateCheckbox_ =
        [self addCheckbox:title action:action toView:view];
  }

  NSImageView* icon = [self addIcon:view];

  // Layout
  CGFloat yPos = 0;

  [translateButton setFrameOrigin:NSMakePoint(
      kContentWidth - NSWidth([translateButton frame]), yPos)];

  NSRect denyPopUpButtonFrame = [denyPopUpButton frame];
  CGFloat diffY = [[denyPopUpButton cell]
    titleRectForBounds:[denyPopUpButton bounds]].origin.y;
  [denyPopUpButton setFrameOrigin:NSMakePoint(
      NSMinX([translateButton frame]) - denyPopUpButtonFrame.size.width
      - kRelatedControlHorizontalSpacing,
      yPos + diffY)];

  yPos += NSHeight([translateButton frame]) +
      kUnrelatedControlVerticalSpacing;

  if (beforeAlwaysTranslateCheckbox_) {
    [beforeAlwaysTranslateCheckbox_
        setFrameOrigin:NSMakePoint(kIconWidth + kIconPadding, yPos)];

    yPos += NSHeight([beforeAlwaysTranslateCheckbox_ frame]) +
            kRelatedControlVerticalSpacing;
  }

  [textLabel setFrameOrigin:NSMakePoint(kIconWidth + kIconPadding, yPos)];

  yPos = NSMaxY([textLabel frame]);
  [icon setFrameOrigin:NSMakePoint(0, yPos - kIconHeight)];
  [view setFrameSize:NSMakeSize(kContentWidth, yPos)];

  return view;
}

- (NSView*)newTranslatingView {
  NSRect contentFrame = NSMakeRect(
      kFramePadding,
      kFramePadding,
      kContentWidth,
      0);
  NSView* view = [[NSView alloc] initWithFrame:contentFrame];

  NSString* message =
      l10n_util::GetNSStringWithFixup(IDS_TRANSLATE_BUBBLE_TRANSLATING);
  NSTextField* textLabel = [self addText:message
                                  toView:view];
  NSString* title =
      l10n_util::GetNSStringWithFixup(IDS_TRANSLATE_BUBBLE_REVERT);
  NSButton* showOriginalButton =
      [self addButton:title
               action:@selector(handleShowOriginalButtonPressed:)
               toView:view];
  [showOriginalButton setEnabled:NO];
  NSImageView* icon = [self addIcon:view];

  // Layout
  // TODO(hajimehoshi): Use l10n_util::VerticallyReflowGroup.
  CGFloat yPos = 0;

  [showOriginalButton setFrameOrigin:NSMakePoint(
      kContentWidth - NSWidth([showOriginalButton frame]), yPos)];

  yPos += NSHeight([showOriginalButton frame]) +
      kUnrelatedControlVerticalSpacing;

  [textLabel setFrameOrigin:NSMakePoint(kIconWidth + kIconPadding, yPos)];

  yPos = NSMaxY([textLabel frame]);
  [icon setFrameOrigin:NSMakePoint(0, yPos - kIconHeight)];
  [view setFrameSize:NSMakeSize(kContentWidth, yPos)];

  return view;
}

- (NSView*)newAfterTranslateView {
  NSRect contentFrame = NSMakeRect(
      kFramePadding,
      kFramePadding,
      kContentWidth,
      0);
  NSView* view = [[NSView alloc] initWithFrame:contentFrame];

  NSString* message =
      l10n_util::GetNSStringWithFixup(IDS_TRANSLATE_BUBBLE_TRANSLATED);
  NSTextField* textLabel = [self addText:message
                                  toView:view];
  message = l10n_util::GetNSStringWithFixup(IDS_TRANSLATE_BUBBLE_ADVANCED_LINK);
  NSButton* advancedLinkButton =
      [self addLinkButtonWithText:message
                           action:@selector(handleAdvancedLinkButtonPressed:)
                           toView:view];
  NSString* title =
      l10n_util::GetNSStringWithFixup(IDS_TRANSLATE_BUBBLE_REVERT);
  NSButton* showOriginalButton =
      [self addButton:title
               action:@selector(handleShowOriginalButtonPressed:)
               toView:view];

  NSImageView* icon = [self addIcon:view];

  // Layout
  CGFloat yPos = 0;

  [showOriginalButton setFrameOrigin:NSMakePoint(
      kContentWidth - NSWidth([showOriginalButton frame]), yPos)];

  yPos += NSHeight([showOriginalButton frame]) +
      kUnrelatedControlVerticalSpacing;

  [textLabel setFrameOrigin:NSMakePoint(kIconWidth + kIconPadding, yPos)];
  [advancedLinkButton setFrameOrigin:NSMakePoint(
      NSMaxX([textLabel frame]), yPos)];

  yPos = NSMaxY([textLabel frame]);
  [icon setFrameOrigin:NSMakePoint(0, yPos - kIconHeight)];
  [view setFrameSize:NSMakeSize(kContentWidth, yPos)];

  return view;
}

- (NSView*)newErrorView {
  NSRect contentFrame = NSMakeRect(
      kFramePadding,
      kFramePadding,
      kContentWidth,
      0);
  NSView* view = [[NSView alloc] initWithFrame:contentFrame];

  NSString* message =
      l10n_util::GetNSString(IDS_TRANSLATE_BUBBLE_COULD_NOT_TRANSLATE);
  NSTextField* textLabel = [self addText:message toView:view];
  message = l10n_util::GetNSStringWithFixup(IDS_TRANSLATE_BUBBLE_ADVANCED_LINK);
  NSButton* advancedLinkButton =
      [self addLinkButtonWithText:message
                           action:@selector(handleAdvancedLinkButtonPressed:)
                           toView:view];
  NSString* title =
      l10n_util::GetNSString(IDS_TRANSLATE_BUBBLE_TRY_AGAIN);
  tryAgainButton_ = [self addButton:title
                             action:@selector(handleTryAgainButtonPressed:)
                             toView:view];

  NSImageView* icon = [self addIcon:view];

  // Layout
  CGFloat yPos = 0;

  [tryAgainButton_
      setFrameOrigin:NSMakePoint(
                         kContentWidth - NSWidth([tryAgainButton_ frame]),
                         yPos)];

  yPos += NSHeight([tryAgainButton_ frame]) + kUnrelatedControlVerticalSpacing;

  [textLabel setFrameOrigin:NSMakePoint(kIconWidth + kIconPadding, yPos)];
  [advancedLinkButton
      setFrameOrigin:NSMakePoint(NSMaxX([textLabel frame]), yPos)];

  yPos = NSMaxY([textLabel frame]);
  [icon setFrameOrigin:NSMakePoint(0, yPos - kIconHeight)];
  [view setFrameSize:NSMakeSize(kContentWidth, yPos)];

  return view;
}

- (NSView*)newAdvancedView {
  NSRect contentFrame = NSMakeRect(
      kFramePadding,
      kFramePadding,
      kContentWidth,
      0);
  NSView* view = [[NSView alloc] initWithFrame:contentFrame];

  NSString* title = l10n_util::GetNSStringWithFixup(
      IDS_TRANSLATE_BUBBLE_PAGE_LANGUAGE);
  NSTextField* sourceLanguageLabel = [self addText:title
                                            toView:view];
  title = l10n_util::GetNSStringWithFixup(
      IDS_TRANSLATE_BUBBLE_TRANSLATION_LANGUAGE);
  NSTextField* targetLanguageLabel = [self addText:title
                                            toView:view];

  // combobox
  int sourceDefaultIndex = model_->GetOriginalLanguageIndex();
  int targetDefaultIndex = model_->GetTargetLanguageIndex();
  sourceLanguageComboboxModel_.reset(
      new LanguageComboboxModel(sourceDefaultIndex, model_.get()));
  targetLanguageComboboxModel_.reset(
      new LanguageComboboxModel(targetDefaultIndex, model_.get()));
  SEL action = @selector(handleSourceLanguagePopUpButtonSelectedItemChanged:);
  NSPopUpButton* sourcePopUpButton =
      [self addPopUpButton:sourceLanguageComboboxModel_.get()
                    action:action
                    toView:view];
  action = @selector(handleTargetLanguagePopUpButtonSelectedItemChanged:);
  NSPopUpButton* targetPopUpButton =
      [self addPopUpButton:targetLanguageComboboxModel_.get()
                    action:action
                    toView:view];

  // 'Always translate' checkbox
  if (!webContents_->GetBrowserContext()->IsOffTheRecord()) {
    NSString* title =
        l10n_util::GetNSStringWithFixup(IDS_TRANSLATE_BUBBLE_ALWAYS);
    action = @selector(handleAlwaysTranslateCheckboxPressed:);
    advancedAlwaysTranslateCheckbox_ =
        [self addCheckbox:title action:action toView:view];
  }

  // Buttons
  advancedDoneButton_ =
      [self addButton:l10n_util::GetNSStringWithFixup(IDS_DONE)
               action:@selector(handleDoneButtonPressed:)
               toView:view];
  [advancedDoneButton_ setKeyEquivalent:@"\r"];
  advancedCancelButton_ =
      [self addButton:l10n_util::GetNSStringWithFixup(IDS_CANCEL)
               action:@selector(handleCancelButtonPressed:)
               toView:view];

  NSString* message = l10n_util::GetNSStringWithFixup(
        IDS_TRANSLATE_BUBBLE_LANGUAGE_SETTINGS);
  action = @selector(handleLanguageSettingsLinkButtonPressed:);
  NSButton* languageSettingsLinkButton =
      [self addLinkButtonWithText:message
                           action:action
                           toView:view];

  // Layout
  CGFloat textLabelWidth = NSWidth([sourceLanguageLabel frame]);
  if (textLabelWidth < NSWidth([targetLanguageLabel frame]))
    textLabelWidth = NSWidth([targetLanguageLabel frame]);

  CGFloat yPos = 0;

  [advancedDoneButton_ setFrameOrigin:NSMakePoint(0, yPos)];
  [advancedCancelButton_ setFrameOrigin:NSMakePoint(0, yPos)];

  [languageSettingsLinkButton setFrameOrigin:NSMakePoint(0, yPos)];

  // Vertical center the languageSettingsLinkButton with the
  // advancedDoneButton_. Move the link position by 1px to make the baseline of
  // the text inside the link vertically align with the text inside the buttons.
  yPos = 1 + floor((NSHeight([advancedDoneButton_ frame]) -
         NSHeight([languageSettingsLinkButton frame])) / 2);
  [languageSettingsLinkButton setFrameOrigin:NSMakePoint(0, yPos)];

  yPos = NSHeight([advancedDoneButton_ frame]) +
      kUnrelatedControlVerticalSpacing;

  if (advancedAlwaysTranslateCheckbox_) {
    [advancedAlwaysTranslateCheckbox_
        setFrameOrigin:NSMakePoint(textLabelWidth, yPos)];

    yPos += NSHeight([advancedAlwaysTranslateCheckbox_ frame]) +
            kRelatedControlVerticalSpacing;
  }

  CGFloat diffY = [[sourcePopUpButton cell]
                   titleRectForBounds:[sourcePopUpButton bounds]].origin.y;

  [targetLanguageLabel setFrameOrigin:NSMakePoint(
      textLabelWidth - NSWidth([targetLanguageLabel frame]), yPos + diffY)];

  NSRect frame = [targetPopUpButton frame];
  frame.origin = NSMakePoint(textLabelWidth, yPos);
  frame.size.width = (kWindowWidth - 2 * kFramePadding) - textLabelWidth;
  [targetPopUpButton setFrame:frame];

  yPos += NSHeight([targetPopUpButton frame]) +
      kRelatedControlVerticalSpacing;

  [sourceLanguageLabel setFrameOrigin:NSMakePoint(
      textLabelWidth - NSWidth([sourceLanguageLabel frame]), yPos + diffY)];

  frame = [sourcePopUpButton frame];
  frame.origin = NSMakePoint(textLabelWidth, yPos);
  frame.size.width = NSWidth([targetPopUpButton frame]);
  [sourcePopUpButton setFrame:frame];

  [view
      setFrameSize:NSMakeSize(kContentWidth, NSMaxY([sourcePopUpButton frame]) +
                                                 kIconPadding)];

  [self updateAdvancedView];

  return view;
}

- (void)updateAdvancedView {
  NSString* title;
  if (model_->IsPageTranslatedInCurrentLanguages())
    title = l10n_util::GetNSStringWithFixup(IDS_DONE);
  else
    title = l10n_util::GetNSStringWithFixup(IDS_TRANSLATE_BUBBLE_ACCEPT);
  [advancedDoneButton_ setTitle:title];
  [advancedDoneButton_ sizeToFit];

  NSRect frame = [advancedDoneButton_ frame];
  frame.origin.x = (kWindowWidth - 2 * kFramePadding) - NSWidth(frame);
  [advancedDoneButton_ setFrameOrigin:frame.origin];

  frame = [advancedCancelButton_ frame];
  frame.origin.x = NSMinX([advancedDoneButton_ frame]) - NSWidth(frame)
      - kRelatedControlHorizontalSpacing;
  [advancedCancelButton_ setFrameOrigin:frame.origin];
}

- (void)updateAlwaysCheckboxes {
  NSInteger state = shouldAlwaysTranslate_ ? NSOnState : NSOffState;
  [beforeAlwaysTranslateCheckbox_ setState:state];
  [advancedAlwaysTranslateCheckbox_ setState:state];
}

- (NSImageView*)addIcon:(NSView*)view {
  NSRect imageFrame = NSMakeRect(0, 0, kIconWidth, kIconHeight);
  base::scoped_nsobject<NSImageView> image(
      [[NSImageView alloc] initWithFrame:imageFrame]);
  [image setImage:(ui::ResourceBundle::GetSharedInstance()
                       .GetImageNamed(IDR_TRANSLATE_BUBBLE_ICON)
                       .ToNSImage())];
  [view addSubview:image];
  return image.get();
}

- (NSTextView*)addStyledTextView:(NSString*)message
                          toView:(NSView*)view
                      withRanges:(std::vector<NSRange>)ranges
                        delegate:(id<NSTextViewDelegate>)delegate {
  NSRect frame =
      NSMakeRect(kFramePadding + kIconWidth + kIconPadding, kFramePadding,
                 kContentWidth - kIconWidth - kIconPadding, 0);
  base::scoped_nsobject<HyperlinkTextView> styledText(
      [[HyperlinkTextView alloc] initWithFrame:frame]);
  [styledText setMessage:message
                withFont:[NSFont systemFontOfSize:[NSFont smallSystemFontSize]]
            messageColor:(skia::SkColorToCalibratedNSColor(SK_ColorBLACK))];
  [styledText setDelegate:delegate];

  NSColor* linkColor =
      skia::SkColorToCalibratedNSColor(chrome_style::GetLinkColor());
  // Create the link with no underlining.
  [styledText setLinkTextAttributes:nil];
  NSTextStorage* storage = [styledText textStorage];
  for (const auto& range : ranges) {
    [styledText addLinkRange:range withURL:nil linkColor:linkColor];
    [storage addAttribute:NSUnderlineStyleAttributeName
                    value:@(NSUnderlineStyleNone)
                    range:range];
  }

  [view addSubview:styledText];
  [styledText setVerticallyResizable:YES];
  [styledText sizeToFit];
  return styledText.get();
}

- (NSTextField*)addText:(NSString*)text
                 toView:(NSView*)view {
  base::scoped_nsobject<NSTextField> textField(
      [[NSTextField alloc] initWithFrame:NSZeroRect]);
  [textField setEditable:NO];
  [textField setSelectable:YES];
  [textField setDrawsBackground:NO];
  [textField setBezeled:NO];
  [textField setStringValue:text];
  NSFont* font = [NSFont systemFontOfSize:[NSFont smallSystemFontSize]];
  [textField setFont:font];
  [textField setAutoresizingMask:NSViewWidthSizable];
  [view addSubview:textField.get()];

  [textField sizeToFit];
  return textField.get();
}

- (NSButton*)addLinkButtonWithText:(NSString*)text
                            action:(SEL)action
                            toView:(NSView*)view {
  base::scoped_nsobject<NSButton> button(
      [[HyperlinkButtonCell buttonWithString:text] retain]);

  [button setButtonType:NSMomentaryPushInButton];
  [button setBezelStyle:NSRegularSquareBezelStyle];
  [button setFont:[NSFont systemFontOfSize:[NSFont smallSystemFontSize]]];
  [button sizeToFit];
  [button setTarget:self];
  [button setAction:action];

  [view addSubview:button.get()];

  return button.get();
}

- (NSButton*)addButton:(NSString*)title
                action:(SEL)action
                toView:(NSView*)view {
  base::scoped_nsobject<NSButton> button(
      [[NSButton alloc] initWithFrame:NSZeroRect]);
  [button setFont:[NSFont systemFontOfSize:[NSFont smallSystemFontSize]]];
  [button setTitle:title];
  [button setBezelStyle:NSRoundedBezelStyle];
  [[button cell] setControlSize:NSSmallControlSize];
  [button sizeToFit];
  [button setTarget:self];
  [button setAction:action];

  [view addSubview:button.get()];

  return button.get();
}

- (NSButton*)addCheckbox:(NSString*)title
                  action:(SEL)action
                  toView:(NSView*)view {
  base::scoped_nsobject<NSButton> button(
      [[NSButton alloc] initWithFrame:NSZeroRect]);
  [button setFont:[NSFont systemFontOfSize:[NSFont smallSystemFontSize]]];
  [button setTitle:title];
  [[button cell] setControlSize:NSSmallControlSize];
  [button setButtonType:NSSwitchButton];
  [button sizeToFit];
  [button setTarget:self];
  [button setAction:action];

  [view addSubview:button.get()];

  return button.get();
}

- (NSButton*)addCloseButton:(NSView*)view action:(SEL)action {
  const int extent = chrome_style::GetCloseButtonSize();
  NSRect frame = NSMakeRect(0, 0, extent, extent);
  base::scoped_nsobject<NSButton> button(
      [[WebUIHoverCloseButton alloc] initWithFrame:frame]);
  [button setTarget:self];
  [button setAction:action];
  [view addSubview:button.get()];
  return button.get();
}

- (NSPopUpButton*)addPopUpButton:(ui::ComboboxModel*)model
                          action:(SEL)action
                          toView:(NSView*)view {
  base::scoped_nsobject<NSPopUpButton> button(
      [[BubbleCombobox alloc] initWithFrame:NSZeroRect
                                  pullsDown:NO
                                      model:model]);
  [button setTarget:self];
  [button setAction:action];
  [button sizeToFit];
  [view addSubview:button.get()];
  return button.get();
}

- (IBAction)handleTranslateButtonPressed:(id)sender {
  model_->SetAlwaysTranslate(shouldAlwaysTranslate_);
  translate::ReportUiAction(translate::TRANSLATE_BUTTON_CLICKED);
  translateExecuted_ = YES;
  model_->Translate();
}

- (IBAction)handleAlwaysTranslateCheckboxPressed:(id)sender {
  shouldAlwaysTranslate_ = [sender state] == NSOnState;
  translate::ReportUiAction(shouldAlwaysTranslate_
                                ? translate::ALWAYS_TRANSLATE_CHECKED
                                : translate::ALWAYS_TRANSLATE_UNCHECKED);
}

- (IBAction)handleDoneButtonPressed:(id)sender {
  translate::ReportUiAction(translate::DONE_BUTTON_CLICKED);
  model_->SetAlwaysTranslate(shouldAlwaysTranslate_);
  if (model_->IsPageTranslatedInCurrentLanguages()) {
    model_->GoBackFromAdvanced();
    [self performLayout];
  } else {
    translateExecuted_ = true;
    model_->Translate();
    [self switchView:TranslateBubbleModel::VIEW_STATE_TRANSLATING];
  }
}

- (IBAction)handleCancelButtonPressed:(id)sender {
  translate::ReportUiAction(translate::CANCEL_BUTTON_CLICKED);
  model_->GoBackFromAdvanced();
  [self performLayout];
}

- (IBAction)handleCloseButtonPressed:(id)sender {
  model_->DeclineTranslation();
  translate::ReportUiAction(translate::CLOSE_BUTTON_CLICKED);
  [self close];
}

- (IBAction)handleShowOriginalButtonPressed:(id)sender {
  translate::ReportUiAction(translate::SHOW_ORIGINAL_BUTTON_CLICKED);
  model_->RevertTranslation();
  [self close];
}

- (IBAction)handleTryAgainButtonPressed:(id)sender {
  model_->Translate();
  translate::ReportUiAction(translate::TRY_AGAIN_BUTTON_CLICKED);
}

- (IBAction)handleAdvancedLinkButtonPressed:(id)sender {
  translate::ReportUiAction(translate::ADVANCED_LINK_CLICKED);
  [self switchView:TranslateBubbleModel::VIEW_STATE_ADVANCED];
}

- (IBAction)handleLanguageSettingsLinkButtonPressed:(id)sender {
  GURL url = chrome::GetSettingsUrl(chrome::kLanguageOptionsSubPage);
  webContents_->OpenURL(content::OpenURLParams(
      url, content::Referrer(), WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui::PAGE_TRANSITION_LINK, false));
  translate::ReportUiAction(translate::SETTINGS_LINK_CLICKED);
  [self close];
}

- (IBAction)handleDenialPopUpButtonNeverTranslateLanguageSelected:(id)sender {
  translate::ReportUiAction(translate::NEVER_TRANSLATE_LANGUAGE_MENU_CLICKED);
  model_->DeclineTranslation();
  model_->SetNeverTranslateLanguage(true);
  [self close];
}

- (IBAction)handleDenialPopUpButtonNeverTranslateSiteSelected:(id)sender {
  translate::ReportUiAction(translate::NEVER_TRANSLATE_SITE_MENU_CLICKED);
  model_->DeclineTranslation();
  model_->SetNeverTranslateSite(true);
  [self close];
}

- (IBAction)handleSourceLanguagePopUpButtonSelectedItemChanged:(id)sender {
  translate::ReportUiAction(translate::SOURCE_LANGUAGE_MENU_CLICKED);
  NSPopUpButton* button = base::mac::ObjCCastStrict<NSPopUpButton>(sender);
  model_->UpdateOriginalLanguageIndex([button indexOfSelectedItem]);
  [self updateAdvancedView];
}

- (IBAction)handleTargetLanguagePopUpButtonSelectedItemChanged:(id)sender {
  translate::ReportUiAction(translate::TARGET_LANGUAGE_MENU_CLICKED);
  NSPopUpButton* button = base::mac::ObjCCastStrict<NSPopUpButton>(sender);
  model_->UpdateTargetLanguageIndex([button indexOfSelectedItem]);
  [self updateAdvancedView];
}

// The NSTextViewDelegate method which called when user click on the
// source or target language on the before translate view.
- (BOOL)textView:(NSTextView*)aTextView
    clickedOnLink:(id)link
          atIndex:(NSUInteger)charIndex {
  translate::ReportUiAction(translate::ADVANCED_LINK_CLICKED);
  [self switchView:TranslateBubbleModel::VIEW_STATE_ADVANCED];
  return YES;
}

@end
