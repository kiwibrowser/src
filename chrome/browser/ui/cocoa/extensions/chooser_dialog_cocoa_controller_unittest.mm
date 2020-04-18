// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/extensions/chooser_dialog_cocoa_controller.h"

#import <Cocoa/Cocoa.h>

#include <memory>
#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chooser_controller/mock_chooser_controller.h"
#import "chrome/browser/ui/cocoa/device_chooser_content_view_cocoa.h"
#import "chrome/browser/ui/cocoa/extensions/chooser_dialog_cocoa.h"
#include "chrome/browser/ui/cocoa/spinner_view.h"
#import "chrome/browser/ui/cocoa/test/cocoa_profile_test.h"
#include "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "chrome/grit/generated_resources.h"
#include "components/vector_icons/vector_icons.h"
#include "skia/ext/skia_utils_mac.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_unittest_util.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/resources/grit/ui_resources.h"

namespace {

// The lookup table for signal strength level image.
const int kSignalStrengthLevelImageIds[5] = {IDR_SIGNAL_0_BAR, IDR_SIGNAL_1_BAR,
                                             IDR_SIGNAL_2_BAR, IDR_SIGNAL_3_BAR,
                                             IDR_SIGNAL_4_BAR};
const int kSignalStrengthLevelImageSelectedIds[5] = {
    IDR_SIGNAL_0_BAR_SELECTED, IDR_SIGNAL_1_BAR_SELECTED,
    IDR_SIGNAL_2_BAR_SELECTED, IDR_SIGNAL_3_BAR_SELECTED,
    IDR_SIGNAL_4_BAR_SELECTED};

}  // namespace

class ChooserDialogCocoaControllerTest : public CocoaProfileTest {
 protected:
  ChooserDialogCocoaControllerTest()
      : rb_(ui::ResourceBundle::GetSharedInstance()) {}
  ~ChooserDialogCocoaControllerTest() override {}

  void SetUp() override {
    CocoaProfileTest::SetUp();
    ASSERT_TRUE(browser());
  }

  // Create a ChooserDialogCocoa.
  void CreateChooserDialog() {
    content::WebContents* web_contents =
        content::WebContents::Create(content::WebContents::CreateParams(
            profile(), content::SiteInstance::Create(profile())));
    ASSERT_TRUE(web_contents);
    auto chooser_controller = std::make_unique<MockChooserController>();
    mock_chooser_controller_ = chooser_controller.get();
    chooser_dialog_.reset(
        new ChooserDialogCocoa(web_contents, std::move(chooser_controller)));
    ASSERT_TRUE(chooser_dialog_);
    chooser_dialog_controller_ =
        chooser_dialog_->chooser_dialog_cocoa_controller_.get();
    ASSERT_TRUE(chooser_dialog_controller_);
    device_chooser_content_view_ =
        [chooser_dialog_controller_ deviceChooserContentView];
    ASSERT_TRUE(device_chooser_content_view_);
    adapter_off_help_button_ =
        [device_chooser_content_view_ adapterOffHelpButton];
    ASSERT_TRUE(adapter_off_help_button_);
    table_view_ = [device_chooser_content_view_ tableView];
    ASSERT_TRUE(table_view_);
    spinner_ = [device_chooser_content_view_ spinner];
    ASSERT_TRUE(spinner_);
    connect_button_ = [device_chooser_content_view_ connectButton];
    ASSERT_TRUE(connect_button_);
    cancel_button_ = [device_chooser_content_view_ cancelButton];
    ASSERT_TRUE(cancel_button_);
    help_button_ = [device_chooser_content_view_ helpButton];
    ASSERT_TRUE(help_button_);
    scanning_message_ = [device_chooser_content_view_ scanningMessage];
    ASSERT_TRUE(scanning_message_);
    word_connector_ = [device_chooser_content_view_ wordConnector];
    ASSERT_TRUE(word_connector_);
    rescan_button_ = [device_chooser_content_view_ rescanButton];
    ASSERT_TRUE(rescan_button_);
  }

  void ExpectNoRowImage(int row) {
    ASSERT_FALSE([device_chooser_content_view_
        tableRowViewImage:static_cast<NSInteger>(row)]);
  }

  void ExpectSignalStrengthLevelImageIs(int row,
                                        int expected_signal_strength_level,
                                        int expected_color) {
    NSImageView* image_view = [device_chooser_content_view_
        tableRowViewImage:static_cast<NSInteger>(row)];
    ASSERT_TRUE(image_view);
    int image_id =
        expected_color == MockChooserController::kImageColorUnselected
            ? kSignalStrengthLevelImageIds[expected_signal_strength_level]
            : kSignalStrengthLevelImageSelectedIds
                  [expected_signal_strength_level];
    EXPECT_NSEQ(rb_.GetNativeImageNamed(image_id).ToNSImage(),
                [image_view image]);
  }

  void ExpectRowImageIsConnectedImage(int row, SkColor expected_color) {
    NSImageView* image_view = [device_chooser_content_view_
        tableRowViewImage:static_cast<NSInteger>(row)];
    ASSERT_TRUE(image_view);
    EXPECT_TRUE(gfx::test::AreImagesEqual(
        gfx::Image(gfx::CreateVectorIcon(vector_icons::kBluetoothConnectedIcon,
                                         expected_color)),
        gfx::Image([[image_view image] copy])));
  }

  void ExpectRowTextIs(int row, NSString* expected_text) {
    EXPECT_NSEQ(expected_text,
                [[device_chooser_content_view_
                    tableRowViewText:static_cast<NSInteger>(row)] stringValue]);
  }

  void ExpectRowTextColorIs(int row, NSColor* expected_color) {
    EXPECT_NSEQ(expected_color,
                [[device_chooser_content_view_
                    tableRowViewText:static_cast<NSInteger>(row)] textColor]);
  }

  bool IsRowPaired(int row) {
    NSTextField* paired_status = [device_chooser_content_view_
        tableRowViewPairedStatus:static_cast<NSInteger>(row)];
    if (paired_status) {
      EXPECT_NSEQ(l10n_util::GetNSString(IDS_DEVICE_CHOOSER_PAIRED_STATUS_TEXT),
                  [paired_status stringValue]);
      return true;
    } else {
      return false;
    }
  }

  void ExpectPairedTextColorIs(int row, NSColor* expected_color) {
    EXPECT_NSEQ(
        expected_color,
        [[device_chooser_content_view_
            tableRowViewPairedStatus:static_cast<NSInteger>(row)] textColor]);
  }

  ui::ResourceBundle& rb_;

  std::unique_ptr<ChooserDialogCocoa> chooser_dialog_;

  MockChooserController* mock_chooser_controller_;
  ChooserDialogCocoaController* chooser_dialog_controller_;
  DeviceChooserContentViewCocoa* device_chooser_content_view_;
  NSButton* adapter_off_help_button_;
  NSTableView* table_view_;
  SpinnerView* spinner_;
  NSButton* connect_button_;
  NSButton* cancel_button_;
  NSButton* help_button_;
  NSTextField* scanning_message_;
  NSTextField* word_connector_;
  NSButton* rescan_button_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ChooserDialogCocoaControllerTest);
};

TEST_F(ChooserDialogCocoaControllerTest, InitialState) {
  CreateChooserDialog();

  // Since "No devices found." needs to be displayed on the |table_view_|,
  // the number of rows is 1.
  EXPECT_EQ(1, table_view_.numberOfRows);
  EXPECT_EQ(1, table_view_.numberOfColumns);
  // No image shown.
  ExpectNoRowImage(0);
  ExpectRowTextIs(
      0, l10n_util::GetNSString(IDS_DEVICE_CHOOSER_NO_DEVICES_FOUND_PROMPT));
  EXPECT_FALSE(IsRowPaired(0));
  // |table_view_| should be disabled since there is no option shown.
  EXPECT_FALSE(table_view_.enabled);
  // No option selected.
  EXPECT_EQ(-1, table_view_.selectedRow);
  // |connect_button_| should be disabled since no option selected.
  EXPECT_FALSE(connect_button_.enabled);
  EXPECT_TRUE(cancel_button_.enabled);
  EXPECT_TRUE(help_button_.enabled);
  EXPECT_NSEQ(l10n_util::GetNSStringF(
                  IDS_DEVICE_CHOOSER_GET_HELP_LINK_WITH_SCANNING_STATUS,
                  base::string16()),
              help_button_.title);
}

TEST_F(ChooserDialogCocoaControllerTest, AddOption) {
  CreateChooserDialog();

  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("a"),
      MockChooserController::kNoSignalStrengthLevelImage,
      MockChooserController::ConnectedPairedStatus::CONNECTED |
          MockChooserController::ConnectedPairedStatus::PAIRED);
  EXPECT_EQ(1, table_view_.numberOfRows);
  EXPECT_EQ(1, table_view_.numberOfColumns);
  // |table_view_| should be enabled since there is an option.
  EXPECT_TRUE(table_view_.enabled);
  EXPECT_EQ(-1, table_view_.selectedRow);
  ExpectRowImageIsConnectedImage(0, gfx::kChromeIconGrey);
  ExpectRowTextIs(0, @"a");
  EXPECT_TRUE(IsRowPaired(0));
  EXPECT_FALSE(connect_button_.enabled);
  EXPECT_TRUE(cancel_button_.enabled);
  EXPECT_TRUE(help_button_.enabled);

  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("b"), MockChooserController::kSignalStrengthLevel0Bar,
      MockChooserController::ConnectedPairedStatus::NONE);
  EXPECT_EQ(2, table_view_.numberOfRows);
  EXPECT_EQ(1, table_view_.numberOfColumns);
  EXPECT_TRUE(table_view_.enabled);
  EXPECT_EQ(-1, table_view_.selectedRow);
  ExpectSignalStrengthLevelImageIs(
      1, MockChooserController::kSignalStrengthLevel0Bar,
      MockChooserController::kImageColorUnselected);
  ExpectRowTextIs(1, @"b");
  EXPECT_FALSE(IsRowPaired(1));

  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("c"), MockChooserController::kSignalStrengthLevel1Bar,
      MockChooserController::ConnectedPairedStatus::NONE);
  EXPECT_EQ(3, table_view_.numberOfRows);
  EXPECT_EQ(1, table_view_.numberOfColumns);
  EXPECT_TRUE(table_view_.enabled);
  EXPECT_EQ(-1, table_view_.selectedRow);
  ExpectSignalStrengthLevelImageIs(
      2, MockChooserController::kSignalStrengthLevel1Bar,
      MockChooserController::kImageColorUnselected);
  ExpectRowTextIs(2, @"c");
  EXPECT_FALSE(IsRowPaired(2));

  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("d"), MockChooserController::kSignalStrengthLevel2Bar,
      MockChooserController::ConnectedPairedStatus::NONE);
  ExpectSignalStrengthLevelImageIs(
      3, MockChooserController::kSignalStrengthLevel2Bar,
      MockChooserController::kImageColorUnselected);
  ExpectRowTextIs(3, @"d");
  EXPECT_FALSE(IsRowPaired(3));

  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("e"), MockChooserController::kSignalStrengthLevel3Bar,
      MockChooserController::ConnectedPairedStatus::NONE);
  ExpectSignalStrengthLevelImageIs(
      4, MockChooserController::kSignalStrengthLevel3Bar,
      MockChooserController::kImageColorUnselected);
  ExpectRowTextIs(4, @"e");
  EXPECT_FALSE(IsRowPaired(4));

  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("f"), MockChooserController::kSignalStrengthLevel4Bar,
      MockChooserController::ConnectedPairedStatus::NONE);
  ExpectSignalStrengthLevelImageIs(
      5, MockChooserController::kSignalStrengthLevel4Bar,
      MockChooserController::kImageColorUnselected);
  ExpectRowTextIs(5, @"f");
  EXPECT_FALSE(IsRowPaired(5));
}

TEST_F(ChooserDialogCocoaControllerTest, RemoveOption) {
  CreateChooserDialog();

  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("a"),
      MockChooserController::kNoSignalStrengthLevelImage,
      MockChooserController::ConnectedPairedStatus::CONNECTED |
          MockChooserController::ConnectedPairedStatus::PAIRED);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("b"), MockChooserController::kSignalStrengthLevel0Bar,
      MockChooserController::ConnectedPairedStatus::NONE);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("c"), MockChooserController::kSignalStrengthLevel1Bar,
      MockChooserController::ConnectedPairedStatus::NONE);

  mock_chooser_controller_->OptionRemoved(base::ASCIIToUTF16("b"));
  EXPECT_EQ(2, table_view_.numberOfRows);
  EXPECT_EQ(1, table_view_.numberOfColumns);
  EXPECT_TRUE(table_view_.enabled);
  EXPECT_EQ(-1, table_view_.selectedRow);
  ExpectRowImageIsConnectedImage(0, gfx::kChromeIconGrey);
  ExpectRowTextIs(0, @"a");
  EXPECT_TRUE(IsRowPaired(0));
  ExpectSignalStrengthLevelImageIs(
      1, MockChooserController::kSignalStrengthLevel1Bar,
      MockChooserController::kImageColorUnselected);
  ExpectRowTextIs(1, @"c");
  EXPECT_FALSE(IsRowPaired(1));

  // Remove a non-existent option, the number of rows should not change.
  mock_chooser_controller_->OptionRemoved(base::ASCIIToUTF16("non-existent"));
  EXPECT_EQ(2, table_view_.numberOfRows);
  EXPECT_EQ(1, table_view_.numberOfColumns);
  EXPECT_TRUE(table_view_.enabled);
  EXPECT_EQ(-1, table_view_.selectedRow);
  ExpectRowTextIs(0, @"a");
  ExpectRowTextIs(1, @"c");

  mock_chooser_controller_->OptionRemoved(base::ASCIIToUTF16("c"));
  EXPECT_EQ(1, table_view_.numberOfRows);
  EXPECT_EQ(1, table_view_.numberOfColumns);
  EXPECT_TRUE(table_view_.enabled);
  EXPECT_EQ(-1, table_view_.selectedRow);
  ExpectRowTextIs(0, @"a");

  mock_chooser_controller_->OptionRemoved(base::ASCIIToUTF16("a"));
  // There is no option shown now. But since "No devices found."
  // needs to be displayed on the |table_view_|, the number of rows is 1.
  EXPECT_EQ(1, table_view_.numberOfRows);
  EXPECT_EQ(1, table_view_.numberOfColumns);
  // |table_view_| should be disabled since all options are removed.
  EXPECT_FALSE(table_view_.enabled);
  EXPECT_EQ(-1, table_view_.selectedRow);
  ExpectNoRowImage(0);
  ExpectRowTextIs(
      0, l10n_util::GetNSString(IDS_DEVICE_CHOOSER_NO_DEVICES_FOUND_PROMPT));
  EXPECT_FALSE(IsRowPaired(0));
}

TEST_F(ChooserDialogCocoaControllerTest, UpdateOption) {
  CreateChooserDialog();

  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("a"),
      MockChooserController::kNoSignalStrengthLevelImage,
      MockChooserController::ConnectedPairedStatus::CONNECTED |
          MockChooserController::ConnectedPairedStatus::PAIRED);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("b"), MockChooserController::kSignalStrengthLevel0Bar,
      MockChooserController::ConnectedPairedStatus::NONE);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("c"), MockChooserController::kSignalStrengthLevel1Bar,
      MockChooserController::ConnectedPairedStatus::NONE);

  mock_chooser_controller_->OptionUpdated(
      base::ASCIIToUTF16("b"), base::ASCIIToUTF16("d"),
      MockChooserController::kNoSignalStrengthLevelImage,
      MockChooserController::ConnectedPairedStatus::CONNECTED |
          MockChooserController::ConnectedPairedStatus::PAIRED);

  EXPECT_EQ(3, table_view_.numberOfRows);
  EXPECT_EQ(1, table_view_.numberOfColumns);
  EXPECT_TRUE(table_view_.enabled);
  EXPECT_EQ(-1, table_view_.selectedRow);
  ExpectRowImageIsConnectedImage(0, gfx::kChromeIconGrey);
  ExpectRowTextIs(0, @"a");
  EXPECT_TRUE(IsRowPaired(0));
  ExpectRowImageIsConnectedImage(1, gfx::kChromeIconGrey);
  ExpectRowTextIs(1, @"d");
  EXPECT_TRUE(IsRowPaired(1));
  ExpectSignalStrengthLevelImageIs(
      2, MockChooserController::kSignalStrengthLevel1Bar,
      MockChooserController::kImageColorUnselected);
  ExpectRowTextIs(2, @"c");
  EXPECT_FALSE(IsRowPaired(2));
}

TEST_F(ChooserDialogCocoaControllerTest, AddAndRemoveOption) {
  CreateChooserDialog();

  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("a"),
      MockChooserController::kNoSignalStrengthLevelImage,
      MockChooserController::ConnectedPairedStatus::CONNECTED |
          MockChooserController::ConnectedPairedStatus::PAIRED);
  EXPECT_EQ(1, table_view_.numberOfRows);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("b"), MockChooserController::kSignalStrengthLevel0Bar,
      MockChooserController::ConnectedPairedStatus::NONE);
  EXPECT_EQ(2, table_view_.numberOfRows);
  mock_chooser_controller_->OptionRemoved(base::ASCIIToUTF16("b"));
  EXPECT_EQ(1, table_view_.numberOfRows);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("c"), MockChooserController::kSignalStrengthLevel1Bar,
      MockChooserController::ConnectedPairedStatus::NONE);
  EXPECT_EQ(2, table_view_.numberOfRows);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("d"), MockChooserController::kSignalStrengthLevel2Bar,
      MockChooserController::ConnectedPairedStatus::NONE);
  EXPECT_EQ(3, table_view_.numberOfRows);
  mock_chooser_controller_->OptionRemoved(base::ASCIIToUTF16("d"));
  EXPECT_EQ(2, table_view_.numberOfRows);
  mock_chooser_controller_->OptionRemoved(base::ASCIIToUTF16("c"));
  EXPECT_EQ(1, table_view_.numberOfRows);
}

TEST_F(ChooserDialogCocoaControllerTest, UpdateAndRemoveTheUpdatedOption) {
  CreateChooserDialog();

  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("a"),
      MockChooserController::kNoSignalStrengthLevelImage,
      MockChooserController::ConnectedPairedStatus::CONNECTED |
          MockChooserController::ConnectedPairedStatus::PAIRED);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("b"), MockChooserController::kSignalStrengthLevel0Bar,
      MockChooserController::ConnectedPairedStatus::NONE);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("c"), MockChooserController::kSignalStrengthLevel1Bar,
      MockChooserController::ConnectedPairedStatus::NONE);

  mock_chooser_controller_->OptionUpdated(
      base::ASCIIToUTF16("b"), base::ASCIIToUTF16("d"),
      MockChooserController::kNoSignalStrengthLevelImage,
      MockChooserController::ConnectedPairedStatus::CONNECTED |
          MockChooserController::ConnectedPairedStatus::PAIRED);

  mock_chooser_controller_->OptionRemoved(base::ASCIIToUTF16("d"));

  EXPECT_EQ(2, table_view_.numberOfRows);
  EXPECT_EQ(1, table_view_.numberOfColumns);
  EXPECT_TRUE(table_view_.enabled);
  EXPECT_EQ(-1, table_view_.selectedRow);
  ExpectRowImageIsConnectedImage(0, gfx::kChromeIconGrey);
  ExpectRowTextIs(0, @"a");
  EXPECT_TRUE(IsRowPaired(0));
  ExpectSignalStrengthLevelImageIs(
      1, MockChooserController::kSignalStrengthLevel1Bar,
      MockChooserController::kImageColorUnselected);
  ExpectRowTextIs(1, @"c");
  EXPECT_FALSE(IsRowPaired(1));
}

TEST_F(ChooserDialogCocoaControllerTest,
       RowImageAndTextChangeColorWhenSelectionChanges) {
  CreateChooserDialog();

  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("a"),
      MockChooserController::kNoSignalStrengthLevelImage,
      MockChooserController::ConnectedPairedStatus::CONNECTED |
          MockChooserController::ConnectedPairedStatus::PAIRED);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("b"), MockChooserController::kSignalStrengthLevel0Bar,
      MockChooserController::ConnectedPairedStatus::NONE);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("c"), MockChooserController::kSignalStrengthLevel1Bar,
      MockChooserController::ConnectedPairedStatus::NONE);

  ExpectRowImageIsConnectedImage(0, gfx::kChromeIconGrey);
  ExpectSignalStrengthLevelImageIs(
      1, MockChooserController::kSignalStrengthLevel0Bar,
      MockChooserController::kImageColorUnselected);
  ExpectSignalStrengthLevelImageIs(
      2, MockChooserController::kSignalStrengthLevel1Bar,
      MockChooserController::kImageColorUnselected);
  ExpectRowTextColorIs(0, [NSColor blackColor]);
  ExpectRowTextColorIs(1, [NSColor blackColor]);
  ExpectRowTextColorIs(2, [NSColor blackColor]);
  ExpectPairedTextColorIs(
      0, skia::SkColorToCalibratedNSColor(gfx::kGoogleGreen700));

  // Option 0 shows a Bluetooth connected image, the following code tests the
  // color of that image and text change when the option is selected or
  // deselected.
  // Select option 0.
  [table_view_ selectRowIndexes:[NSIndexSet indexSetWithIndex:0]
           byExtendingSelection:NO];
  EXPECT_EQ(0, table_view_.selectedRow);
  ExpectRowImageIsConnectedImage(0, SK_ColorWHITE);
  ExpectSignalStrengthLevelImageIs(
      1, MockChooserController::kSignalStrengthLevel0Bar,
      MockChooserController::kImageColorUnselected);
  ExpectSignalStrengthLevelImageIs(
      2, MockChooserController::kSignalStrengthLevel1Bar,
      MockChooserController::kImageColorUnselected);
  ExpectRowTextColorIs(0, [NSColor whiteColor]);
  ExpectRowTextColorIs(1, [NSColor blackColor]);
  ExpectRowTextColorIs(2, [NSColor blackColor]);
  ExpectPairedTextColorIs(0, [NSColor whiteColor]);

  // Deselect option 0.
  [table_view_ deselectRow:0];
  EXPECT_EQ(-1, table_view_.selectedRow);
  ExpectRowImageIsConnectedImage(0, gfx::kChromeIconGrey);
  ExpectSignalStrengthLevelImageIs(
      1, MockChooserController::kSignalStrengthLevel0Bar,
      MockChooserController::kImageColorUnselected);
  ExpectSignalStrengthLevelImageIs(
      2, MockChooserController::kSignalStrengthLevel1Bar,
      MockChooserController::kImageColorUnselected);
  ExpectRowTextColorIs(0, [NSColor blackColor]);
  ExpectRowTextColorIs(1, [NSColor blackColor]);
  ExpectRowTextColorIs(2, [NSColor blackColor]);
  ExpectPairedTextColorIs(
      0, skia::SkColorToCalibratedNSColor(gfx::kGoogleGreen700));

  // Option 1 shows a signal strengh level image, the following code tests the
  // color of that image and text change when the option is selected or
  // deselected.
  // Select option 1.
  [table_view_ selectRowIndexes:[NSIndexSet indexSetWithIndex:1]
           byExtendingSelection:NO];
  EXPECT_EQ(1, table_view_.selectedRow);
  ExpectRowImageIsConnectedImage(0, gfx::kChromeIconGrey);
  ExpectSignalStrengthLevelImageIs(
      1, MockChooserController::kSignalStrengthLevel0Bar,
      MockChooserController::kImageColorSelected);
  ExpectSignalStrengthLevelImageIs(
      2, MockChooserController::kSignalStrengthLevel1Bar,
      MockChooserController::kImageColorUnselected);
  ExpectRowTextColorIs(0, [NSColor blackColor]);
  ExpectRowTextColorIs(1, [NSColor whiteColor]);
  ExpectRowTextColorIs(2, [NSColor blackColor]);
  ExpectPairedTextColorIs(
      0, skia::SkColorToCalibratedNSColor(gfx::kGoogleGreen700));

  // Deselect option 1.
  [table_view_ deselectRow:1];
  EXPECT_EQ(-1, table_view_.selectedRow);
  ExpectRowImageIsConnectedImage(0, gfx::kChromeIconGrey);
  ExpectSignalStrengthLevelImageIs(
      1, MockChooserController::kSignalStrengthLevel0Bar,
      MockChooserController::kImageColorUnselected);
  ExpectSignalStrengthLevelImageIs(
      2, MockChooserController::kSignalStrengthLevel1Bar,
      MockChooserController::kImageColorUnselected);
  ExpectRowTextColorIs(0, [NSColor blackColor]);
  ExpectRowTextColorIs(1, [NSColor blackColor]);
  ExpectRowTextColorIs(2, [NSColor blackColor]);
  ExpectPairedTextColorIs(
      0, skia::SkColorToCalibratedNSColor(gfx::kGoogleGreen700));

  // The following code tests the color of the image and text change when
  // selecting another option without deselecting the first.
  // Select option 0.
  [table_view_ selectRowIndexes:[NSIndexSet indexSetWithIndex:0]
           byExtendingSelection:NO];

  // Select option 1.
  [table_view_ selectRowIndexes:[NSIndexSet indexSetWithIndex:1]
           byExtendingSelection:NO];
  EXPECT_EQ(1, table_view_.selectedRow);
  ExpectRowImageIsConnectedImage(0, gfx::kChromeIconGrey);
  ExpectSignalStrengthLevelImageIs(
      1, MockChooserController::kSignalStrengthLevel0Bar,
      MockChooserController::kImageColorSelected);
  ExpectSignalStrengthLevelImageIs(
      2, MockChooserController::kSignalStrengthLevel1Bar,
      MockChooserController::kImageColorUnselected);
  ExpectRowTextColorIs(0, [NSColor blackColor]);
  ExpectRowTextColorIs(1, [NSColor whiteColor]);
  ExpectRowTextColorIs(2, [NSColor blackColor]);
  ExpectPairedTextColorIs(
      0, skia::SkColorToCalibratedNSColor(gfx::kGoogleGreen700));

  // The following code tests the color of the image and text of a selected
  // option when it is updated.
  // Select option 2.
  [table_view_ selectRowIndexes:[NSIndexSet indexSetWithIndex:2]
           byExtendingSelection:NO];

  // Update option 2 from one signal strength to another.
  mock_chooser_controller_->OptionUpdated(
      base::ASCIIToUTF16("c"), base::ASCIIToUTF16("e"),
      MockChooserController::kSignalStrengthLevel2Bar,
      MockChooserController::ConnectedPairedStatus::NONE);
  ExpectSignalStrengthLevelImageIs(
      2, MockChooserController::kSignalStrengthLevel2Bar,
      MockChooserController::kImageColorSelected);
  ExpectRowTextColorIs(0, [NSColor blackColor]);
  ExpectRowTextColorIs(1, [NSColor blackColor]);
  ExpectRowTextColorIs(2, [NSColor whiteColor]);

  // Update option 2 again from non-connected and non-paired to connected
  // and paired.
  mock_chooser_controller_->OptionUpdated(
      base::ASCIIToUTF16("e"), base::ASCIIToUTF16("f"),
      MockChooserController::kNoSignalStrengthLevelImage,
      MockChooserController::ConnectedPairedStatus::CONNECTED |
          MockChooserController::ConnectedPairedStatus::PAIRED);
  ExpectRowImageIsConnectedImage(2, SK_ColorWHITE);
  ExpectRowTextColorIs(0, [NSColor blackColor]);
  ExpectRowTextColorIs(1, [NSColor blackColor]);
  ExpectRowTextColorIs(2, [NSColor whiteColor]);
  ExpectPairedTextColorIs(2, [NSColor whiteColor]);
}

TEST_F(ChooserDialogCocoaControllerTest, SelectAndDeselectAnOption) {
  CreateChooserDialog();

  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("a"),
      MockChooserController::kNoSignalStrengthLevelImage,
      MockChooserController::ConnectedPairedStatus::CONNECTED |
          MockChooserController::ConnectedPairedStatus::PAIRED);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("b"), MockChooserController::kSignalStrengthLevel0Bar,
      MockChooserController::ConnectedPairedStatus::NONE);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("c"), MockChooserController::kSignalStrengthLevel1Bar,
      MockChooserController::ConnectedPairedStatus::NONE);

  // Select option 0.
  [table_view_ selectRowIndexes:[NSIndexSet indexSetWithIndex:0]
           byExtendingSelection:NO];
  EXPECT_EQ(0, table_view_.selectedRow);
  EXPECT_TRUE(connect_button_.enabled);

  // Deselect option 0.
  [table_view_ deselectRow:0];
  EXPECT_EQ(-1, table_view_.selectedRow);
  EXPECT_FALSE(connect_button_.enabled);

  // Select option 1.
  [table_view_ selectRowIndexes:[NSIndexSet indexSetWithIndex:1]
           byExtendingSelection:NO];
  EXPECT_EQ(1, table_view_.selectedRow);
  EXPECT_TRUE(connect_button_.enabled);

  // Deselect option 1.
  [table_view_ deselectRow:1];
  EXPECT_EQ(-1, table_view_.selectedRow);
  EXPECT_FALSE(connect_button_.enabled);
}

TEST_F(ChooserDialogCocoaControllerTest,
       SelectAnOptionAndThenSelectAnotherOption) {
  CreateChooserDialog();

  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("a"),
      MockChooserController::kNoSignalStrengthLevelImage,
      MockChooserController::ConnectedPairedStatus::CONNECTED |
          MockChooserController::ConnectedPairedStatus::PAIRED);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("b"), MockChooserController::kSignalStrengthLevel0Bar,
      MockChooserController::ConnectedPairedStatus::NONE);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("c"), MockChooserController::kSignalStrengthLevel1Bar,
      MockChooserController::ConnectedPairedStatus::NONE);

  // Select option 0.
  [table_view_ selectRowIndexes:[NSIndexSet indexSetWithIndex:0]
           byExtendingSelection:NO];
  EXPECT_EQ(0, table_view_.selectedRow);
  EXPECT_TRUE(connect_button_.enabled);

  // Select option 1.
  [table_view_ selectRowIndexes:[NSIndexSet indexSetWithIndex:1]
           byExtendingSelection:NO];
  EXPECT_EQ(1, table_view_.selectedRow);
  EXPECT_TRUE(connect_button_.enabled);

  // Select option 2.
  [table_view_ selectRowIndexes:[NSIndexSet indexSetWithIndex:2]
           byExtendingSelection:NO];
  EXPECT_EQ(2, table_view_.selectedRow);
  EXPECT_TRUE(connect_button_.enabled);
}

TEST_F(ChooserDialogCocoaControllerTest, SelectAnOptionAndRemoveAnotherOption) {
  CreateChooserDialog();

  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("a"),
      MockChooserController::kNoSignalStrengthLevelImage,
      MockChooserController::ConnectedPairedStatus::CONNECTED |
          MockChooserController::ConnectedPairedStatus::PAIRED);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("b"), MockChooserController::kSignalStrengthLevel0Bar,
      MockChooserController::ConnectedPairedStatus::NONE);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("c"), MockChooserController::kSignalStrengthLevel1Bar,
      MockChooserController::ConnectedPairedStatus::NONE);

  // Select option 1.
  [table_view_ selectRowIndexes:[NSIndexSet indexSetWithIndex:1]
           byExtendingSelection:NO];
  EXPECT_EQ(3, table_view_.numberOfRows);
  EXPECT_EQ(1, table_view_.selectedRow);
  EXPECT_TRUE(connect_button_.enabled);

  // Remove option 0. The list becomes: b c. And the index of the previously
  // selected item "b" becomes 0.
  mock_chooser_controller_->OptionRemoved(base::ASCIIToUTF16("a"));
  EXPECT_EQ(2, table_view_.numberOfRows);
  EXPECT_EQ(0, table_view_.selectedRow);
  EXPECT_TRUE(connect_button_.enabled);

  // Remove option 1. The list becomes: b.
  mock_chooser_controller_->OptionRemoved(base::ASCIIToUTF16("c"));
  EXPECT_EQ(1, table_view_.numberOfRows);
  EXPECT_EQ(0, table_view_.selectedRow);
  EXPECT_TRUE(connect_button_.enabled);
}

TEST_F(ChooserDialogCocoaControllerTest,
       SelectAnOptionAndRemoveTheSelectedOption) {
  CreateChooserDialog();

  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("a"),
      MockChooserController::kNoSignalStrengthLevelImage,
      MockChooserController::ConnectedPairedStatus::CONNECTED |
          MockChooserController::ConnectedPairedStatus::PAIRED);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("b"), MockChooserController::kSignalStrengthLevel0Bar,
      MockChooserController::ConnectedPairedStatus::NONE);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("c"), MockChooserController::kSignalStrengthLevel1Bar,
      MockChooserController::ConnectedPairedStatus::NONE);

  // Select option 1.
  [table_view_ selectRowIndexes:[NSIndexSet indexSetWithIndex:1]
           byExtendingSelection:NO];
  EXPECT_EQ(3, table_view_.numberOfRows);
  EXPECT_EQ(1, table_view_.selectedRow);
  EXPECT_TRUE(connect_button_.enabled);

  // Remove option 1
  mock_chooser_controller_->OptionRemoved(base::ASCIIToUTF16("b"));
  EXPECT_EQ(2, table_view_.numberOfRows);
  // No option selected.
  EXPECT_EQ(-1, table_view_.selectedRow);
  // Since no option selected, the "Connect" button should be disabled.
  EXPECT_FALSE(connect_button_.enabled);
}

TEST_F(ChooserDialogCocoaControllerTest,
       SelectAnOptionAndUpdateTheSelectedOption) {
  CreateChooserDialog();

  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("a"),
      MockChooserController::kNoSignalStrengthLevelImage,
      MockChooserController::ConnectedPairedStatus::CONNECTED |
          MockChooserController::ConnectedPairedStatus::PAIRED);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("b"), MockChooserController::kSignalStrengthLevel0Bar,
      MockChooserController::ConnectedPairedStatus::NONE);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("c"), MockChooserController::kSignalStrengthLevel1Bar,
      MockChooserController::ConnectedPairedStatus::NONE);

  // Select option 1.
  [table_view_ selectRowIndexes:[NSIndexSet indexSetWithIndex:1]
           byExtendingSelection:NO];

  // Update option 1.
  mock_chooser_controller_->OptionUpdated(
      base::ASCIIToUTF16("b"), base::ASCIIToUTF16("d"),
      MockChooserController::kNoSignalStrengthLevelImage,
      MockChooserController::ConnectedPairedStatus::CONNECTED |
          MockChooserController::ConnectedPairedStatus::PAIRED);

  EXPECT_EQ(1, table_view_.selectedRow);
  ExpectRowImageIsConnectedImage(0, gfx::kChromeIconGrey);
  ExpectRowTextIs(0, @"a");
  EXPECT_TRUE(IsRowPaired(0));
  ExpectRowImageIsConnectedImage(1, SK_ColorWHITE);
  ExpectRowTextIs(1, @"d");
  EXPECT_TRUE(IsRowPaired(1));
  ExpectSignalStrengthLevelImageIs(
      2, MockChooserController::kSignalStrengthLevel1Bar,
      MockChooserController::kImageColorUnselected);
  ExpectRowTextIs(2, @"c");
  EXPECT_FALSE(IsRowPaired(2));
  EXPECT_TRUE(connect_button_.enabled);
}

TEST_F(ChooserDialogCocoaControllerTest,
       AddAnOptionAndSelectItAndRemoveTheSelectedOption) {
  CreateChooserDialog();

  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("a"),
      MockChooserController::kNoSignalStrengthLevelImage,
      MockChooserController::ConnectedPairedStatus::CONNECTED |
          MockChooserController::ConnectedPairedStatus::PAIRED);

  // Select option 0.
  [table_view_ selectRowIndexes:[NSIndexSet indexSetWithIndex:0]
           byExtendingSelection:NO];
  EXPECT_EQ(1, table_view_.numberOfRows);
  EXPECT_EQ(0, table_view_.selectedRow);
  EXPECT_TRUE(connect_button_.enabled);

  // Remove option 0.
  mock_chooser_controller_->OptionRemoved(base::ASCIIToUTF16("a"));
  // There is no option shown now. But since "No devices found."
  // needs to be displayed on the |table_view_|, the number of rows is 1.
  EXPECT_EQ(1, table_view_.numberOfRows);
  // No option selected.
  EXPECT_EQ(-1, table_view_.selectedRow);
  // |table_view_| should be disabled since there is no option shown.
  EXPECT_FALSE(table_view_.enabled);
  ExpectNoRowImage(0);
  ExpectRowTextIs(
      0, l10n_util::GetNSString(IDS_DEVICE_CHOOSER_NO_DEVICES_FOUND_PROMPT));
  EXPECT_FALSE(IsRowPaired(0));
  // Since no option selected, the "Connect" button should be disabled.
  EXPECT_FALSE(connect_button_.enabled);
}

TEST_F(ChooserDialogCocoaControllerTest, NoOptionSelectedAndPressCancelButton) {
  CreateChooserDialog();

  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("a"),
      MockChooserController::kNoSignalStrengthLevelImage,
      MockChooserController::ConnectedPairedStatus::CONNECTED |
          MockChooserController::ConnectedPairedStatus::PAIRED);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("b"), MockChooserController::kSignalStrengthLevel0Bar,
      MockChooserController::ConnectedPairedStatus::NONE);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("c"), MockChooserController::kSignalStrengthLevel1Bar,
      MockChooserController::ConnectedPairedStatus::NONE);

  EXPECT_CALL(*mock_chooser_controller_, Select(testing::_)).Times(0);
  EXPECT_CALL(*mock_chooser_controller_, Cancel()).Times(1);
  [cancel_button_ performClick:chooser_dialog_controller_];
}

TEST_F(ChooserDialogCocoaControllerTest, SelectAnOptionAndPressConnectButton) {
  CreateChooserDialog();

  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("a"),
      MockChooserController::kNoSignalStrengthLevelImage,
      MockChooserController::ConnectedPairedStatus::CONNECTED |
          MockChooserController::ConnectedPairedStatus::PAIRED);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("b"), MockChooserController::kSignalStrengthLevel0Bar,
      MockChooserController::ConnectedPairedStatus::NONE);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("c"), MockChooserController::kSignalStrengthLevel1Bar,
      MockChooserController::ConnectedPairedStatus::NONE);

  // Select option 0 and press "Connect" button.
  [table_view_ selectRowIndexes:[NSIndexSet indexSetWithIndex:0]
           byExtendingSelection:NO];
  std::vector<size_t> indices(1);
  indices[0] = 0;
  EXPECT_CALL(*mock_chooser_controller_, Select(indices)).Times(1);
  EXPECT_CALL(*mock_chooser_controller_, Cancel()).Times(0);
  [connect_button_ performClick:chooser_dialog_controller_];

  // Select option 2 and press "Connect" button.
  [table_view_ selectRowIndexes:[NSIndexSet indexSetWithIndex:2]
           byExtendingSelection:NO];
  indices[0] = 2;
  EXPECT_CALL(*mock_chooser_controller_, Select(indices)).Times(1);
  EXPECT_CALL(*mock_chooser_controller_, Cancel()).Times(0);
  [connect_button_ performClick:chooser_dialog_controller_];
}

TEST_F(ChooserDialogCocoaControllerTest, SelectAnOptionAndPressCancelButton) {
  CreateChooserDialog();

  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("a"),
      MockChooserController::kNoSignalStrengthLevelImage,
      MockChooserController::ConnectedPairedStatus::CONNECTED |
          MockChooserController::ConnectedPairedStatus::PAIRED);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("b"), MockChooserController::kSignalStrengthLevel0Bar,
      MockChooserController::ConnectedPairedStatus::NONE);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("c"), MockChooserController::kSignalStrengthLevel1Bar,
      MockChooserController::ConnectedPairedStatus::NONE);

  // Select option 0 and press "Cancel" button.
  [table_view_ selectRowIndexes:[NSIndexSet indexSetWithIndex:0]
           byExtendingSelection:NO];
  EXPECT_CALL(*mock_chooser_controller_, Select(testing::_)).Times(0);
  EXPECT_CALL(*mock_chooser_controller_, Cancel()).Times(1);
  [cancel_button_ performClick:chooser_dialog_controller_];
}

TEST_F(ChooserDialogCocoaControllerTest, AdapterOnAndOffAndOn) {
  CreateChooserDialog();

  mock_chooser_controller_->OnAdapterPresenceChanged(
      content::BluetoothChooser::AdapterPresence::POWERED_ON);
  EXPECT_TRUE(adapter_off_help_button_.hidden);
  EXPECT_FALSE(table_view_.hidden);
  // There is no option shown now. But since "No devices found."
  // needs to be displayed on the |table_view_|, the number of rows is 1.
  EXPECT_EQ(1, table_view_.numberOfRows);
  // |table_view_| should be disabled since there is no option shown.
  EXPECT_FALSE(table_view_.enabled);
  // No option selected.
  EXPECT_EQ(-1, table_view_.selectedRow);
  ExpectNoRowImage(0);
  ExpectRowTextIs(
      0, l10n_util::GetNSString(IDS_DEVICE_CHOOSER_NO_DEVICES_FOUND_PROMPT));
  EXPECT_FALSE(IsRowPaired(0));
  EXPECT_TRUE(spinner_.hidden);
  EXPECT_TRUE(scanning_message_.hidden);
  EXPECT_FALSE(word_connector_.hidden);
  EXPECT_FALSE(rescan_button_.hidden);
  EXPECT_NSEQ(l10n_util::GetNSString(IDS_BLUETOOTH_DEVICE_CHOOSER_RE_SCAN),
              rescan_button_.title);
  EXPECT_FALSE(connect_button_.enabled);
  EXPECT_TRUE(cancel_button_.enabled);

  // Add options
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("a"),
      MockChooserController::kNoSignalStrengthLevelImage,
      MockChooserController::ConnectedPairedStatus::CONNECTED |
          MockChooserController::ConnectedPairedStatus::PAIRED);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("b"), MockChooserController::kSignalStrengthLevel0Bar,
      MockChooserController::ConnectedPairedStatus::NONE);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("c"), MockChooserController::kSignalStrengthLevel1Bar,
      MockChooserController::ConnectedPairedStatus::NONE);
  EXPECT_TRUE(table_view_.enabled);
  EXPECT_EQ(3, table_view_.numberOfRows);
  // Select option 1.
  [table_view_ selectRowIndexes:[NSIndexSet indexSetWithIndex:1]
           byExtendingSelection:NO];
  EXPECT_EQ(1, table_view_.selectedRow);
  EXPECT_TRUE(connect_button_.enabled);
  EXPECT_TRUE(cancel_button_.enabled);

  mock_chooser_controller_->OnAdapterPresenceChanged(
      content::BluetoothChooser::AdapterPresence::POWERED_OFF);
  EXPECT_FALSE(adapter_off_help_button_.hidden);
  EXPECT_NSEQ(l10n_util::GetNSString(
                  IDS_BLUETOOTH_DEVICE_CHOOSER_TURN_ON_BLUETOOTH_LINK_TEXT),
              adapter_off_help_button_.title);
  EXPECT_TRUE(table_view_.hidden);
  EXPECT_TRUE(spinner_.hidden);
  EXPECT_TRUE(scanning_message_.hidden);
  EXPECT_TRUE(word_connector_.hidden);
  EXPECT_TRUE(rescan_button_.hidden);
  // Since the adapter is turned off, the previously selected option
  // becomes invalid, the OK button is disabled.
  EXPECT_EQ(0u, mock_chooser_controller_->NumOptions());
  EXPECT_FALSE(connect_button_.enabled);
  EXPECT_TRUE(cancel_button_.enabled);

  mock_chooser_controller_->OnAdapterPresenceChanged(
      content::BluetoothChooser::AdapterPresence::POWERED_ON);
  EXPECT_TRUE(adapter_off_help_button_.hidden);
  EXPECT_FALSE(table_view_.hidden);
  ExpectNoRowImage(0);
  ExpectRowTextIs(
      0, l10n_util::GetNSString(IDS_DEVICE_CHOOSER_NO_DEVICES_FOUND_PROMPT));
  EXPECT_FALSE(IsRowPaired(0));
  EXPECT_EQ(0u, mock_chooser_controller_->NumOptions());
  EXPECT_FALSE(connect_button_.enabled);
  EXPECT_TRUE(cancel_button_.enabled);
}

TEST_F(ChooserDialogCocoaControllerTest, DiscoveringAndNoOptionAddedAndIdle) {
  CreateChooserDialog();

  // Add options
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("a"),
      MockChooserController::kNoSignalStrengthLevelImage,
      MockChooserController::ConnectedPairedStatus::CONNECTED |
          MockChooserController::ConnectedPairedStatus::PAIRED);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("b"), MockChooserController::kSignalStrengthLevel0Bar,
      MockChooserController::ConnectedPairedStatus::NONE);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("c"), MockChooserController::kSignalStrengthLevel1Bar,
      MockChooserController::ConnectedPairedStatus::NONE);
  EXPECT_FALSE(table_view_.hidden);
  EXPECT_TRUE(table_view_.enabled);
  EXPECT_EQ(3, table_view_.numberOfRows);
  // Select option 1.
  [table_view_ selectRowIndexes:[NSIndexSet indexSetWithIndex:1]
           byExtendingSelection:NO];
  EXPECT_EQ(1, table_view_.selectedRow);
  EXPECT_TRUE(spinner_.hidden);
  EXPECT_TRUE(scanning_message_.hidden);
  EXPECT_TRUE(word_connector_.hidden);
  EXPECT_TRUE(rescan_button_.hidden);
  EXPECT_TRUE(connect_button_.enabled);
  EXPECT_TRUE(cancel_button_.enabled);

  mock_chooser_controller_->OnDiscoveryStateChanged(
      content::BluetoothChooser::DiscoveryState::DISCOVERING);
  EXPECT_TRUE(table_view_.hidden);
  EXPECT_FALSE(spinner_.hidden);
  EXPECT_FALSE(scanning_message_.hidden);
  EXPECT_TRUE(word_connector_.hidden);
  EXPECT_TRUE(rescan_button_.hidden);
  // OK button is disabled since the chooser is refreshing options.
  EXPECT_FALSE(connect_button_.enabled);
  EXPECT_TRUE(cancel_button_.enabled);

  mock_chooser_controller_->OnDiscoveryStateChanged(
      content::BluetoothChooser::DiscoveryState::IDLE);
  EXPECT_FALSE(table_view_.hidden);
  // There is no option shown now. But since "No devices found."
  // needs to be displayed on the |table_view_|, the number of rows is 1.
  EXPECT_EQ(1, table_view_.numberOfRows);
  // |table_view_| should be disabled since there is no option shown.
  EXPECT_FALSE(table_view_.enabled);
  // No option selected.
  EXPECT_EQ(-1, table_view_.selectedRow);
  ExpectNoRowImage(0);
  ExpectRowTextIs(
      0, l10n_util::GetNSString(IDS_DEVICE_CHOOSER_NO_DEVICES_FOUND_PROMPT));
  EXPECT_FALSE(IsRowPaired(0));
  EXPECT_TRUE(spinner_.hidden);
  EXPECT_TRUE(scanning_message_.hidden);
  EXPECT_FALSE(word_connector_.hidden);
  EXPECT_FALSE(rescan_button_.hidden);
  EXPECT_NSEQ(l10n_util::GetNSString(IDS_BLUETOOTH_DEVICE_CHOOSER_RE_SCAN),
              rescan_button_.title);
  // OK button is disabled since the chooser refreshed options.
  EXPECT_FALSE(connect_button_.enabled);
  EXPECT_TRUE(cancel_button_.enabled);
}

TEST_F(ChooserDialogCocoaControllerTest,
       DiscoveringAndOneOptionAddedAndSelectedAndIdle) {
  CreateChooserDialog();

  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("a"),
      MockChooserController::kNoSignalStrengthLevelImage,
      MockChooserController::ConnectedPairedStatus::CONNECTED |
          MockChooserController::ConnectedPairedStatus::PAIRED);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("b"), MockChooserController::kSignalStrengthLevel0Bar,
      MockChooserController::ConnectedPairedStatus::NONE);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("c"), MockChooserController::kSignalStrengthLevel1Bar,
      MockChooserController::ConnectedPairedStatus::NONE);
  [table_view_ selectRowIndexes:[NSIndexSet indexSetWithIndex:1]
           byExtendingSelection:NO];

  mock_chooser_controller_->OnDiscoveryStateChanged(
      content::BluetoothChooser::DiscoveryState::DISCOVERING);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("d"), MockChooserController::kSignalStrengthLevel2Bar,
      MockChooserController::ConnectedPairedStatus::NONE);
  EXPECT_FALSE(table_view_.hidden);
  // |table_view_| should be enabled since there is an option.
  EXPECT_TRUE(table_view_.enabled);
  EXPECT_EQ(1, table_view_.numberOfRows);
  // No option selected.
  EXPECT_EQ(-1, table_view_.selectedRow);
  ExpectSignalStrengthLevelImageIs(
      0, MockChooserController::kSignalStrengthLevel2Bar,
      MockChooserController::kImageColorUnselected);
  ExpectRowTextIs(0, @"d");
  EXPECT_FALSE(IsRowPaired(0));
  EXPECT_TRUE(spinner_.hidden);
  EXPECT_FALSE(scanning_message_.hidden);
  EXPECT_TRUE(word_connector_.hidden);
  EXPECT_TRUE(rescan_button_.hidden);
  // OK button is disabled since no option is selected.
  EXPECT_FALSE(connect_button_.enabled);
  EXPECT_TRUE(cancel_button_.enabled);
  [table_view_ selectRowIndexes:[NSIndexSet indexSetWithIndex:0]
           byExtendingSelection:NO];
  EXPECT_EQ(0, table_view_.selectedRow);
  EXPECT_TRUE(connect_button_.enabled);
  EXPECT_TRUE(cancel_button_.enabled);

  mock_chooser_controller_->OnDiscoveryStateChanged(
      content::BluetoothChooser::DiscoveryState::IDLE);
  EXPECT_FALSE(table_view_.hidden);
  EXPECT_TRUE(table_view_.enabled);
  EXPECT_EQ(1, table_view_.numberOfRows);
  EXPECT_EQ(0, table_view_.selectedRow);
  ExpectRowTextIs(0, @"d");
  EXPECT_FALSE(IsRowPaired(0));
  EXPECT_TRUE(spinner_.hidden);
  EXPECT_TRUE(scanning_message_.hidden);
  EXPECT_FALSE(word_connector_.hidden);
  EXPECT_FALSE(rescan_button_.hidden);
  EXPECT_NSEQ(l10n_util::GetNSString(IDS_BLUETOOTH_DEVICE_CHOOSER_RE_SCAN),
              rescan_button_.title);
  EXPECT_TRUE(connect_button_.enabled);
  EXPECT_TRUE(cancel_button_.enabled);
}

TEST_F(ChooserDialogCocoaControllerTest, MultipleSelection) {
  CreateChooserDialog();
  [table_view_ setAllowsMultipleSelection:YES];

  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("a"),
      MockChooserController::kNoSignalStrengthLevelImage,
      MockChooserController::ConnectedPairedStatus::CONNECTED |
          MockChooserController::ConnectedPairedStatus::PAIRED);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("b"), MockChooserController::kSignalStrengthLevel0Bar,
      MockChooserController::ConnectedPairedStatus::NONE);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("c"), MockChooserController::kSignalStrengthLevel1Bar,
      MockChooserController::ConnectedPairedStatus::NONE);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("d"), MockChooserController::kSignalStrengthLevel2Bar,
      MockChooserController::ConnectedPairedStatus::NONE);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("e"), MockChooserController::kSignalStrengthLevel3Bar,
      MockChooserController::ConnectedPairedStatus::NONE);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("f"), MockChooserController::kSignalStrengthLevel4Bar,
      MockChooserController::ConnectedPairedStatus::NONE);

  NSMutableIndexSet* selected_rows = [NSMutableIndexSet indexSet];
  // Select options "a", "c", "d".
  [selected_rows addIndex:0];
  [selected_rows addIndex:2];
  [selected_rows addIndex:3];
  [table_view_ selectRowIndexes:selected_rows byExtendingSelection:NO];

  // The options are: [a] b [c] [d] e f. The option with [] is selected.
  EXPECT_NSEQ(selected_rows, [table_view_ selectedRowIndexes]);
  ExpectRowImageIsConnectedImage(0, SK_ColorWHITE);
  ExpectSignalStrengthLevelImageIs(
      1, MockChooserController::kSignalStrengthLevel0Bar,
      MockChooserController::kImageColorUnselected);
  ExpectSignalStrengthLevelImageIs(
      2, MockChooserController::kSignalStrengthLevel1Bar,
      MockChooserController::kImageColorSelected);
  ExpectSignalStrengthLevelImageIs(
      3, MockChooserController::kSignalStrengthLevel2Bar,
      MockChooserController::kImageColorSelected);
  ExpectSignalStrengthLevelImageIs(
      4, MockChooserController::kSignalStrengthLevel3Bar,
      MockChooserController::kImageColorUnselected);
  ExpectSignalStrengthLevelImageIs(
      5, MockChooserController::kSignalStrengthLevel4Bar,
      MockChooserController::kImageColorUnselected);
  ExpectRowTextColorIs(0, [NSColor whiteColor]);
  ExpectRowTextColorIs(1, [NSColor blackColor]);
  ExpectRowTextColorIs(2, [NSColor whiteColor]);
  ExpectRowTextColorIs(3, [NSColor whiteColor]);
  ExpectRowTextColorIs(4, [NSColor blackColor]);
  ExpectRowTextColorIs(5, [NSColor blackColor]);
  ExpectPairedTextColorIs(0, [NSColor whiteColor]);
  EXPECT_TRUE(connect_button_.enabled);

  // Remove option "b".
  // The options are: [a] [c] [d] e f.
  mock_chooser_controller_->OptionRemoved(base::ASCIIToUTF16("b"));
  EXPECT_TRUE([table_view_ isRowSelected:0]);
  EXPECT_TRUE([table_view_ isRowSelected:1]);
  EXPECT_TRUE([table_view_ isRowSelected:2]);
  EXPECT_FALSE([table_view_ isRowSelected:3]);
  EXPECT_FALSE([table_view_ isRowSelected:4]);
  EXPECT_TRUE(connect_button_.enabled);

  // Remove option "c".
  // The options are: [a] [d] e f.
  mock_chooser_controller_->OptionRemoved(base::ASCIIToUTF16("c"));
  EXPECT_TRUE([table_view_ isRowSelected:0]);
  EXPECT_TRUE([table_view_ isRowSelected:1]);
  EXPECT_FALSE([table_view_ isRowSelected:2]);
  EXPECT_FALSE([table_view_ isRowSelected:3]);
  EXPECT_TRUE(connect_button_.enabled);

  // Remove option "a".
  // The options are: [d] e f.
  mock_chooser_controller_->OptionRemoved(base::ASCIIToUTF16("a"));
  EXPECT_TRUE([table_view_ isRowSelected:0]);
  EXPECT_FALSE([table_view_ isRowSelected:1]);
  EXPECT_FALSE([table_view_ isRowSelected:2]);
  EXPECT_TRUE(connect_button_.enabled);

  // Remove option "d".
  // The options are: e f.
  mock_chooser_controller_->OptionRemoved(base::ASCIIToUTF16("d"));
  EXPECT_FALSE([table_view_ isRowSelected:0]);
  EXPECT_FALSE([table_view_ isRowSelected:1]);
  EXPECT_FALSE(connect_button_.enabled);
}

TEST_F(ChooserDialogCocoaControllerTest, PressAdapterOffHelpButton) {
  CreateChooserDialog();

  EXPECT_CALL(*mock_chooser_controller_, OpenAdapterOffHelpUrl()).Times(1);
  [adapter_off_help_button_ performClick:chooser_dialog_controller_];
}

TEST_F(ChooserDialogCocoaControllerTest, PressRescanButton) {
  CreateChooserDialog();

  EXPECT_CALL(*mock_chooser_controller_, RefreshOptions()).Times(1);
  [rescan_button_ performClick:chooser_dialog_controller_];
}

TEST_F(ChooserDialogCocoaControllerTest, PressHelpButton) {
  CreateChooserDialog();

  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("a"),
      MockChooserController::kNoSignalStrengthLevelImage,
      MockChooserController::ConnectedPairedStatus::CONNECTED |
          MockChooserController::ConnectedPairedStatus::PAIRED);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("b"), MockChooserController::kSignalStrengthLevel0Bar,
      MockChooserController::ConnectedPairedStatus::NONE);
  mock_chooser_controller_->OptionAdded(
      base::ASCIIToUTF16("c"), MockChooserController::kSignalStrengthLevel1Bar,
      MockChooserController::ConnectedPairedStatus::NONE);

  // Select option 0 and press "Get help" button.
  [table_view_ selectRowIndexes:[NSIndexSet indexSetWithIndex:0]
           byExtendingSelection:NO];
  EXPECT_CALL(*mock_chooser_controller_, Select(testing::_)).Times(0);
  EXPECT_CALL(*mock_chooser_controller_, Cancel()).Times(0);
  EXPECT_CALL(*mock_chooser_controller_, OpenHelpCenterUrl()).Times(1);
  [help_button_ performClick:chooser_dialog_controller_];
}
