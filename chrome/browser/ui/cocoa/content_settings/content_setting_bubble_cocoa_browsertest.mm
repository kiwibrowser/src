// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/content_settings/content_setting_bubble_cocoa.h"

#import <Cocoa/Cocoa.h>

#include "base/mac/foundation_util.h"
#include "base/mac/mac_util.h"
#include "base/mac/scoped_nsautorelease_pool.h"
#include "base/mac/scoped_nsobject.h"
#include "base/strings/sys_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/tab_specific_content_settings.h"
#include "chrome/browser/download/download_request_limiter.h"
#include "chrome/browser/media/webrtc/media_capture_devices_dispatcher.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_content_setting_bubble_model_delegate.h"
#import "chrome/browser/ui/cocoa/subresource_filter/subresource_filter_bubble_controller.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "chrome/browser/ui/content_settings/content_setting_bubble_model.h"
#include "chrome/browser/ui/content_settings/content_setting_image_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_features.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/strings/grit/components_strings.h"
#include "components/subresource_filter/core/browser/subresource_filter_constants.h"
#include "content/public/common/media_stream_request.h"
#include "content/public/test/test_navigation_observer.h"
#include "testing/gtest_mac.h"
#include "ui/base/cocoa/touch_bar_util.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

// Touch bar identifier.
NSString* const kContentSettingsBubbleTouchBarId = @"content-settings-bubble";

// Touch bar item identifiers.
NSString* const kManageTouchBarId = @"MANAGE";
NSString* const kDoneTouchBarId = @"DONE";

}  // namespace

class ContentSettingBubbleControllerTest : public InProcessBrowserTest {
 protected:
  ContentSettingBubbleControllerTest() {}

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  Profile* profile() {
    return browser()->profile();
  }

  // Helper function to create the bubble controller.
  ContentSettingBubbleController* CreateBubbleController(
      ContentSettingBubbleModel* bubble);

  base::scoped_nsobject<NSWindow> parent_;

 private:
  base::mac::ScopedNSAutoreleasePool pool_;
};

ContentSettingBubbleController*
ContentSettingBubbleControllerTest::CreateBubbleController(
    ContentSettingBubbleModel* bubble) {
  parent_.reset([[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 800, 600)
                                            styleMask:NSBorderlessWindowMask
                                              backing:NSBackingStoreBuffered
                                                defer:NO]);
  [parent_ setReleasedWhenClosed:NO];
  [parent_ orderFront:nil];

  ContentSettingBubbleController* controller =
      [ContentSettingBubbleController showForModel:bubble
                                       webContents:web_contents()
                                      parentWindow:parent_
                                        decoration:nullptr
                                        anchoredAt:NSMakePoint(50, 20)];

  EXPECT_TRUE(controller);
  EXPECT_TRUE([[controller window] isVisible]);

  return controller;
}

// Check that the bubble doesn't crash or leak for any image model.
IN_PROC_BROWSER_TEST_F(ContentSettingBubbleControllerTest, Init) {
  TabSpecificContentSettings::FromWebContents(web_contents())->
      BlockAllContentForTesting();

  // Automatic downloads are handled by DownloadRequestLimiter.
  DownloadRequestLimiter::TabDownloadState* tab_download_state =
      g_browser_process->download_request_limiter()->GetDownloadState(
          web_contents(), web_contents(), true);
  tab_download_state->set_download_seen();
  tab_download_state->SetDownloadStatusAndNotify(
      DownloadRequestLimiter::DOWNLOADS_NOT_ALLOWED);

  std::vector<std::unique_ptr<ContentSettingImageModel>> models =
      ContentSettingImageModel::GenerateContentSettingImageModels();
  for (const auto& model : models) {
    ContentSettingBubbleModel* bubble =
        model->CreateBubbleModel(nullptr, web_contents(), profile());

    // Skip this test for the ContentSettingFramebustBlockBubbleModel, which
    // doesn't have a Cocoa version.
    if (bubble->AsFramebustBlockBubbleModel())
      continue;

    ContentSettingBubbleController* controller = CreateBubbleController(bubble);
    // No bubble except the one for media should have media menus.
    if (!bubble->AsMediaStreamBubbleModel())
      EXPECT_EQ(0u, [controller mediaMenus]->size());
    [parent_ close];
  }
}

// The media bubble is specific in that in serves two content setting types.
// Test that it works too.
IN_PROC_BROWSER_TEST_F(ContentSettingBubbleControllerTest, MediaStreamBubble) {
  TabSpecificContentSettings::FromWebContents(web_contents())->
      BlockAllContentForTesting();
  MediaCaptureDevicesDispatcher::GetInstance()->
      DisableDeviceEnumerationForTesting();
  ContentSettingBubbleController* controller = CreateBubbleController(
      new ContentSettingMediaStreamBubbleModel(
          nullptr, web_contents(), profile()));
  content_setting_bubble::MediaMenuPartsMap* mediaMenus =
      [controller mediaMenus];
  EXPECT_EQ(2u, mediaMenus->size());
  NSString* title = l10n_util::GetNSString(IDS_MEDIA_MENU_NO_DEVICE_TITLE);
  for (content_setting_bubble::MediaMenuPartsMap::const_iterator i =
       mediaMenus->begin(); i != mediaMenus->end(); ++i) {
    EXPECT_TRUE((content::MEDIA_DEVICE_AUDIO_CAPTURE == i->second->type) ||
                (content::MEDIA_DEVICE_VIDEO_CAPTURE == i->second->type));
    EXPECT_EQ(0, [i->first numberOfItems]);
    EXPECT_NSEQ(title, [i->first title]);
    EXPECT_FALSE([i->first isEnabled]);
  }

 [parent_ close];
}

// Subresource Filter bubble.

IN_PROC_BROWSER_TEST_F(ContentSettingBubbleControllerTest,
                       InitSubresourceFilter) {
  ContentSettingBubbleController* controller =
      CreateBubbleController(new ContentSettingSubresourceFilterBubbleModel(
          nullptr, web_contents(), profile()));
  EXPECT_TRUE(controller);

  SubresourceFilterBubbleController* filterController =
      base::mac::ObjCCast<SubresourceFilterBubbleController>(controller);

  EXPECT_TRUE([filterController messageLabel]);
  NSString* label = base::SysUTF16ToNSString(
      l10n_util::GetStringUTF16(IDS_BLOCKED_ADS_PROMPT_EXPLANATION));
  EXPECT_NSEQ([[filterController messageLabel] stringValue], label);

  NSString* link =
      base::SysUTF16ToNSString(l10n_util::GetStringUTF16(IDS_LEARN_MORE));
  EXPECT_NSEQ([[filterController learnMoreLink] title], link);

  EXPECT_TRUE([filterController manageCheckbox]);
  label =
      base::SysUTF16ToNSString(l10n_util::GetStringUTF16(IDS_ALWAYS_ALLOW_ADS));
  EXPECT_NSEQ([[filterController manageCheckbox] title], label);

  EXPECT_TRUE([filterController doneButton]);
  label = base::SysUTF16ToNSString(l10n_util::GetStringUTF16(IDS_OK));
  EXPECT_NSEQ([[filterController doneButton] title], label);

  [parent_ close];
}

IN_PROC_BROWSER_TEST_F(ContentSettingBubbleControllerTest,
                       ManageCheckboxSubresourceFilter) {
  ContentSettingSubresourceFilterBubbleModel* model =
      new ContentSettingSubresourceFilterBubbleModel(nullptr, web_contents(),
                                                     profile());
  ContentSettingBubbleController* controller = CreateBubbleController(model);
  EXPECT_TRUE(controller);

  SubresourceFilterBubbleController* filterController =
      base::mac::ObjCCast<SubresourceFilterBubbleController>(controller);
  NSButton* manageCheckbox = [filterController manageCheckbox];
  NSButton* doneButton = [filterController doneButton];

  EXPECT_EQ([manageCheckbox state], NSOffState);

  NSString* label = base::SysUTF16ToNSString(l10n_util::GetStringUTF16(IDS_OK));
  EXPECT_NSEQ([doneButton title], label);

  [manageCheckbox setState:NSOnState];
  [filterController manageCheckboxChecked:manageCheckbox];
  EXPECT_EQ([manageCheckbox state], NSOnState);

  label =
      base::SysUTF16ToNSString(l10n_util::GetStringUTF16(IDS_APP_MENU_RELOAD));
  EXPECT_NSEQ([doneButton title], label);

  [parent_ close];
}

IN_PROC_BROWSER_TEST_F(ContentSettingBubbleControllerTest,
                       LearnMoreLinkClicked) {
  ContentSettingBubbleController* controller =
      CreateBubbleController(new ContentSettingSubresourceFilterBubbleModel(
          browser()->content_setting_bubble_model_delegate(), web_contents(),
          profile()));
  EXPECT_TRUE(controller);

  SubresourceFilterBubbleController* filter_controller =
      base::mac::ObjCCast<SubresourceFilterBubbleController>(controller);

  NSString* link =
      base::SysUTF16ToNSString(l10n_util::GetStringUTF16(IDS_LEARN_MORE));
  EXPECT_NSEQ([[filter_controller learnMoreLink] title], link);

  NSButton* learn_more_link = [filter_controller learnMoreLink];
  [filter_controller learnMoreLinkClicked:learn_more_link];

  content::TestNavigationObserver observer(web_contents());
  observer.Wait();

  std::string link_value(subresource_filter::kLearnMoreLink);
  EXPECT_EQ(link_value, web_contents()->GetLastCommittedURL().spec());

  [parent_ close];
}

// Verifies the bubble's touch bar.
IN_PROC_BROWSER_TEST_F(ContentSettingBubbleControllerTest, TouchBar) {
  if (@available(macOS 10.12.2, *)) {
    base::test::ScopedFeatureList feature_list;
    feature_list.InitAndEnableFeature(features::kDialogTouchBar);

    TabSpecificContentSettings::FromWebContents(web_contents())
        ->BlockAllContentForTesting();
    ContentSettingBubbleController* controller =
        CreateBubbleController(new ContentSettingMediaStreamBubbleModel(
            nullptr, web_contents(), profile()));
    EXPECT_TRUE(controller);

    NSTouchBar* touch_bar = nil;
    touch_bar = [controller makeTouchBar];
    NSArray* touch_bar_items = [touch_bar itemIdentifiers];
    EXPECT_TRUE([touch_bar_items
        containsObject:ui::GetTouchBarItemId(kContentSettingsBubbleTouchBarId,
                                             kDoneTouchBarId)]);
    EXPECT_TRUE([touch_bar_items
        containsObject:ui::GetTouchBarItemId(kContentSettingsBubbleTouchBarId,
                                             kManageTouchBarId)]);
  }

  [parent_ close];
}
