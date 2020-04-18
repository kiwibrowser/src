// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/contextual_search/touch_to_search_permissions_mediator.h"

#import <UIKit/UIKit.h>
#include <memory>

#include "base/command_line.h"
#include "base/metrics/field_trial.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#import "base/test/ios/wait_util.h"
#include "base/time/time.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data.h"
#include "components/search_engines/template_url_service.h"
#include "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#include "ios/chrome/browser/pref_names.h"
#include "ios/chrome/browser/search_engines/template_url_service_factory.h"
#include "ios/chrome/browser/sync/ios_chrome_profile_sync_service_factory.h"
#include "ios/chrome/browser/sync/ios_chrome_profile_sync_test_util.h"
#include "ios/chrome/browser/sync/sync_setup_service_factory.h"
#include "ios/chrome/browser/sync/sync_setup_service_mock.h"
#include "ios/chrome/browser/ui/contextual_search/switches.h"
#import "ios/chrome/browser/ui/contextual_search/touch_to_search_permissions_mediator+testing.h"
#include "ios/web/public/test/test_web_thread_bundle.h"
#include "net/base/network_change_notifier.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#include "third_party/ocmock/gtest_support.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using testing::Return;

@interface TestTouchToSearchPermissionsAudience
    : NSObject<TouchToSearchPermissionsChangeAudience>
@property BOOL updated;
@end

@implementation TestTouchToSearchPermissionsAudience
@synthesize updated = _updated;
- (void)touchToSearchPermissionsUpdated {
  self.updated = YES;
}
@end

namespace {

std::unique_ptr<KeyedService> CreateSyncSetupService(
    web::BrowserState* context) {
  ios::ChromeBrowserState* chrome_browser_state =
      ios::ChromeBrowserState::FromBrowserState(context);
  syncer::SyncService* sync_service =
      IOSChromeProfileSyncServiceFactory::GetForBrowserState(
          chrome_browser_state);
  return std::make_unique<SyncSetupServiceMock>(
      sync_service, chrome_browser_state->GetPrefs());
}

// Override NetworkChangeNotifier to simulate connection type changes for tests.
class TestNetworkChangeNotifier : public net::NetworkChangeNotifier {
 public:
  TestNetworkChangeNotifier()
      : net::NetworkChangeNotifier(),
        connection_type_to_return_(
            net::NetworkChangeNotifier::CONNECTION_UNKNOWN) {}

  // Simulates a change of the connection type to |type|. This will notify any
  // objects that are NetworkChangeNotifiers.
  void SimulateNetworkConnectionChange(
      net::NetworkChangeNotifier::ConnectionType type) {
    connection_type_to_return_ = type;
    net::NetworkChangeNotifier::NotifyObserversOfConnectionTypeChange();
    base::RunLoop().RunUntilIdle();
  }

 private:
  ConnectionType GetCurrentConnectionType() const override {
    return connection_type_to_return_;
  }

  // The currently simulated network connection type. If this is set to
  // CONNECTION_NONE, then NetworkChangeNotifier::IsOffline will return true.
  net::NetworkChangeNotifier::ConnectionType connection_type_to_return_;

  DISALLOW_COPY_AND_ASSIGN(TestNetworkChangeNotifier);
};

class TouchToSearchPermissionsMediatorTest : public PlatformTest {
 protected:
  TouchToSearchPermissionsMediatorTest() {
    TestChromeBrowserState::Builder browserStateBuilder;
    browserStateBuilder.AddTestingFactory(
        SyncSetupServiceFactory::GetInstance(), &CreateSyncSetupService);
    browserStateBuilder.AddTestingFactory(
        ios::TemplateURLServiceFactory::GetInstance(),
        ios::TemplateURLServiceFactory::GetDefaultFactory());
    browser_state_ = browserStateBuilder.Build();
    template_url_service_ =
        ios::TemplateURLServiceFactory::GetForBrowserState(BrowserState());
    template_url_service_->Load();
    tts_permissions_ = [[TouchToSearchPermissionsMediator alloc]
        initWithBrowserState:BrowserState()];
  }

  ios::ChromeBrowserState* BrowserState() { return browser_state_.get(); }

  web::TestWebThreadBundle thread_bundle_;
  std::unique_ptr<TestChromeBrowserState> browser_state_;
  TouchToSearchPermissionsMediator* tts_permissions_;
  TemplateURLService* template_url_service_;
};

TEST_F(TouchToSearchPermissionsMediatorTest, PrefStates) {
  // Expect empty browser state to have undecided pref value.
  EXPECT_EQ(TouchToSearch::UNDECIDED, [tts_permissions_ preferenceState]);

  const struct {
    const std::string pref_value;
    TouchToSearch::TouchToSearchPreferenceState state;
  } tests[] = {
      {"false", TouchToSearch::DISABLED},
      {"true", TouchToSearch::ENABLED},
      {"", TouchToSearch::UNDECIDED},
  };

  for (const auto& test : tests) {
    // set state, test value
    BrowserState()->GetPrefs()->SetString(prefs::kContextualSearchEnabled,
                                          test.pref_value);
    EXPECT_EQ(test.state, [tts_permissions_ preferenceState]);
    // set state from value
    [tts_permissions_ setPreferenceState:test.state];
    std::string prefValue =
        BrowserState()->GetPrefs()->GetString(prefs::kContextualSearchEnabled);
    EXPECT_EQ(test.pref_value, prefValue);
  }

// For non-debug builds, also test cases that would DCHECK.
#if !DCHECK_IS_ON()
  // Check that an unexpected pref value returns DISABLED.
  BrowserState()->GetPrefs()->SetString(prefs::kContextualSearchEnabled,
                                        "somethingElse");
  EXPECT_EQ(TouchToSearch::DISABLED, [tts_permissions_ preferenceState]);
#endif
}

TEST_F(TouchToSearchPermissionsMediatorTest, TapContext) {
  const GURL secure_url("https://www.some-website.com/");
  const GURL insecure_url("http://www.some-website.com/");
  const struct {
    TouchToSearch::TouchToSearchPreferenceState state;
    const GURL* url;
    BOOL expected_permission;
  } tests[] = {
      // DISABLED should always deny extraction.
      // ENABLED should permit it.
      // UNDECIDED should permit it only on http pages.
      {TouchToSearch::DISABLED, &secure_url, NO},
      {TouchToSearch::DISABLED, &insecure_url, NO},
      {TouchToSearch::ENABLED, &secure_url, YES},
      {TouchToSearch::ENABLED, &insecure_url, YES},
      {TouchToSearch::UNDECIDED, &secure_url, NO},
      {TouchToSearch::UNDECIDED, &insecure_url, YES},
  };

  for (const auto& test : tests) {
    [tts_permissions_ setPreferenceState:test.state];
    EXPECT_EQ(test.expected_permission,
              [tts_permissions_ canExtractTapContextForURL:*test.url]);
  }
}

TEST_F(TouchToSearchPermissionsMediatorTest, URLSending) {
  // Wrap the tests of -canSendPageURLs states in contexts of the different
  // pref states; DISABLED should mean that the expected result is false
  // regardless of other tests.
  const struct {
    TouchToSearch::TouchToSearchPreferenceState prefState;
    BOOL performs_check;
  } state_contexts[] = {
      {TouchToSearch::DISABLED, NO},
      {TouchToSearch::UNDECIDED, YES},
      {TouchToSearch::ENABLED, YES},
  };

  const struct {
    bool sync_enabled;
    BOOL expected_permission;
  } tests[] = {
      {false, NO}, {true, YES},
  };

  for (const auto& context : state_contexts) {
    [tts_permissions_ setPreferenceState:context.prefState];
    for (const auto& test : tests) {
      SyncSetupServiceMock* syncSetupService =
          static_cast<SyncSetupServiceMock*>(
              SyncSetupServiceFactory::GetForBrowserState(BrowserState()));
      if (context.performs_check) {
        EXPECT_CALL(*syncSetupService, IsSyncEnabled())
            .WillOnce(Return(test.sync_enabled));
      }
      EXPECT_EQ(test.expected_permission && context.performs_check,
                [tts_permissions_ canSendPageURLs]);
    }
  }
}

TEST_F(TouchToSearchPermissionsMediatorTest, SearchPreloading) {
  TestNetworkChangeNotifier network_change_notifier;

  const struct {
    TouchToSearch::TouchToSearchPreferenceState prefState;
    BOOL expectation_mask;
  } state_contexts[] = {
      {TouchToSearch::DISABLED, NO},
      {TouchToSearch::UNDECIDED, YES},
      {TouchToSearch::ENABLED, YES},
  };

  const struct {
    bool prefetch_enabled;
    bool wifi_prefetch_only;
    net::NetworkChangeNotifier::ConnectionType network_connection_type;
    BOOL expected_permission;
  } tests[] = {
      // clang-format off
      // If prefectching is off, network preloading should never be allowed.
      {false, false, net::NetworkChangeNotifier::CONNECTION_WIFI, NO},
      {false, false, net::NetworkChangeNotifier::CONNECTION_3G,   NO},
      {false, true,  net::NetworkChangeNotifier::CONNECTION_WIFI, NO},
      {false, true,  net::NetworkChangeNotifier::CONNECTION_3G,   NO},

      // If prefetching is on and not wifi-only, preloading should always be OK.
      {true,  false, net::NetworkChangeNotifier::CONNECTION_WIFI, YES},
      {true,  false, net::NetworkChangeNotifier::CONNECTION_3G,   YES},

      // Wifi only prefetching should not allow prefetch over cellular.
      {true,  true,  net::NetworkChangeNotifier::CONNECTION_WIFI, YES},
      {true,  true,  net::NetworkChangeNotifier::CONNECTION_3G,   NO},

      // clang-format on
  };

  for (const auto& context : state_contexts) {
    [tts_permissions_ setPreferenceState:context.prefState];
    for (const auto& test : tests) {
      BrowserState()->GetPrefs()->SetBoolean(prefs::kNetworkPredictionEnabled,
                                             test.prefetch_enabled);
      BrowserState()->GetPrefs()->SetBoolean(prefs::kNetworkPredictionWifiOnly,
                                             test.wifi_prefetch_only);
      network_change_notifier.SimulateNetworkConnectionChange(
          test.network_connection_type);
      EXPECT_EQ(test.expected_permission && context.expectation_mask,
                [tts_permissions_ canPreloadSearchResults]);
    }
  }
}

TEST_F(TouchToSearchPermissionsMediatorTest,
       AreQueriesDisallowedWithoutTemplateService) {
  TestChromeBrowserState::Builder browserStateBuilder;
  std::unique_ptr<TestChromeBrowserState> browser_state(
      browserStateBuilder.Build());
  TouchToSearchPermissionsMediator* tts_permissions =
      [[TouchToSearchPermissionsMediator alloc]
          initWithBrowserState:browser_state.get()];
  EXPECT_FALSE([tts_permissions areContextualSearchQueriesSupported]);
}

TEST_F(TouchToSearchPermissionsMediatorTest, AreQueriesAllowed) {
  // The initial default search engine should support contextual search.
  EXPECT_TRUE([tts_permissions_ areContextualSearchQueriesSupported]);

  // Make a user-defined search engine and set it as the default.
  TemplateURLData data;
  data.SetShortName(base::ASCIIToUTF16("cs-not-supported"));

  TemplateURL* newUrl = template_url_service_->Add(
      std::unique_ptr<TemplateURL>(new TemplateURL(data)));
  // |template_url_service_| will take ownership of newUrl
  ASSERT_TRUE(newUrl);
  template_url_service_->SetUserSelectedDefaultSearchProvider(newUrl);

  // The newly-selected search engine doesn't support contextual search.
  EXPECT_FALSE([tts_permissions_ areContextualSearchQueriesSupported]);
}

TEST_F(TouchToSearchPermissionsMediatorTest, CanEnable) {
  MockTouchToSearchPermissionsMediator* permissions =
      [[MockTouchToSearchPermissionsMediator alloc]
          initWithBrowserState:BrowserState()];

  const struct {
    BOOL enabled_on_device;
    TouchToSearch::TouchToSearchPreferenceState prefState;
    BOOL voiceover_enabled;
    BOOL queries_allowed;
    BOOL expect_available;
  } tests[] = {
      // clang-format off
      // Enabled cases:
      // Enabled on command line, voiceover off, queries supported,
      // preference state not disabled.
      {YES, TouchToSearch::ENABLED,   NO,  YES, YES},
      {YES, TouchToSearch::UNDECIDED, NO,  YES, YES},

      // Disabled cases: All others.
      {NO,  TouchToSearch::DISABLED,  NO,  NO,  NO},
      {NO,  TouchToSearch::DISABLED,  NO,  YES, NO},
      {NO,  TouchToSearch::DISABLED,  YES, NO,  NO},
      {NO,  TouchToSearch::DISABLED,  YES, YES, NO},
      {NO,  TouchToSearch::UNDECIDED, NO,  NO,  NO},
      {NO,  TouchToSearch::UNDECIDED, NO,  YES, NO},
      {NO,  TouchToSearch::UNDECIDED, YES, NO,  NO},
      {NO,  TouchToSearch::UNDECIDED, YES, YES, NO},
      {NO,  TouchToSearch::ENABLED,   NO,  NO,  NO},
      {NO,  TouchToSearch::ENABLED,   NO,  YES, NO},
      {NO,  TouchToSearch::ENABLED,   YES, NO,  NO},
      {NO,  TouchToSearch::ENABLED,   YES, YES, NO},
      {YES, TouchToSearch::DISABLED,  NO,  NO,  NO},
      {YES, TouchToSearch::DISABLED,  NO,  YES, NO},
      {YES, TouchToSearch::DISABLED,  YES, NO,  NO},
      {YES, TouchToSearch::DISABLED,  YES, YES, NO},
      {YES, TouchToSearch::UNDECIDED, NO,  NO,  NO},
      {YES, TouchToSearch::UNDECIDED, YES, NO,  NO},
      {YES, TouchToSearch::UNDECIDED, YES, YES, NO},
      {YES, TouchToSearch::ENABLED,   NO,  NO,  NO},
      {YES, TouchToSearch::ENABLED,   YES, NO,  NO},
      {YES, TouchToSearch::ENABLED,   YES, YES, NO},
      // clang-format on
  };

  for (const auto& test : tests) {
    [[permissions class]
        setIsTouchToSearchAvailableOnDevice:test.enabled_on_device];
    permissions.preferenceState = test.prefState;
    permissions.isVoiceOverEnabled = test.voiceover_enabled;
    permissions.areContextualSearchQueriesSupported = test.queries_allowed;
    EXPECT_EQ(test.expect_available, [permissions canEnable]);
  }
}

TEST_F(TouchToSearchPermissionsMediatorTest, AudienceNotifications) {
  id audience = [[TestTouchToSearchPermissionsAudience alloc] init];
  [tts_permissions_ setAudience:audience];
  base::TimeDelta delay = base::TimeDelta::FromMilliseconds(50);
  [[NSNotificationCenter defaultCenter]
      postNotificationName:UIApplicationWillEnterForegroundNotification
                    object:[UIApplication sharedApplication]];
  // Audience call is asynchronous, so expect nothing yet.
  EXPECT_FALSE([audience updated]);
  // Wait for async call.
  base::test::ios::WaitUntilCondition(
      ^bool(void) {
        return [audience updated];
      },
      false, delay);
  EXPECT_TRUE([audience updated]);
  // Reset |audience|.
  [audience setUpdated:NO];

  [[NSNotificationCenter defaultCenter]
      postNotificationName:UIAccessibilityVoiceOverStatusChanged
                    object:nil];
  // Audience call is asynchronous, so expect nothing yet.
  EXPECT_FALSE([audience updated]);
  // Wait for async call.
  base::test::ios::WaitUntilCondition(^bool(void) {
    return [audience updated];
  });
  EXPECT_TRUE([audience updated]);
  // Reset |audience|.
  [audience setUpdated:NO];

  BrowserState()->GetPrefs()->SetString(prefs::kContextualSearchEnabled,
                                        "true");
  // Audience call is asynchronous, so expect nothing yet.
  EXPECT_FALSE([audience updated]);
  // Wait for async call.
  base::test::ios::WaitUntilCondition(^bool(void) {
    return [audience updated];
  });
  EXPECT_TRUE([audience updated]);
  // Reset |audience|.
  [audience setUpdated:NO];

  // Make a user-defined search engine and set it as the default. This should
  // trigger a prefs update that will in turn trigger an audience notification.
  TemplateURLData data;
  data.SetShortName(base::ASCIIToUTF16("cs-not-supported"));
  TemplateURL* newUrl = template_url_service_->Add(
      std::unique_ptr<TemplateURL>(new TemplateURL(data)));

  // |template_url_service_| will take ownership of newUrl
  ASSERT_TRUE(newUrl);
  template_url_service_->SetUserSelectedDefaultSearchProvider(newUrl);
  // Audience call is asynchronous, so expect nothing yet.
  EXPECT_FALSE([audience updated]);
  // Wait for async call.
  base::test::ios::WaitUntilCondition(
      ^bool(void) {
        return [audience updated];
      },
      false, delay);
  EXPECT_TRUE([audience updated]);
  // Reset |audience|.
  [audience setUpdated:NO];

  // Once set to nil, audience shouldn't be notified.
  [tts_permissions_ setAudience:nil];
  [[NSNotificationCenter defaultCenter]
      postNotificationName:UIAccessibilityVoiceOverStatusChanged
                    object:nil];
  BrowserState()->GetPrefs()->SetString(prefs::kContextualSearchEnabled,
                                        "true");
  // Audience call is asynchronous, so expect nothing yet.
  EXPECT_FALSE([audience updated]);
  // Wait long enough for async call not to occur.
  base::test::ios::SpinRunLoopWithMaxDelay(delay);
  EXPECT_FALSE([audience updated]);
  // Reset |audience|.
  [audience setUpdated:NO];

  id audience2 = [[TestTouchToSearchPermissionsAudience alloc] init];
  // If the permissions object is destroyed, queued notifications should still
  // be sent.
  [tts_permissions_ setAudience:audience2];
  [[NSNotificationCenter defaultCenter]
      postNotificationName:UIAccessibilityVoiceOverStatusChanged
                    object:nil];
  tts_permissions_ = nil;
  base::test::ios::WaitUntilCondition(
      ^bool(void) {
        return [audience2 updated];
      },
      false, delay);
  EXPECT_TRUE([audience2 updated]);
}

TEST_F(TouchToSearchPermissionsMediatorTest, AudiencePrefsSynchronous) {
  id audience = [OCMockObject
      niceMockForProtocol:@protocol(TouchToSearchPermissionsChangeAudience)];
  [tts_permissions_ setAudience:audience];

  // Test that setting preferences through the same permissions object triggers
  // audience methods.
  [[audience expect]
      touchToSearchDidChangePreferenceState:TouchToSearch::ENABLED];
  [tts_permissions_ setPreferenceState:TouchToSearch::ENABLED];
  EXPECT_OCMOCK_VERIFY(audience);
  [tts_permissions_ setPreferenceState:TouchToSearch::DISABLED];

  // Test that setting preferences through another permissions object triggers
  // audience methods.
  TouchToSearchPermissionsMediator* other_permissions =
      [[TouchToSearchPermissionsMediator alloc]
          initWithBrowserState:BrowserState()];
  [[audience expect]
      touchToSearchDidChangePreferenceState:TouchToSearch::ENABLED];
  [other_permissions setPreferenceState:TouchToSearch::ENABLED];
  EXPECT_OCMOCK_VERIFY(audience);
  [tts_permissions_ setPreferenceState:TouchToSearch::DISABLED];

  // Test that setting preferences through the prefs system triggers audience
  // methods.
  [[audience expect]
      touchToSearchDidChangePreferenceState:TouchToSearch::ENABLED];
  BrowserState()->GetPrefs()->SetString(prefs::kContextualSearchEnabled,
                                        "true");
  EXPECT_OCMOCK_VERIFY(audience);
}

TEST_F(TouchToSearchPermissionsMediatorTest, OTR) {
  ios::ChromeBrowserState* otr_state =
      BrowserState()->GetOffTheRecordChromeBrowserState();
  TouchToSearchPermissionsMediator* permissions =
      [[TouchToSearchPermissionsMediator alloc] initWithBrowserState:otr_state];

  EXPECT_FALSE([permissions canEnable]);
  EXPECT_FALSE([permissions canSendPageURLs]);
  EXPECT_FALSE([permissions canEnable]);
  EXPECT_FALSE([permissions canPreloadSearchResults]);
  EXPECT_EQ(TouchToSearch::DISABLED, [permissions preferenceState]);

  otr_state->GetPrefs()->SetString(prefs::kContextualSearchEnabled, "true");
  EXPECT_EQ(TouchToSearch::DISABLED, [permissions preferenceState]);
  EXPECT_FALSE([permissions canEnable]);

  EXPECT_EQ(nil, [permissions audience]);
  id audience = [OCMockObject
      niceMockForProtocol:@protocol(TouchToSearchPermissionsChangeAudience)];
  [permissions setAudience:audience];
  EXPECT_EQ(nil, [permissions audience]);
}

TEST_F(TouchToSearchPermissionsMediatorTest, AudienceRemovedNotifications) {
  @autoreleasepool {
    id audience = [[TestTouchToSearchPermissionsAudience alloc] init];
    [tts_permissions_ setAudience:audience];
    EXPECT_TRUE([tts_permissions_ observing]);
    audience = nil;
  }
  // Permissions shouldn't be observing after notifying a nil audience.
  [[NSNotificationCenter defaultCenter]
      postNotificationName:UIAccessibilityVoiceOverStatusChanged
                    object:nil];
  EXPECT_FALSE([tts_permissions_ observing]);

  // Permissions shouldn't observe while still observing.
  id audience = [[TestTouchToSearchPermissionsAudience alloc] init];
  [tts_permissions_ setAudience:audience];
  audience = nil;
  audience = [[TestTouchToSearchPermissionsAudience alloc] init];
  [tts_permissions_ setAudience:audience];
}

#pragma mark - Unit tests for availability class method.

using TouchToSearchPermissionsAvailabilityTest = PlatformTest;

TEST_F(TouchToSearchPermissionsAvailabilityTest, CommandLinePermissions) {
  const struct {
    bool set_disable;
    bool set_enable;
    bool expect_available;
  } tests[] = {
      {true, false, false},
      {true, true, false},
      {false, false, false},
      {false, true, true},
  };

  for (const auto& test : tests) {
    // Reset all flags.
    base::CommandLine::ForCurrentProcess()->InitFromArgv(0, NULL);
    if (test.set_disable) {
      base::CommandLine::ForCurrentProcess()->AppendSwitch(
          contextual_search::switches::kDisableContextualSearch);
    }
    if (test.set_enable) {
      base::CommandLine::ForCurrentProcess()->AppendSwitch(
          contextual_search::switches::kEnableContextualSearch);
    }
    EXPECT_EQ(
        [TouchToSearchPermissionsMediator isTouchToSearchAvailableOnDevice],
        test.expect_available);
  }
}

TEST_F(TouchToSearchPermissionsAvailabilityTest, FieldTrial) {
  // Field trial support is not currently supported, so it is expected
  // that under all field trial configs, the feature will remain disabled.
  // If field trial support is added back in, this test should be updated.
  const struct {
    const std::string trial_group_name;
    bool expect_available;
  } tests[] = {
      {"Enabled", false},
      {"Disabled", false},
      {"Control", false},
      {"Spadoinkle", false},
  };

  // Reset all flags.
  base::CommandLine::ForCurrentProcess()->InitFromArgv(0, NULL);

  for (const auto& test : tests) {
    base::FieldTrialList field_trial_list(nullptr);
    if (!test.trial_group_name.empty()) {
      base::FieldTrialList::CreateFieldTrial("ContextualSearchIOS",
                                             test.trial_group_name);
    }
    EXPECT_EQ(
        [TouchToSearchPermissionsMediator isTouchToSearchAvailableOnDevice],
        test.expect_available);
  }
}

#pragma mark - Unit tests for mock class

using MockTouchToSearchPermissionsTest = PlatformTest;

TEST_F(MockTouchToSearchPermissionsTest, Mocking) {
  MockTouchToSearchPermissionsMediator* scoped_permissions =
      [[MockTouchToSearchPermissionsMediator alloc]
          initWithBrowserState:nullptr];
  MockTouchToSearchPermissionsMediator* permissions = scoped_permissions;

  const GURL test_urls[] = {
      GURL("https://www.some-website.com/"),
      GURL("http://www.some-website.com/"), GURL(""),
  };

  // Default expectations from mocked methods.
  EXPECT_TRUE([[permissions class] isTouchToSearchAvailableOnDevice]);
  EXPECT_TRUE([permissions canSendPageURLs]);
  EXPECT_TRUE([permissions canPreloadSearchResults]);
  EXPECT_FALSE([permissions isVoiceOverEnabled]);
  EXPECT_TRUE([permissions areContextualSearchQueriesSupported]);
  for (const auto& url : test_urls)
    EXPECT_TRUE([permissions canExtractTapContextForURL:url]);

  // Set non-default values:
  for (const BOOL value : {NO, YES, NO, YES}) {
    [[permissions class] setIsTouchToSearchAvailableOnDevice:value];
    EXPECT_EQ(value, [[permissions class] isTouchToSearchAvailableOnDevice]);
    permissions.canSendPageURLs = value;
    EXPECT_EQ(value, [permissions canSendPageURLs]);
    permissions.canPreloadSearchResults = value;
    EXPECT_EQ(value, [permissions canPreloadSearchResults]);
    permissions.isVoiceOverEnabled = value;
    EXPECT_EQ(value, [permissions isVoiceOverEnabled]);
    permissions.areContextualSearchQueriesSupported = value;
    EXPECT_EQ(value, [permissions areContextualSearchQueriesSupported]);
    permissions.canExtractTapContextForAllURLs = value;
    for (const auto& url : test_urls) {
      EXPECT_EQ(value, [permissions canExtractTapContextForURL:url]);
    }
  }

  const TouchToSearch::TouchToSearchPreferenceState states[] = {
      TouchToSearch::DISABLED, TouchToSearch::UNDECIDED, TouchToSearch::ENABLED,
  };

  for (const auto& state : states) {
    permissions.preferenceState = state;
    EXPECT_EQ(state, permissions.preferenceState);
  }
}

}  // namespace
