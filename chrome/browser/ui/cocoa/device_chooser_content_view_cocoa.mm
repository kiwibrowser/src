// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/device_chooser_content_view_cocoa.h"

#include <algorithm>
#include <cmath>

#include "base/macros.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/browser/chooser_controller/chooser_controller.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_button.h"
#include "chrome/browser/ui/cocoa/spinner_view.h"
#include "chrome/grit/generated_resources.h"
#include "components/vector_icons/vector_icons.h"
#include "skia/ext/skia_utils_mac.h"
#import "third_party/google_toolbox_for_mac/src/AppKit/GTMUILocalizerAndLayoutTweaker.h"
#import "ui/base/cocoa/controls/hyperlink_button_cell.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/image/image_skia_util_mac.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/resources/grit/ui_resources.h"

namespace {

// Chooser width.
const CGFloat kChooserWidth = 390.0f;

// Chooser height.
const CGFloat kChooserHeight = 330.0f;

// Row view image size.
const CGFloat kRowViewImageSize = 20.0f;

// Table row view height.
const CGFloat kTableRowViewOneLineHeight = 23.0f;
const CGFloat kTableRowViewTwoLinesHeight = 39.0f;

// Spinner size.
const CGFloat kSpinnerSize = 24.0f;

// Distance between the chooser border and the view that is closest to the
// border.
const CGFloat kMarginX = 20.0f;
const CGFloat kMarginY = 20.0f;

// Distance between two views inside the chooser.
const CGFloat kHorizontalPadding = 10.0f;
const CGFloat kVerticalPadding = 10.0f;

// Separator alpha value.
const CGFloat kSeparatorAlphaValue = 0.6f;

// Separator height.
const CGFloat kSeparatorHeight = 1.0f;

// Distance between two views inside the table row view.
const CGFloat kTableRowViewHorizontalPadding = 5.0f;
const CGFloat kTableRowViewVerticalPadding = 1.0f;

// The lookup table for signal strength level image.
const int kSignalStrengthLevelImageIds[5] = {IDR_SIGNAL_0_BAR, IDR_SIGNAL_1_BAR,
                                             IDR_SIGNAL_2_BAR, IDR_SIGNAL_3_BAR,
                                             IDR_SIGNAL_4_BAR};
const int kSignalStrengthLevelImageSelectedIds[5] = {
    IDR_SIGNAL_0_BAR_SELECTED, IDR_SIGNAL_1_BAR_SELECTED,
    IDR_SIGNAL_2_BAR_SELECTED, IDR_SIGNAL_3_BAR_SELECTED,
    IDR_SIGNAL_4_BAR_SELECTED};

// Creates a label with |text|.
base::scoped_nsobject<NSTextField> CreateLabel(NSString* text) {
  base::scoped_nsobject<NSTextField> label(
      [[NSTextField alloc] initWithFrame:NSZeroRect]);
  [label setDrawsBackground:NO];
  [label setBezeled:NO];
  [label setEditable:NO];
  [label setSelectable:NO];
  [label setStringValue:text];
  [label setFont:[NSFont systemFontOfSize:[NSFont systemFontSize]]];
  [label setTextColor:[NSColor blackColor]];
  [label sizeToFit];
  return label;
}

}  // namespace

// A table row view that contains one line of text, and optionally contains an
// image in front of the text.
@interface ChooserContentTableRowView : NSView {
 @private
  base::scoped_nsobject<NSImageView> image_;
  base::scoped_nsobject<NSTextField> text_;
  base::scoped_nsobject<NSTextField> pairedStatus_;
}

// Designated initializer.
// This initializer is used when the chooser needs to show the no-devices-found
// message.
- (instancetype)initWithText:(NSString*)text;

// Designated initializer.
- (instancetype)initWithText:(NSString*)text
         signalStrengthLevel:(NSInteger)level
                 isConnected:(bool)isConnected
                    isPaired:(bool)isPaired
                   rowHeight:(CGFloat)rowHeight
         textNeedIndentation:(bool)textNeedIndentation;

// Gets the image in front of the text.
- (NSImageView*)image;

// Gets the text.
- (NSTextField*)text;

// Gets the paired status.
- (NSTextField*)pairedStatus;

@end

@implementation ChooserContentTableRowView

- (instancetype)initWithText:(NSString*)text {
  if ((self = [super initWithFrame:NSZeroRect])) {
    text_ = CreateLabel(text);
    CGFloat textHeight = NSHeight([text_ frame]);
    CGFloat textOriginX = kTableRowViewHorizontalPadding;
    CGFloat textOriginY = (kTableRowViewOneLineHeight - textHeight) / 2;
    [text_ setFrameOrigin:NSMakePoint(textOriginX, textOriginY)];
    [self addSubview:text_];
  }

  return self;
}

- (instancetype)initWithText:(NSString*)text
         signalStrengthLevel:(NSInteger)level
                 isConnected:(bool)isConnected
                    isPaired:(bool)isPaired
                   rowHeight:(CGFloat)rowHeight
         textNeedIndentation:(bool)textNeedIndentation {
  if ((self = [super initWithFrame:NSZeroRect])) {
    // Create the views.
    // Image.
    ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
    NSImage* image = nullptr;
    if (isConnected) {
      image = gfx::NSImageFromImageSkia(gfx::CreateVectorIcon(
          vector_icons::kBluetoothConnectedIcon, gfx::kChromeIconGrey));
    } else if (level != -1) {
      DCHECK_GE(level, 0);
      DCHECK_LT(level, base::checked_cast<NSInteger>(
                           arraysize(kSignalStrengthLevelImageIds)));
      image = rb.GetNativeImageNamed(kSignalStrengthLevelImageIds[level])
                  .ToNSImage();
    }

    CGFloat imageOriginX = kTableRowViewHorizontalPadding;
    CGFloat imageOriginY = (rowHeight - kRowViewImageSize) / 2;
    if (image) {
      image_.reset([[NSImageView alloc]
          initWithFrame:NSMakeRect(imageOriginX, imageOriginY,
                                   kRowViewImageSize, kRowViewImageSize)]);
      [image_ setImage:image];
      [self addSubview:image_];
    }

    // Text.
    text_ = CreateLabel(text);
    CGFloat textHeight = NSHeight([text_ frame]);

    // Paired status.
    CGFloat pairedStatusHeight = 0.0f;
    if (isPaired) {
      pairedStatus_ = CreateLabel(
          l10n_util::GetNSString(IDS_DEVICE_CHOOSER_PAIRED_STATUS_TEXT));
      [pairedStatus_
          setTextColor:skia::SkColorToCalibratedNSColor(gfx::kGoogleGreen700)];
      pairedStatusHeight = NSHeight([pairedStatus_ frame]);
    }

    // Lay out the views.
    // Text.
    CGFloat textOriginX = kTableRowViewHorizontalPadding;
    if (textNeedIndentation)
      textOriginX += imageOriginX + kRowViewImageSize;
    CGFloat textOriginY;
    if (isPaired) {
      textOriginY = pairedStatusHeight +
                    (rowHeight - textHeight - pairedStatusHeight -
                     kTableRowViewVerticalPadding) /
                        2 +
                    kTableRowViewVerticalPadding;
    } else {
      textOriginY = (rowHeight - textHeight) / 2;
    }

    [text_ setFrameOrigin:NSMakePoint(textOriginX, textOriginY)];
    [self addSubview:text_];

    // Paired status.
    if (isPaired) {
      CGFloat pairedStatusOriginX = textOriginX;
      CGFloat pairedStatusOriginY =
          (rowHeight - textHeight - pairedStatusHeight -
           kTableRowViewVerticalPadding) /
          2;
      [pairedStatus_
          setFrameOrigin:NSMakePoint(pairedStatusOriginX, pairedStatusOriginY)];
      [self addSubview:pairedStatus_];
    }
  }

  return self;
}

- (NSImageView*)image {
  return image_.get();
}

- (NSTextField*)text {
  return text_.get();
}

- (NSTextField*)pairedStatus {
  return pairedStatus_.get();
}

@end

class ChooserContentViewController : public ChooserController::View {
 public:
  ChooserContentViewController(ChooserController* chooser_controller,
                               NSButton* adapter_off_help_button,
                               NSTextField* adapter_off_message,
                               NSTableView* table_view,
                               SpinnerView* spinner,
                               NSTextField* scanning_message,
                               NSTextField* word_connector,
                               NSButton* rescan_button);
  ~ChooserContentViewController() override;

  // ChooserController::View:
  void OnOptionsInitialized() override;
  void OnOptionAdded(size_t index) override;
  void OnOptionRemoved(size_t index) override;
  void OnOptionUpdated(size_t index) override;
  void OnAdapterEnabledChanged(bool enabled) override;
  void OnRefreshStateChanged(bool refreshing) override;

  void UpdateTableView();

 private:
  ChooserController* chooser_controller_;
  NSButton* adapter_off_help_button_;
  NSTextField* adapter_off_message_;
  NSTableView* table_view_;
  SpinnerView* spinner_;
  NSTextField* scanning_message_;
  NSTextField* word_connector_;
  NSButton* rescan_button_;

  DISALLOW_COPY_AND_ASSIGN(ChooserContentViewController);
};

ChooserContentViewController::ChooserContentViewController(
    ChooserController* chooser_controller,
    NSButton* adapter_off_help_button,
    NSTextField* adapter_off_message,
    NSTableView* table_view,
    SpinnerView* spinner,
    NSTextField* scanning_message,
    NSTextField* word_connector,
    NSButton* rescan_button)
    : chooser_controller_(chooser_controller),
      adapter_off_help_button_(adapter_off_help_button),
      adapter_off_message_(adapter_off_message),
      table_view_(table_view),
      spinner_(spinner),
      scanning_message_(scanning_message),
      word_connector_(word_connector),
      rescan_button_(rescan_button) {
  DCHECK(chooser_controller_);
  DCHECK(adapter_off_help_button_);
  DCHECK(adapter_off_message_);
  DCHECK(table_view_);
  DCHECK(spinner_);
  DCHECK(scanning_message_);
  DCHECK(word_connector_);
  DCHECK(rescan_button_);
  chooser_controller_->set_view(this);
}

ChooserContentViewController::~ChooserContentViewController() {
  chooser_controller_->set_view(nullptr);
}

void ChooserContentViewController::OnOptionsInitialized() {
  UpdateTableView();
}

void ChooserContentViewController::OnOptionAdded(size_t index) {
  UpdateTableView();
  [table_view_ setHidden:NO];
  [spinner_ setHidden:YES];
}

void ChooserContentViewController::OnOptionRemoved(size_t index) {
  // |table_view_| will automatically select the removed item's next item.
  // So here it tracks if the removed item is the item that was currently
  // selected, if so, deselect it. Also if the removed item is before the
  // currently selected item, the currently selected item's index needs to
  // be adjusted by one.
  NSIndexSet* selected_rows = [table_view_ selectedRowIndexes];
  NSMutableIndexSet* updated_selected_rows = [NSMutableIndexSet indexSet];
  NSUInteger row = [selected_rows firstIndex];
  while (row != NSNotFound) {
    if (row < index)
      [updated_selected_rows addIndex:row];
    else if (row > index)
      [updated_selected_rows addIndex:row - 1];
    row = [selected_rows indexGreaterThanIndex:row];
  }

  [table_view_ selectRowIndexes:updated_selected_rows byExtendingSelection:NO];

  UpdateTableView();
}

void ChooserContentViewController::OnOptionUpdated(size_t index) {
  UpdateTableView();
}

void ChooserContentViewController::OnAdapterEnabledChanged(bool enabled) {
  // No row is selected since the adapter status has changed.
  // This will also disable the OK button if it was enabled because
  // of a previously selected row.
  [table_view_ deselectAll:nil];
  UpdateTableView();
  [table_view_ setHidden:enabled ? NO : YES];
  [adapter_off_help_button_ setHidden:enabled ? YES : NO];
  [adapter_off_message_ setHidden:enabled ? YES : NO];

  [spinner_ setHidden:YES];

  [scanning_message_ setHidden:YES];
  // When adapter is enabled, show |word_connector_| and |rescan_button_|;
  // otherwise hide them.
  [word_connector_ setHidden:enabled ? NO : YES];
  [rescan_button_ setHidden:enabled ? NO : YES];
}

void ChooserContentViewController::OnRefreshStateChanged(bool refreshing) {
  if (refreshing) {
    // No row is selected since the chooser is refreshing.
    // This will also disable the OK button if it was enabled because
    // of a previously selected row.
    [table_view_ deselectAll:nil];
    UpdateTableView();
  }

  // When refreshing and no option available yet, hide |table_view_| and show
  // |spinner_|. Otherwise show |table_view_| and hide |spinner_|.
  bool table_view_hidden =
      refreshing && (chooser_controller_->NumOptions() == 0);
  [table_view_ setHidden:table_view_hidden ? YES : NO];
  [spinner_ setHidden:table_view_hidden ? NO : YES];

  // When refreshing, show |scanning_message_| and hide |word_connector_| and
  // |rescan_button_|.
  // When complete, show |word_connector_| and |rescan_button_| and hide
  // |scanning_message_|.
  [scanning_message_ setHidden:refreshing ? NO : YES];
  [word_connector_ setHidden:refreshing ? YES : NO];
  [rescan_button_ setHidden:refreshing ? YES : NO];
}

void ChooserContentViewController::UpdateTableView() {
  [table_view_ setEnabled:chooser_controller_->NumOptions() > 0];
  // For NSView-based table views, calling reloadData will deselect the
  // currently selected row, so |selected_rows| stores the currently selected
  // rows in order to select them again.
  NSIndexSet* selected_rows = [table_view_ selectedRowIndexes];
  [table_view_ reloadData];
  [table_view_ selectRowIndexes:selected_rows byExtendingSelection:NO];
}

@implementation DeviceChooserContentViewCocoa

- (instancetype)initWithChooserTitle:(NSString*)chooserTitle
                   chooserController:
                       (std::unique_ptr<ChooserController>)chooserController {
  // ------------------------------------
  // | Chooser title                    |
  // | -------------------------------- |
  // | | option 0                     | |
  // | | option 1                     | |
  // | | option 2                     | |
  // | |                              | |
  // | |                              | |
  // | |                              | |
  // | -------------------------------- |
  // |           [ Cancel ] [ Connect ] |
  // |----------------------------------|
  // | Get help                         |
  // ------------------------------------

  // Determine the dimensions of the chooser.
  // Once the height and width are set, the buttons and permission menus can
  // be laid out correctly.
  NSRect chooserFrame = NSMakeRect(0, 0, kChooserWidth, kChooserHeight);

  if ((self = [super initWithFrame:chooserFrame])) {
    chooserController_ = std::move(chooserController);

    // Create the views.
    // Title.
    titleView_ = [self createChooserTitle:chooserTitle];
    CGFloat titleHeight = NSHeight([titleView_ frame]);

    // Adapter turned off help button.
    adapterOffHelpButton_ = [self
        createHyperlinkButtonWithText:
            l10n_util::GetNSString(
                IDS_BLUETOOTH_DEVICE_CHOOSER_TURN_ON_BLUETOOTH_LINK_TEXT)];
    CGFloat adapterOffHelpButtonWidth = NSWidth([adapterOffHelpButton_ frame]);
    CGFloat adapterOffHelpButtonHeight =
        NSHeight([adapterOffHelpButton_ frame]);

    // Adapter turned off message.
    adapterOffMessage_ = CreateLabel(l10n_util::GetNSStringF(
        IDS_BLUETOOTH_DEVICE_CHOOSER_TURN_ADAPTER_OFF, base::string16()));
    CGFloat adapterOffMessageWidth = NSWidth([adapterOffMessage_ frame]);

    // Cancel button.
    cancelButton_ = [self createCancelButton];
    CGFloat cancelButtonWidth = NSWidth([cancelButton_ frame]);
    CGFloat cancelButtonHeight = NSHeight([cancelButton_ frame]);

    // Connect button.
    connectButton_ = [self createConnectButton];
    CGFloat connectButtonWidth = NSWidth([connectButton_ frame]);

    // Separator.
    separator_ = [self createSeparator];

    // Help button.
    CGFloat helpButtonWidth = 0.0f;
    CGFloat helpButtonHeight = 0.0f;
    if (chooserController_->ShouldShowHelpButton()) {
      helpButton_ =
          [self createHyperlinkButtonWithText:
                    l10n_util::GetNSStringF(
                        IDS_DEVICE_CHOOSER_GET_HELP_LINK_WITH_SCANNING_STATUS,
                        base::string16())];
      helpButtonWidth = NSWidth([helpButton_ frame]);
      helpButtonHeight = NSHeight([helpButton_ frame]);
    }

    // Scanning message.
    scanningMessage_ = CreateLabel(
        l10n_util::GetNSString(IDS_BLUETOOTH_DEVICE_CHOOSER_SCANNING));

    // Word connector.
    wordConnector_ = CreateLabel(l10n_util::GetNSStringF(
        IDS_DEVICE_CHOOSER_GET_HELP_LINK_WITH_RE_SCAN_LINK, base::string16(),
        base::string16()));
    CGFloat wordConnectorWidth = NSWidth([wordConnector_ frame]);

    // Re-scan button.
    rescanButton_ =
        [self createHyperlinkButtonWithText:
                  l10n_util::GetNSString(IDS_BLUETOOTH_DEVICE_CHOOSER_RE_SCAN)];

    // ScollView embedding with TableView.
    CGFloat scrollViewWidth = kChooserWidth - 2 * kMarginX;
    CGFloat scrollViewHeight =
        kChooserHeight - 2 * kMarginY -
        (chooserController_->ShouldShowHelpButton() ? 4 * kVerticalPadding
                                                    : 2 * kVerticalPadding) -
        titleHeight - cancelButtonHeight - helpButtonHeight;
    CGFloat scrollViewOriginX = kMarginX;
    CGFloat scrollViewOriginY =
        kMarginY + helpButtonHeight +
        (chooserController_->ShouldShowHelpButton() ? 3 * kVerticalPadding
                                                    : kVerticalPadding) +
        cancelButtonHeight;
    NSRect scrollFrame = NSMakeRect(scrollViewOriginX, scrollViewOriginY,
                                    scrollViewWidth, scrollViewHeight);
    scrollView_.reset([[NSScrollView alloc] initWithFrame:scrollFrame]);
    [scrollView_ setBorderType:NSBezelBorder];
    [scrollView_ setHasVerticalScroller:YES];
    [scrollView_ setHasHorizontalScroller:YES];
    [scrollView_ setAutohidesScrollers:YES];
    [scrollView_ setDrawsBackground:NO];

    // TableView.
    tableView_.reset([[NSTableView alloc] initWithFrame:NSZeroRect]);
    tableColumn_.reset([[NSTableColumn alloc] initWithIdentifier:@""]);
    [tableColumn_ setWidth:(scrollViewWidth - kMarginX)];
    [tableView_ addTableColumn:tableColumn_];
    // Make the column title invisible.
    [tableView_ setHeaderView:nil];
    [tableView_ setFocusRingType:NSFocusRingTypeNone];
    [tableView_
        setAllowsMultipleSelection:chooserController_->AllowMultipleSelection()
                                       ? YES
                                       : NO];

    // Spinner.
    // Set the spinner in the center of the scroll view.
    CGFloat spinnerOriginX =
        scrollViewOriginX + (scrollViewWidth - kSpinnerSize) / 2;
    CGFloat spinnerOriginY =
        scrollViewOriginY + (scrollViewHeight - kSpinnerSize) / 2;
    spinner_.reset([[SpinnerView alloc]
        initWithFrame:NSMakeRect(spinnerOriginX, spinnerOriginY, kSpinnerSize,
                                 kSpinnerSize)]);

    // Lay out the views.
    // Title.
    CGFloat titleOriginX = kMarginX;
    CGFloat titleOriginY = kChooserHeight - kMarginY - titleHeight;
    [titleView_ setFrameOrigin:NSMakePoint(titleOriginX, titleOriginY)];
    [self addSubview:titleView_];

    // Adapter turned off help button.
    CGFloat adapterOffHelpButtonOriginX = std::floor(
        scrollViewOriginX +
        (scrollViewWidth - adapterOffHelpButtonWidth - adapterOffMessageWidth) /
            2);
    CGFloat adapterOffHelpButtonOriginY =
        std::floor(scrollViewOriginY +
                   (scrollViewHeight - adapterOffHelpButtonHeight) / 2);
    [adapterOffHelpButton_
        setFrameOrigin:NSMakePoint(adapterOffHelpButtonOriginX,
                                   adapterOffHelpButtonOriginY)];
    [adapterOffHelpButton_ setTarget:self];
    [adapterOffHelpButton_ setAction:@selector(onAdapterOffHelp:)];
    [adapterOffHelpButton_ setHidden:YES];
    [self addSubview:adapterOffHelpButton_];

    // Adapter turned off message.
    CGFloat adapterOffMessageOriginX = adapterOffHelpButtonOriginX +
                                       adapterOffHelpButtonWidth -
                                       kHorizontalPadding / 2;
    CGFloat adapterOffMessageOriginY = adapterOffHelpButtonOriginY;
    [adapterOffMessage_ setFrameOrigin:NSMakePoint(adapterOffMessageOriginX,
                                                   adapterOffMessageOriginY)];
    [adapterOffMessage_ setHidden:YES];
    [self addSubview:adapterOffMessage_];

    // ScollView and Spinner. Only one of them is shown.
    [scrollView_ setDocumentView:tableView_];
    [self addSubview:scrollView_];
    [spinner_ setHidden:YES];
    [self addSubview:spinner_];

    // Cancel button.
    CGFloat cancelButtonOriginX = kChooserWidth - kMarginX -
                                  kHorizontalPadding - cancelButtonWidth -
                                  connectButtonWidth;
    CGFloat cancelButtonOriginY =
        kMarginY + helpButtonHeight +
        (chooserController_->ShouldShowHelpButton() ? 2 * kVerticalPadding
                                                    : 0.0f);
    [cancelButton_
        setFrameOrigin:NSMakePoint(cancelButtonOriginX, cancelButtonOriginY)];
    [self addSubview:cancelButton_];

    // Connect button.
    CGFloat connectButtonOriginX =
        kChooserWidth - kMarginX - connectButtonWidth;
    CGFloat connectButtonOriginY = cancelButtonOriginY;
    [connectButton_
        setFrameOrigin:NSMakePoint(connectButtonOriginX, connectButtonOriginY)];
    [connectButton_ setEnabled:NO];
    [self addSubview:connectButton_];

    if (chooserController_->ShouldShowHelpButton()) {
      // Separator.
      CGFloat separatorOriginX = 0.0f;
      separatorOriginY_ = kMarginY + helpButtonHeight + kVerticalPadding;
      [separator_
          setFrameOrigin:NSMakePoint(separatorOriginX, separatorOriginY_)];
      [self addSubview:separator_];

      // Help button.
      CGFloat helpButtonOriginX = kMarginX;
      CGFloat helpButtonOriginY = (kVerticalPadding + kMarginY) / 2;
      [helpButton_
          setFrameOrigin:NSMakePoint(helpButtonOriginX, helpButtonOriginY)];
      [helpButton_ setTarget:self];
      [helpButton_ setAction:@selector(onHelpPressed:)];
      [self addSubview:helpButton_];

      // Scanning message.
      CGFloat scanningMessageOriginX =
          kMarginX + helpButtonWidth - kHorizontalPadding / 2;
      CGFloat scanningMessageOriginY = helpButtonOriginY;
      [scanningMessage_ setFrameOrigin:NSMakePoint(scanningMessageOriginX,
                                                   scanningMessageOriginY)];
      [self addSubview:scanningMessage_];
      [scanningMessage_ setHidden:YES];

      // Word connector.
      CGFloat wordConnectorOriginX =
          kMarginX + helpButtonWidth - kHorizontalPadding / 2;
      CGFloat wordConnectorOriginY = helpButtonOriginY;
      [wordConnector_ setFrameOrigin:NSMakePoint(wordConnectorOriginX,
                                                 wordConnectorOriginY)];
      [self addSubview:wordConnector_];
      [wordConnector_ setHidden:YES];

      // Re-scan button.
      CGFloat reScanButtonOriginX =
          kMarginX + helpButtonWidth + wordConnectorWidth - kHorizontalPadding;
      CGFloat reScanButtonOriginY = helpButtonOriginY;
      [rescanButton_
          setFrameOrigin:NSMakePoint(reScanButtonOriginX, reScanButtonOriginY)];
      [rescanButton_ setTarget:self];
      [rescanButton_ setAction:@selector(onRescan:)];
      [self addSubview:rescanButton_];
      [rescanButton_ setHidden:YES];
    }

    chooserContentViewController_.reset(new ChooserContentViewController(
        chooserController_.get(), adapterOffHelpButton_.get(),
        adapterOffMessage_.get(), tableView_.get(), spinner_.get(),
        scanningMessage_.get(), wordConnector_.get(), rescanButton_.get()));
  }

  return self;
}

- (base::scoped_nsobject<NSTextField>)createChooserTitle:(NSString*)title {
  base::scoped_nsobject<NSTextField> titleView(
      [[NSTextField alloc] initWithFrame:NSZeroRect]);
  [titleView setDrawsBackground:NO];
  [titleView setBezeled:NO];
  [titleView setEditable:NO];
  [titleView setSelectable:NO];
  [titleView setStringValue:title];
  [titleView setFont:[NSFont systemFontOfSize:[NSFont systemFontSize]]];
  // The height is arbitrary as it will be adjusted later.
  [titleView setFrameSize:NSMakeSize(kChooserWidth - 2 * kMarginX, 0.0f)];
  [GTMUILocalizerAndLayoutTweaker sizeToFitFixedWidthTextField:titleView];
  return titleView;
}

- (base::scoped_nsobject<NSView>)createTableRowView:(NSInteger)row {
  NSInteger level = -1;
  bool isConnected = false;
  bool isPaired = false;
  size_t numOptions = chooserController_->NumOptions();
  base::scoped_nsobject<NSView> tableRowView;
  if (numOptions == 0) {
    DCHECK_EQ(0, row);
    tableRowView.reset([[ChooserContentTableRowView alloc]
        initWithText:[self optionAtIndex:row]]);
  } else {
    DCHECK_GE(row, 0);
    DCHECK_LT(row, base::checked_cast<NSInteger>(numOptions));
    size_t rowIndex = base::checked_cast<size_t>(row);
    if (chooserController_->ShouldShowIconBeforeText()) {
      level = base::checked_cast<NSInteger>(
          chooserController_->GetSignalStrengthLevel(rowIndex));
    }
    isConnected = chooserController_->IsConnected(rowIndex);
    isPaired = chooserController_->IsPaired(rowIndex);
    bool textNeedIndentation = false;
    for (size_t i = 0; i < numOptions; ++i) {
      if (chooserController_->GetSignalStrengthLevel(i) != -1 ||
          chooserController_->IsConnected(i)) {
        textNeedIndentation = true;
        break;
      }
    }
    tableRowView.reset([[ChooserContentTableRowView alloc]
               initWithText:[self optionAtIndex:row]
        signalStrengthLevel:level
                isConnected:isConnected
                   isPaired:isPaired
                  rowHeight:[self tableRowViewHeight:row]
        textNeedIndentation:textNeedIndentation]);
  }

  return tableRowView;
}

- (CGFloat)tableRowViewHeight:(NSInteger)row {
  size_t numOptions = chooserController_->NumOptions();
  if (numOptions == 0) {
    DCHECK_EQ(0, row);
    return kTableRowViewOneLineHeight;
  }

  DCHECK_GE(row, 0);
  DCHECK_LT(row, base::checked_cast<NSInteger>(numOptions));
  size_t rowIndex = base::checked_cast<size_t>(row);
  return chooserController_->IsPaired(rowIndex) ? kTableRowViewTwoLinesHeight
                                                : kTableRowViewOneLineHeight;
}

- (base::scoped_nsobject<NSButton>)createButtonWithTitle:(NSString*)title {
  base::scoped_nsobject<NSButton> button(
      [[ConstrainedWindowButton alloc] initWithFrame:NSZeroRect]);
  [button setButtonType:NSMomentaryPushInButton];
  [button setTitle:title];
  [button sizeToFit];
  return button;
}

- (base::scoped_nsobject<NSButton>)createConnectButton {
  NSString* connectTitle =
      base::SysUTF16ToNSString(chooserController_->GetOkButtonLabel());
  return [self createButtonWithTitle:connectTitle];
}

- (base::scoped_nsobject<NSButton>)createCancelButton {
  NSString* cancelTitle =
      l10n_util::GetNSString(IDS_DEVICE_CHOOSER_CANCEL_BUTTON_TEXT);
  return [self createButtonWithTitle:cancelTitle];
}

- (base::scoped_nsobject<NSBox>)createSeparator {
  base::scoped_nsobject<NSBox> spacer([[NSBox alloc] initWithFrame:NSZeroRect]);
  [spacer setBoxType:NSBoxSeparator];
  [spacer setBorderType:NSLineBorder];
  [spacer setAlphaValue:kSeparatorAlphaValue];
  [spacer setFrameSize:NSMakeSize(kChooserWidth, kSeparatorHeight)];
  return spacer;
}

- (base::scoped_nsobject<NSButton>)createHyperlinkButtonWithText:
    (NSString*)text {
  base::scoped_nsobject<NSButton> button(
      [[NSButton alloc] initWithFrame:NSZeroRect]);
  base::scoped_nsobject<HyperlinkButtonCell> cell(
      [[HyperlinkButtonCell alloc] initTextCell:text]);
  [button setCell:cell.get()];
  [button sizeToFit];
  return button;
}

- (NSButton*)adapterOffHelpButton {
  return adapterOffHelpButton_.get();
}

- (NSTableView*)tableView {
  return tableView_.get();
}

- (SpinnerView*)spinner {
  return spinner_.get();
}

- (NSButton*)connectButton {
  return connectButton_.get();
}

- (NSButton*)cancelButton {
  return cancelButton_.get();
}

- (NSButton*)helpButton {
  return helpButton_.get();
}

- (NSTextField*)scanningMessage {
  return scanningMessage_.get();
}

- (NSTextField*)wordConnector {
  return wordConnector_.get();
}

- (NSButton*)rescanButton {
  return rescanButton_.get();
}

- (NSInteger)numberOfOptions {
  // When there are no devices, the table contains a message saying there are
  // no devices, so the number of rows is always at least 1.
  return std::max(static_cast<NSInteger>(chooserController_->NumOptions()),
                  static_cast<NSInteger>(1));
}

// When this function is called with numOptions == 0, it is to show the
// message saying there are no devices.
- (NSString*)optionAtIndex:(NSInteger)index {
  NSInteger numOptions =
      static_cast<NSInteger>(chooserController_->NumOptions());
  if (numOptions == 0) {
    DCHECK_EQ(0, index);
    return base::SysUTF16ToNSString(chooserController_->GetNoOptionsText());
  }

  DCHECK_GE(index, 0);
  DCHECK_LT(index, numOptions);

  return base::SysUTF16ToNSString(
      chooserController_->GetOption(static_cast<size_t>(index)));
}

- (void)updateTableView {
  chooserContentViewController_->UpdateTableView();
}

- (void)accept {
  NSIndexSet* selectedRows = [tableView_ selectedRowIndexes];
  NSUInteger index = [selectedRows firstIndex];
  std::vector<size_t> indices;
  while (index != NSNotFound) {
    indices.push_back(index);
    index = [selectedRows indexGreaterThanIndex:index];
  }
  chooserController_->Select(indices);
}

- (void)cancel {
  chooserController_->Cancel();
}

- (void)close {
  chooserController_->Close();
}

- (void)onAdapterOffHelp:(id)sender {
  chooserController_->OpenAdapterOffHelpUrl();
}

- (void)onRescan:(id)sender {
  chooserController_->RefreshOptions();
}

- (void)onHelpPressed:(id)sender {
  chooserController_->OpenHelpCenterUrl();
}

- (void)updateContentRowColor {
  NSIndexSet* selectedRows = [tableView_ selectedRowIndexes];
  NSInteger numOptions =
      base::checked_cast<NSInteger>(chooserController_->NumOptions());
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  for (NSInteger rowIndex = 0; rowIndex < numOptions; ++rowIndex) {
    BOOL isSelected = [selectedRows containsIndex:rowIndex];
    // Update the color of the text.
    [[self tableRowViewText:rowIndex]
        setTextColor:(isSelected ? [NSColor whiteColor]
                                 : [NSColor blackColor])];

    // Update the color of the image.
    if (chooserController_->ShouldShowIconBeforeText()) {
      if (chooserController_->IsConnected(rowIndex)) {
        [[self tableRowViewImage:rowIndex]
            setImage:gfx::NSImageFromImageSkia(gfx::CreateVectorIcon(
                         vector_icons::kBluetoothConnectedIcon,
                         isSelected ? SK_ColorWHITE : gfx::kChromeIconGrey))];
      } else {
        int signalStrengthLevel =
            chooserController_->GetSignalStrengthLevel(rowIndex);
        if (signalStrengthLevel != -1) {
          int imageId =
              isSelected
                  ? kSignalStrengthLevelImageSelectedIds[signalStrengthLevel]
                  : kSignalStrengthLevelImageIds[signalStrengthLevel];
          [[self tableRowViewImage:rowIndex]
              setImage:rb.GetNativeImageNamed(imageId).ToNSImage()];
        }
      }
    }

    // Update the color of paired status.
    NSTextField* pairedStatusText = [self tableRowViewPairedStatus:rowIndex];
    if (pairedStatusText) {
      [pairedStatusText
          setTextColor:(isSelected ? [NSColor whiteColor]
                                   : skia::SkColorToCalibratedNSColor(
                                         gfx::kGoogleGreen700))];
    }
  }
}

- (NSImageView*)tableRowViewImage:(NSInteger)row {
  ChooserContentTableRowView* tableRowView =
      [tableView_ viewAtColumn:0 row:row makeIfNecessary:YES];
  return [tableRowView image];
}

- (NSTextField*)tableRowViewText:(NSInteger)row {
  ChooserContentTableRowView* tableRowView =
      [tableView_ viewAtColumn:0 row:row makeIfNecessary:YES];
  return [tableRowView text];
}

- (NSTextField*)tableRowViewPairedStatus:(NSInteger)row {
  ChooserContentTableRowView* tableRowView =
      [tableView_ viewAtColumn:0 row:row makeIfNecessary:YES];
  return [tableRowView pairedStatus];
}

- (void)drawRect:(NSRect)rect {
  [[NSColor colorWithCalibratedWhite:245.0f / 255.0f alpha:1.0f] setFill];
  NSRect footnoteFrame =
      NSMakeRect(0.0f, 0.0f, kChooserWidth, separatorOriginY_);
  NSRectFill(footnoteFrame);
  [super drawRect:footnoteFrame];
}

@end
