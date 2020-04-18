// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/renderer_context_menu/render_view_context_menu.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "base/strings/string16.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "base/threading/thread_restrictions.h"
#include "build/build_config.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/extensions/browsertest_util.h"
#include "chrome/browser/pdf/pdf_extension_test_util.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/renderer_context_menu/render_view_context_menu_browsertest_util.h"
#include "chrome/browser/renderer_context_menu/render_view_context_menu_test_util.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/extensions/app_launch_params.h"
#include "chrome/browser/ui/extensions/application_launch.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/chrome_render_frame.mojom.h"
#include "chrome/common/render_messages.h"
#include "chrome/common/web_application_info.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/search_test_utils.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_switches.h"
#include "components/guest_view/browser/guest_view_manager_delegate.h"
#include "components/guest_view/browser/test_guest_view_manager.h"
#include "components/search_engines/template_url_data.h"
#include "components/search_engines/template_url_service.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/browser_plugin_guest_manager.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "extensions/browser/api/extensions_api_client.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/guest_view/mime_handler_view/mime_handler_view_guest.h"
#include "extensions/browser/guest_view/mime_handler_view/test_mime_handler_view_guest.h"
#include "net/base/load_flags.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/platform/web_input_event.h"
#include "third_party/blink/public/web/web_context_menu_data.h"
#include "ui/base/emoji/emoji_panel_helper.h"
#include "ui/base/models/menu_model.h"
#include "ui/base/ui_base_features.h"

#if defined(OS_CHROMEOS)
#include "ash/public/cpp/window_properties.h"
#include "ash/public/interfaces/window_pin_type.mojom.h"
#include "ui/aura/window.h"
#endif

using content::WebContents;
using extensions::MimeHandlerViewGuest;
using extensions::TestMimeHandlerViewGuest;

namespace {

const char kAppUrl1[] = "https://www.google.com/";
const char kAppUrl2[] = "https://docs.google.com/";

class ContextMenuBrowserTest : public InProcessBrowserTest {
 public:
  ContextMenuBrowserTest() {}

 protected:
  std::unique_ptr<TestRenderViewContextMenu> CreateContextMenuMediaTypeNone(
      const GURL& unfiltered_url,
      const GURL& url) {
    return CreateContextMenu(unfiltered_url, url, base::string16(),
                             blink::WebContextMenuData::kMediaTypeNone,
                             ui::MENU_SOURCE_NONE);
  }

  std::unique_ptr<TestRenderViewContextMenu>
  CreateContextMenuMediaTypeNoneInWebContents(WebContents* web_contents,
                                              const GURL& unfiltered_url,
                                              const GURL& url) {
    return CreateContextMenuInWebContents(
        web_contents, unfiltered_url, url, base::string16(),
        blink::WebContextMenuData::kMediaTypeNone, ui::MENU_SOURCE_NONE);
  }

  std::unique_ptr<TestRenderViewContextMenu> CreateContextMenuMediaTypeImage(
      const GURL& url) {
    return CreateContextMenu(GURL(), url, base::string16(),
                             blink::WebContextMenuData::kMediaTypeImage,
                             ui::MENU_SOURCE_NONE);
  }

  std::unique_ptr<TestRenderViewContextMenu> CreateContextMenu(
      const GURL& unfiltered_url,
      const GURL& url,
      const base::string16& link_text,
      blink::WebContextMenuData::MediaType media_type,
      ui::MenuSourceType source_type) {
    return CreateContextMenuInWebContents(
        browser()->tab_strip_model()->GetActiveWebContents(), unfiltered_url,
        url, link_text, media_type, source_type);
  }

  std::unique_ptr<TestRenderViewContextMenu> CreateContextMenuInWebContents(
      WebContents* web_contents,
      const GURL& unfiltered_url,
      const GURL& url,
      const base::string16& link_text,
      blink::WebContextMenuData::MediaType media_type,
      ui::MenuSourceType source_type) {
    content::ContextMenuParams params;
    params.media_type = media_type;
    params.unfiltered_link_url = unfiltered_url;
    params.link_url = url;
    params.src_url = url;
    params.link_text = link_text;
    params.page_url = web_contents->GetController().GetActiveEntry()->GetURL();
    params.source_type = source_type;
#if defined(OS_MACOSX)
    params.writing_direction_default = 0;
    params.writing_direction_left_to_right = 0;
    params.writing_direction_right_to_left = 0;
#endif
    auto menu = std::make_unique<TestRenderViewContextMenu>(
        web_contents->GetMainFrame(), params);
    menu->Init();
    return menu;
  }

  // Does not work on ChromeOS.
  Profile* CreateSecondaryProfile(int profile_num) {
    base::ScopedAllowBlockingForTesting allow_blocking;
    ProfileManager* profile_manager = g_browser_process->profile_manager();
    base::FilePath profile_path = profile_manager->user_data_dir();
    profile_path = profile_path.AppendASCII(
        base::StringPrintf("New Profile %d", profile_num));
    return profile_manager->GetProfile(profile_path);
  }

  const extensions::Extension* InstallTestBookmarkApp(
      const GURL& app_url,
      bool open_as_window = true) {
    WebApplicationInfo web_app_info;
    web_app_info.app_url = app_url;
    web_app_info.scope = app_url;
    web_app_info.title = base::UTF8ToUTF16("Test app \xF0\x9F\x90\x90");
    web_app_info.description =
        base::UTF8ToUTF16("Test description \xF0\x9F\x90\x90");
    web_app_info.open_as_window = open_as_window;

    return extensions::browsertest_util::InstallBookmarkApp(
        browser()->profile(), web_app_info);
  }

  Browser* OpenTestBookmarkApp(const extensions::Extension* bookmark_app) {
    return extensions::browsertest_util::LaunchAppBrowser(browser()->profile(),
                                                          bookmark_app);
  }
};

class PdfPluginContextMenuBrowserTest : public InProcessBrowserTest {
 public:
  PdfPluginContextMenuBrowserTest() = default;
  ~PdfPluginContextMenuBrowserTest() override = default;

  void SetUpOnMainThread() override {
    guest_view::GuestViewManager::set_factory_for_testing(&factory_);
    test_guest_view_manager_ = static_cast<guest_view::TestGuestViewManager*>(
        guest_view::GuestViewManager::CreateWithDelegate(
            browser()->profile(),
            extensions::ExtensionsAPIClient::Get()
                ->CreateGuestViewManagerDelegate(browser()->profile())));
  }

 protected:
  guest_view::TestGuestViewManager* test_guest_view_manager() const {
    return test_guest_view_manager_;
  }

  // Helper function for testing context menu of a pdf plugin inside a web page.
  void TestContextMenuOfPdfInsideWebPage(
      const base::FilePath::CharType* file_name) {
    // Load a page with pdf file inside.
    GURL page_url = ui_test_utils::GetTestUrl(
        base::FilePath(FILE_PATH_LITERAL("pdf")), base::FilePath(file_name));
    ui_test_utils::NavigateToURL(browser(), page_url);

    WebContents* web_contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    // Prepare to load a pdf plugin inside.
    test_guest_view_manager_->RegisterTestGuestViewType<MimeHandlerViewGuest>(
        base::Bind(&TestMimeHandlerViewGuest::Create));
    ASSERT_TRUE(
        content::ExecuteScript(web_contents,
                               "var l = document.getElementById('link1');"
                               "l.click();"));

    // Wait for the guest contents of the PDF plugin is created.
    WebContents* guest_contents =
        test_guest_view_manager_->WaitForSingleGuestCreated();
    TestMimeHandlerViewGuest* guest = static_cast<TestMimeHandlerViewGuest*>(
        extensions::MimeHandlerViewGuest::FromWebContents(guest_contents));
    ASSERT_TRUE(guest);
    // Wait for the guest is attached to the embedder.
    guest->WaitForGuestAttached();
    ASSERT_NE(web_contents, guest_contents);
    // Get the pdf plugin's main frame.
    content::RenderFrameHost* frame = guest_contents->GetMainFrame();
    ASSERT_TRUE(frame);

    content::ContextMenuParams params;
    params.page_url = page_url;
    params.frame_url = frame->GetLastCommittedURL();
    params.media_type = blink::WebContextMenuData::kMediaTypePlugin;
    TestRenderViewContextMenu menu(frame, params);
    menu.Init();

    // The full page related items such as 'reload' should not be displayed.
    ASSERT_FALSE(menu.IsItemPresent(IDC_RELOAD));
  }

 private:
  guest_view::TestGuestViewManagerFactory factory_;
  guest_view::TestGuestViewManager* test_guest_view_manager_;

  DISALLOW_COPY_AND_ASSIGN(PdfPluginContextMenuBrowserTest);
};

IN_PROC_BROWSER_TEST_F(ContextMenuBrowserTest,
                       NonExtensionMenuItemsAlwaysVisible) {
  std::unique_ptr<TestRenderViewContextMenu> menu1 =
      CreateContextMenuMediaTypeNone(GURL("http://www.google.com/"),
                                     GURL("http://www.google.com/"));

  EXPECT_TRUE(menu1->IsCommandIdVisible(IDC_CONTENT_CONTEXT_OPENLINKNEWTAB));
  EXPECT_TRUE(menu1->IsCommandIdVisible(IDC_CONTENT_CONTEXT_OPENLINKNEWWINDOW));
  EXPECT_TRUE(menu1->IsCommandIdVisible(IDC_CONTENT_CONTEXT_COPYLINKLOCATION));
  EXPECT_TRUE(menu1->IsCommandIdVisible(IDC_CONTENT_CONTEXT_OPENLINKINPROFILE));

  std::unique_ptr<TestRenderViewContextMenu> menu2 =
      CreateContextMenuMediaTypeNone(GURL("chrome://history"), GURL());

  EXPECT_TRUE(menu2->IsCommandIdVisible(IDC_CONTENT_CONTEXT_OPENLINKNEWTAB));
  EXPECT_TRUE(menu2->IsCommandIdVisible(IDC_CONTENT_CONTEXT_OPENLINKNEWWINDOW));
  EXPECT_TRUE(menu2->IsCommandIdVisible(IDC_CONTENT_CONTEXT_COPYLINKLOCATION));
  EXPECT_TRUE(menu2->IsCommandIdVisible(IDC_CONTENT_CONTEXT_OPENLINKINPROFILE));

  std::unique_ptr<TestRenderViewContextMenu> menu3 = CreateContextMenu(
      GURL("http://www.google.com/"), GURL("http://www.google.com/"),
      base::ASCIIToUTF16(""), blink::WebContextMenuData::kMediaTypeNone,
      ui::MENU_SOURCE_TOUCH);

  EXPECT_TRUE(menu3->IsCommandIdVisible(IDC_CONTENT_CONTEXT_COPYLINKTEXT));
}

#if defined(OS_CHROMEOS)
IN_PROC_BROWSER_TEST_F(ContextMenuBrowserTest,
                       ContextMenuEntriesAreDisabledInLockedFullscreen) {
  int entries_to_test[] = {
      IDC_VIEW_SOURCE, IDC_CONTENT_CONTEXT_OPENLINKNEWTAB,
      IDC_CONTENT_CONTEXT_INSPECTELEMENT,
  };
  std::unique_ptr<TestRenderViewContextMenu> menu =
      CreateContextMenuMediaTypeNone(GURL("http://www.google.com/"),
                                     GURL("http://www.google.com/"));

  // Entries are enabled.
  for (auto entry : entries_to_test)
    EXPECT_TRUE(menu->IsCommandIdEnabled(entry));

  // Set locked fullscreen state.
  browser()->window()->GetNativeWindow()->SetProperty(
      ash::kWindowPinTypeKey, ash::mojom::WindowPinType::TRUSTED_PINNED);

  // All entries are disabled in locked fullscreen (testing only a subset here).
  for (auto entry : entries_to_test)
    EXPECT_FALSE(menu->IsCommandIdEnabled(entry));
}
#endif  // defined(OS_CHROMEOS)

IN_PROC_BROWSER_TEST_F(ContextMenuBrowserTest, OpenEntryPresentForNormalURLs) {
  std::unique_ptr<TestRenderViewContextMenu> menu =
      CreateContextMenuMediaTypeNone(GURL("http://www.google.com/"),
                                     GURL("http://www.google.com/"));

  ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKNEWTAB));
  ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKNEWWINDOW));
  ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_COPYLINKLOCATION));
  ASSERT_FALSE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKBOOKMARKAPP));
  ASSERT_FALSE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKINPROFILE));
  ASSERT_FALSE(menu->IsItemInRangePresent(IDC_OPEN_LINK_IN_PROFILE_FIRST,
                                          IDC_OPEN_LINK_IN_PROFILE_LAST));
}

IN_PROC_BROWSER_TEST_F(ContextMenuBrowserTest,
                       OpenInAppAbsentForURLsInScopeWhenDesktopPWAsDisabled) {
  {
    base::test::ScopedFeatureList feature_list;
    feature_list.InitAndEnableFeature(features::kDesktopPWAWindowing);
    InstallTestBookmarkApp(GURL(kAppUrl1));
  }
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(features::kDesktopPWAWindowing);

  std::unique_ptr<TestRenderViewContextMenu> menu =
      CreateContextMenuMediaTypeNone(GURL(kAppUrl1), GURL(kAppUrl1));

  ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKNEWTAB));
  ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKNEWWINDOW));
  ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_COPYLINKLOCATION));
  ASSERT_FALSE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKBOOKMARKAPP));
  ASSERT_FALSE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKINPROFILE));
  ASSERT_FALSE(menu->IsItemInRangePresent(IDC_OPEN_LINK_IN_PROFILE_FIRST,
                                          IDC_OPEN_LINK_IN_PROFILE_LAST));
}

IN_PROC_BROWSER_TEST_F(ContextMenuBrowserTest,
                       OpenInAppPresentForURLsInScopeOfBookmarkApp) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(features::kDesktopPWAWindowing);
  InstallTestBookmarkApp(GURL(kAppUrl1));

  std::unique_ptr<TestRenderViewContextMenu> menu =
      CreateContextMenuMediaTypeNone(GURL(kAppUrl1), GURL(kAppUrl1));

  ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKNEWTAB));
  ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKNEWWINDOW));
  ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKBOOKMARKAPP));
  ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_COPYLINKLOCATION));
  ASSERT_FALSE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKINPROFILE));
  ASSERT_FALSE(menu->IsItemInRangePresent(IDC_OPEN_LINK_IN_PROFILE_FIRST,
                                          IDC_OPEN_LINK_IN_PROFILE_LAST));
}
IN_PROC_BROWSER_TEST_F(ContextMenuBrowserTest,
                       OpenInAppPresentForURLsInScopeOfNonWindowedBookmarkApp) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(features::kDesktopPWAWindowing);
  InstallTestBookmarkApp(GURL(kAppUrl1), false);

  std::unique_ptr<TestRenderViewContextMenu> menu =
      CreateContextMenuMediaTypeNone(GURL(kAppUrl1), GURL(kAppUrl1));

  ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKNEWTAB));
  ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKNEWWINDOW));
  ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKBOOKMARKAPP));
  ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_COPYLINKLOCATION));
  ASSERT_FALSE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKINPROFILE));
  ASSERT_FALSE(menu->IsItemInRangePresent(IDC_OPEN_LINK_IN_PROFILE_FIRST,
                                          IDC_OPEN_LINK_IN_PROFILE_LAST));
}

IN_PROC_BROWSER_TEST_F(ContextMenuBrowserTest,
                       OpenEntryInAppAbsentForURLsOutOfScopeOfBookmarkApp) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(features::kDesktopPWAWindowing);
  InstallTestBookmarkApp(GURL(kAppUrl1));

  std::unique_ptr<TestRenderViewContextMenu> menu =
      CreateContextMenuMediaTypeNone(GURL("http://www.example.com/"),
                                     GURL("http://www.example.com/"));

  ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKNEWTAB));
  ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKNEWWINDOW));
  ASSERT_FALSE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKBOOKMARKAPP));
  ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_COPYLINKLOCATION));
  ASSERT_FALSE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKINPROFILE));
  ASSERT_FALSE(menu->IsItemInRangePresent(IDC_OPEN_LINK_IN_PROFILE_FIRST,
                                          IDC_OPEN_LINK_IN_PROFILE_LAST));
}

IN_PROC_BROWSER_TEST_F(ContextMenuBrowserTest,
                       InAppOpenEntryPresentForRegularURLs) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(features::kDesktopPWAWindowing);
  const extensions::Extension* bookmark_app =
      InstallTestBookmarkApp(GURL(kAppUrl1));
  Browser* app_window = OpenTestBookmarkApp(bookmark_app);

  std::unique_ptr<TestRenderViewContextMenu> menu =
      CreateContextMenuMediaTypeNoneInWebContents(
          app_window->tab_strip_model()->GetActiveWebContents(),
          GURL("http://www.example.com"), GURL("http://www.example.com"));

  ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKNEWTAB));
  ASSERT_FALSE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKNEWWINDOW));
  ASSERT_FALSE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKBOOKMARKAPP));
  ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_COPYLINKLOCATION));
  ASSERT_FALSE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKINPROFILE));
  ASSERT_FALSE(menu->IsItemInRangePresent(IDC_OPEN_LINK_IN_PROFILE_FIRST,
                                          IDC_OPEN_LINK_IN_PROFILE_LAST));
}

IN_PROC_BROWSER_TEST_F(ContextMenuBrowserTest,
                       InAppOpenEntryPresentForSameAppURLs) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(features::kDesktopPWAWindowing);
  const extensions::Extension* bookmark_app =
      InstallTestBookmarkApp(GURL(kAppUrl1));
  Browser* app_window = OpenTestBookmarkApp(bookmark_app);

  std::unique_ptr<TestRenderViewContextMenu> menu =
      CreateContextMenuMediaTypeNoneInWebContents(
          app_window->tab_strip_model()->GetActiveWebContents(), GURL(kAppUrl1),
          GURL(kAppUrl1));

  ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKNEWTAB));
  ASSERT_FALSE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKNEWWINDOW));
  ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKBOOKMARKAPP));
  ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_COPYLINKLOCATION));
  ASSERT_FALSE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKINPROFILE));
  ASSERT_FALSE(menu->IsItemInRangePresent(IDC_OPEN_LINK_IN_PROFILE_FIRST,
                                          IDC_OPEN_LINK_IN_PROFILE_LAST));
}

IN_PROC_BROWSER_TEST_F(ContextMenuBrowserTest,
                       InAppOpenEntryPresentForOtherAppURLs) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(features::kDesktopPWAWindowing);
  const extensions::Extension* bookmark_app =
      InstallTestBookmarkApp(GURL(kAppUrl1));
  InstallTestBookmarkApp(GURL(kAppUrl2));

  Browser* app_window = OpenTestBookmarkApp(bookmark_app);

  std::unique_ptr<TestRenderViewContextMenu> menu =
      CreateContextMenuMediaTypeNoneInWebContents(
          app_window->tab_strip_model()->GetActiveWebContents(), GURL(kAppUrl2),
          GURL(kAppUrl2));

  ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKNEWTAB));
  ASSERT_FALSE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKNEWWINDOW));
  ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKBOOKMARKAPP));
  ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_COPYLINKLOCATION));
  ASSERT_FALSE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKINPROFILE));
  ASSERT_FALSE(menu->IsItemInRangePresent(IDC_OPEN_LINK_IN_PROFILE_FIRST,
                                          IDC_OPEN_LINK_IN_PROFILE_LAST));
}

IN_PROC_BROWSER_TEST_F(ContextMenuBrowserTest, OpenEntryAbsentForFilteredURLs) {
  std::unique_ptr<TestRenderViewContextMenu> menu =
      CreateContextMenuMediaTypeNone(GURL("chrome://history"), GURL());

  ASSERT_FALSE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKNEWTAB));
  ASSERT_FALSE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKNEWWINDOW));
  ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_COPYLINKLOCATION));
  ASSERT_FALSE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKINPROFILE));
  ASSERT_FALSE(menu->IsItemInRangePresent(IDC_OPEN_LINK_IN_PROFILE_FIRST,
                                          IDC_OPEN_LINK_IN_PROFILE_LAST));
}

IN_PROC_BROWSER_TEST_F(ContextMenuBrowserTest, ContextMenuForCanvas) {
  content::ContextMenuParams params;
  params.media_type = blink::WebContextMenuData::kMediaTypeCanvas;

  TestRenderViewContextMenu menu(
      browser()->tab_strip_model()->GetActiveWebContents()->GetMainFrame(),
      params);
  menu.Init();

  ASSERT_TRUE(menu.IsItemPresent(IDC_CONTENT_CONTEXT_SAVEIMAGEAS));
  ASSERT_TRUE(menu.IsItemPresent(IDC_CONTENT_CONTEXT_COPYIMAGE));
}

IN_PROC_BROWSER_TEST_F(ContextMenuBrowserTest,
                       ContextMenuForEmojiPanel_Enabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(features::kEnableEmojiContextMenu);

  content::ContextMenuParams params;
  params.is_editable = true;

  TestRenderViewContextMenu menu(
      browser()->tab_strip_model()->GetActiveWebContents()->GetMainFrame(),
      params);
  menu.Init();

  EXPECT_EQ(ui::IsEmojiPanelSupported(),
            menu.IsItemPresent(IDC_CONTENT_CONTEXT_EMOJI));
}

IN_PROC_BROWSER_TEST_F(ContextMenuBrowserTest,
                       ContextMenuForEmojiPanel_Enabled_NonEditable) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(features::kEnableEmojiContextMenu);

  content::ContextMenuParams params;
  params.is_editable = false;

  TestRenderViewContextMenu menu(
      browser()->tab_strip_model()->GetActiveWebContents()->GetMainFrame(),
      params);
  menu.Init();

  // Emoji context menu item should never be present on a non-editable field.
  EXPECT_FALSE(menu.IsItemPresent(IDC_CONTENT_CONTEXT_EMOJI));
}

IN_PROC_BROWSER_TEST_F(ContextMenuBrowserTest,
                       ContextMenuForEmojiPanel_Disabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(features::kEnableEmojiContextMenu);

  content::ContextMenuParams params;
  params.is_editable = true;

  TestRenderViewContextMenu menu(
      browser()->tab_strip_model()->GetActiveWebContents()->GetMainFrame(),
      params);
  menu.Init();

  // If the feature is disabled, the emoji context menu should never be present.
  EXPECT_FALSE(menu.IsItemPresent(IDC_CONTENT_CONTEXT_EMOJI));
}

IN_PROC_BROWSER_TEST_F(ContextMenuBrowserTest, CopyLinkTextMouse) {
  std::unique_ptr<TestRenderViewContextMenu> menu = CreateContextMenu(
      GURL("http://www.google.com/"), GURL("http://www.google.com/"),
      base::ASCIIToUTF16("Google"), blink::WebContextMenuData::kMediaTypeNone,
      ui::MENU_SOURCE_MOUSE);

  ASSERT_FALSE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_COPYLINKTEXT));
}

IN_PROC_BROWSER_TEST_F(ContextMenuBrowserTest, CopyLinkTextTouchNoText) {
  std::unique_ptr<TestRenderViewContextMenu> menu = CreateContextMenu(
      GURL("http://www.google.com/"), GURL("http://www.google.com/"),
      base::ASCIIToUTF16(""), blink::WebContextMenuData::kMediaTypeNone,
      ui::MENU_SOURCE_TOUCH);

  ASSERT_FALSE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_COPYLINKTEXT));
}

IN_PROC_BROWSER_TEST_F(ContextMenuBrowserTest, CopyLinkTextTouchTextOnly) {
  std::unique_ptr<TestRenderViewContextMenu> menu = CreateContextMenu(
      GURL("http://www.google.com/"), GURL("http://www.google.com/"),
      base::ASCIIToUTF16("Google"), blink::WebContextMenuData::kMediaTypeNone,
      ui::MENU_SOURCE_TOUCH);

  ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_COPYLINKTEXT));
}

IN_PROC_BROWSER_TEST_F(ContextMenuBrowserTest, CopyLinkTextTouchTextImage) {
  std::unique_ptr<TestRenderViewContextMenu> menu = CreateContextMenu(
      GURL("http://www.google.com/"), GURL("http://www.google.com/"),
      base::ASCIIToUTF16("Google"), blink::WebContextMenuData::kMediaTypeImage,
      ui::MENU_SOURCE_TOUCH);

  ASSERT_FALSE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_COPYLINKTEXT));
}

// Opens a link in a new tab via a "real" context menu.
IN_PROC_BROWSER_TEST_F(ContextMenuBrowserTest, RealMenu) {
  ContextMenuNotificationObserver menu_observer(
      IDC_CONTENT_CONTEXT_OPENLINKNEWTAB);
  ui_test_utils::WindowedTabAddedNotificationObserver tab_observer(
      content::NotificationService::AllSources());

  // Go to a page with a link
  ui_test_utils::NavigateToURL(
      browser(), GURL("data:text/html,<a href='about:blank'>link</a>"));

  // Open a context menu.
  blink::WebMouseEvent mouse_event(
      blink::WebInputEvent::kMouseDown, blink::WebInputEvent::kNoModifiers,
      blink::WebInputEvent::GetStaticTimeStampForTests());
  mouse_event.button = blink::WebMouseEvent::Button::kRight;
  mouse_event.SetPositionInWidget(15, 15);
  content::WebContents* tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  gfx::Rect offset = tab->GetContainerBounds();
  mouse_event.SetPositionInScreen(15 + offset.x(), 15 + offset.y());
  mouse_event.click_count = 1;
  tab->GetRenderViewHost()->GetWidget()->ForwardMouseEvent(mouse_event);
  mouse_event.SetType(blink::WebInputEvent::kMouseUp);
  tab->GetRenderViewHost()->GetWidget()->ForwardMouseEvent(mouse_event);

  // The menu_observer will select "Open in new tab", wait for the new tab to
  // be added.
  tab_observer.Wait();
  tab = tab_observer.GetTab();
  content::WaitForLoadStop(tab);

  // Verify that it's the correct tab.
  EXPECT_EQ(GURL("about:blank"), tab->GetURL());
}

// Verify that "Open Link in New Tab" doesn't send URL fragment as referrer.
IN_PROC_BROWSER_TEST_F(ContextMenuBrowserTest, OpenInNewTabReferrer) {
  ui_test_utils::WindowedTabAddedNotificationObserver tab_observer(
      content::NotificationService::AllSources());

  ASSERT_TRUE(embedded_test_server()->Start());
  GURL echoheader(embedded_test_server()->GetURL("/echoheader?Referer"));

  // Go to a |page| with a link to echoheader URL.
  GURL page("data:text/html,<a href='" + echoheader.spec() + "'>link</a>");
  ui_test_utils::NavigateToURL(browser(), page);

  // Set up referrer URL with fragment.
  const GURL kReferrerWithFragment("http://foo.com/test#fragment");
  const std::string kCorrectReferrer("http://foo.com/test");

  // Set up menu with link URL.
  content::ContextMenuParams context_menu_params;
  context_menu_params.page_url = kReferrerWithFragment;
  context_menu_params.link_url = echoheader;

  // Select "Open Link in New Tab" and wait for the new tab to be added.
  TestRenderViewContextMenu menu(
      browser()->tab_strip_model()->GetActiveWebContents()->GetMainFrame(),
      context_menu_params);
  menu.Init();
  menu.ExecuteCommand(IDC_CONTENT_CONTEXT_OPENLINKNEWTAB, 0);

  tab_observer.Wait();
  content::WebContents* tab = tab_observer.GetTab();
  content::WaitForLoadStop(tab);

  // Verify that it's the correct tab.
  ASSERT_EQ(echoheader, tab->GetURL());
  // Verify that the text on the page matches |kCorrectReferrer|.
  std::string actual_referrer;
  ASSERT_TRUE(content::ExecuteScriptAndExtractString(
      tab,
      "window.domAutomationController.send(window.document.body.textContent);",
      &actual_referrer));
  ASSERT_EQ(kCorrectReferrer, actual_referrer);

  // Verify that the referrer on the page matches |kCorrectReferrer|.
  std::string page_referrer;
  ASSERT_TRUE(content::ExecuteScriptAndExtractString(
      tab, "window.domAutomationController.send(window.document.referrer);",
      &page_referrer));
  ASSERT_EQ(kCorrectReferrer, page_referrer);
}

// Verify that "Open Link in Incognito Window " doesn't send referrer URL.
IN_PROC_BROWSER_TEST_F(ContextMenuBrowserTest, OpenIncognitoNoneReferrer) {
  ui_test_utils::WindowedTabAddedNotificationObserver tab_observer(
      content::NotificationService::AllSources());

  ASSERT_TRUE(embedded_test_server()->Start());
  GURL echoheader(embedded_test_server()->GetURL("/echoheader?Referer"));

  // Go to a |page| with a link to echoheader URL.
  GURL page("data:text/html,<a href='" + echoheader.spec() + "'>link</a>");
  ui_test_utils::NavigateToURL(browser(), page);

  // Set up referrer URL with fragment.
  const GURL kReferrerWithFragment("http://foo.com/test#fragment");
  const std::string kNoneReferrer("None");
  const std::string kEmptyReferrer("");

  // Set up menu with link URL.
  content::ContextMenuParams context_menu_params;
  context_menu_params.page_url = kReferrerWithFragment;
  context_menu_params.link_url = echoheader;

  // Select "Open Link in Incognito Window" and wait for window to be added.
  TestRenderViewContextMenu menu(
      browser()->tab_strip_model()->GetActiveWebContents()->GetMainFrame(),
      context_menu_params);
  menu.Init();
  menu.ExecuteCommand(IDC_CONTENT_CONTEXT_OPENLINKOFFTHERECORD, 0);

  tab_observer.Wait();
  content::WebContents* tab = tab_observer.GetTab();
  content::WaitForLoadStop(tab);

  // Verify that it's the correct tab.
  ASSERT_EQ(echoheader, tab->GetURL());
  // Verify that the text on the page matches |kNoneReferrer|.
  std::string actual_referrer;
  ASSERT_TRUE(content::ExecuteScriptAndExtractString(
      tab,
      "window.domAutomationController.send(window.document.body.textContent);",
      &actual_referrer));
  ASSERT_EQ(kNoneReferrer, actual_referrer);

  // Verify that the referrer on the page matches |kEmptyReferrer|.
  std::string page_referrer;
  ASSERT_TRUE(content::ExecuteScriptAndExtractString(
      tab, "window.domAutomationController.send(window.document.referrer);",
      &page_referrer));
  ASSERT_EQ(kEmptyReferrer, page_referrer);
}

// Verify that "Open link in [App Name]" opens a new App window.
IN_PROC_BROWSER_TEST_F(ContextMenuBrowserTest, OpenLinkInBookmarkApp) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(features::kDesktopPWAWindowing);
  InstallTestBookmarkApp(GURL(kAppUrl1));

  ASSERT_TRUE(embedded_test_server()->Start());

  size_t num_browsers = chrome::GetBrowserCount(browser()->profile());
  int num_tabs = browser()->tab_strip_model()->count();
  content::WebContents* initial_tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  const GURL initial_url = initial_tab->GetLastCommittedURL();

  const GURL app_url(kAppUrl1);
  ui_test_utils::UrlLoadObserver url_observer(
      app_url, content::NotificationService::AllSources());
  content::ContextMenuParams params;
  params.page_url = GURL("https://www.example.com/");
  params.link_url = app_url;
  TestRenderViewContextMenu menu(initial_tab->GetMainFrame(), params);
  menu.Init();
  menu.ExecuteCommand(IDC_CONTENT_CONTEXT_OPENLINKBOOKMARKAPP,
                      0 /* event_flags */);
  url_observer.Wait();

  EXPECT_EQ(num_tabs, browser()->tab_strip_model()->count());
  EXPECT_EQ(++num_browsers, chrome::GetBrowserCount(browser()->profile()));
  Browser* app_browser = chrome::FindLastActive();
  EXPECT_NE(browser(), app_browser);
  EXPECT_EQ(initial_url, initial_tab->GetLastCommittedURL());
  EXPECT_EQ(app_url, app_browser->tab_strip_model()
                         ->GetActiveWebContents()
                         ->GetLastCommittedURL());
}

// Check filename on clicking "Save Link As" via a "real" context menu.
IN_PROC_BROWSER_TEST_F(ContextMenuBrowserTest, SuggestedFileName) {
  // Register observer.
  ContextMenuWaiter menu_observer;

  ASSERT_TRUE(embedded_test_server()->Start());
  GURL url(embedded_test_server()->GetURL("/download-anchor-same-origin.html"));

  // Go to a page with a link having download attribute.
  const std::string kSuggestedFilename("test_filename.png");
  ui_test_utils::NavigateToURL(browser(), url);

  // Open a context menu.
  blink::WebMouseEvent mouse_event(
      blink::WebInputEvent::kMouseDown, blink::WebInputEvent::kNoModifiers,
      blink::WebInputEvent::GetStaticTimeStampForTests());
  mouse_event.button = blink::WebMouseEvent::Button::kRight;
  mouse_event.SetPositionInWidget(15, 15);
  content::WebContents* tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  tab->GetRenderViewHost()->GetWidget()->ForwardMouseEvent(mouse_event);
  mouse_event.SetType(blink::WebInputEvent::kMouseUp);
  tab->GetRenderViewHost()->GetWidget()->ForwardMouseEvent(mouse_event);

  // Wait for context menu to be visible.
  menu_observer.WaitForMenuOpenAndClose();

  // Compare filename.
  base::string16 suggested_filename = menu_observer.params().suggested_filename;
  ASSERT_EQ(kSuggestedFilename, base::UTF16ToUTF8(suggested_filename).c_str());
}

// Check filename on clicking "Save Link As" is ignored for cross origin.
IN_PROC_BROWSER_TEST_F(ContextMenuBrowserTest, SuggestedFileNameCrossOrigin) {
  // Register observer.
  ContextMenuWaiter menu_observer;

  ASSERT_TRUE(embedded_test_server()->Start());
  GURL url(
      embedded_test_server()->GetURL("/download-anchor-cross-origin.html"));

  // Go to a page with a link having download attribute.
  ui_test_utils::NavigateToURL(browser(), url);

  // Open a context menu.
  blink::WebMouseEvent mouse_event(
      blink::WebInputEvent::kMouseDown, blink::WebInputEvent::kNoModifiers,
      blink::WebInputEvent::GetStaticTimeStampForTests());
  mouse_event.button = blink::WebMouseEvent::Button::kRight;
  mouse_event.SetPositionInWidget(15, 15);
  content::WebContents* tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  tab->GetRenderViewHost()->GetWidget()->ForwardMouseEvent(mouse_event);
  mouse_event.SetType(blink::WebInputEvent::kMouseUp);
  tab->GetRenderViewHost()->GetWidget()->ForwardMouseEvent(mouse_event);

  // Wait for context menu to be visible.
  menu_observer.WaitForMenuOpenAndClose();

  // Compare filename.
  base::string16 suggested_filename = menu_observer.params().suggested_filename;
  ASSERT_TRUE(suggested_filename.empty());
}

IN_PROC_BROWSER_TEST_F(ContextMenuBrowserTest, DataSaverOpenOrigImageInNewTab) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  command_line->AppendSwitch(
      data_reduction_proxy::switches::kEnableDataReductionProxy);

  std::unique_ptr<TestRenderViewContextMenu> menu =
      CreateContextMenuMediaTypeImage(GURL("http://url.com/image.png"));

  ASSERT_FALSE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENIMAGENEWTAB));
  ASSERT_TRUE(
      menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPEN_ORIGINAL_IMAGE_NEW_TAB));
}

IN_PROC_BROWSER_TEST_F(ContextMenuBrowserTest,
                       DataSaverHttpsOpenImageInNewTab) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  command_line->AppendSwitch(
      data_reduction_proxy::switches::kEnableDataReductionProxy);

  std::unique_ptr<TestRenderViewContextMenu> menu =
      CreateContextMenuMediaTypeImage(GURL("https://url.com/image.png"));

  ASSERT_FALSE(
      menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPEN_ORIGINAL_IMAGE_NEW_TAB));
  ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENIMAGENEWTAB));
}

IN_PROC_BROWSER_TEST_F(ContextMenuBrowserTest, OpenImageInNewTab) {
  std::unique_ptr<TestRenderViewContextMenu> menu =
      CreateContextMenuMediaTypeImage(GURL("http://url.com/image.png"));
  ASSERT_FALSE(
      menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPEN_ORIGINAL_IMAGE_NEW_TAB));
  ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENIMAGENEWTAB));
}

// Functionality is not present on ChromeOS.
#if !defined(OS_CHROMEOS)
IN_PROC_BROWSER_TEST_F(ContextMenuBrowserTest, OpenLinkInProfileEntryPresent) {
  {
    std::unique_ptr<TestRenderViewContextMenu> menu(
        CreateContextMenuMediaTypeNone(GURL("http://www.google.com/"),
                                       GURL("http://www.google.com/")));

    ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKNEWTAB));
    ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKNEWWINDOW));
    ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_COPYLINKLOCATION));
    // With only one profile exists, we don't add any items to the context menu
    // for opening links in other profiles.
    ASSERT_FALSE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKINPROFILE));
    ASSERT_FALSE(menu->IsItemInRangePresent(IDC_OPEN_LINK_IN_PROFILE_FIRST,
                                            IDC_OPEN_LINK_IN_PROFILE_LAST));
  }

  // Create one additional profile, but do not yet open windows in it.
  Profile* profile = CreateSecondaryProfile(1);

  {
    std::unique_ptr<TestRenderViewContextMenu> menu(
        CreateContextMenuMediaTypeNone(GURL("http://www.google.com/"),
                                       GURL("http://www.google.com/")));

    ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKNEWTAB));
    ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKNEWWINDOW));
    ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_COPYLINKLOCATION));
    // With the second profile not open, no entry is created.
    ASSERT_FALSE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKINPROFILE));
    ASSERT_FALSE(menu->IsItemPresent(IDC_OPEN_LINK_IN_PROFILE_FIRST));
  }

  profiles::FindOrCreateNewWindowForProfile(
      profile, chrome::startup::IS_NOT_PROCESS_STARTUP,
      chrome::startup::IS_NOT_FIRST_RUN, false);

  {
    std::unique_ptr<TestRenderViewContextMenu> menu(
        CreateContextMenuMediaTypeNone(GURL("http://www.google.com/"),
                                       GURL("http://www.google.com/")));

    ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKNEWTAB));
    ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKNEWWINDOW));
    ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_COPYLINKLOCATION));
    // With the second profile open, an inline menu entry is created.
    ASSERT_FALSE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKINPROFILE));
    ASSERT_TRUE(menu->IsItemPresent(IDC_OPEN_LINK_IN_PROFILE_FIRST));
  }

  CreateSecondaryProfile(2);

  {
    std::unique_ptr<TestRenderViewContextMenu> menu(
        CreateContextMenuMediaTypeNone(GURL("http://www.google.com/"),
                                       GURL("http://www.google.com/")));

    ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKNEWTAB));
    ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKNEWWINDOW));
    ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_COPYLINKLOCATION));
    // As soon as at least three profiles exist, we show all profiles in a
    // submenu.
    ui::MenuModel* model = NULL;
    int index = -1;
    ASSERT_TRUE(menu->GetMenuModelAndItemIndex(IDC_OPEN_LINK_IN_PROFILE_FIRST,
                                               &model, &index));
    ASSERT_EQ(2, model->GetItemCount());
    ASSERT_FALSE(menu->IsItemInRangePresent(IDC_OPEN_LINK_IN_PROFILE_FIRST,
                                            IDC_OPEN_LINK_IN_PROFILE_LAST));
  }
}

IN_PROC_BROWSER_TEST_F(ContextMenuBrowserTest, OpenLinkInProfile) {
  // Create |num_profiles| extra profiles for testing.
  const int num_profiles = 8;
  // The following are the profile numbers that are omitted and need signin.
  // These profiles are not added to the menu. Omitted profiles refers to
  // supervised profiles in the process of creation.
  std::vector<int> profiles_omit;
  profiles_omit.push_back(4);

  std::vector<int> profiles_signin_required;
  profiles_signin_required.push_back(1);
  profiles_signin_required.push_back(3);
  profiles_signin_required.push_back(6);

  // Create the profiles.
  ProfileAttributesStorage& storage =
      g_browser_process->profile_manager()->GetProfileAttributesStorage();
  std::vector<Profile*> profiles_in_menu;
  for (int i = 0; i < num_profiles; ++i) {
    Profile* profile = CreateSecondaryProfile(i);
    ProfileAttributesEntry* entry;
    ASSERT_TRUE(
        storage.GetProfileAttributesWithPath(profile->GetPath(), &entry));
    // Open a browser window for the profile if and only if the profile is not
    // omitted nor needing signin.
    if (std::binary_search(profiles_omit.begin(), profiles_omit.end(), i)) {
      entry->SetIsOmitted(true);
    } else if (std::binary_search(profiles_signin_required.begin(),
                                  profiles_signin_required.end(), i)) {
      entry->SetIsSigninRequired(true);
    } else {
      profiles::FindOrCreateNewWindowForProfile(
          profile, chrome::startup::IS_NOT_PROCESS_STARTUP,
          chrome::startup::IS_NOT_FIRST_RUN, false);
      profiles_in_menu.push_back(profile);
    }
  }

  ui_test_utils::WindowedTabAddedNotificationObserver tab_observer(
      content::NotificationService::AllSources());

  ASSERT_TRUE(embedded_test_server()->Start());
  GURL url(embedded_test_server()->GetURL("/"));

  std::unique_ptr<TestRenderViewContextMenu> menu(
      CreateContextMenuMediaTypeNone(url, url));

  // Verify that the size of the menu is correct.
  ui::MenuModel* model = NULL;
  int index = -1;
  ASSERT_TRUE(menu->GetMenuModelAndItemIndex(IDC_OPEN_LINK_IN_PROFILE_FIRST,
                                             &model, &index));
  ASSERT_EQ(static_cast<int>(profiles_in_menu.size()), model->GetItemCount());

  // Open the menu items. They should match their corresponding profiles in
  // |profiles_in_menu|.
  for (Profile* profile : profiles_in_menu) {
    int command_id = menu->GetCommandIDByProfilePath(profile->GetPath());
    ASSERT_NE(-1, command_id);
    menu->ExecuteCommand(command_id, 0);

    tab_observer.Wait();
    content::WebContents* tab = tab_observer.GetTab();
    content::WaitForLoadStop(tab);

    // Verify that it's the correct tab and profile.
    EXPECT_EQ(url, tab->GetURL());
    EXPECT_EQ(profile, Profile::FromBrowserContext(tab->GetBrowserContext()));
  }
}
#endif  // !defined(OS_CHROMEOS)

// Maintains image search test state. In particular, note that |menu_observer_|
// must live until the right-click completes asynchronously.
class SearchByImageBrowserTest : public InProcessBrowserTest {
 protected:
  void SetupAndLoadImagePage(const std::string& image_path) {
    // The test server must start first, so that we know the port that the test
    // server is using.
    ASSERT_TRUE(embedded_test_server()->Start());
    SetupImageSearchEngine();

    // Go to a page with an image in it. The test server doesn't serve the image
    // with the right MIME type, so use a data URL to make a page containing it.
    GURL image_url(embedded_test_server()->GetURL(image_path));
    GURL page("data:text/html,<img src='" + image_url.spec() + "'>");
    ui_test_utils::NavigateToURL(browser(), page);
  }

  void AttemptImageSearch() {
    // |menu_observer_| will cause the search-by-image menu item to be clicked.
    menu_observer_ = std::make_unique<ContextMenuNotificationObserver>(
        IDC_CONTENT_CONTEXT_SEARCHWEBFORIMAGE);
    RightClickImage();
  }

  // Right-click where the image should be.
  void RightClickImage() {
    content::WebContents* tab =
        browser()->tab_strip_model()->GetActiveWebContents();
    content::SimulateMouseClickAt(tab, 0, blink::WebMouseEvent::Button::kRight,
                                  gfx::Point(15, 15));
  }

  GURL GetImageSearchURL() {
    static const char kImageSearchURL[] = "/imagesearch";
    return embedded_test_server()->GetURL(kImageSearchURL);
  }

 private:
  void SetupImageSearchEngine() {
    static const char kShortName[] = "test";
    static const char kSearchURL[] = "/search?q={searchTerms}";
    static const char kImageSearchPostParams[] =
        "thumb={google:imageThumbnail}";

    TemplateURLService* model =
        TemplateURLServiceFactory::GetForProfile(browser()->profile());
    ASSERT_TRUE(model);
    search_test_utils::WaitForTemplateURLServiceToLoad(model);
    ASSERT_TRUE(model->loaded());

    TemplateURLData data;
    data.SetShortName(base::ASCIIToUTF16(kShortName));
    data.SetKeyword(data.short_name());
    data.SetURL(embedded_test_server()->GetURL(kSearchURL).spec());
    data.image_url = GetImageSearchURL().spec();
    data.image_url_post_params = kImageSearchPostParams;

    TemplateURL* template_url = model->Add(std::make_unique<TemplateURL>(data));
    ASSERT_TRUE(template_url);
    model->SetUserSelectedDefaultSearchProvider(template_url);
  }

  void TearDownInProcessBrowserTestFixture() override {
    menu_observer_.reset();
  }

  std::unique_ptr<ContextMenuNotificationObserver> menu_observer_;
};

IN_PROC_BROWSER_TEST_F(SearchByImageBrowserTest, ImageSearchWithValidImage) {
  static const char kValidImage[] = "/image_search/valid.png";
  SetupAndLoadImagePage(kValidImage);

  ui_test_utils::WindowedTabAddedNotificationObserver tab_observer(
      content::NotificationService::AllSources());
  AttemptImageSearch();

  // The browser should open a new tab for an image search.
  tab_observer.Wait();
  content::WebContents* new_tab = tab_observer.GetTab();
  content::WaitForLoadStop(new_tab);
  EXPECT_EQ(GetImageSearchURL(), new_tab->GetURL());
}

IN_PROC_BROWSER_TEST_F(SearchByImageBrowserTest, ImageSearchWithCorruptImage) {
  static const char kCorruptImage[] = "/image_search/corrupt.png";
  SetupAndLoadImagePage(kCorruptImage);

  // Open and close a context menu.
  ContextMenuWaiter waiter;
  RightClickImage();
  waiter.WaitForMenuOpenAndClose();

  chrome::mojom::ChromeRenderFrameAssociatedPtr chrome_render_frame;
  browser()
      ->tab_strip_model()
      ->GetActiveWebContents()
      ->GetMainFrame()
      ->GetRemoteAssociatedInterfaces()
      ->GetInterface(&chrome_render_frame);

  auto callback = [](bool* response_received, const base::Closure& quit,
                     const std::vector<uint8_t>& thumbnail_data,
                     const gfx::Size& original_size) {
    *response_received = true;
    quit.Run();
  };

  base::RunLoop run_loop;
  bool response_received = false;
  chrome_render_frame->RequestThumbnailForContextNode(
      0, gfx::Size(2048, 2048), chrome::mojom::ImageFormat::JPEG,
      base::Bind(callback, &response_received, run_loop.QuitClosure()));
  run_loop.Run();

  // The browser should receive a response from the renderer, because the
  // renderer should not crash.
  ASSERT_TRUE(response_received);
}

IN_PROC_BROWSER_TEST_F(PdfPluginContextMenuBrowserTest,
                       FullPagePdfHasPageItems) {
  // Load a pdf page.
  GURL page_url =
      ui_test_utils::GetTestUrl(base::FilePath(FILE_PATH_LITERAL("pdf")),
                                base::FilePath(FILE_PATH_LITERAL("test.pdf")));
  ui_test_utils::NavigateToURL(browser(), page_url);

  WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  // Wait for the PDF plugin is loaded.
  pdf_extension_test_util::EnsurePDFHasLoaded(web_contents);
  content::BrowserPluginGuestManager* guest_manager =
      web_contents->GetBrowserContext()->GetGuestManager();
  WebContents* guest_contents = guest_manager->GetFullPageGuest(web_contents);
  ASSERT_TRUE(guest_contents);
  // Get the pdf plugin's main frame.
  content::RenderFrameHost* frame = guest_contents->GetMainFrame();
  ASSERT_TRUE(frame);
  ASSERT_NE(frame, web_contents->GetMainFrame());

  content::ContextMenuParams params;
  params.page_url = page_url;
  params.frame_url = frame->GetLastCommittedURL();
  params.media_type = blink::WebContextMenuData::kMediaTypePlugin;
  TestRenderViewContextMenu menu(frame, params);
  menu.Init();

  // The full page related items such as 'reload' should be there.
  ASSERT_TRUE(menu.IsItemPresent(IDC_RELOAD));
}

IN_PROC_BROWSER_TEST_F(PdfPluginContextMenuBrowserTest,
                       IframedPdfHasNoPageItems) {
  TestContextMenuOfPdfInsideWebPage(FILE_PATH_LITERAL("test-iframe-pdf.html"));
}

class LoadImageRequestObserver : public content::WebContentsObserver {
 public:
  LoadImageRequestObserver(content::WebContents* web_contents,
                           const std::string& path)
      : content::WebContentsObserver(web_contents), path_(path) {}

  void ResourceLoadComplete(
      content::RenderFrameHost* render_frame_host,
      const content::mojom::ResourceLoadInfo& resource_load_info) override {
    if (resource_load_info.url.path() == path_)
      run_loop_.Quit();
  }

  void WaitForRequest() { run_loop_.Run(); }

 private:
  std::string path_;
  base::RunLoop run_loop_;
};

class LoadImageBrowserTest : public InProcessBrowserTest {
 protected:
  void SetupAndLoadImagePage(const std::string& image_path) {
    ASSERT_TRUE(embedded_test_server()->Start());
    // Go to a page with an image in it. The test server doesn't serve the image
    // with the right MIME type, so use a data URL to make a page containing it.
    GURL image_url(embedded_test_server()->GetURL(image_path));
    GURL page(
        "data:text/html,<img width=50 height=50 "
        "src='" +
        image_url.spec() + "'>");
    ui_test_utils::NavigateToURL(browser(), page);
  }

  void AttemptLoadImage() {
    // Right-click where the image should be.
    // |menu_observer_| will cause the "Load image" menu item to be clicked.
    menu_observer_ = std::make_unique<ContextMenuNotificationObserver>(
        IDC_CONTENT_CONTEXT_LOAD_ORIGINAL_IMAGE);
    content::WebContents* tab =
        browser()->tab_strip_model()->GetActiveWebContents();
    content::SimulateMouseClickAt(tab, 0, blink::WebMouseEvent::Button::kRight,
                                  gfx::Point(15, 15));
  }

 private:
  std::unique_ptr<ContextMenuNotificationObserver> menu_observer_;
};

IN_PROC_BROWSER_TEST_F(LoadImageBrowserTest, LoadImage) {
  static const char kValidImage[] = "/load_image/image.png";
  SetupAndLoadImagePage(kValidImage);

  WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  LoadImageRequestObserver observer(web_contents, kValidImage);
  AttemptLoadImage();
  observer.WaitForRequest();
}

}  // namespace
