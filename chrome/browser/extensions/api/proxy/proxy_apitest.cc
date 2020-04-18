// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_switches.h"
#include "components/prefs/pref_service.h"
#include "components/proxy_config/proxy_config_dictionary.h"
#include "components/proxy_config/proxy_config_pref_names.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/test_management_policy.h"
#include "extensions/common/extension.h"
#include "extensions/test/result_catcher.h"

namespace extensions {

namespace {

const char kNoServer[] = "";
const char kNoBypass[] = "";
const char kNoPac[] = "";

}  // namespace

class ProxySettingsApiTest : public ExtensionApiTest {
 public:
  ProxySettingsApiTest() {}

 protected:
  void ValidateSettings(int expected_mode,
                        const std::string& expected_server,
                        const std::string& bypass,
                        const std::string& expected_pac_url,
                        PrefService* pref_service) {
    const PrefService::Preference* pref =
        pref_service->FindPreference(proxy_config::prefs::kProxy);
    ASSERT_TRUE(pref != NULL);
    EXPECT_TRUE(pref->IsExtensionControlled());

    ProxyConfigDictionary dict(
        pref_service->GetDictionary(proxy_config::prefs::kProxy)
            ->CreateDeepCopy());

    ProxyPrefs::ProxyMode mode;
    ASSERT_TRUE(dict.GetMode(&mode));
    EXPECT_EQ(expected_mode, mode);

    std::string value;
    if (!bypass.empty()) {
       ASSERT_TRUE(dict.GetBypassList(&value));
       EXPECT_EQ(bypass, value);
     } else {
       EXPECT_FALSE(dict.GetBypassList(&value));
     }

    if (!expected_pac_url.empty()) {
       ASSERT_TRUE(dict.GetPacUrl(&value));
       EXPECT_EQ(expected_pac_url, value);
     } else {
       EXPECT_FALSE(dict.GetPacUrl(&value));
     }

    if (!expected_server.empty()) {
      ASSERT_TRUE(dict.GetProxyServer(&value));
      EXPECT_EQ(expected_server, value);
    } else {
      EXPECT_FALSE(dict.GetProxyServer(&value));
    }
  }

  void ExpectNoSettings(PrefService* pref_service) {
    const PrefService::Preference* pref =
        pref_service->FindPreference(proxy_config::prefs::kProxy);
    ASSERT_TRUE(pref != NULL);
    EXPECT_FALSE(pref->IsExtensionControlled());
  }

  bool SetIsIncognitoEnabled(bool enabled) {
    ResultCatcher catcher;
    extensions::util::SetIsIncognitoEnabled(
        GetSingleLoadedExtension()->id(), browser()->profile(), enabled);
    if (!catcher.GetNextResult()) {
      message_ = catcher.message();
      return false;
    }
    return true;
  }

  extensions::ManagementPolicy* GetManagementPolicy() {
    return ExtensionSystem::Get(profile())->management_policy();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ProxySettingsApiTest);
};

// Tests direct connection settings.
IN_PROC_BROWSER_TEST_F(ProxySettingsApiTest, ProxyDirectSettings) {
  ASSERT_TRUE(RunExtensionTestIncognito("proxy/direct")) << message_;
  const Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension);

  PrefService* pref_service = browser()->profile()->GetPrefs();
  ValidateSettings(ProxyPrefs::MODE_DIRECT, kNoServer, kNoBypass, kNoPac,
                   pref_service);

  // As the extension is executed with incognito permission, the settings
  // should propagate to incognito mode.
  pref_service = browser()->profile()->GetOffTheRecordProfile()->GetPrefs();
  ValidateSettings(ProxyPrefs::MODE_DIRECT, kNoServer, kNoBypass, kNoPac,
                   pref_service);
}

// Tests that proxy settings are changed appropriately when the extension is
// disabled or enabled.
IN_PROC_BROWSER_TEST_F(ProxySettingsApiTest, SettingsChangeOnDisableEnable) {
  ASSERT_TRUE(RunExtensionTestIncognito("proxy/direct")) << message_;
  const Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension);

  PrefService* pref_service = browser()->profile()->GetPrefs();
  ValidateSettings(ProxyPrefs::MODE_DIRECT, kNoServer, kNoBypass, kNoPac,
                   pref_service);

  DisableExtension(extension->id());
  ExpectNoSettings(pref_service);

  EnableExtension(extension->id());
  ValidateSettings(ProxyPrefs::MODE_DIRECT, kNoServer, kNoBypass, kNoPac,
                   pref_service);
}

// Tests that proxy settings corresponding to an extension are removed when
// the extension is uninstalled.
IN_PROC_BROWSER_TEST_F(ProxySettingsApiTest, SettingsRemovedOnUninstall) {
  ASSERT_TRUE(RunExtensionTestIncognito("proxy/direct")) << message_;
  const Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension);

  PrefService* pref_service = browser()->profile()->GetPrefs();
  ValidateSettings(ProxyPrefs::MODE_DIRECT, kNoServer, kNoBypass, kNoPac,
                   pref_service);

  UninstallExtension(extension->id());
  ExpectNoSettings(pref_service);
}

// Tests that proxy settings corresponding to an extension are removed when
// the extension is blacklisted by management policy. Regression test for
// crbug.com/709264.
IN_PROC_BROWSER_TEST_F(ProxySettingsApiTest,
                       PRE_SettingsRemovedOnPolicyBlacklist) {
  ASSERT_TRUE(RunExtensionTestIncognito("proxy/direct")) << message_;
  const Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension);

  PrefService* pref_service = browser()->profile()->GetPrefs();
  ValidateSettings(ProxyPrefs::MODE_DIRECT, kNoServer, kNoBypass, kNoPac,
                   pref_service);

  GetManagementPolicy()->UnregisterAllProviders();
  extensions::TestManagementPolicyProvider provider(
      extensions::TestManagementPolicyProvider::PROHIBIT_LOAD);
  GetManagementPolicy()->RegisterProvider(&provider);

  // Run the policy check.
  extension_service()->CheckManagementPolicy();
  ExpectNoSettings(pref_service);

  // Remove the extension from policy blacklist. It should get enabled again.
  GetManagementPolicy()->UnregisterAllProviders();
  extension_service()->CheckManagementPolicy();
  ValidateSettings(ProxyPrefs::MODE_DIRECT, kNoServer, kNoBypass, kNoPac,
                   pref_service);

  // Block the extension again for the next test.
  GetManagementPolicy()->RegisterProvider(&provider);
  extension_service()->CheckManagementPolicy();
  ExpectNoSettings(pref_service);
}

// Tests that proxy settings corresponding to an extension take effect again
// on browser restart, when the extension is removed from the policy blacklist.
IN_PROC_BROWSER_TEST_F(ProxySettingsApiTest, SettingsRemovedOnPolicyBlacklist) {
  PrefService* pref_service = browser()->profile()->GetPrefs();
  ValidateSettings(ProxyPrefs::MODE_DIRECT, kNoServer, kNoBypass, kNoPac,
                   pref_service);
}

// Tests auto-detect settings.
IN_PROC_BROWSER_TEST_F(ProxySettingsApiTest, ProxyAutoSettings) {
  ASSERT_TRUE(RunExtensionTestIncognito("proxy/auto")) << message_;
  const Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension);

  PrefService* pref_service = browser()->profile()->GetPrefs();
  ValidateSettings(ProxyPrefs::MODE_AUTO_DETECT, kNoServer, kNoBypass, kNoPac,
                   pref_service);
}

// Tests PAC proxy settings.
IN_PROC_BROWSER_TEST_F(ProxySettingsApiTest, ProxyPacScript) {
  ASSERT_TRUE(RunExtensionTest("proxy/pac")) << message_;
  const Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension);

  PrefService* pref_service = browser()->profile()->GetPrefs();
  ValidateSettings(ProxyPrefs::MODE_PAC_SCRIPT, kNoServer, kNoBypass,
                   "http://wpad/windows.pac", pref_service);

  // As the extension is not executed with incognito permission, the settings
  // should not propagate to incognito mode.
  pref_service = browser()->profile()->GetOffTheRecordProfile()->GetPrefs();
  ExpectNoSettings(pref_service);

  // Now we enable the extension in incognito mode and verify that settings
  // are applied.
  ASSERT_TRUE(SetIsIncognitoEnabled(true));
  ValidateSettings(ProxyPrefs::MODE_PAC_SCRIPT, kNoServer, kNoBypass,
                   "http://wpad/windows.pac", pref_service);

  // Disabling incognito permission should revoke the settings for incognito
  // mode.
  ASSERT_TRUE(SetIsIncognitoEnabled(false));
  ExpectNoSettings(pref_service);
}

// Tests PAC proxy settings.
IN_PROC_BROWSER_TEST_F(ProxySettingsApiTest, ProxyPacDataUrl) {
  ASSERT_TRUE(RunExtensionTest("proxy/pacdataurl")) << message_;
  const Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension);
  const char url[] =
       "data:;base64,ZnVuY3Rpb24gRmluZFByb3h5R"
       "m9yVVJMKHVybCwgaG9zdCkgewogIGlmIChob3N0ID09ICdmb29iYXIuY29tJykKICAgIHJl"
       "dHVybiAnUFJPWFkgYmxhY2tob2xlOjgwJzsKICByZXR1cm4gJ0RJUkVDVCc7Cn0=";
  PrefService* pref_service = browser()->profile()->GetPrefs();
  ValidateSettings(ProxyPrefs::MODE_PAC_SCRIPT, kNoServer, kNoBypass,
                   url, pref_service);
}

// Tests PAC proxy settings.
IN_PROC_BROWSER_TEST_F(ProxySettingsApiTest, ProxyPacData) {
  ASSERT_TRUE(RunExtensionTest("proxy/pacdata")) << message_;
  const Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension);
  const char url[] =
      "data:application/x-ns-proxy-autoconfig;base64,ZnVuY3Rpb24gRmluZFByb3h5R"
      "m9yVVJMKHVybCwgaG9zdCkgewogIGlmIChob3N0ID09ICdmb29iYXIuY29tJykKICAgIHJl"
      "dHVybiAnUFJPWFkgYmxhY2tob2xlOjgwJzsKICByZXR1cm4gJ0RJUkVDVCc7Cn0=";
  PrefService* pref_service = browser()->profile()->GetPrefs();
  ValidateSettings(ProxyPrefs::MODE_PAC_SCRIPT, kNoServer, kNoBypass,
                   url, pref_service);
}

// Tests setting a single proxy to cover all schemes.
IN_PROC_BROWSER_TEST_F(ProxySettingsApiTest, ProxyFixedSingle) {
  ASSERT_TRUE(RunExtensionTest("proxy/single")) << message_;
  const Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension);

  PrefService* pref_service = browser()->profile()->GetPrefs();
  ValidateSettings(ProxyPrefs::MODE_FIXED_SERVERS,
                 "127.0.0.1:100",
                 kNoBypass,
                 kNoPac,
                 pref_service);
}

// Tests setting to use the system's proxy settings.
IN_PROC_BROWSER_TEST_F(ProxySettingsApiTest, ProxySystem) {
  ASSERT_TRUE(RunExtensionTest("proxy/system")) << message_;
  const Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension);

  PrefService* pref_service = browser()->profile()->GetPrefs();
  ValidateSettings(ProxyPrefs::MODE_SYSTEM, kNoServer, kNoBypass, kNoPac,
                   pref_service);
}

// Tests setting separate proxies for each scheme.
IN_PROC_BROWSER_TEST_F(ProxySettingsApiTest, ProxyFixedIndividual) {
  ASSERT_TRUE(RunExtensionTestIncognito("proxy/individual")) << message_;
  const Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension);

  PrefService* pref_service = browser()->profile()->GetPrefs();
  ValidateSettings(ProxyPrefs::MODE_FIXED_SERVERS,
                   "http=quic://1.1.1.1:443;"
                       "https=2.2.2.2:80;"  // http:// is pruned.
                       "ftp=3.3.3.3:9000;"  // http:// is pruned.
                       "socks=socks4://4.4.4.4:9090",
                   kNoBypass,
                   kNoPac,
                   pref_service);

  // Now check the incognito preferences.
  pref_service = browser()->profile()->GetOffTheRecordProfile()->GetPrefs();
  ValidateSettings(ProxyPrefs::MODE_FIXED_SERVERS,
                   "http=quic://1.1.1.1:443;"
                       "https=2.2.2.2:80;"
                       "ftp=3.3.3.3:9000;"
                       "socks=socks4://4.4.4.4:9090",
                   kNoBypass,
                   kNoPac,
                   pref_service);
}

// Tests setting values only for incognito mode
IN_PROC_BROWSER_TEST_F(ProxySettingsApiTest,
                       ProxyFixedIndividualIncognitoOnly) {
  ASSERT_TRUE(RunExtensionTestIncognito("proxy/individual_incognito_only")) <<
      message_;
  const Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension);

  PrefService* pref_service = browser()->profile()->GetPrefs();
  ExpectNoSettings(pref_service);

  // Now check the incognito preferences.
  pref_service = browser()->profile()->GetOffTheRecordProfile()->GetPrefs();
  ValidateSettings(ProxyPrefs::MODE_FIXED_SERVERS,
                   "http=1.1.1.1:80;"
                       "https=socks5://2.2.2.2:1080;"
                       "ftp=3.3.3.3:9000;"
                       "socks=socks4://4.4.4.4:9090",
                   kNoBypass,
                   kNoPac,
                   pref_service);
}

// Tests setting values also for incognito mode
// Test disabled due to http://crbug.com/88972.
IN_PROC_BROWSER_TEST_F(ProxySettingsApiTest,
                       DISABLED_ProxyFixedIndividualIncognitoAlso) {
  ASSERT_TRUE(RunExtensionTestIncognito("proxy/individual_incognito_also")) <<
      message_;
  const Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension);

  PrefService* pref_service = browser()->profile()->GetPrefs();
  ValidateSettings(ProxyPrefs::MODE_FIXED_SERVERS,
                   "http=1.1.1.1:80;"
                       "https=socks5://2.2.2.2:1080;"
                       "ftp=3.3.3.3:9000;"
                       "socks=socks4://4.4.4.4:9090",
                   kNoBypass,
                   kNoPac,
                   pref_service);

  // Now check the incognito preferences.
  pref_service = browser()->profile()->GetOffTheRecordProfile()->GetPrefs();
  ValidateSettings(ProxyPrefs::MODE_FIXED_SERVERS,
                   "http=5.5.5.5:80;"
                       "https=socks5://6.6.6.6:1080;"
                       "ftp=7.7.7.7:9000;"
                       "socks=socks4://8.8.8.8:9090",
                   kNoBypass,
                   kNoPac,
                   pref_service);
}

// Tests setting and unsetting values
IN_PROC_BROWSER_TEST_F(ProxySettingsApiTest, ProxyFixedIndividualRemove) {
  ASSERT_TRUE(RunExtensionTest("proxy/individual_remove")) << message_;
  const Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension);

  PrefService* pref_service = browser()->profile()->GetPrefs();
  ExpectNoSettings(pref_service);
}

IN_PROC_BROWSER_TEST_F(ProxySettingsApiTest,
    ProxyBypass) {
  ASSERT_TRUE(RunExtensionTestIncognito("proxy/bypass")) << message_;
  const Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension);

  PrefService* pref_service = browser()->profile()->GetPrefs();
  ValidateSettings(ProxyPrefs::MODE_FIXED_SERVERS,
                   "http=1.1.1.1:80",
                   "localhost,::1,foo.bar,<local>",
                   kNoPac,
                   pref_service);

  // Now check the incognito preferences.
  pref_service = browser()->profile()->GetOffTheRecordProfile()->GetPrefs();
  ValidateSettings(ProxyPrefs::MODE_FIXED_SERVERS,
                   "http=1.1.1.1:80",
                   "localhost,::1,foo.bar,<local>",
                   kNoPac,
                   pref_service);
}

// This test sets proxy to an inavalid host "does.not.exist" and then fetches
// a page from localhost, expecting an error since host is invalid.
// On ChromeOS, localhost is by default bypassed, so the page from localhost
// will be fetched successfully, resulting in no error.  Hence this test
// shouldn't run on ChromeOS.
#if defined(OS_CHROMEOS)
#define MAYBE_ProxyEventsInvalidProxy DISABLED_ProxyEventsInvalidProxy
#else
#define MAYBE_ProxyEventsInvalidProxy ProxyEventsInvalidProxy
#endif  // defined(OS_CHROMEOS)

// Tests error events: invalid proxy
IN_PROC_BROWSER_TEST_F(ProxySettingsApiTest, MAYBE_ProxyEventsInvalidProxy) {
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(
      RunExtensionSubtest("proxy/events", "invalid_proxy.html")) << message_;
}

// Tests error events: PAC script parse error.
IN_PROC_BROWSER_TEST_F(ProxySettingsApiTest, ProxyEventsParseError) {
  ASSERT_TRUE(
      RunExtensionSubtest("proxy/events", "parse_error.html")) << message_;
}

}  // namespace extensions
