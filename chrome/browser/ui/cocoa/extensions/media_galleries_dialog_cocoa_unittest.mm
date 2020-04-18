// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/media_galleries/media_galleries_dialog_controller_mock.h"
#include "chrome/browser/ui/cocoa/extensions/media_galleries_dialog_cocoa.h"
#include "components/storage_monitor/storage_info.h"
#include "extensions/common/extension.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Mock;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnPointee;
using ::testing::ReturnRef;

@interface MediaGalleryListEntry (testing)
- (NSInteger)state;
- (void)performClick:(id)sender;
@end

@implementation MediaGalleryListEntry (testing)

- (NSInteger)state {
  return [checkbox_ state];
}

- (void)performClick:(id)sender {
  [checkbox_ performClick:sender];
}

@end


MediaGalleryPrefInfo MakePrefInfoForTesting(MediaGalleryPrefId pref_id) {
  MediaGalleryPrefInfo gallery;
  gallery.pref_id = pref_id;
  gallery.device_id = storage_monitor::StorageInfo::MakeDeviceId(
      storage_monitor::StorageInfo::FIXED_MASS_STORAGE,
      base::Int64ToString(pref_id));
  gallery.display_name = base::ASCIIToUTF16("name");
  return gallery;
}

class MediaGalleriesDialogTest : public testing::Test {
 public:
  MediaGalleriesDialogTest() {}
  ~MediaGalleriesDialogTest() override {}

  void SetUp() override {
    std::vector<base::string16> headers;
    headers.push_back(base::string16());
    headers.push_back(base::ASCIIToUTF16("header2"));
    ON_CALL(controller_, GetSectionHeaders()).
        WillByDefault(Return(headers));
    EXPECT_CALL(controller_, GetSectionEntries(_)).
        Times(AnyNumber());
  }

  void TearDown() override {
    Mock::VerifyAndClearExpectations(&controller_);
    dialog_.reset();
  }

  NiceMock<MediaGalleriesDialogControllerMock>* controller() {
    return &controller_;
  }

  MediaGalleriesDialogCocoa* GetOrCreateDialog() {
    if (!dialog_.get()) {
      dialog_.reset(static_cast<MediaGalleriesDialogCocoa*>(
          MediaGalleriesDialog::Create(&controller_)));
    }
    return dialog_.get();
  }

 private:
  NiceMock<MediaGalleriesDialogControllerMock> controller_;

  std::unique_ptr<MediaGalleriesDialogCocoa> dialog_;

  DISALLOW_COPY_AND_ASSIGN(MediaGalleriesDialogTest);
};

// Tests that checkboxes are initialized according to the contents of
// permissions().
TEST_F(MediaGalleriesDialogTest, InitializeCheckboxes) {
  MediaGalleriesDialogController::Entries attached_permissions;
  attached_permissions.push_back(
      MediaGalleriesDialogController::Entry(MakePrefInfoForTesting(1), true));
  attached_permissions.push_back(
      MediaGalleriesDialogController::Entry(MakePrefInfoForTesting(2), false));
  EXPECT_CALL(*controller(), GetSectionEntries(0)).
      WillRepeatedly(Return(attached_permissions));

  // Initializing checkboxes should not cause them to be toggled.
  EXPECT_CALL(*controller(), DidToggleEntry(_, _)).
      Times(0);

  EXPECT_EQ(2U, [[GetOrCreateDialog()->checkbox_container_ subviews] count]);

  NSButton* checkbox1 =
      [[GetOrCreateDialog()->checkbox_container_ subviews] objectAtIndex:0];
  EXPECT_EQ([checkbox1 state], NSOnState);

  NSButton* checkbox2 =
      [[GetOrCreateDialog()->checkbox_container_ subviews] objectAtIndex:1];
  EXPECT_EQ([checkbox2 state], NSOffState);
}

// Tests that toggling checkboxes updates the controller.
TEST_F(MediaGalleriesDialogTest, ToggleCheckboxes) {
  MediaGalleriesDialogController::Entries attached_permissions;
  attached_permissions.push_back(
      MediaGalleriesDialogController::Entry(MakePrefInfoForTesting(1), true));
  EXPECT_CALL(*controller(), GetSectionEntries(0)).
      WillRepeatedly(Return(attached_permissions));

  EXPECT_EQ(1U, [[GetOrCreateDialog()->checkbox_container_ subviews] count]);

  NSButton* checkbox =
      [[GetOrCreateDialog()->checkbox_container_ subviews] objectAtIndex:0];
  EXPECT_EQ([checkbox state], NSOnState);

  EXPECT_CALL(*controller(), DidToggleEntry(1, false));
  [checkbox performClick:nil];
  EXPECT_EQ([checkbox state], NSOffState);

  EXPECT_CALL(*controller(), DidToggleEntry(1, true));
  [checkbox performClick:nil];
  EXPECT_EQ([checkbox state], NSOnState);
}

// Tests that UpdateGalleries will add a new checkbox, but only if it refers to
// a gallery that the dialog hasn't seen before.
TEST_F(MediaGalleriesDialogTest, UpdateAdds) {
  MediaGalleriesDialogController::Entries attached_permissions;
  EXPECT_CALL(*controller(), GetSectionEntries(0)).
      WillRepeatedly(ReturnPointee(&attached_permissions));

  EXPECT_EQ(0U, [[GetOrCreateDialog()->checkbox_container_ subviews] count]);
  CGFloat old_container_height =
      NSHeight([GetOrCreateDialog()->checkbox_container_ frame]);

  attached_permissions.push_back(
      MediaGalleriesDialogController::Entry(MakePrefInfoForTesting(1), true));
  GetOrCreateDialog()->UpdateGalleries();
  EXPECT_EQ(1U, [[GetOrCreateDialog()->checkbox_container_ subviews] count]);

  // The checkbox container should be taller.
  CGFloat new_container_height =
      NSHeight([GetOrCreateDialog()->checkbox_container_ frame]);
  EXPECT_GT(new_container_height, old_container_height);
  old_container_height = new_container_height;

  attached_permissions.push_back(
      MediaGalleriesDialogController::Entry(MakePrefInfoForTesting(2), true));
  GetOrCreateDialog()->UpdateGalleries();
  EXPECT_EQ(2U, [[GetOrCreateDialog()->checkbox_container_ subviews] count]);

  // The checkbox container should be taller.
  new_container_height =
      NSHeight([GetOrCreateDialog()->checkbox_container_ frame]);
  EXPECT_GT(new_container_height, old_container_height);
  old_container_height = new_container_height;

  attached_permissions[1].selected = false;
  GetOrCreateDialog()->UpdateGalleries();
  EXPECT_EQ(2U, [[GetOrCreateDialog()->checkbox_container_ subviews] count]);

  // The checkbox container height should not have changed.
  new_container_height =
      NSHeight([GetOrCreateDialog()->checkbox_container_ frame]);
  EXPECT_EQ(new_container_height, old_container_height);
}

TEST_F(MediaGalleriesDialogTest, ForgetDeletes) {
  MediaGalleriesDialogController::Entries attached_permissions;
  EXPECT_CALL(*controller(), GetSectionEntries(0)).
      WillRepeatedly(ReturnPointee(&attached_permissions));

  GetOrCreateDialog();

  // Add a couple of galleries.
  attached_permissions.push_back(
      MediaGalleriesDialogController::Entry(MakePrefInfoForTesting(1), true));
  GetOrCreateDialog()->UpdateGalleries();
  attached_permissions.push_back(
      MediaGalleriesDialogController::Entry(MakePrefInfoForTesting(2), true));
  GetOrCreateDialog()->UpdateGalleries();
  EXPECT_EQ(2U, [[GetOrCreateDialog()->checkbox_container_ subviews] count]);
  CGFloat old_container_height =
      NSHeight([GetOrCreateDialog()->checkbox_container_ frame]);

  // Remove a gallery.
  attached_permissions.erase(attached_permissions.begin());
  GetOrCreateDialog()->UpdateGalleries();
  EXPECT_EQ(1U, [[GetOrCreateDialog()->checkbox_container_ subviews] count]);

  // The checkbox container should be shorter.
  CGFloat new_container_height =
      NSHeight([GetOrCreateDialog()->checkbox_container_ frame]);
  EXPECT_LT(new_container_height, old_container_height);
}
