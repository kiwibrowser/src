// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/media_picker/desktop_media_picker_controller.h"

#include "base/bind.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/media/webrtc/desktop_media_list_observer.h"
#include "chrome/browser/media/webrtc/fake_desktop_media_list.h"
#import "chrome/browser/ui/cocoa/media_picker/desktop_media_picker_item.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest_mac.h"

using content::DesktopMediaID;

@interface DesktopMediaPickerController (ExposedForTesting)
- (IKImageBrowserView*)screenBrowser;
- (IKImageBrowserView*)windowBrowser;
- (NSTableView*)tabBrowser;
- (NSSegmentedControl*)sourceTypeControl;
- (NSButton*)shareButton;
- (NSButton*)audioShareCheckbox;
- (NSArray*)screenItems;
- (NSArray*)windowItems;
- (NSArray*)tabItems;
@end

@implementation DesktopMediaPickerController (ExposedForTesting)
- (IKImageBrowserView*)screenBrowser {
  return screenBrowser_;
}

- (IKImageBrowserView*)windowBrowser {
  return windowBrowser_;
}

- (NSTableView*)tabBrowser {
  return tabBrowser_;
}

- (NSSegmentedControl*)sourceTypeControl {
  return sourceTypeControl_;
}

- (NSButton*)shareButton {
  return shareButton_;
}

- (NSButton*)cancelButton {
  return cancelButton_;
}

- (NSButton*)audioShareCheckbox {
  return audioShareCheckbox_;
}

- (NSArray*)screenItems {
  return screenItems_;
}

- (NSArray*)windowItems {
  return windowItems_;
}

- (NSArray*)tabItems {
  return tabItems_;
}

@end

class DesktopMediaPickerControllerTest : public CocoaTest {
 public:
  DesktopMediaPickerControllerTest() {}

  void SetUp() override {
    CocoaTest::SetUp();

    std::vector<DesktopMediaID::Type> source_types = {
        DesktopMediaID::TYPE_SCREEN, DesktopMediaID::TYPE_WINDOW,
        DesktopMediaID::TYPE_WEB_CONTENTS};

    screen_list_ = new FakeDesktopMediaList(DesktopMediaID::TYPE_SCREEN);
    window_list_ = new FakeDesktopMediaList(DesktopMediaID::TYPE_WINDOW);
    tab_list_ = new FakeDesktopMediaList(DesktopMediaID::TYPE_WEB_CONTENTS);

    std::vector<std::unique_ptr<DesktopMediaList>> source_lists;
    source_lists.push_back(std::unique_ptr<DesktopMediaList>(screen_list_));
    source_lists.push_back(std::unique_ptr<DesktopMediaList>(window_list_));
    source_lists.push_back(std::unique_ptr<DesktopMediaList>(tab_list_));

    DesktopMediaPicker::DoneCallback callback =
        base::Bind(&DesktopMediaPickerControllerTest::OnResult,
                   base::Unretained(this));

    controller_.reset([[DesktopMediaPickerController alloc]
        initWithSourceLists:std::move(source_lists)
                     parent:nil
                   callback:callback
                    appName:base::ASCIIToUTF16("Screenshare Test")
                 targetName:base::ASCIIToUTF16("https://foo.com")
               requestAudio:true]);
  }

  void TearDown() override {
    controller_.reset();
    CocoaTest::TearDown();
  }

  bool WaitForCallback() {
    if (!callback_called_) {
      base::RunLoop().RunUntilIdle();
    }
    return callback_called_;
  }

  void ChangeType(DesktopMediaID::Type sourceType) {
    NSSegmentedControl* control = [controller_ sourceTypeControl];
    [control selectSegmentWithTag:sourceType];
    // [control selectSegmentWithTag] does not trigger handler, so we need to
    // trigger it manually.
    [[control target] performSelector:[control action] withObject:control];
  }

  void AddWindow(int id) {
    window_list_->AddSourceByFullMediaID(
        DesktopMediaID(DesktopMediaID::TYPE_WINDOW, id));
  }

  void AddScreen(int id) {
    screen_list_->AddSourceByFullMediaID(
        DesktopMediaID(DesktopMediaID::TYPE_SCREEN, id));
  }

  void AddTab(int id) {
    tab_list_->AddSourceByFullMediaID(
        DesktopMediaID(DesktopMediaID::TYPE_WEB_CONTENTS, id));
  }

 protected:
  void OnResult(DesktopMediaID source) {
    EXPECT_FALSE(callback_called_);
    callback_called_ = true;
    source_reported_ = source;
  }

  content::TestBrowserThreadBundle thread_bundle_;
  bool callback_called_ = false;
  DesktopMediaID source_reported_;
  FakeDesktopMediaList* screen_list_ = nullptr;
  FakeDesktopMediaList* window_list_ = nullptr;
  FakeDesktopMediaList* tab_list_ = nullptr;
  base::scoped_nsobject<DesktopMediaPickerController> controller_;
};

TEST_F(DesktopMediaPickerControllerTest, ShowAndDismiss) {
  [controller_ showWindow:nil];
  ChangeType(DesktopMediaID::TYPE_SCREEN);

  AddScreen(0);
  AddScreen(1);
  screen_list_->SetSourceThumbnail(1);

  NSArray* items = [controller_ screenItems];
  EXPECT_EQ(2U, [items count]);
  EXPECT_NSEQ(@"0", [[items objectAtIndex:0] imageTitle]);
  EXPECT_EQ(nil, [[items objectAtIndex:0] imageRepresentation]);
  EXPECT_NSEQ(@"1", [[items objectAtIndex:1] imageTitle]);
  EXPECT_TRUE([[items objectAtIndex:1] imageRepresentation] != nil);
}

TEST_F(DesktopMediaPickerControllerTest, ClickShareScreen) {
  [controller_ showWindow:nil];
  ChangeType(DesktopMediaID::TYPE_SCREEN);

  EXPECT_FALSE([[controller_ shareButton] isEnabled]);
  AddScreen(0);
  screen_list_->SetSourceThumbnail(0);
  // First screen will be automatically selected.
  EXPECT_TRUE([[controller_ shareButton] isEnabled]);

  AddScreen(1);
  screen_list_->SetSourceThumbnail(1);

  EXPECT_EQ(2U, [[controller_ screenItems] count]);

  [[controller_ shareButton] performClick:nil];
  EXPECT_TRUE(WaitForCallback());
  EXPECT_EQ(screen_list_->GetSource(0).id, source_reported_);
}

TEST_F(DesktopMediaPickerControllerTest, ClickShareWindow) {
  [controller_ showWindow:nil];
  ChangeType(DesktopMediaID::TYPE_WINDOW);
  AddWindow(0);
  window_list_->SetSourceThumbnail(0);
  AddWindow(1);
  window_list_->SetSourceThumbnail(1);

  EXPECT_EQ(2U, [[controller_ windowItems] count]);
  EXPECT_FALSE([[controller_ shareButton] isEnabled]);

  NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:1];
  [[controller_ windowBrowser] setSelectionIndexes:index_set
                              byExtendingSelection:NO];
  EXPECT_TRUE([[controller_ shareButton] isEnabled]);

  [[controller_ shareButton] performClick:nil];
  EXPECT_TRUE(WaitForCallback());
  EXPECT_EQ(window_list_->GetSource(1).id, source_reported_);
}

TEST_F(DesktopMediaPickerControllerTest, ClickShareTab) {
  [controller_ showWindow:nil];
  ChangeType(DesktopMediaID::TYPE_WEB_CONTENTS);
  AddTab(0);
  tab_list_->SetSourceThumbnail(0);
  AddTab(1);
  tab_list_->SetSourceThumbnail(1);

  EXPECT_EQ(2U, [[controller_ tabItems] count]);
  EXPECT_FALSE([[controller_ shareButton] isEnabled]);

  NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:1];
  [[controller_ tabBrowser] selectRowIndexes:index_set byExtendingSelection:NO];
  EXPECT_TRUE([[controller_ shareButton] isEnabled]);

  // Disable audio share here, otherwise the |source_reported_| will be
  // different from original Id, because audio share is by default on.
  [[controller_ audioShareCheckbox] setState:NSOffState];
  [[controller_ shareButton] performClick:nil];
  EXPECT_TRUE(WaitForCallback());
  EXPECT_EQ(tab_list_->GetSource(1).id, source_reported_);
}

TEST_F(DesktopMediaPickerControllerTest, ClickCancel) {
  [controller_ showWindow:nil];
  ChangeType(DesktopMediaID::TYPE_WINDOW);

  AddWindow(0);
  window_list_->SetSourceThumbnail(0);
  AddWindow(1);
  window_list_->SetSourceThumbnail(1);

  NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:1];
  [[controller_ windowBrowser] setSelectionIndexes:index_set
                              byExtendingSelection:NO];
  [[controller_ cancelButton] performClick:nil];
  EXPECT_TRUE(WaitForCallback());
  EXPECT_EQ(DesktopMediaID(), source_reported_);
}

TEST_F(DesktopMediaPickerControllerTest, CloseWindow) {
  [controller_ showWindow:nil];
  ChangeType(DesktopMediaID::TYPE_SCREEN);

  AddScreen(0);
  screen_list_->SetSourceThumbnail(0);
  AddScreen(1);
  screen_list_->SetSourceThumbnail(1);

  [controller_ close];
  EXPECT_TRUE(WaitForCallback());
  EXPECT_EQ(DesktopMediaID(), source_reported_);
}

TEST_F(DesktopMediaPickerControllerTest, UpdateThumbnail) {
  [controller_ showWindow:nil];
  ChangeType(DesktopMediaID::TYPE_WEB_CONTENTS);

  AddTab(0);
  tab_list_->SetSourceThumbnail(0);
  AddTab(1);
  tab_list_->SetSourceThumbnail(1);

  NSArray* items = [controller_ tabItems];
  EXPECT_EQ(2U, [items count]);
  NSUInteger version = [[items objectAtIndex:0] imageVersion];

  tab_list_->SetSourceThumbnail(0);
  EXPECT_NE(version, [[items objectAtIndex:0] imageVersion]);
}

TEST_F(DesktopMediaPickerControllerTest, UpdateName) {
  [controller_ showWindow:nil];
  ChangeType(DesktopMediaID::TYPE_WINDOW);

  AddWindow(0);
  window_list_->SetSourceThumbnail(0);
  AddWindow(1);
  window_list_->SetSourceThumbnail(1);

  NSArray* items = [controller_ windowItems];
  EXPECT_EQ(2U, [items count]);
  NSUInteger version = [[items objectAtIndex:0] imageVersion];

  window_list_->SetSourceThumbnail(0);
  EXPECT_NE(version, [[items objectAtIndex:0] imageVersion]);
}

TEST_F(DesktopMediaPickerControllerTest, RemoveSource) {
  [controller_ showWindow:nil];
  ChangeType(DesktopMediaID::TYPE_SCREEN);

  AddScreen(0);
  AddScreen(1);
  AddScreen(2);
  screen_list_->SetSourceName(1, base::ASCIIToUTF16("foo"));

  NSArray* items = [controller_ screenItems];
  EXPECT_EQ(3U, [items count]);
  EXPECT_NSEQ(@"foo", [[items objectAtIndex:1] imageTitle]);
}

TEST_F(DesktopMediaPickerControllerTest, MoveSource) {
  [controller_ showWindow:nil];
  ChangeType(DesktopMediaID::TYPE_WINDOW);

  AddWindow(0);
  AddWindow(1);
  window_list_->SetSourceName(1, base::ASCIIToUTF16("foo"));
  NSArray* items = [controller_ windowItems];
  EXPECT_NSEQ(@"foo", [[items objectAtIndex:1] imageTitle]);

  window_list_->MoveSource(1, 0);
  EXPECT_NSEQ(@"foo", [[items objectAtIndex:0] imageTitle]);

  window_list_->MoveSource(0, 1);
  EXPECT_NSEQ(@"foo", [[items objectAtIndex:1] imageTitle]);
}

// Make sure the audio share checkbox' state reacts correctly with
// the source selection. Namely the checkbox is enabled only for tab
// sharing on Mac.
TEST_F(DesktopMediaPickerControllerTest, AudioShareCheckboxState) {
  [controller_ showWindow:nil];

  AddScreen(0);
  AddWindow(1);
  AddTab(2);

  NSButton* checkbox = [controller_ audioShareCheckbox];
  EXPECT_EQ(YES, [checkbox isHidden]);

  [checkbox setHidden:NO];
  ChangeType(DesktopMediaID::TYPE_WINDOW);
  EXPECT_EQ(YES, [checkbox isHidden]);

  [checkbox setHidden:YES];
  ChangeType(DesktopMediaID::TYPE_WEB_CONTENTS);
  EXPECT_EQ(NO, [checkbox isHidden]);

  [checkbox setHidden:NO];
  ChangeType(DesktopMediaID::TYPE_SCREEN);
  EXPECT_EQ(YES, [checkbox isHidden]);
}

TEST_F(DesktopMediaPickerControllerTest, TabShareWithAudio) {
  [controller_ showWindow:nil];
  ChangeType(DesktopMediaID::TYPE_WEB_CONTENTS);

  DesktopMediaID origin_id =
      DesktopMediaID(DesktopMediaID::TYPE_WEB_CONTENTS, 123);
  DesktopMediaID id_with_audio = origin_id;
  id_with_audio.audio_share = true;

  tab_list_->AddSourceByFullMediaID(origin_id);

  NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:0];
  [[controller_ tabBrowser] selectRowIndexes:index_set byExtendingSelection:NO];
  EXPECT_TRUE([[controller_ shareButton] isEnabled]);

  [[controller_ shareButton] performClick:nil];

  EXPECT_TRUE(WaitForCallback());
  EXPECT_EQ(id_with_audio, source_reported_);
}

TEST_F(DesktopMediaPickerControllerTest, TabBrowserFocusAlgorithm) {
  [controller_ showWindow:nil];
  ChangeType(DesktopMediaID::TYPE_WEB_CONTENTS);
  AddTab(0);
  AddTab(1);
  AddTab(2);
  AddTab(3);

  NSArray* items = [controller_ tabItems];
  NSTableView* browser = [controller_ tabBrowser];

  NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:1];
  [browser selectRowIndexes:index_set byExtendingSelection:NO];

  // Move source [0, 1, 2, 3]-->[1, 2, 3, 0]
  tab_list_->MoveSource(0, 3);
  NSUInteger selected_index = [[browser selectedRowIndexes] firstIndex];
  EXPECT_EQ(1, [[items objectAtIndex:selected_index] sourceID].id);

  // Move source [1, 2, 3, 0]-->[3, 1, 2, 0]
  tab_list_->MoveSource(2, 0);
  selected_index = [[browser selectedRowIndexes] firstIndex];
  EXPECT_EQ(1, [[items objectAtIndex:selected_index] sourceID].id);

  // Remove a source [3, 1, 2, 0]-->[1, 2, 0]
  tab_list_->RemoveSource(0);
  selected_index = [[browser selectedRowIndexes] firstIndex];
  EXPECT_EQ(1, [[items objectAtIndex:selected_index] sourceID].id);

  // Change source type back and forth, browser should memorize the selection.
  ChangeType(DesktopMediaID::TYPE_SCREEN);
  ChangeType(DesktopMediaID::TYPE_WEB_CONTENTS);
  selected_index = [[browser selectedRowIndexes] firstIndex];
  EXPECT_EQ(1, [[items objectAtIndex:selected_index] sourceID].id);
}

TEST_F(DesktopMediaPickerControllerTest, SingleScreenNoLabel) {
  [controller_ showWindow:nil];
  ChangeType(DesktopMediaID::TYPE_SCREEN);

  NSArray* items = [controller_ screenItems];

  AddScreen(0);
  screen_list_->SetSourceThumbnail(0);
  EXPECT_EQ(1U, [items count]);
  EXPECT_EQ(nil, [[items objectAtIndex:0] imageTitle]);

  AddScreen(1);
  screen_list_->SetSourceThumbnail(1);
  EXPECT_EQ(2U, [items count]);
  EXPECT_NE(nil, [[items objectAtIndex:0] imageTitle]);
  EXPECT_NE(nil, [[items objectAtIndex:1] imageTitle]);
}
