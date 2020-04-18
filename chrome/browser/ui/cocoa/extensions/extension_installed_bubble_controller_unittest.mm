// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/extensions/extension_installed_bubble_controller.h"

#import <Cocoa/Cocoa.h>
#include <stddef.h>

#include <memory>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/values.h"
#include "chrome/browser/extensions/api/commands/command_service.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/test_extension_system.h"
#include "chrome/browser/ui/browser_window.h"
#import "chrome/browser/ui/cocoa/info_bubble_window.h"
#import "chrome/browser/ui/cocoa/test/cocoa_profile_test.h"
#include "chrome/browser/ui/extensions/extension_installed_bubble.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/testing_profile.h"
#include "components/crx_file/id_util.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/feature_switch.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/value_builder.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#include "third_party/ocmock/gtest_support.h"
#include "ui/gfx/codec/png_codec.h"

using extensions::Extension;
using extensions::DictionaryBuilder;

class ExtensionInstalledBubbleControllerTest : public CocoaProfileTest {
 protected:
  ExtensionInstalledBubbleControllerTest() {}
  ~ExtensionInstalledBubbleControllerTest() override {}

  enum ExtensionType {
    BROWSER_ACTION,
    PAGE_ACTION,
    APP,
  };

  void SetUp() override {
    CocoaProfileTest::SetUp();
    ASSERT_TRUE(browser());
    window_ = browser()->window()->GetNativeWindow();
    icon_ = LoadTestIcon();
    base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
    extensionService_ = static_cast<extensions::TestExtensionSystem*>(
        extensions::ExtensionSystem::Get(profile()))->CreateExtensionService(
            &command_line, base::FilePath(), false);
  }

  // Adds a WebContents to the tab strip.
  void AddWebContents() {
    std::unique_ptr<content::WebContents> web_contents = base::WrapUnique(
        content::WebContents::Create(content::WebContents::CreateParams(
            profile(), content::SiteInstance::Create(profile()))));
    browser()->tab_strip_model()->AppendWebContents(std::move(web_contents),
                                                    true);
  }

  // Create a simple extension of the given |type| and manifest |location|, and
  // optionally with an associated keybinding.
  void CreateExtension(ExtensionType type,
                       bool has_keybinding,
                       extensions::Manifest::Location location) {
    DictionaryBuilder manifest;
    manifest.Set("version", "1.0");
    manifest.Set("name", "extension");
    manifest.Set("manifest_version", 2);
    switch (type) {
      case PAGE_ACTION:
        manifest.Set("page_action", DictionaryBuilder().Build());
        break;
      case BROWSER_ACTION:
        manifest.Set("browser_action", DictionaryBuilder().Build());
        break;
      case APP:
        manifest.Set(
            "app",
            DictionaryBuilder()
                .Set("launch", DictionaryBuilder()
                                   .Set("web_url", "http://www.example.com")
                                   .Build())
                .Build());
        break;
    }

    if (has_keybinding) {
      DictionaryBuilder command;
      command.Set(type == PAGE_ACTION ? "_execute_page_action"
                                      : "_execute_browser_action",
                  DictionaryBuilder()
                      .Set("suggested_key", DictionaryBuilder()
                                                .Set("mac", "MacCtrl+Shift+E")
                                                .Set("default", "Ctrl+Shift+E")
                                                .Build())
                      .Build());
      manifest.Set("commands", command.Build());
    }

    extension_ = extensions::ExtensionBuilder()
                     .SetManifest(manifest.Build())
                     .SetID(crx_file::id_util::GenerateId("foo"))
                     .SetLocation(location)
                     .Build();
    extensionService_->AddExtension(extension_.get());
    if (has_keybinding) {
      // Slight hack: manually notify the command service of the extension since
      // it doesn't go through the normal installation flow.
      extensions::CommandService::Get(profile())->UpdateKeybindingsForTest(
          extension_.get());
    }
  }
  void CreateExtension(ExtensionType type, bool has_keybinding) {
    CreateExtension(type, has_keybinding, extensions::Manifest::INTERNAL);
  }

  // Create and return an ExtensionInstalledBubbleController and instruct it to
  // show itself.
  ExtensionInstalledBubbleController* CreateController() {
    extensionBubble_.reset(
        new ExtensionInstalledBubble(extension_.get(), browser(), icon_));
    extensionBubble_->Initialize();
    ExtensionInstalledBubbleController* controller =
        [[ExtensionInstalledBubbleController alloc]
            initWithParentWindow:window_
                 extensionBubble:extensionBubble_.get()];

    // Bring up the window and disable close animation.
    [controller showWindow:nil];
    NSWindow* bubbleWindow = [controller window];
    CHECK([bubbleWindow isKindOfClass:[InfoBubbleWindow class]]);
    [static_cast<InfoBubbleWindow*>(bubbleWindow)
        setAllowedAnimations:info_bubble::kAnimateNone];

    return controller;
  }

  NSWindow* window() { return window_; }

 private:
  // Load test icon from extension test directory.
  SkBitmap LoadTestIcon() {
    base::FilePath path;
    base::PathService::Get(chrome::DIR_TEST_DATA, &path);
    path = path.AppendASCII("extensions").AppendASCII("icon1.png");

    std::string file_contents;
    base::ReadFileToString(path, &file_contents);
    const unsigned char* data =
        reinterpret_cast<const unsigned char*>(file_contents.data());

    SkBitmap bitmap;
    gfx::PNGCodec::Decode(data, file_contents.length(), &bitmap);
    return bitmap;
  }

  // Required to initialize the extension installed bubble.
  NSWindow* window_;  // weak, owned by CocoaProfileTest.

  // The associated ExtensionService, owned by the ExtensionSystem.
  ExtensionService* extensionService_;

  // Skeleton extension to be tested; reinitialized for each test.
  scoped_refptr<Extension> extension_;

  // The bubble that tests are run on.
  std::unique_ptr<ExtensionInstalledBubble> extensionBubble_;

  // The icon_ to be loaded into the bubble window.
  SkBitmap icon_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionInstalledBubbleControllerTest);
};

// We don't want to just test the bounds of these frames, because that results
// in a change detector test (and just duplicates the logic in the class).
// Instead, we do a few sanity checks.
void SanityCheckFrames(NSRect frames[], size_t size) {
  for (size_t i = 0; i < size; ++i) {
    // Check 1: Non-hidden views should have a non-empty frame.
    EXPECT_FALSE(NSIsEmptyRect(frames[i])) <<
        "Frame at index " << i << " is empty";
    // Check 2: No frames should overlap.
    for (size_t j = 0; j < i; ++j) {
      EXPECT_FALSE(NSIntersectsRect(frames[i], frames[j])) <<
          "Frame at index " << i << " intersects frame at index " << j;
    }
  }
}

// Test the basic layout of the bubble for an extension that is from the store.
TEST_F(ExtensionInstalledBubbleControllerTest,
       BubbleLayoutFromStoreNoKeybinding) {
  CreateExtension(BROWSER_ACTION, false);
  ExtensionInstalledBubbleController* controller = CreateController();
  ASSERT_TRUE(controller);

  // The extension bubble should have the "how to use", "how to manage", and
  // "sign in promo" areas. Since it doesn't have an associated keybinding, it
  // shouldn't have the "manage shortcut" view.
  EXPECT_FALSE([[controller howToUse] isHidden]);
  EXPECT_FALSE([[controller howToManage] isHidden]);
  EXPECT_FALSE([[controller promoContainer] isHidden]);
  EXPECT_TRUE([[controller manageShortcutLink] isHidden]);

  NSRect headingFrame = [[controller heading] frame];
  NSRect closeFrame = [[controller closeButton] frame];
  NSRect howToUseFrame = [[controller howToUse] frame];
  NSRect howToManageFrame = [[controller howToManage] frame];
  NSRect syncPromoFrame = [[controller promoContainer] frame];
  NSRect iconFrame = [[controller iconImage] frame];

  NSRect frames[] = {headingFrame, closeFrame, howToUseFrame, howToManageFrame,
                     syncPromoFrame, iconFrame};
  SanityCheckFrames(frames, arraysize(frames));

  // Check the overall layout of the bubble; it should be:
  // |------| | Heading        |
  // | icon | | How to Use     |
  // |------| | How to Manage  |
  // |-------------------------|
  // |       Sync Promo        |
  // |-------------------------|
  EXPECT_GT(NSMinY(headingFrame), NSMinY(howToUseFrame));
  EXPECT_GT(NSMinY(howToUseFrame), NSMinY(howToManageFrame));
  EXPECT_GT(NSMinY(howToManageFrame), NSMinY(syncPromoFrame));
  EXPECT_GT(NSMinY(iconFrame), NSMinY(syncPromoFrame));
  EXPECT_GT(NSMinY(iconFrame), 0);
  EXPECT_EQ(NSMinY(syncPromoFrame), 0);

  [controller close];
}

// Test the layout of a bubble for an extension that is from the store with an
// associated keybinding.
TEST_F(ExtensionInstalledBubbleControllerTest,
       BubbleLayoutFromStoreWithKeybinding) {
  CreateExtension(BROWSER_ACTION, true);
  ExtensionInstalledBubbleController* controller = CreateController();
  ASSERT_TRUE(controller);

  // Since the extension has a keybinding, the "how to manage" section is
  // hidden. The other fields are present.
  EXPECT_FALSE([[controller howToUse] isHidden]);
  EXPECT_TRUE([[controller howToManage] isHidden]);
  EXPECT_FALSE([[controller manageShortcutLink] isHidden]);
  EXPECT_FALSE([[controller promoContainer] isHidden]);

  NSRect headingFrame = [[controller heading] frame];
  NSRect closeFrame = [[controller closeButton] frame];
  NSRect howToUseFrame = [[controller howToUse] frame];
  NSRect manageShortcutFrame = [[controller manageShortcutLink] frame];
  NSRect syncPromoFrame = [[controller promoContainer] frame];
  NSRect iconFrame = [[controller iconImage] frame];

  NSRect frames[] = {headingFrame, closeFrame, howToUseFrame,
                     manageShortcutFrame, syncPromoFrame, iconFrame};
  SanityCheckFrames(frames, arraysize(frames));

  // Layout should be:
  // |------| | Heading         |
  // | icon | | How to Use      |
  // |------| | Manage shortcut |
  // |--------------------------|
  // |       Sync Promo         |
  // |--------------------------|
  EXPECT_GT(NSMinY(headingFrame), NSMinY(howToUseFrame));
  EXPECT_GT(NSMinY(howToUseFrame), NSMinY(manageShortcutFrame));
  EXPECT_GT(NSMinY(manageShortcutFrame), NSMinY(syncPromoFrame));
  EXPECT_GT(NSMinY(iconFrame), NSMinY(syncPromoFrame));
  EXPECT_GT(NSMinY(iconFrame), 0);
  EXPECT_EQ(NSMinY(syncPromoFrame), 0);

  [controller close];
}

// Test the layout of a bubble for an unpacked extension (which is not
// syncable).
TEST_F(ExtensionInstalledBubbleControllerTest,
       BubbleLayoutBrowserActionUnpacked) {
  CreateExtension(BROWSER_ACTION, true, extensions::Manifest::UNPACKED);
  ExtensionInstalledBubbleController* controller = CreateController();
  ASSERT_TRUE(controller);

  // The extension has a keybinding (so the "how to manage" view is hidden) and
  // is an unpacked extension (so the "sign in promo" view is also hidden).
  EXPECT_FALSE([[controller howToUse] isHidden]);
  EXPECT_TRUE([[controller howToManage] isHidden]);
  EXPECT_TRUE([[controller promoContainer] isHidden]);
  EXPECT_FALSE([[controller manageShortcutLink] isHidden]);

  NSRect headingFrame = [[controller heading] frame];
  NSRect closeFrame = [[controller closeButton] frame];
  NSRect howToUseFrame = [[controller howToUse] frame];
  NSRect howToManageFrame = [[controller howToManage] frame];
  NSRect iconFrame = [[controller iconImage] frame];

  NSRect frames[] = {headingFrame, closeFrame, howToUseFrame, howToManageFrame,
                     iconFrame};
  SanityCheckFrames(frames, arraysize(frames));

  // Layout should be:
  // |------| | Heading         |
  // | icon | | How to Use      |
  // |------| | Manage shortcut |
  EXPECT_FALSE(NSIntersectsRect(howToUseFrame, howToManageFrame));
  EXPECT_GT(NSMinY(headingFrame), NSMinY(howToManageFrame));
  EXPECT_GT(NSMinY(howToUseFrame), NSMinY(howToManageFrame));
  EXPECT_GT(NSMinY(iconFrame), 0);
}

TEST_F(ExtensionInstalledBubbleControllerTest, ParentClose) {
  CreateExtension(BROWSER_ACTION, false);
  ExtensionInstalledBubbleController* controller = CreateController();
  EXPECT_TRUE(controller);

  NSWindow* bubbleWindow = [controller window];

  // Observe whether the bubble window closes.
  NSString* notification = NSWindowWillCloseNotification;
  id observer = [OCMockObject observerMock];
  [[observer expect] notificationWithName:notification object:bubbleWindow];
  [[NSNotificationCenter defaultCenter]
    addMockObserver:observer name:notification object:bubbleWindow];

  // The bubble window goes from visible to not-visible.
  EXPECT_TRUE([bubbleWindow isVisible]);
  [window() close];
  EXPECT_FALSE([bubbleWindow isVisible]);

  [[NSNotificationCenter defaultCenter] removeObserver:observer];

  // Check that the appropriate notification was received.
  EXPECT_OCMOCK_VERIFY(observer);
}

TEST_F(ExtensionInstalledBubbleControllerTest, AppTest) {
  CreateExtension(APP, false);
  ExtensionInstalledBubbleController* controller = CreateController();
  EXPECT_TRUE(controller);

  int height = NSHeight([[controller window] frame]);

  // Make sure there is always enough room for the icon and margin.
  int minHeight = extension_installed_bubble::kIconSize +
    (2 * extension_installed_bubble::kOuterVerticalMargin);
  EXPECT_GT(height, minHeight);

  // Make sure the "show me" link is visible.
  EXPECT_FALSE([[controller appInstalledShortcutLink] isHidden]);

  [controller close];
}
