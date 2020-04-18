// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/stringprintf.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "extensions/common/value_builder.h"
#include "extensions/test/test_extension_dir.h"

namespace extensions {

namespace {

class ExtensionCSPBypassTest : public ExtensionBrowserTest {
 public:
  ExtensionCSPBypassTest() {}

  void SetUpOnMainThread() override {
    ExtensionBrowserTest::SetUpOnMainThread();
    ASSERT_TRUE(embedded_test_server()->Start());
  }

 protected:
  const Extension* AddExtension(bool is_component, bool all_urls_permission) {
    auto dir = std::make_unique<TestExtensionDir>();

    std::string unique_name = base::StringPrintf(
        "component=%d, all_urls=%d", is_component, all_urls_permission);
    DictionaryBuilder manifest;
    manifest.Set("name", unique_name)
        .Set("version", "1")
        .Set("manifest_version", 2)
        .Set("web_accessible_resources", ListBuilder().Append("*").Build());

    if (all_urls_permission) {
      manifest.Set("permissions", ListBuilder().Append("<all_urls>").Build());
    }
    if (is_component) {
      // LoadExtensionAsComponent requires the manifest to contain a key.
      std::string key;
      EXPECT_TRUE(Extension::ProducePEM(unique_name, &key));
      manifest.Set("key", key);
    }

    dir->WriteFile(FILE_PATH_LITERAL("script.js"), "");
    dir->WriteManifest(manifest.ToJSON());

    const Extension* extension = nullptr;
    if (is_component) {
      extension = LoadExtensionAsComponent(dir->UnpackedPath());
    } else {
      extension = LoadExtension(dir->UnpackedPath());
    }
    CHECK(extension);
    temp_dirs_.push_back(std::move(dir));
    return extension;
  }

  bool CanLoadScript(const Extension* extension) {
    content::RenderFrameHost* rfh =
        browser()->tab_strip_model()->GetActiveWebContents()->GetMainFrame();
    std::string code = base::StringPrintf(
        R"(
        var s = document.createElement('script');
        s.src = '%s';
        s.onload = function() {
          // Not blocked by CSP.
          window.domAutomationController.send(true);
        };
        s.onerror = function() {
          // Blocked by CSP.
          window.domAutomationController.send(false);
        };
        document.body.appendChild(s);)",
        extension->GetResourceURL("script.js").spec().c_str());
    bool script_loaded = false;
    EXPECT_TRUE(ExecuteScriptAndExtractBool(rfh, code, &script_loaded));
    return script_loaded;
  }

 private:
  std::vector<std::unique_ptr<TestExtensionDir>> temp_dirs_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionCSPBypassTest);
};

}  // namespace

IN_PROC_BROWSER_TEST_F(ExtensionCSPBypassTest, LoadWebAccessibleScript) {
  const Extension* component_ext_with_permission = AddExtension(true, true);
  const Extension* component_ext_without_permission = AddExtension(true, false);
  const Extension* ext_with_permission = AddExtension(false, true);
  const Extension* ext_without_permission = AddExtension(false, false);

  // chrome-extension:-URLs can always bypass CSP in normal pages.
  GURL non_webui_url(embedded_test_server()->GetURL("/empty.html"));
  ui_test_utils::NavigateToURL(browser(), non_webui_url);

  EXPECT_TRUE(CanLoadScript(component_ext_with_permission));
  EXPECT_TRUE(CanLoadScript(component_ext_without_permission));
  EXPECT_TRUE(CanLoadScript(ext_with_permission));
  EXPECT_TRUE(CanLoadScript(ext_without_permission));

  // chrome-extension:-URLs can never bypass CSP in WebUI.
  ui_test_utils::NavigateToURL(browser(), GURL(chrome::kChromeUIVersionURL));

  EXPECT_FALSE(CanLoadScript(component_ext_with_permission));
  EXPECT_FALSE(CanLoadScript(component_ext_without_permission));
  EXPECT_FALSE(CanLoadScript(ext_with_permission));
  EXPECT_FALSE(CanLoadScript(ext_without_permission));
}

}  // namespace extensions
