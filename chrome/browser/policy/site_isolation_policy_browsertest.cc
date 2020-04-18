// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "build/build_config.h"
#include "chrome/browser/chrome_content_browser_client.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/site_instance.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "url/gurl.h"

class SiteIsolationPolicyBrowserTest : public InProcessBrowserTest {
 protected:
  SiteIsolationPolicyBrowserTest() {}

  struct Expectations {
    const char* url;
    bool isolated;
  };

  void CheckExpectations(Expectations* expectations, size_t count) {
    content::BrowserContext* context = browser()->profile();
    for (size_t i = 0; i < count; ++i) {
      const GURL url(expectations[i].url);
      auto instance = content::SiteInstance::CreateForURL(context, url);
      EXPECT_EQ(expectations[i].isolated, instance->RequiresDedicatedProcess());
    }
  }

  policy::MockConfigurationPolicyProvider provider_;

 private:
  DISALLOW_COPY_AND_ASSIGN(SiteIsolationPolicyBrowserTest);
};

template <bool policy_value>
class SitePerProcessPolicyBrowserTest : public SiteIsolationPolicyBrowserTest {
 protected:
  SitePerProcessPolicyBrowserTest() {}

  void SetUpInProcessBrowserTestFixture() override {
    // We setup the policy here, because the policy must be 'live' before
    // the renderer is created, since the value for this policy is passed
    // to the renderer via a command-line. Setting the policy in the test
    // itself or in SetUpOnMainThread works for update-able policies, but
    // is too late for this one.
    EXPECT_CALL(provider_, IsInitializationComplete(testing::_))
        .WillRepeatedly(testing::Return(true));
    policy::BrowserPolicyConnector::SetPolicyProviderForTesting(&provider_);

    policy::PolicyMap values;
    values.Set(policy::key::kSitePerProcess, policy::POLICY_LEVEL_MANDATORY,
               policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_CLOUD,
               std::make_unique<base::Value>(policy_value), nullptr);
    provider_.UpdateChromePolicy(values);

    // Append the automation switch which should disable Site Isolation when the
    // "WebDriverOverridesIncompatiblePolicies" is set. This is tested in the
    // WebDriverSitePerProcessPolicyBrowserTest class below.
    // NOTE: This flag is on for some tests per default but we still force it
    // it here to make sure to avoid possible regressions being missed
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        switches::kEnableAutomation);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(SitePerProcessPolicyBrowserTest);
};

typedef SitePerProcessPolicyBrowserTest<true>
    SitePerProcessPolicyBrowserTestEnabled;
typedef SitePerProcessPolicyBrowserTest<false>
    SitePerProcessPolicyBrowserTestDisabled;

class IsolateOriginsPolicyBrowserTest : public SiteIsolationPolicyBrowserTest {
 protected:
  IsolateOriginsPolicyBrowserTest() {}

  void SetUpInProcessBrowserTestFixture() override {
    // We setup the policy here, because the policy must be 'live' before
    // the renderer is created, since the value for this policy is passed
    // to the renderer via a command-line. Setting the policy in the test
    // itself or in SetUpOnMainThread works for update-able policies, but
    // is too late for this one.
    EXPECT_CALL(provider_, IsInitializationComplete(testing::_))
        .WillRepeatedly(testing::Return(true));
    policy::BrowserPolicyConnector::SetPolicyProviderForTesting(&provider_);

    policy::PolicyMap values;
    values.Set(policy::key::kIsolateOrigins, policy::POLICY_LEVEL_MANDATORY,
               policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_CLOUD,
               std::make_unique<base::Value>(
                   "https://example.org/,http://example.com"),
               nullptr);
    provider_.UpdateChromePolicy(values);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(IsolateOriginsPolicyBrowserTest);
};

class WebDriverSitePerProcessPolicyBrowserTest
    : public SitePerProcessPolicyBrowserTestEnabled {
 protected:
  WebDriverSitePerProcessPolicyBrowserTest()
      : are_sites_isolated_for_testing_(false) {}

  void SetUpInProcessBrowserTestFixture() override {
    // First take note if tests are running in site isolated environment as this
    // will change the outcome of the test. We can't just call this method after
    // the call to the base setup method because setting the Site Isolation
    // policy is indistinguishable from setting the the command line flag
    // directly.
#if defined(OFFICIAL_BUILD)
    // Official builds still default to no site isolation (i.e. official builds
    // are not covered by testing/variations/fieldtrial_testing_config.json).
    // See also https://crbug.com/836261.
    are_sites_isolated_for_testing_ = false;
#else
    // Otherwise, site-per-process is turned on by default, via field trial
    // configured with testing/variations/fieldtrial_testing_config.json.
    // The only exception is the not_site_per_process_browser_tests step run on
    // some trybots - in this step the --disable-site-isolation-trials flag
    // counteracts the effects of fieldtrial_testing_config.json.
    are_sites_isolated_for_testing_ =
        !base::CommandLine::ForCurrentProcess()->HasSwitch(
            switches::kDisableSiteIsolationTrials);
#endif

    // We setup the policy here, because the policy must be 'live' before the
    // renderer is created, since the value for this policy is passed to the
    // renderer via a command-line. Setting the policy in the test itself or in
    // SetUpOnMainThread works for update-able policies, but is too late for
    // this one.
    SitePerProcessPolicyBrowserTest::SetUpInProcessBrowserTestFixture();

    policy::PolicyMap values;
    values.Set(policy::key::kWebDriverOverridesIncompatiblePolicies,
               policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
               policy::POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(true),
               nullptr);
    provider_.UpdateChromePolicy(values);
  }

  bool are_sites_isolated_for_testing_;

 private:
  DISALLOW_COPY_AND_ASSIGN(WebDriverSitePerProcessPolicyBrowserTest);
};

// Ensure that --disable-site-isolation-trials does not override policies.
class NoOverrideSitePerProcessPolicyBrowserTest
    : public SitePerProcessPolicyBrowserTestEnabled {
 protected:
  NoOverrideSitePerProcessPolicyBrowserTest() {}
  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(switches::kDisableSiteIsolationTrials);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(NoOverrideSitePerProcessPolicyBrowserTest);
};

IN_PROC_BROWSER_TEST_F(SitePerProcessPolicyBrowserTestEnabled, Simple) {
  Expectations expectations[] = {
      {"https://foo.com/noodles.html", true},
      {"http://foo.com/", true},
      {"http://example.org/pumpkins.html", true},
  };
  CheckExpectations(expectations, arraysize(expectations));
}

IN_PROC_BROWSER_TEST_F(IsolateOriginsPolicyBrowserTest, Simple) {
  // Skip this test if all sites are isolated.
  if (content::AreAllSitesIsolatedForTesting())
    return;

  Expectations expectations[] = {
      {"https://foo.com/noodles.html", false},
      {"http://foo.com/", false},
      {"https://example.org/pumpkins.html", true},
      {"http://example.com/index.php", true},
  };
  CheckExpectations(expectations, arraysize(expectations));
}

IN_PROC_BROWSER_TEST_F(WebDriverSitePerProcessPolicyBrowserTest, Simple) {
  Expectations expectations[] = {
      {"https://foo.com/noodles.html", are_sites_isolated_for_testing_},
      {"http://example.org/pumpkins.html", are_sites_isolated_for_testing_},
  };
  CheckExpectations(expectations, arraysize(expectations));
}

IN_PROC_BROWSER_TEST_F(NoOverrideSitePerProcessPolicyBrowserTest, Simple) {
  Expectations expectations[] = {
      {"https://foo.com/noodles.html", true},
      {"http://example.org/pumpkins.html", true},
  };
  CheckExpectations(expectations, arraysize(expectations));
}

class SitePerProcessPolicyBrowserTestFieldTrialTest
    : public SitePerProcessPolicyBrowserTestDisabled {
 public:
  SitePerProcessPolicyBrowserTestFieldTrialTest() {
    scoped_feature_list_.InitAndEnableFeature(features::kSitePerProcess);
  }
  ~SitePerProcessPolicyBrowserTestFieldTrialTest() override {}

 private:
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(SitePerProcessPolicyBrowserTestFieldTrialTest);
};

IN_PROC_BROWSER_TEST_F(SitePerProcessPolicyBrowserTestFieldTrialTest, Simple) {
  // Skip this test if all sites are isolated.
  if (content::AreAllSitesIsolatedForTesting())
    return;
  ASSERT_TRUE(base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kDisableSiteIsolationTrials));
  Expectations expectations[] = {
      {"https://foo.com/noodles.html", false},
      {"http://example.org/pumpkins.html", false},
  };
  CheckExpectations(expectations, arraysize(expectations));
}

// https://crbug.com/833423: The test is incompatible with the
// not_site_per_process_browser_tests step on the trybots.
#if defined(OS_LINUX)
#define MAYBE_NoPolicyNoTrialsFlags DISABLED_NoPolicyNoTrialsFlags
#else
#define MAYBE_NoPolicyNoTrialsFlags NoPolicyNoTrialsFlags
#endif
IN_PROC_BROWSER_TEST_F(SiteIsolationPolicyBrowserTest,
                       MAYBE_NoPolicyNoTrialsFlags) {
  ASSERT_FALSE(base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kDisableSiteIsolationTrials));
}
