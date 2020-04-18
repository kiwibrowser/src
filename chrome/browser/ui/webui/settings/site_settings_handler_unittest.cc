// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/settings/site_settings_handler.h"

#include <memory>
#include <string>

#include "base/test/histogram_tester.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/test_extension_system.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/webui/site_settings_helper.h"
#include "chrome/test/base/browser_with_test_window_test.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/content_settings/core/common/pref_names.h"
#include "components/infobars/core/infobar.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_web_ui.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension_builder.h"
#include "ppapi/buildflags/buildflags.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/login/users/mock_user_manager.h"
#include "components/user_manager/scoped_user_manager.h"
#endif

#if BUILDFLAG(ENABLE_PLUGINS)
#include "chrome/browser/plugins/chrome_plugin_service_filter.h"
#endif

namespace {

constexpr char kCallbackId[] = "test-callback-id";
constexpr char kSetting[] = "setting";
constexpr char kSource[] = "source";
constexpr char kExtensionName[] = "Test Extension";

#if BUILDFLAG(ENABLE_PLUGINS)
// Waits until a change is observed in content settings.
class FlashContentSettingsChangeWaiter : public content_settings::Observer {
 public:
  explicit FlashContentSettingsChangeWaiter(Profile* profile)
      : profile_(profile) {
    HostContentSettingsMapFactory::GetForProfile(profile)->AddObserver(this);
  }
  ~FlashContentSettingsChangeWaiter() override {
    HostContentSettingsMapFactory::GetForProfile(profile_)->RemoveObserver(
        this);
  }

  // content_settings::Observer:
  void OnContentSettingChanged(const ContentSettingsPattern& primary_pattern,
                               const ContentSettingsPattern& secondary_pattern,
                               ContentSettingsType content_type,
                               std::string resource_identifier) override {
    if (content_type == CONTENT_SETTINGS_TYPE_PLUGINS)
      Proceed();
  }

  void Wait() { run_loop_.Run(); }

 private:
  void Proceed() { run_loop_.Quit(); }

  Profile* profile_;
  base::RunLoop run_loop_;

  DISALLOW_COPY_AND_ASSIGN(FlashContentSettingsChangeWaiter);
};
#endif

}  // namespace

namespace settings {

// Helper class for setting ContentSettings via different sources.
class ContentSettingSourceSetter {
 public:
  ContentSettingSourceSetter(TestingProfile* profile,
                             ContentSettingsType content_type)
      : prefs_(profile->GetTestingPrefService()),
        host_content_settings_map_(
            HostContentSettingsMapFactory::GetForProfile(profile)),
        content_type_(content_type) {}

  void SetPolicyDefault(ContentSetting setting) {
    prefs_->SetManagedPref(GetPrefNameForDefaultPermissionSetting(),
                           std::make_unique<base::Value>(setting));
  }

  const char* GetPrefNameForDefaultPermissionSetting() {
    switch (content_type_) {
      case CONTENT_SETTINGS_TYPE_NOTIFICATIONS:
        return prefs::kManagedDefaultNotificationsSetting;
      default:
        // Add support as needed.
        NOTREACHED();
        return "";
    }
  }

 private:
  sync_preferences::TestingPrefServiceSyncable* prefs_;
  HostContentSettingsMap* host_content_settings_map_;
  ContentSettingsType content_type_;

  DISALLOW_COPY_AND_ASSIGN(ContentSettingSourceSetter);
};

class SiteSettingsHandlerTest : public testing::Test {
 public:
  SiteSettingsHandlerTest()
      : kNotifications(site_settings::ContentSettingsTypeToGroupName(
            CONTENT_SETTINGS_TYPE_NOTIFICATIONS)),
        kCookies(site_settings::ContentSettingsTypeToGroupName(
            CONTENT_SETTINGS_TYPE_COOKIES)),
        kFlash(site_settings::ContentSettingsTypeToGroupName(
            CONTENT_SETTINGS_TYPE_PLUGINS)),
        handler_(&profile_) {
#if defined(OS_CHROMEOS)
    user_manager_enabler_ = std::make_unique<user_manager::ScopedUserManager>(
        std::make_unique<chromeos::MockUserManager>());
#endif
  }

  void SetUp() override {
    handler()->set_web_ui(web_ui());
    handler()->AllowJavascript();
    web_ui()->ClearTrackedCalls();
  }

  TestingProfile* profile() { return &profile_; }
  content::TestWebUI* web_ui() { return &web_ui_; }
  SiteSettingsHandler* handler() { return &handler_; }

  void ValidateDefault(const ContentSetting expected_setting,
                       const site_settings::SiteSettingSource expected_source,
                       size_t expected_total_calls) {
    EXPECT_EQ(expected_total_calls, web_ui()->call_data().size());

    const content::TestWebUI::CallData& data = *web_ui()->call_data().back();
    EXPECT_EQ("cr.webUIResponse", data.function_name());

    std::string callback_id;
    ASSERT_TRUE(data.arg1()->GetAsString(&callback_id));
    EXPECT_EQ(kCallbackId, callback_id);

    bool success = false;
    ASSERT_TRUE(data.arg2()->GetAsBoolean(&success));
    ASSERT_TRUE(success);

    const base::DictionaryValue* default_value = nullptr;
    ASSERT_TRUE(data.arg3()->GetAsDictionary(&default_value));
    std::string setting;
    ASSERT_TRUE(default_value->GetString(kSetting, &setting));
    EXPECT_EQ(content_settings::ContentSettingToString(expected_setting),
              setting);
    std::string source;
    if (default_value->GetString(kSource, &source))
      EXPECT_EQ(site_settings::SiteSettingSourceToString(expected_source),
                source);
  }

  void ValidateOrigin(const std::string& expected_origin,
                      const std::string& expected_embedding,
                      const std::string& expected_display_name,
                      const ContentSetting expected_setting,
                      const site_settings::SiteSettingSource expected_source,
                      size_t expected_total_calls) {
    EXPECT_EQ(expected_total_calls, web_ui()->call_data().size());

    const content::TestWebUI::CallData& data = *web_ui()->call_data().back();
    EXPECT_EQ("cr.webUIResponse", data.function_name());

    std::string callback_id;
    ASSERT_TRUE(data.arg1()->GetAsString(&callback_id));
    EXPECT_EQ(kCallbackId, callback_id);
    bool success = false;
    ASSERT_TRUE(data.arg2()->GetAsBoolean(&success));
    ASSERT_TRUE(success);

    const base::ListValue* exceptions;
    ASSERT_TRUE(data.arg3()->GetAsList(&exceptions));
    EXPECT_EQ(1U, exceptions->GetSize());
    const base::DictionaryValue* exception;
    ASSERT_TRUE(exceptions->GetDictionary(0, &exception));
    std::string origin, embedding_origin, display_name, setting, source;
    ASSERT_TRUE(exception->GetString(site_settings::kOrigin, &origin));
    ASSERT_EQ(expected_origin, origin);
    ASSERT_TRUE(
        exception->GetString(site_settings::kDisplayName, &display_name));
    ASSERT_EQ(expected_display_name, display_name);
    ASSERT_TRUE(exception->GetString(
        site_settings::kEmbeddingOrigin, &embedding_origin));
    ASSERT_EQ(expected_embedding, embedding_origin);
    ASSERT_TRUE(exception->GetString(site_settings::kSetting, &setting));
    ASSERT_EQ(content_settings::ContentSettingToString(expected_setting),
              setting);
    ASSERT_TRUE(exception->GetString(site_settings::kSource, &source));
    ASSERT_EQ(site_settings::SiteSettingSourceToString(expected_source),
              source);
  }

  void ValidateNoOrigin(size_t expected_total_calls) {
    EXPECT_EQ(expected_total_calls, web_ui()->call_data().size());

    const content::TestWebUI::CallData& data = *web_ui()->call_data().back();
    EXPECT_EQ("cr.webUIResponse", data.function_name());

    std::string callback_id;
    ASSERT_TRUE(data.arg1()->GetAsString(&callback_id));
    EXPECT_EQ(kCallbackId, callback_id);

    bool success = false;
    ASSERT_TRUE(data.arg2()->GetAsBoolean(&success));
    ASSERT_TRUE(success);

    const base::ListValue* exceptions;
    ASSERT_TRUE(data.arg3()->GetAsList(&exceptions));
    EXPECT_EQ(0U, exceptions->GetSize());
  }

  void ValidatePattern(bool expected_validity, size_t expected_total_calls) {
    EXPECT_EQ(expected_total_calls, web_ui()->call_data().size());

    const content::TestWebUI::CallData& data = *web_ui()->call_data().back();
    EXPECT_EQ("cr.webUIResponse", data.function_name());

    std::string callback_id;
    ASSERT_TRUE(data.arg1()->GetAsString(&callback_id));
    EXPECT_EQ(kCallbackId, callback_id);

    bool success = false;
    ASSERT_TRUE(data.arg2()->GetAsBoolean(&success));
    ASSERT_TRUE(success);

    bool valid;
    ASSERT_TRUE(data.arg3()->GetAsBoolean(&valid));
    EXPECT_EQ(expected_validity, valid);
  }

  void ValidateIncognitoExists(
      bool expected_incognito, size_t expected_total_calls) {
    EXPECT_EQ(expected_total_calls, web_ui()->call_data().size());

    const content::TestWebUI::CallData& data = *web_ui()->call_data().back();
    EXPECT_EQ("cr.webUIListenerCallback", data.function_name());

    std::string callback_id;
    ASSERT_TRUE(data.arg1()->GetAsString(&callback_id));
    EXPECT_EQ("onIncognitoStatusChanged", callback_id);

    bool incognito;
    ASSERT_TRUE(data.arg2()->GetAsBoolean(&incognito));
    EXPECT_EQ(expected_incognito, incognito);
  }

  void ValidateZoom(const std::string& expected_host,
      const std::string& expected_zoom, size_t expected_total_calls) {
    EXPECT_EQ(expected_total_calls, web_ui()->call_data().size());

    const content::TestWebUI::CallData& data = *web_ui()->call_data().back();
    EXPECT_EQ("cr.webUIListenerCallback", data.function_name());

    std::string callback_id;
    ASSERT_TRUE(data.arg1()->GetAsString(&callback_id));
    EXPECT_EQ("onZoomLevelsChanged", callback_id);

    const base::ListValue* exceptions;
    ASSERT_TRUE(data.arg2()->GetAsList(&exceptions));
    if (expected_host.empty()) {
      EXPECT_EQ(0U, exceptions->GetSize());
    } else {
      EXPECT_EQ(1U, exceptions->GetSize());

      const base::DictionaryValue* exception;
      ASSERT_TRUE(exceptions->GetDictionary(0, &exception));

      std::string host;
      ASSERT_TRUE(exception->GetString("origin", &host));
      ASSERT_EQ(expected_host, host);

      std::string zoom;
      ASSERT_TRUE(exception->GetString("zoom", &zoom));
      ASSERT_EQ(expected_zoom, zoom);
    }
  }

  void CreateIncognitoProfile() {
    incognito_profile_ = TestingProfile::Builder().BuildIncognito(&profile_);
  }

  void DestroyIncognitoProfile() {
    content::NotificationService::current()->Notify(
        chrome::NOTIFICATION_PROFILE_DESTROYED,
        content::Source<Profile>(static_cast<Profile*>(incognito_profile_)),
        content::NotificationService::NoDetails());
    profile_.SetOffTheRecordProfile(nullptr);
    ASSERT_FALSE(profile_.HasOffTheRecordProfile());
    incognito_profile_ = nullptr;
  }

  // Content setting group name for the relevant ContentSettingsType.
  const std::string kNotifications;
  const std::string kCookies;
  const std::string kFlash;

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  TestingProfile profile_;
  TestingProfile* incognito_profile_;
  content::TestWebUI web_ui_;
  SiteSettingsHandler handler_;
#if defined(OS_CHROMEOS)
  std::unique_ptr<user_manager::ScopedUserManager> user_manager_enabler_;
#endif
};

TEST_F(SiteSettingsHandlerTest, GetAndSetDefault) {
  // Test the JS -> C++ -> JS callback path for getting and setting defaults.
  base::ListValue get_args;
  get_args.AppendString(kCallbackId);
  get_args.AppendString(kNotifications);
  handler()->HandleGetDefaultValueForContentType(&get_args);
  ValidateDefault(CONTENT_SETTING_ASK,
                  site_settings::SiteSettingSource::kDefault, 1U);

  // Set the default to 'Blocked'.
  base::ListValue set_args;
  set_args.AppendString(kNotifications);
  set_args.AppendString(
      content_settings::ContentSettingToString(CONTENT_SETTING_BLOCK));
  handler()->HandleSetDefaultValueForContentType(&set_args);

  EXPECT_EQ(2U, web_ui()->call_data().size());

  // Verify that the default has been set to 'Blocked'.
  handler()->HandleGetDefaultValueForContentType(&get_args);
  ValidateDefault(CONTENT_SETTING_BLOCK,
                  site_settings::SiteSettingSource::kDefault, 3U);
}

TEST_F(SiteSettingsHandlerTest, Origins) {
  const std::string google("https://www.google.com:443");
  const std::string uma_base("WebsiteSettings.Menu.PermissionChanged");
  {
    // Test the JS -> C++ -> JS callback path for configuring origins, by
    // setting Google.com to blocked.
    base::ListValue set_args;
    set_args.AppendString(google);  // Primary pattern.
    set_args.AppendString(google);  // Secondary pattern.
    set_args.AppendString(kNotifications);
    set_args.AppendString(
        content_settings::ContentSettingToString(CONTENT_SETTING_BLOCK));
    set_args.AppendBoolean(false);  // Incognito.
    base::HistogramTester histograms;
    handler()->HandleSetCategoryPermissionForPattern(&set_args);
    EXPECT_EQ(1U, web_ui()->call_data().size());
    histograms.ExpectTotalCount(uma_base, 1);
    histograms.ExpectTotalCount(uma_base + ".Allowed", 0);
    histograms.ExpectTotalCount(uma_base + ".Blocked", 1);
    histograms.ExpectTotalCount(uma_base + ".Reset", 0);
    histograms.ExpectTotalCount(uma_base + ".SessionOnly", 0);
  }

  base::ListValue get_exception_list_args;
  get_exception_list_args.AppendString(kCallbackId);
  get_exception_list_args.AppendString(kNotifications);
  handler()->HandleGetExceptionList(&get_exception_list_args);
  ValidateOrigin(google, google, google, CONTENT_SETTING_BLOCK,
                 site_settings::SiteSettingSource::kPreference, 2U);

  {
    // Reset things back to how they were.
    base::ListValue reset_args;
    reset_args.AppendString(google);
    reset_args.AppendString(google);
    reset_args.AppendString(kNotifications);
    reset_args.AppendBoolean(false);  // Incognito.
    base::HistogramTester histograms;
    handler()->HandleResetCategoryPermissionForPattern(&reset_args);
    EXPECT_EQ(3U, web_ui()->call_data().size());
    histograms.ExpectTotalCount(uma_base, 1);
    histograms.ExpectTotalCount(uma_base + ".Allowed", 0);
    histograms.ExpectTotalCount(uma_base + ".Blocked", 0);
    histograms.ExpectTotalCount(uma_base + ".Reset", 1);
  }

  // Verify the reset was successful.
  handler()->HandleGetExceptionList(&get_exception_list_args);
  ValidateNoOrigin(4U);
}

TEST_F(SiteSettingsHandlerTest, DefaultSettingSource) {
  // Use a non-default port to verify the display name does not strip this off.
  const std::string google("https://www.google.com:183");
  ContentSettingSourceSetter source_setter(profile(),
                                           CONTENT_SETTINGS_TYPE_NOTIFICATIONS);

  base::ListValue get_origin_permissions_args;
  get_origin_permissions_args.AppendString(kCallbackId);
  get_origin_permissions_args.AppendString(google);
  auto category_list = std::make_unique<base::ListValue>();
  category_list->AppendString(kNotifications);
  get_origin_permissions_args.Append(std::move(category_list));

  // Test Chrome built-in defaults are marked as default.
  handler()->HandleGetOriginPermissions(&get_origin_permissions_args);
  ValidateOrigin(google, google, google, CONTENT_SETTING_ASK,
                 site_settings::SiteSettingSource::kDefault, 1U);

  base::ListValue default_value_args;
  default_value_args.AppendString(kNotifications);
  default_value_args.AppendString(
      content_settings::ContentSettingToString(CONTENT_SETTING_BLOCK));
  handler()->HandleSetDefaultValueForContentType(&default_value_args);
  // A user-set global default should also show up as default.
  handler()->HandleGetOriginPermissions(&get_origin_permissions_args);
  ValidateOrigin(google, google, google, CONTENT_SETTING_BLOCK,
                 site_settings::SiteSettingSource::kDefault, 3U);

  base::ListValue set_notification_pattern_args;
  set_notification_pattern_args.AppendString("[*.]google.com");
  set_notification_pattern_args.AppendString("*");
  set_notification_pattern_args.AppendString(kNotifications);
  set_notification_pattern_args.AppendString(
      content_settings::ContentSettingToString(CONTENT_SETTING_ALLOW));
  set_notification_pattern_args.AppendBoolean(false);
  handler()->HandleSetCategoryPermissionForPattern(
      &set_notification_pattern_args);
  // A user-set pattern should not show up as default.
  handler()->HandleGetOriginPermissions(&get_origin_permissions_args);
  ValidateOrigin(google, google, google, CONTENT_SETTING_ALLOW,
                 site_settings::SiteSettingSource::kPreference, 5U);

  base::ListValue set_notification_origin_args;
  set_notification_origin_args.AppendString(google);
  set_notification_origin_args.AppendString(google);
  set_notification_origin_args.AppendString(kNotifications);
  set_notification_origin_args.AppendString(
      content_settings::ContentSettingToString(CONTENT_SETTING_BLOCK));
  set_notification_origin_args.AppendBoolean(false);
  handler()->HandleSetCategoryPermissionForPattern(
      &set_notification_origin_args);
  // A user-set per-origin permission should not show up as default.
  handler()->HandleGetOriginPermissions(&get_origin_permissions_args);
  ValidateOrigin(google, google, google, CONTENT_SETTING_BLOCK,
                 site_settings::SiteSettingSource::kPreference, 7U);

  // Enterprise-policy set defaults should not show up as default.
  source_setter.SetPolicyDefault(CONTENT_SETTING_ALLOW);
  handler()->HandleGetOriginPermissions(&get_origin_permissions_args);
  ValidateOrigin(google, google, google, CONTENT_SETTING_ALLOW,
                 site_settings::SiteSettingSource::kPolicy, 8U);
}

TEST_F(SiteSettingsHandlerTest, GetAndSetOriginPermissions) {
  const std::string origin_with_port("https://www.example.com:443");
  // The display name won't show the port if it's default for that scheme.
  const std::string origin("https://www.example.com");
  base::ListValue get_args;
  get_args.AppendString(kCallbackId);
  get_args.AppendString(origin_with_port);
  {
    auto category_list = std::make_unique<base::ListValue>();
    category_list->AppendString(kNotifications);
    get_args.Append(std::move(category_list));
  }
  handler()->HandleGetOriginPermissions(&get_args);
  ValidateOrigin(origin_with_port, origin_with_port, origin,
                 CONTENT_SETTING_ASK,
                 site_settings::SiteSettingSource::kDefault, 1U);

  // Block notifications.
  base::ListValue set_args;
  set_args.AppendString(origin_with_port);
  {
    auto category_list = std::make_unique<base::ListValue>();
    category_list->AppendString(kNotifications);
    set_args.Append(std::move(category_list));
  }
  set_args.AppendString(
      content_settings::ContentSettingToString(CONTENT_SETTING_BLOCK));
  handler()->HandleSetOriginPermissions(&set_args);
  EXPECT_EQ(2U, web_ui()->call_data().size());

  // Reset things back to how they were.
  base::ListValue reset_args;
  reset_args.AppendString(origin_with_port);
  auto category_list = std::make_unique<base::ListValue>();
  category_list->AppendString(kNotifications);
  reset_args.Append(std::move(category_list));
  reset_args.AppendString(
      content_settings::ContentSettingToString(CONTENT_SETTING_DEFAULT));

  handler()->HandleSetOriginPermissions(&reset_args);
  EXPECT_EQ(3U, web_ui()->call_data().size());

  // Verify the reset was successful.
  handler()->HandleGetOriginPermissions(&get_args);
  ValidateOrigin(origin_with_port, origin_with_port, origin,
                 CONTENT_SETTING_ASK,
                 site_settings::SiteSettingSource::kDefault, 4U);
}

#if BUILDFLAG(ENABLE_PLUGINS)
TEST_F(SiteSettingsHandlerTest, ChangingFlashSettingForSiteIsRemembered) {
  ChromePluginServiceFilter::GetInstance()->RegisterResourceContext(
      profile(), profile()->GetResourceContext());
  FlashContentSettingsChangeWaiter waiter(profile());

  const std::string origin_with_port("https://www.example.com:443");
  // The display name won't show the port if it's default for that scheme.
  const std::string origin("https://www.example.com");
  base::ListValue get_args;
  get_args.AppendString(kCallbackId);
  get_args.AppendString(origin_with_port);
  const GURL url(origin_with_port);

  HostContentSettingsMap* map =
      HostContentSettingsMapFactory::GetForProfile(profile());
  // Make sure the site being tested doesn't already have this marker set.
  EXPECT_EQ(nullptr,
            map->GetWebsiteSetting(url, url, CONTENT_SETTINGS_TYPE_PLUGINS_DATA,
                                   std::string(), nullptr));

  // Change the Flash setting.
  base::ListValue set_args;
  set_args.AppendString(origin_with_port);
  {
    auto category_list = std::make_unique<base::ListValue>();
    category_list->AppendString(kFlash);
    set_args.Append(std::move(category_list));
  }
  set_args.AppendString(
      content_settings::ContentSettingToString(CONTENT_SETTING_BLOCK));
  handler()->HandleSetOriginPermissions(&set_args);
  EXPECT_EQ(1U, web_ui()->call_data().size());
  waiter.Wait();

  // Check that this site has now been marked for displaying Flash always, then
  // clear it and check this works.
  EXPECT_NE(nullptr,
            map->GetWebsiteSetting(url, url, CONTENT_SETTINGS_TYPE_PLUGINS_DATA,
                                   std::string(), nullptr));
  base::ListValue clear_args;
  clear_args.AppendString(origin_with_port);
  handler()->HandleSetOriginPermissions(&set_args);
  handler()->HandleClearFlashPref(&clear_args);
  EXPECT_EQ(nullptr,
            map->GetWebsiteSetting(url, url, CONTENT_SETTINGS_TYPE_PLUGINS_DATA,
                                   std::string(), nullptr));
}
#endif

TEST_F(SiteSettingsHandlerTest, GetAndSetForInvalidURLs) {
  const std::string origin("arbitrary string");
  EXPECT_FALSE(GURL(origin).is_valid());
  base::ListValue get_args;
  get_args.AppendString(kCallbackId);
  get_args.AppendString(origin);
  {
    auto category_list = std::make_unique<base::ListValue>();
    category_list->AppendString(kNotifications);
    get_args.Append(std::move(category_list));
  }
  handler()->HandleGetOriginPermissions(&get_args);
  // Verify that it'll return CONTENT_SETTING_BLOCK as |origin| is not a secure
  // context, a requirement for notifications. Note that the display string
  // will be blank since it's an invalid URL.
  ValidateOrigin(origin, origin, "", CONTENT_SETTING_BLOCK,
                 site_settings::SiteSettingSource::kInsecureOrigin, 1U);

  // Make sure setting a permission on an invalid origin doesn't crash.
  base::ListValue set_args;
  set_args.AppendString(origin);
  {
    auto category_list = std::make_unique<base::ListValue>();
    category_list->AppendString(kNotifications);
    set_args.Append(std::move(category_list));
  }
  set_args.AppendString(
      content_settings::ContentSettingToString(CONTENT_SETTING_ALLOW));
  handler()->HandleSetOriginPermissions(&set_args);

  // Also make sure the content setting for |origin| wasn't actually changed.
  handler()->HandleGetOriginPermissions(&get_args);
  ValidateOrigin(origin, origin, "", CONTENT_SETTING_BLOCK,
                 site_settings::SiteSettingSource::kInsecureOrigin, 2U);
}

TEST_F(SiteSettingsHandlerTest, ExceptionHelpers) {
  ContentSettingsPattern pattern =
      ContentSettingsPattern::FromString("[*.]google.com");
  std::unique_ptr<base::DictionaryValue> exception =
      site_settings::GetExceptionForPage(
          pattern, pattern, pattern.ToString(), CONTENT_SETTING_BLOCK,
          site_settings::SiteSettingSourceToString(
              site_settings::SiteSettingSource::kPreference),
          false);

  std::string primary_pattern, secondary_pattern, display_name, type;
  bool incognito;
  CHECK(exception->GetString(site_settings::kOrigin, &primary_pattern));
  CHECK(exception->GetString(site_settings::kDisplayName, &display_name));
  CHECK(exception->GetString(site_settings::kEmbeddingOrigin,
                             &secondary_pattern));
  CHECK(exception->GetString(site_settings::kSetting, &type));
  CHECK(exception->GetBoolean(site_settings::kIncognito, &incognito));

  base::ListValue args;
  args.AppendString(primary_pattern);
  args.AppendString(secondary_pattern);
  args.AppendString(kNotifications);  // Chosen arbitrarily.
  args.AppendString(type);
  args.AppendBoolean(incognito);

  // We don't need to check the results. This is just to make sure it doesn't
  // crash on the input.
  handler()->HandleSetCategoryPermissionForPattern(&args);

  scoped_refptr<const extensions::Extension> extension;
  extension = extensions::ExtensionBuilder()
                  .SetManifest(extensions::DictionaryBuilder()
                                   .Set("name", kExtensionName)
                                   .Set("version", "1.0.0")
                                   .Set("manifest_version", 2)
                                   .Build())
                  .SetID("ahfgeienlihckogmohjhadlkjgocpleb")
                  .Build();

  std::unique_ptr<base::ListValue> exceptions(new base::ListValue);
  site_settings::AddExceptionForHostedApp(
      "[*.]google.com", *extension.get(), exceptions.get());

  const base::DictionaryValue* dictionary;
  CHECK(exceptions->GetDictionary(0, &dictionary));
  CHECK(dictionary->GetString(site_settings::kOrigin, &primary_pattern));
  CHECK(dictionary->GetString(site_settings::kDisplayName, &display_name));
  CHECK(dictionary->GetString(site_settings::kEmbeddingOrigin,
                              &secondary_pattern));
  CHECK(dictionary->GetString(site_settings::kSetting, &type));
  CHECK(dictionary->GetBoolean(site_settings::kIncognito, &incognito));

  // Again, don't need to check the results.
  handler()->HandleSetCategoryPermissionForPattern(&args);
}

TEST_F(SiteSettingsHandlerTest, ExtensionDisplayName) {
  auto* extension_registry = extensions::ExtensionRegistry::Get(profile());
  std::string test_extension_id = "test-extension-url";
  std::string test_extension_url = "chrome-extension://" + test_extension_id;
  scoped_refptr<const extensions::Extension> extension =
      extensions::ExtensionBuilder()
          .SetManifest(extensions::DictionaryBuilder()
                           .Set("name", kExtensionName)
                           .Set("version", "1.0.0")
                           .Set("manifest_version", 2)
                           .Build())
          .SetID(test_extension_id)
          .Build();
  extension_registry->AddEnabled(extension);

  base::ListValue get_origin_permissions_args;
  get_origin_permissions_args.AppendString(kCallbackId);
  get_origin_permissions_args.AppendString(test_extension_url);
  {
    auto category_list = std::make_unique<base::ListValue>();
    category_list->AppendString(kNotifications);
    get_origin_permissions_args.Append(std::move(category_list));
  }
  handler()->HandleGetOriginPermissions(&get_origin_permissions_args);
  ValidateOrigin(test_extension_url, test_extension_url, kExtensionName,
                 CONTENT_SETTING_ASK,
                 site_settings::SiteSettingSource::kDefault, 1U);
}

TEST_F(SiteSettingsHandlerTest, Patterns) {
  base::ListValue args;
  std::string pattern("[*.]google.com");
  args.AppendString(kCallbackId);
  args.AppendString(pattern);
  handler()->HandleIsPatternValid(&args);
  ValidatePattern(true, 1U);

  base::ListValue invalid;
  std::string bad_pattern(";");
  invalid.AppendString(kCallbackId);
  invalid.AppendString(bad_pattern);
  handler()->HandleIsPatternValid(&invalid);
  ValidatePattern(false, 2U);

  // The wildcard pattern ('*') is a valid pattern, but not allowed to be
  // entered in site settings as it changes the default setting.
  // (crbug.com/709539).
  base::ListValue invalid_wildcard;
  std::string bad_pattern_wildcard("*");
  invalid_wildcard.AppendString(kCallbackId);
  invalid_wildcard.AppendString(bad_pattern_wildcard);
  handler()->HandleIsPatternValid(&invalid_wildcard);
  ValidatePattern(false, 3U);
}

TEST_F(SiteSettingsHandlerTest, Incognito) {
  base::ListValue args;
  handler()->HandleUpdateIncognitoStatus(&args);
  ValidateIncognitoExists(false, 1U);

  CreateIncognitoProfile();
  ValidateIncognitoExists(true, 2U);

  DestroyIncognitoProfile();
  ValidateIncognitoExists(false, 3U);
}

TEST_F(SiteSettingsHandlerTest, ZoomLevels) {
  std::string host("http://www.google.com");
  double zoom_level = 1.1;

  content::HostZoomMap* host_zoom_map =
      content::HostZoomMap::GetDefaultForBrowserContext(profile());
  host_zoom_map->SetZoomLevelForHost(host, zoom_level);
  ValidateZoom(host, "122%", 1U);

  base::ListValue args;
  handler()->HandleFetchZoomLevels(&args);
  ValidateZoom(host, "122%", 2U);

  args.AppendString("http://www.google.com");
  handler()->HandleRemoveZoomLevel(&args);
  ValidateZoom("", "", 3U);

  double default_level = host_zoom_map->GetDefaultZoomLevel();
  double level = host_zoom_map->GetZoomLevelForHostAndScheme("http", host);
  EXPECT_EQ(default_level, level);
}

class SiteSettingsHandlerInfobarTest : public BrowserWithTestWindowTest {
 public:
  SiteSettingsHandlerInfobarTest()
      : kNotifications(site_settings::ContentSettingsTypeToGroupName(
            CONTENT_SETTINGS_TYPE_NOTIFICATIONS)) {}

  void SetUp() override {
    BrowserWithTestWindowTest::SetUp();
    handler_ = std::make_unique<SiteSettingsHandler>(profile());
    handler()->set_web_ui(web_ui());
    handler()->AllowJavascript();
    web_ui()->ClearTrackedCalls();

    window2_ = base::WrapUnique(CreateBrowserWindow());
    browser2_ = base::WrapUnique(
        CreateBrowser(profile(), browser()->type(), false, window2_.get()));

    extensions::TestExtensionSystem* extension_system =
        static_cast<extensions::TestExtensionSystem*>(
            extensions::ExtensionSystem::Get(profile()));
    extension_system->CreateExtensionService(
        base::CommandLine::ForCurrentProcess(), base::FilePath(), false);
  }

  void TearDown() override {
    // SiteSettingsHandler maintains a HostZoomMap::Subscription internally, so
    // make sure that's cleared before BrowserContext / profile destruction.
    handler()->DisallowJavascript();

    // Also destroy |browser2_| before the profile. browser()'s destruction is
    // handled in BrowserWithTestWindowTest::TearDown().
    browser2()->tab_strip_model()->CloseAllTabs();
    browser2_.reset();
    BrowserWithTestWindowTest::TearDown();
  }

  InfoBarService* GetInfobarServiceForTab(Browser* browser,
                                          int tab_index,
                                          GURL* tab_url) {
    content::WebContents* web_contents =
        browser->tab_strip_model()->GetWebContentsAt(tab_index);
    if (tab_url)
      *tab_url = web_contents->GetLastCommittedURL();
    return InfoBarService::FromWebContents(web_contents);
  }

  content::TestWebUI* web_ui() { return &web_ui_; }

  SiteSettingsHandler* handler() { return handler_.get(); }

  Browser* browser2() { return browser2_.get(); }

  const std::string kNotifications;

 private:
  content::TestWebUI web_ui_;
  std::unique_ptr<SiteSettingsHandler> handler_;
  std::unique_ptr<BrowserWindow> window2_;
  std::unique_ptr<Browser> browser2_;

  DISALLOW_COPY_AND_ASSIGN(SiteSettingsHandlerInfobarTest);
};

TEST_F(SiteSettingsHandlerInfobarTest, SettingPermissionsTriggersInfobar) {
  // Note all GURLs starting with 'origin' below belong to the same origin.
  //               _____  _______________  ________  ________  ___________
  //   Window 1:  / foo \' origin_anchor \' chrome \' origin \' extension \
  // -------------       -----------------------------------------------------
  std::string origin_anchor_string =
      "https://www.example.com/with/path/blah#heading";
  const GURL foo("http://foo");
  const GURL origin_anchor(origin_anchor_string);
  const GURL chrome("chrome://about");
  const GURL origin("https://www.example.com/");
  const GURL extension(
      "chrome-extension://fooooooooooooooooooooooooooooooo/bar.html");

  // Make sure |extension|'s extension ID exists before navigating to it. This
  // fixes a test timeout that occurs with --enable-browser-side-navigation on.
  scoped_refptr<const extensions::Extension> test_extension =
      extensions::ExtensionBuilder("Test")
          .SetID("fooooooooooooooooooooooooooooooo")
          .Build();
  extensions::ExtensionSystem::Get(profile())
      ->extension_service()
      ->AddExtension(test_extension.get());

  //               __________  ______________  ___________________  _______
  //   Window 2:  / insecure '/ origin_query \' example_subdomain \' about \
  // -------------------------                --------------------------------
  const GURL insecure("http://www.example.com/");
  const GURL origin_query("https://www.example.com/?param=value");
  const GURL example_subdomain("https://subdomain.example.com/");
  const GURL about(url::kAboutBlankURL);

  // Set up. Note AddTab() adds tab at index 0, so add them in reverse order.
  AddTab(browser(), extension);
  AddTab(browser(), origin);
  AddTab(browser(), chrome);
  AddTab(browser(), origin_anchor);
  AddTab(browser(), foo);
  for (int i = 0; i < browser()->tab_strip_model()->count(); ++i) {
    EXPECT_EQ(0u,
              GetInfobarServiceForTab(browser(), i, nullptr)->infobar_count());
  }

  AddTab(browser2(), about);
  AddTab(browser2(), example_subdomain);
  AddTab(browser2(), origin_query);
  AddTab(browser2(), insecure);
  for (int i = 0; i < browser2()->tab_strip_model()->count(); ++i) {
    EXPECT_EQ(0u,
              GetInfobarServiceForTab(browser2(), i, nullptr)->infobar_count());
  }

  // Block notifications.
  base::ListValue set_args;
  set_args.AppendString(origin_anchor_string);
  {
    auto category_list = std::make_unique<base::ListValue>();
    category_list->AppendString(kNotifications);
    set_args.Append(std::move(category_list));
  }
  set_args.AppendString(
      content_settings::ContentSettingToString(CONTENT_SETTING_BLOCK));
  handler()->HandleSetOriginPermissions(&set_args);

  // Make sure all tabs belonging to the same origin as |origin_anchor| have an
  // infobar shown.
  GURL tab_url;
  for (int i = 0; i < browser()->tab_strip_model()->count(); ++i) {
    if (i == /*origin_anchor=*/1 || i == /*origin=*/3) {
      EXPECT_EQ(
          1u, GetInfobarServiceForTab(browser(), i, &tab_url)->infobar_count());
      EXPECT_TRUE(url::IsSameOriginWith(origin, tab_url));
    } else {
      EXPECT_EQ(
          0u, GetInfobarServiceForTab(browser(), i, &tab_url)->infobar_count());
      EXPECT_FALSE(url::IsSameOriginWith(origin, tab_url));
    }
  }
  for (int i = 0; i < browser2()->tab_strip_model()->count(); ++i) {
    if (i == /*origin_query=*/1) {
      EXPECT_EQ(
          1u,
          GetInfobarServiceForTab(browser2(), i, &tab_url)->infobar_count());
      EXPECT_TRUE(url::IsSameOriginWith(origin, tab_url));
    } else {
      EXPECT_EQ(
          0u,
          GetInfobarServiceForTab(browser2(), i, &tab_url)->infobar_count());
      EXPECT_FALSE(url::IsSameOriginWith(origin, tab_url));
    }
  }

  // Navigate the |foo| tab to the same origin as |origin_anchor|, and the
  // |origin_query| tab to a different origin.
  const GURL origin_path("https://www.example.com/path/to/page.html");
  content::NavigationController* foo_controller =
      &browser()
           ->tab_strip_model()
           ->GetWebContentsAt(/*foo=*/0)
           ->GetController();
  NavigateAndCommit(foo_controller, origin_path);

  const GURL example_without_www("https://example.com/");
  content::NavigationController* origin_query_controller =
      &browser2()
           ->tab_strip_model()
           ->GetWebContentsAt(/*origin_query=*/1)
           ->GetController();
  NavigateAndCommit(origin_query_controller, example_without_www);

  // Reset all permissions.
  base::ListValue reset_args;
  reset_args.AppendString(origin_anchor_string);
  auto category_list = std::make_unique<base::ListValue>();
  category_list->AppendString(kNotifications);
  reset_args.Append(std::move(category_list));
  reset_args.AppendString(
      content_settings::ContentSettingToString(CONTENT_SETTING_DEFAULT));
  handler()->HandleSetOriginPermissions(&reset_args);

  // Check the same tabs (plus the tab navigated to |origin_path|) still have
  // infobars showing.
  for (int i = 0; i < browser()->tab_strip_model()->count(); ++i) {
    if (i == /*origin_path=*/0 || i == /*origin_anchor=*/1 ||
        i == /*origin=*/3) {
      EXPECT_EQ(
          1u, GetInfobarServiceForTab(browser(), i, &tab_url)->infobar_count());
      EXPECT_TRUE(url::IsSameOriginWith(origin, tab_url));
    } else {
      EXPECT_EQ(
          0u, GetInfobarServiceForTab(browser(), i, &tab_url)->infobar_count());
      EXPECT_FALSE(url::IsSameOriginWith(origin, tab_url));
    }
  }
  // The infobar on the original |origin_query| tab (which has now been
  // navigated to |example_without_www|) should disappear.
  for (int i = 0; i < browser2()->tab_strip_model()->count(); ++i) {
    EXPECT_EQ(
        0u, GetInfobarServiceForTab(browser2(), i, &tab_url)->infobar_count());
    EXPECT_FALSE(url::IsSameOriginWith(origin, tab_url));
  }

  // Make sure it's the correct infobar that's being shown.
  EXPECT_EQ(infobars::InfoBarDelegate::PAGE_INFO_INFOBAR_DELEGATE,
            GetInfobarServiceForTab(browser(), /*origin_path=*/0, &tab_url)
                ->infobar_at(0)
                ->delegate()
                ->GetIdentifier());
  EXPECT_TRUE(url::IsSameOriginWith(origin, tab_url));
}

TEST_F(SiteSettingsHandlerTest, SessionOnlyException) {
  const std::string google_with_port("https://www.google.com:443");
  const std::string uma_base("WebsiteSettings.Menu.PermissionChanged");
  base::ListValue set_args;
  set_args.AppendString(google_with_port);  // Primary pattern.
  set_args.AppendString(google_with_port);  // Secondary pattern.
  set_args.AppendString(kCookies);
  set_args.AppendString(
      content_settings::ContentSettingToString(CONTENT_SETTING_SESSION_ONLY));
  set_args.AppendBoolean(false);  // Incognito.
  base::HistogramTester histograms;
  handler()->HandleSetCategoryPermissionForPattern(&set_args);
  EXPECT_EQ(1U, web_ui()->call_data().size());
  histograms.ExpectTotalCount(uma_base, 1);
  histograms.ExpectTotalCount(uma_base + ".SessionOnly", 1);
}

}  // namespace settings
