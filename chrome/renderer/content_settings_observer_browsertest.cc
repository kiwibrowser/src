// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/values.h"
#include "chrome/common/render_messages.h"
#include "chrome/renderer/content_settings_observer.h"
#include "chrome/test/base/chrome_render_view_test.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_utils.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "ipc/ipc_message_macros.h"
#include "ipc/ipc_test_sink.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"
#include "third_party/blink/public/web/web_frame_content_dumper.h"
#include "third_party/blink/public/web/web_view.h"

using testing::_;
using testing::DeleteArg;

namespace {

constexpr char kScriptHtml[] =
    "<html>"
    "<head>"
    "<script src='data:foo'></script>"
    "</head>"
    "<body>"
    "</body>"
    "</html>";

class MockContentSettingsObserver : public ContentSettingsObserver {
 public:
  MockContentSettingsObserver(content::RenderFrame* render_frame,
                              service_manager::BinderRegistry* registry);
  ~MockContentSettingsObserver() override {}

  bool Send(IPC::Message* message) override;

  MOCK_METHOD2(OnContentBlocked,
               void(ContentSettingsType, const base::string16&));

  MOCK_METHOD5(OnAllowDOMStorage,
               void(int, const GURL&, const GURL&, bool, IPC::Message*));

  const GURL& image_url() const { return image_url_; }
  const std::string& image_origin() const { return image_origin_; }

 private:
  const GURL image_url_;
  const std::string image_origin_;

  DISALLOW_COPY_AND_ASSIGN(MockContentSettingsObserver);
};

MockContentSettingsObserver::MockContentSettingsObserver(
    content::RenderFrame* render_frame,
    service_manager::BinderRegistry* registry)
    : ContentSettingsObserver(render_frame, NULL, false, registry),
      image_url_("http://www.foo.com/image.jpg"),
      image_origin_("http://www.foo.com") {}

bool MockContentSettingsObserver::Send(IPC::Message* message) {
  IPC_BEGIN_MESSAGE_MAP(MockContentSettingsObserver, *message)
    IPC_MESSAGE_HANDLER(ChromeViewHostMsg_ContentBlocked, OnContentBlocked)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(ChromeViewHostMsg_AllowDOMStorage,
                                    OnAllowDOMStorage)
    IPC_MESSAGE_UNHANDLED(ADD_FAILURE())
  IPC_END_MESSAGE_MAP()

  // Our super class deletes the message.
  return RenderFrameObserver::Send(message);
}

// Evaluates a boolean |predicate| every time a provisional load is committed in
// the given |frame| while the instance of this class is in scope, and verifies
// that the result matches the |expectation|.
class CommitTimeConditionChecker : public content::RenderFrameObserver {
 public:
  using Predicate = base::RepeatingCallback<bool()>;

  CommitTimeConditionChecker(content::RenderFrame* frame,
                             const Predicate& predicate,
                             bool expectation)
      : content::RenderFrameObserver(frame),
        predicate_(predicate),
        expectation_(expectation) {}

 protected:
  // RenderFrameObserver:
  void OnDestruct() override {}
  void DidCommitProvisionalLoad(bool is_new_navigation,
                                bool is_same_document_navigation) override {
    EXPECT_EQ(expectation_, predicate_.Run());
  }

 private:
  const Predicate predicate_;
  const bool expectation_;

  DISALLOW_COPY_AND_ASSIGN(CommitTimeConditionChecker);
};

}  // namespace

class ContentSettingsObserverBrowserTest : public ChromeRenderViewTest {
  void SetUp() override {
    ChromeRenderViewTest::SetUp();
    // Unbind the ContentSettingsRenderer interface that would be registered by
    // the ContentSettingsObserver created when the render frame is created.
    view_->GetMainRenderFrame()
        ->GetAssociatedInterfaceRegistry()
        ->RemoveInterface(chrome::mojom::ContentSettingsRenderer::Name_);
  }
};

TEST_F(ContentSettingsObserverBrowserTest, DidBlockContentType) {
  MockContentSettingsObserver observer(view_->GetMainRenderFrame(),
                                       registry_.get());
  EXPECT_CALL(observer, OnContentBlocked(CONTENT_SETTINGS_TYPE_COOKIES,
                                         base::string16()));
  observer.DidBlockContentType(CONTENT_SETTINGS_TYPE_COOKIES);

  // Blocking the same content type a second time shouldn't send a notification.
  observer.DidBlockContentType(CONTENT_SETTINGS_TYPE_COOKIES);
  ::testing::Mock::VerifyAndClearExpectations(&observer);
}

// Tests that multiple invokations of AllowDOMStorage result in a single IPC.
TEST_F(ContentSettingsObserverBrowserTest, AllowDOMStorage) {
  // Load some HTML, so we have a valid security origin.
  LoadHTMLWithUrlOverride("<html></html>", "https://example.com/");
  MockContentSettingsObserver observer(view_->GetMainRenderFrame(),
                                       registry_.get());
  ON_CALL(observer,
          OnAllowDOMStorage(_, _, _, _, _)).WillByDefault(DeleteArg<4>());
  EXPECT_CALL(observer,
              OnAllowDOMStorage(_, _, _, _, _));
  observer.AllowStorage(true);

  // Accessing localStorage from the same origin again shouldn't result in a
  // new IPC.
  observer.AllowStorage(true);
  ::testing::Mock::VerifyAndClearExpectations(&observer);
}

// Regression test for http://crbug.com/35011
TEST_F(ContentSettingsObserverBrowserTest, JSBlockSentAfterPageLoad) {
  // 1. Load page with JS.
  const char kHtml[] =
      "<html>"
      "<head>"
      "<script>document.createElement('div');</script>"
      "</head>"
      "<body>"
      "</body>"
      "</html>";
  render_thread_->sink().ClearMessages();
  LoadHTML(kHtml);

  // 2. Block JavaScript.
  RendererContentSettingRules content_setting_rules;
  ContentSettingsForOneType& script_setting_rules =
      content_setting_rules.script_rules;
  script_setting_rules.push_back(ContentSettingPatternSource(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      base::Value::FromUniquePtrValue(
          content_settings::ContentSettingToValue(CONTENT_SETTING_BLOCK)),
      std::string(), false));
  ContentSettingsObserver* observer = ContentSettingsObserver::Get(
      view_->GetMainRenderFrame());
  observer->SetContentSettingRules(&content_setting_rules);

  // Make sure no pending messages are in the queue.
  base::RunLoop().RunUntilIdle();
  render_thread_->sink().ClearMessages();

  const auto HasSentChromeViewHostMsgContentBlocked =
      [](content::MockRenderThread* render_thread) {
        return !!render_thread->sink().GetFirstMessageMatching(
            ChromeViewHostMsg_ContentBlocked::ID);
      };

  // 3. Reload page. Verify that the notification that javascript was blocked
  // has not yet been sent at the time when the navigation commits.
  CommitTimeConditionChecker checker(
      view_->GetMainRenderFrame(),
      base::Bind(HasSentChromeViewHostMsgContentBlocked,
                 base::Unretained(render_thread_.get())),
      false);

  std::string url_str = "data:text/html;charset=utf-8,";
  url_str.append(kHtml);
  GURL url(url_str);
  Reload(url);
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(HasSentChromeViewHostMsgContentBlocked(render_thread_.get()));
}

TEST_F(ContentSettingsObserverBrowserTest, PluginsTemporarilyAllowed) {
  // Load some HTML.
  LoadHTML("<html>Foo</html>");

  std::string foo_plugin = "foo";
  std::string bar_plugin = "bar";

  ContentSettingsObserver* observer =
      ContentSettingsObserver::Get(view_->GetMainRenderFrame());
  EXPECT_FALSE(observer->IsPluginTemporarilyAllowed(foo_plugin));

  // Temporarily allow the "foo" plugin.
  observer->OnLoadBlockedPlugins(foo_plugin);
  EXPECT_TRUE(observer->IsPluginTemporarilyAllowed(foo_plugin));
  EXPECT_FALSE(observer->IsPluginTemporarilyAllowed(bar_plugin));

  // Simulate same document navigation.
  OnSameDocumentNavigation(GetMainFrame(), true, true);
  EXPECT_TRUE(observer->IsPluginTemporarilyAllowed(foo_plugin));
  EXPECT_FALSE(observer->IsPluginTemporarilyAllowed(bar_plugin));

  // Navigate to a different page.
  LoadHTML("<html>Bar</html>");
  EXPECT_FALSE(observer->IsPluginTemporarilyAllowed(foo_plugin));
  EXPECT_FALSE(observer->IsPluginTemporarilyAllowed(bar_plugin));

  // Temporarily allow all plugins.
  observer->OnLoadBlockedPlugins(std::string());
  EXPECT_TRUE(observer->IsPluginTemporarilyAllowed(foo_plugin));
  EXPECT_TRUE(observer->IsPluginTemporarilyAllowed(bar_plugin));
}

TEST_F(ContentSettingsObserverBrowserTest, ImagesBlockedByDefault) {
  MockContentSettingsObserver mock_observer(view_->GetMainRenderFrame(),
                                            registry_.get());

  // Load some HTML.
  LoadHTML("<html>Foo</html>");

  // Set the default image blocking setting.
  RendererContentSettingRules content_setting_rules;
  ContentSettingsForOneType& image_setting_rules =
      content_setting_rules.image_rules;
  image_setting_rules.push_back(ContentSettingPatternSource(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      base::Value::FromUniquePtrValue(
          content_settings::ContentSettingToValue(CONTENT_SETTING_BLOCK)),
      std::string(), false));

  ContentSettingsObserver* observer = ContentSettingsObserver::Get(
      view_->GetMainRenderFrame());
  observer->SetContentSettingRules(&content_setting_rules);
  EXPECT_CALL(mock_observer,
              OnContentBlocked(CONTENT_SETTINGS_TYPE_IMAGES, base::string16()));
  EXPECT_FALSE(observer->AllowImage(true, mock_observer.image_url()));
  ::testing::Mock::VerifyAndClearExpectations(&observer);

  // Create an exception which allows the image.
  image_setting_rules.insert(
      image_setting_rules.begin(),
      ContentSettingPatternSource(
          ContentSettingsPattern::Wildcard(),
          ContentSettingsPattern::FromString(mock_observer.image_origin()),
          base::Value::FromUniquePtrValue(
              content_settings::ContentSettingToValue(CONTENT_SETTING_ALLOW)),
          std::string(), false));

  EXPECT_CALL(mock_observer, OnContentBlocked(CONTENT_SETTINGS_TYPE_IMAGES,
                                              base::string16())).Times(0);
  EXPECT_TRUE(observer->AllowImage(true, mock_observer.image_url()));
  ::testing::Mock::VerifyAndClearExpectations(&observer);
}

TEST_F(ContentSettingsObserverBrowserTest, ImagesAllowedByDefault) {
  MockContentSettingsObserver mock_observer(view_->GetMainRenderFrame(),
                                            registry_.get());

  // Load some HTML.
  LoadHTML("<html>Foo</html>");

  // Set the default image blocking setting.
  RendererContentSettingRules content_setting_rules;
  ContentSettingsForOneType& image_setting_rules =
      content_setting_rules.image_rules;
  image_setting_rules.push_back(ContentSettingPatternSource(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      base::Value::FromUniquePtrValue(
          content_settings::ContentSettingToValue(CONTENT_SETTING_ALLOW)),
      std::string(), false));

  ContentSettingsObserver* observer =
      ContentSettingsObserver::Get(view_->GetMainRenderFrame());
  observer->SetContentSettingRules(&content_setting_rules);
  EXPECT_CALL(mock_observer, OnContentBlocked(CONTENT_SETTINGS_TYPE_IMAGES,
                                              base::string16())).Times(0);
  EXPECT_TRUE(observer->AllowImage(true, mock_observer.image_url()));
  ::testing::Mock::VerifyAndClearExpectations(&observer);

  // Create an exception which blocks the image.
  image_setting_rules.insert(
      image_setting_rules.begin(),
      ContentSettingPatternSource(
          ContentSettingsPattern::Wildcard(),
          ContentSettingsPattern::FromString(mock_observer.image_origin()),
          base::Value::FromUniquePtrValue(
              content_settings::ContentSettingToValue(CONTENT_SETTING_BLOCK)),
          std::string(), false));
  EXPECT_CALL(mock_observer,
              OnContentBlocked(CONTENT_SETTINGS_TYPE_IMAGES, base::string16()));
  EXPECT_FALSE(observer->AllowImage(true, mock_observer.image_url()));
  ::testing::Mock::VerifyAndClearExpectations(&observer);
}

TEST_F(ContentSettingsObserverBrowserTest, ContentSettingsBlockScripts) {
  // Set the content settings for scripts.
  RendererContentSettingRules content_setting_rules;
  ContentSettingsForOneType& script_setting_rules =
      content_setting_rules.script_rules;
  script_setting_rules.push_back(ContentSettingPatternSource(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      base::Value::FromUniquePtrValue(
          content_settings::ContentSettingToValue(CONTENT_SETTING_BLOCK)),
      std::string(), false));

  ContentSettingsObserver* observer =
      ContentSettingsObserver::Get(view_->GetMainRenderFrame());
  observer->SetContentSettingRules(&content_setting_rules);

  // Load a page which contains a script.
  LoadHTML(kScriptHtml);

  // Verify that the script was blocked.
  EXPECT_TRUE(render_thread_->sink().GetFirstMessageMatching(
      ChromeViewHostMsg_ContentBlocked::ID));
}

TEST_F(ContentSettingsObserverBrowserTest, ContentSettingsAllowScripts) {
  // Set the content settings for scripts.
  RendererContentSettingRules content_setting_rules;
  ContentSettingsForOneType& script_setting_rules =
      content_setting_rules.script_rules;
  script_setting_rules.push_back(ContentSettingPatternSource(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      base::Value::FromUniquePtrValue(
          content_settings::ContentSettingToValue(CONTENT_SETTING_ALLOW)),
      std::string(), false));

  ContentSettingsObserver* observer =
      ContentSettingsObserver::Get(view_->GetMainRenderFrame());
  observer->SetContentSettingRules(&content_setting_rules);

  // Load a page which contains a script.
  LoadHTML(kScriptHtml);

  // Verify that the script was not blocked.
  EXPECT_FALSE(render_thread_->sink().GetFirstMessageMatching(
      ChromeViewHostMsg_ContentBlocked::ID));
}

// Regression test for crbug.com/232410: Load a page with JS blocked. Then,
// allow JS and reload the page. In each case, only one of noscript or script
// tags should be enabled, but never both.
TEST_F(ContentSettingsObserverBrowserTest, ContentSettingsNoscriptTag) {
  // 1. Block JavaScript.
  RendererContentSettingRules content_setting_rules;
  ContentSettingsForOneType& script_setting_rules =
      content_setting_rules.script_rules;
  script_setting_rules.push_back(ContentSettingPatternSource(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      base::Value::FromUniquePtrValue(
          content_settings::ContentSettingToValue(CONTENT_SETTING_BLOCK)),
      std::string(), false));

  ContentSettingsObserver* observer =
      ContentSettingsObserver::Get(view_->GetMainRenderFrame());
  observer->SetContentSettingRules(&content_setting_rules);

  // 2. Load a page which contains a noscript tag and a script tag. Note that
  // the page doesn't have a body tag.
  const char kHtml[] =
      "<html>"
      "<noscript>JS_DISABLED</noscript>"
      "<script>document.write('JS_ENABLED');</script>"
      "</html>";
  LoadHTML(kHtml);
  EXPECT_NE(
      std::string::npos,
      blink::WebFrameContentDumper::DumpLayoutTreeAsText(
          GetMainFrame(), blink::WebFrameContentDumper::kLayoutAsTextNormal)
          .Utf8()
          .find("JS_DISABLED"));
  EXPECT_EQ(
      std::string::npos,
      blink::WebFrameContentDumper::DumpLayoutTreeAsText(
          GetMainFrame(), blink::WebFrameContentDumper::kLayoutAsTextNormal)
          .Utf8()
          .find("JS_ENABLED"));

  // 3. Allow JavaScript.
  script_setting_rules.clear();
  script_setting_rules.push_back(ContentSettingPatternSource(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      base::Value::FromUniquePtrValue(
          content_settings::ContentSettingToValue(CONTENT_SETTING_ALLOW)),
      std::string(), false));
  observer->SetContentSettingRules(&content_setting_rules);

  // 4. Reload the page.
  std::string url_str = "data:text/html;charset=utf-8,";
  url_str.append(kHtml);
  GURL url(url_str);
  Reload(url);
  EXPECT_NE(
      std::string::npos,
      blink::WebFrameContentDumper::DumpLayoutTreeAsText(
          GetMainFrame(), blink::WebFrameContentDumper::kLayoutAsTextNormal)
          .Utf8()
          .find("JS_ENABLED"));
  EXPECT_EQ(
      std::string::npos,
      blink::WebFrameContentDumper::DumpLayoutTreeAsText(
          GetMainFrame(), blink::WebFrameContentDumper::kLayoutAsTextNormal)
          .Utf8()
          .find("JS_DISABLED"));
}

// Checks that same document navigations don't update content settings for the
// page.
TEST_F(ContentSettingsObserverBrowserTest,
       ContentSettingsSameDocumentNavigation) {
  MockContentSettingsObserver mock_observer(view_->GetMainRenderFrame(),
                                            registry_.get());
  // Load a page which contains a script.
  LoadHTML(kScriptHtml);

  // Verify that the script was not blocked.
  EXPECT_FALSE(render_thread_->sink().GetFirstMessageMatching(
      ChromeViewHostMsg_ContentBlocked::ID));

  // Block JavaScript.
  RendererContentSettingRules content_setting_rules;
  ContentSettingsForOneType& script_setting_rules =
      content_setting_rules.script_rules;
  script_setting_rules.push_back(ContentSettingPatternSource(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      base::Value::FromUniquePtrValue(
          content_settings::ContentSettingToValue(CONTENT_SETTING_BLOCK)),
      std::string(), false));

  ContentSettingsObserver* observer =
      ContentSettingsObserver::Get(view_->GetMainRenderFrame());
  observer->SetContentSettingRules(&content_setting_rules);

  // The page shouldn't see the change to script blocking setting after a
  // same document navigation.
  OnSameDocumentNavigation(GetMainFrame(), true, true);
  EXPECT_TRUE(observer->AllowScript(true));
}

TEST_F(ContentSettingsObserverBrowserTest, ContentSettingsInterstitialPages) {
  MockContentSettingsObserver mock_observer(view_->GetMainRenderFrame(),
                                            registry_.get());
  // Block scripts.
  RendererContentSettingRules content_setting_rules;
  ContentSettingsForOneType& script_setting_rules =
      content_setting_rules.script_rules;
  script_setting_rules.push_back(ContentSettingPatternSource(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      base::Value::FromUniquePtrValue(
          content_settings::ContentSettingToValue(CONTENT_SETTING_BLOCK)),
      std::string(), false));
  // Block images.
  ContentSettingsForOneType& image_setting_rules =
      content_setting_rules.image_rules;
  image_setting_rules.push_back(ContentSettingPatternSource(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      base::Value::FromUniquePtrValue(
          content_settings::ContentSettingToValue(CONTENT_SETTING_BLOCK)),
      std::string(), false));

  ContentSettingsObserver* observer =
      ContentSettingsObserver::Get(view_->GetMainRenderFrame());
  observer->SetContentSettingRules(&content_setting_rules);
  observer->SetAsInterstitial();

  // Load a page which contains a script.
  LoadHTML(kScriptHtml);

  // Verify that the script was allowed.
  EXPECT_FALSE(render_thread_->sink().GetFirstMessageMatching(
      ChromeViewHostMsg_ContentBlocked::ID));

  // Verify that images are allowed.
  EXPECT_CALL(mock_observer, OnContentBlocked(CONTENT_SETTINGS_TYPE_IMAGES,
                                              base::string16())).Times(0);
  EXPECT_TRUE(observer->AllowImage(true, mock_observer.image_url()));
  ::testing::Mock::VerifyAndClearExpectations(&observer);
}

TEST_F(ContentSettingsObserverBrowserTest, AutoplayContentSettings) {
  MockContentSettingsObserver mock_observer(view_->GetMainRenderFrame(),
                                            registry_.get());

  // Load some HTML.
  LoadHTML("<html>Foo</html>");

  // Set the default setting.
  RendererContentSettingRules content_setting_rules;
  ContentSettingsForOneType& autoplay_setting_rules =
      content_setting_rules.autoplay_rules;
  autoplay_setting_rules.push_back(ContentSettingPatternSource(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      base::Value::FromUniquePtrValue(
          content_settings::ContentSettingToValue(CONTENT_SETTING_ALLOW)),
      std::string(), false));

  ContentSettingsObserver* observer =
      ContentSettingsObserver::Get(view_->GetMainRenderFrame());
  observer->SetContentSettingRules(&content_setting_rules);

  EXPECT_TRUE(observer->AllowAutoplay(false));
  ::testing::Mock::VerifyAndClearExpectations(&observer);

  // Add rule to block autoplay.
  autoplay_setting_rules.insert(
      autoplay_setting_rules.begin(),
      ContentSettingPatternSource(
          ContentSettingsPattern::Wildcard(),
          ContentSettingsPattern::Wildcard(),
          base::Value::FromUniquePtrValue(
              content_settings::ContentSettingToValue(CONTENT_SETTING_BLOCK)),
          std::string(), false));

  EXPECT_FALSE(observer->AllowAutoplay(true));
  ::testing::Mock::VerifyAndClearExpectations(&observer);
}
