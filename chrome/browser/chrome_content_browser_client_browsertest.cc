// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chrome_content_browser_client.h"

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/macros.h"
#include "base/sys_info.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/instant_service.h"
#include "chrome/browser/search/instant_service_factory.h"
#include "chrome/browser/search/search.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/search/instant_test_base.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/site_isolation_policy.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"

namespace content {

// Use a test class with SetUpCommandLine to ensure the flag is sent to the
// first renderer process.
class ChromeContentBrowserClientBrowserTest : public InProcessBrowserTest {
 public:
  ChromeContentBrowserClientBrowserTest() {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    IsolateAllSitesForTesting(command_line);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ChromeContentBrowserClientBrowserTest);
};

// Test that a basic navigation works in --site-per-process mode.  This prevents
// regressions when that mode calls out into the ChromeContentBrowserClient,
// such as http://crbug.com/164223.
IN_PROC_BROWSER_TEST_F(ChromeContentBrowserClientBrowserTest,
                       SitePerProcessNavigation) {
  ASSERT_TRUE(embedded_test_server()->Start());
  const GURL url(embedded_test_server()->GetURL("/title1.html"));

  ui_test_utils::NavigateToURL(browser(), url);
  NavigationEntry* entry = browser()
                               ->tab_strip_model()
                               ->GetWebContentsAt(0)
                               ->GetController()
                               .GetLastCommittedEntry();

  ASSERT_TRUE(entry != NULL);
  EXPECT_EQ(url, entry->GetURL());
  EXPECT_EQ(url, entry->GetVirtualURL());
}

// Helper class to mark "https://ntp.com/" as an isolated origin.
class IsolatedOriginNTPBrowserTest : public InProcessBrowserTest,
                                     public InstantTestBase {
 public:
  IsolatedOriginNTPBrowserTest() {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    ASSERT_TRUE(https_test_server().InitializeAndListen());

    // Mark ntp.com (with an appropriate port from the test server) as an
    // isolated origin.
    GURL isolated_url(https_test_server().GetURL("ntp.com", "/"));
    command_line->AppendSwitchASCII(switches::kIsolateOrigins,
                                    isolated_url.spec());
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    https_test_server().StartAcceptingConnections();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(IsolatedOriginNTPBrowserTest);
};

// Verifies that when the remote NTP URL has an origin which is also marked as
// an isolated origin (i.e., requiring a dedicated process), the NTP URL
// still loads successfully, and the resulting process is marked as an Instant
// process.  See https://crbug.com/755595.
IN_PROC_BROWSER_TEST_F(IsolatedOriginNTPBrowserTest,
                       IsolatedOriginDoesNotInterfereWithNTP) {
  GURL base_url =
      https_test_server().GetURL("ntp.com", "/instant_extended.html");
  GURL ntp_url =
      https_test_server().GetURL("ntp.com", "/instant_extended_ntp.html");
  InstantTestBase::Init(base_url, ntp_url, false);

  SetupInstant(browser());

  // Sanity check that a SiteInstance for a generic ntp.com URL requires a
  // dedicated process.
  content::BrowserContext* context = browser()->profile();
  GURL isolated_url(https_test_server().GetURL("ntp.com", "/title1.html"));
  scoped_refptr<SiteInstance> site_instance =
      SiteInstance::CreateForURL(context, isolated_url);
  EXPECT_TRUE(site_instance->RequiresDedicatedProcess());

  // The site URL for the NTP URL should resolve to a chrome-search:// URL via
  // GetEffectiveURL(), even if the NTP URL matches an isolated origin.
  GURL site_url(content::SiteInstance::GetSiteForURL(context, ntp_url));
  EXPECT_TRUE(site_url.SchemeIs(chrome::kChromeSearchScheme));

  // Navigate to the NTP URL and verify that the resulting process is marked as
  // an Instant process.
  ui_test_utils::NavigateToURL(browser(), ntp_url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  InstantService* instant_service =
      InstantServiceFactory::GetForProfile(browser()->profile());
  EXPECT_TRUE(instant_service->IsInstantProcess(
      contents->GetMainFrame()->GetProcess()->GetID()));

  // Navigating to a non-NTP URL on ntp.com should not result in an Instant
  // process.
  ui_test_utils::NavigateToURL(browser(), isolated_url);
  EXPECT_FALSE(instant_service->IsInstantProcess(
      contents->GetMainFrame()->GetProcess()->GetID()));
}

// Helper class to mark "https://ntp.com/" as an isolated origin.
class SitePerProcessMemoryThresholdBrowserTest : public InProcessBrowserTest {
 public:
  SitePerProcessMemoryThresholdBrowserTest() = default;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);

    // This way the test always sees the same amount of physical memory
    // (kLowMemoryDeviceThresholdMB = 512MB), regardless of how much memory is
    // available in the testing environment.
    command_line->AppendSwitch(switches::kEnableLowEndDeviceMode);
    EXPECT_EQ(512, base::SysInfo::AmountOfPhysicalMemoryMB());
  }

  // Some command-line switches override field trials - the tests need to be
  // skipped in this case.
  bool ShouldSkipBecauseOfConflictingCommandLineSwitches() {
    if (base::CommandLine::ForCurrentProcess()->HasSwitch(
            switches::kSitePerProcess))
      return true;

    if (base::CommandLine::ForCurrentProcess()->HasSwitch(
            switches::kDisableSiteIsolationTrials))
      return true;

    return false;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(SitePerProcessMemoryThresholdBrowserTest);
};

IN_PROC_BROWSER_TEST_F(SitePerProcessMemoryThresholdBrowserTest,
                       SitePerProcessEnabled_HighThreshold) {
  if (ShouldSkipBecauseOfConflictingCommandLineSwitches())
    return;

  // 512MB of physical memory that the test simulates is below the 768MB
  // threshold.
  base::test::ScopedFeatureList memory_feature;
  memory_feature.InitAndEnableFeatureWithParameters(
      features::kSitePerProcessOnlyForHighMemoryClients,
      {{features::kSitePerProcessOnlyForHighMemoryClientsParamName, "768"}});

  base::test::ScopedFeatureList site_per_process;
  site_per_process.InitAndEnableFeature(features::kSitePerProcess);

  // Despite enabled site-per-process trial, there should be no isolation
  // because the device has too little memory.
  EXPECT_FALSE(
      content::SiteIsolationPolicy::UseDedicatedProcessesForAllSites());
}

IN_PROC_BROWSER_TEST_F(SitePerProcessMemoryThresholdBrowserTest,
                       SitePerProcessEnabled_LowThreshold) {
  if (ShouldSkipBecauseOfConflictingCommandLineSwitches())
    return;

  // 512MB of physical memory that the test simulates is above the 128MB
  // threshold.
  base::test::ScopedFeatureList memory_feature;
  memory_feature.InitAndEnableFeatureWithParameters(
      features::kSitePerProcessOnlyForHighMemoryClients,
      {{features::kSitePerProcessOnlyForHighMemoryClientsParamName, "128"}});

  base::test::ScopedFeatureList site_per_process;
  site_per_process.InitAndEnableFeature(features::kSitePerProcess);

  // site-per-process trial is enabled, and the memory threshold is above the
  // memory present on the device.
  EXPECT_TRUE(content::SiteIsolationPolicy::UseDedicatedProcessesForAllSites());
}

IN_PROC_BROWSER_TEST_F(SitePerProcessMemoryThresholdBrowserTest,
                       SitePerProcessEnabled_NoThreshold) {
  if (ShouldSkipBecauseOfConflictingCommandLineSwitches())
    return;

  base::test::ScopedFeatureList site_per_process;
  site_per_process.InitAndEnableFeature(features::kSitePerProcess);

  EXPECT_TRUE(content::SiteIsolationPolicy::UseDedicatedProcessesForAllSites());
}

IN_PROC_BROWSER_TEST_F(SitePerProcessMemoryThresholdBrowserTest,
                       SitePerProcessDisabled_HighThreshold) {
  if (ShouldSkipBecauseOfConflictingCommandLineSwitches())
    return;

  // 512MB of physical memory that the test simulates is below the 768MB
  // threshold.
  base::test::ScopedFeatureList memory_feature;
  memory_feature.InitAndEnableFeatureWithParameters(
      features::kSitePerProcessOnlyForHighMemoryClients,
      {{features::kSitePerProcessOnlyForHighMemoryClientsParamName, "768"}});

  base::test::ScopedFeatureList site_per_process;
  site_per_process.InitAndDisableFeature(features::kSitePerProcess);

  // site-per-process trial is disabled, so isolation should be disabled
  // (i.e. the memory threshold should be ignored).
  EXPECT_FALSE(
      content::SiteIsolationPolicy::UseDedicatedProcessesForAllSites());
}

IN_PROC_BROWSER_TEST_F(SitePerProcessMemoryThresholdBrowserTest,
                       SitePerProcessDisabled_LowThreshold) {
  if (ShouldSkipBecauseOfConflictingCommandLineSwitches())
    return;

  // 512MB of physical memory that the test simulates is above the 128MB
  // threshold.
  base::test::ScopedFeatureList memory_feature;
  memory_feature.InitAndEnableFeatureWithParameters(
      features::kSitePerProcessOnlyForHighMemoryClients,
      {{features::kSitePerProcessOnlyForHighMemoryClientsParamName, "128"}});

  base::test::ScopedFeatureList site_per_process;
  site_per_process.InitAndDisableFeature(features::kSitePerProcess);

  // site-per-process trial is disabled, so isolation should be disabled
  // (i.e. the memory threshold should be ignored).
  EXPECT_FALSE(
      content::SiteIsolationPolicy::UseDedicatedProcessesForAllSites());
}

IN_PROC_BROWSER_TEST_F(SitePerProcessMemoryThresholdBrowserTest,
                       SitePerProcessDisabled_NoThreshold) {
  if (ShouldSkipBecauseOfConflictingCommandLineSwitches())
    return;

  base::test::ScopedFeatureList site_per_process;
  site_per_process.InitAndDisableFeature(features::kSitePerProcess);

  // site-per-process trial is disabled, so isolation should be disabled
  // (i.e. the memory threshold should be ignored).
  EXPECT_FALSE(
      content::SiteIsolationPolicy::UseDedicatedProcessesForAllSites());
}

}  // namespace content
