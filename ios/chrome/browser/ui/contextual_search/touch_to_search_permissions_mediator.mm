// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/contextual_search/touch_to_search_permissions_mediator.h"

#import <UIKit/UIKit.h>

#include "base/command_line.h"
#include "base/logging.h"
#import "components/prefs/ios/pref_observer_bridge.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/template_url_service.h"
#include "ios/chrome/app/tests_hook.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/pref_names.h"
#include "ios/chrome/browser/search_engines/template_url_service_factory.h"
#include "ios/chrome/browser/sync/sync_setup_service.h"
#include "ios/chrome/browser/sync/sync_setup_service_factory.h"
#include "ios/chrome/browser/ui/contextual_search/switches.h"
#include "net/base/network_change_notifier.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Maps pref string values to state enum.
const struct {
  const char* value;
  TouchToSearch::TouchToSearchPreferenceState state;
} valueStateMappings[] = {
    {"false", TouchToSearch::DISABLED},
    {"true", TouchToSearch::ENABLED},
    {"", TouchToSearch::UNDECIDED},
};
}  // namespace

@interface TouchToSearchPermissionsMediator ()<PrefObserverDelegate> {
  ios::ChromeBrowserState* _browserState;
  SyncSetupService* _syncService;
  __weak NSObject<TouchToSearchPermissionsChangeAudience>* _audience;
  // Pref observer to track changes to the touch-to-search and search engine
  // prefs.
  std::unique_ptr<PrefObserverBridge> _prefObserverBridge;

  // Registrar for pref changes notifications.
  PrefChangeRegistrar _prefChangeRegistrar;
}

// YES if notifications are being observed.
@property(nonatomic, assign) BOOL observing;

// YES if the current configured search engine supports a contextual search
// query.
- (BOOL)areContextualSearchQueriesSupported;

// YES if VoiceOver is enabled (encapsulated into a method for testing).
- (BOOL)isVoiceOverEnabled;

@end

@implementation TouchToSearchPermissionsMediator

@synthesize observing = _observing;

+ (BOOL)isTouchToSearchAvailableOnDevice {
  // By default the feature is not available.
  // - If the enable flag (contextual_search::switches::kEnableContextualSearch)
  //   is flipped, then it is available.
  // - A disable switch (contextual_search::switches::kDisableContextualSearch)
  //   is also supported, although it is only useful when Finch experiments are
  //   also supported.

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          contextual_search::switches::kDisableContextualSearch)) {
    // If both enable and disable flags are present, disable wins.
    return NO;
  }

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          contextual_search::switches::kEnableContextualSearch)) {
    // Even if the command line flag is flipped, don't enable the feature if
    // test hooks disable it.
    return !tests_hook::DisableContextualSearch();
  }

  return NO;
}

- (instancetype)initWithBrowserState:(ios::ChromeBrowserState*)browserState {
  if ((self = [super init])) {
    if (browserState && browserState->IsOffTheRecord()) {
      // Discard the allocated object and return a nil object.
      return nil;
    }
    [self setUpBrowserState:browserState];
  }
  return self;
}

- (void)dealloc {
  // Set audience to nil to stop observation.
  self.audience = nil;
}

- (void)setUpBrowserState:(ios::ChromeBrowserState*)browserState {
  DCHECK(browserState);
  _browserState = browserState;
  _syncService = SyncSetupServiceFactory::GetForBrowserState(_browserState);
}

#pragma mark - Property implementation

- (TouchToSearch::TouchToSearchPreferenceState)preferenceState {
  std::string prefValue =
      _browserState->GetPrefs()->GetString(prefs::kContextualSearchEnabled);
  for (const auto& mapping : valueStateMappings) {
    if (mapping.value == prefValue)
      return mapping.state;
  }
  NOTREACHED() << "Unexpected preference value (" << prefValue << ") for "
               << prefs::kContextualSearchEnabled;
  return TouchToSearch::DISABLED;
}

- (void)setPreferenceState:(TouchToSearch::TouchToSearchPreferenceState)state {
  for (const auto& mapping : valueStateMappings) {
    if (mapping.state == state) {
      _browserState->GetPrefs()->SetString(prefs::kContextualSearchEnabled,
                                           mapping.value);
      return;
    }
  }
  NOTREACHED() << "Unexpected preference state (" << state << ")";
}

- (NSObject<TouchToSearchPermissionsChangeAudience>*)audience {
  return _audience;
}

- (void)setAudience:
    (NSObject<TouchToSearchPermissionsChangeAudience>*)audience {
  [self stopObserving];
  _audience = audience;
  [self startObserving];
}

#pragma mark - Public methods

- (BOOL)canEnable {
  if (![[self class] isTouchToSearchAvailableOnDevice]) {
    return NO;
  } else if (self.preferenceState == TouchToSearch::DISABLED) {
    return NO;
  } else if ([self isVoiceOverEnabled]) {
    return NO;
  }

  return [self areContextualSearchQueriesSupported];
}

- (BOOL)canExtractTapContextForURL:(const GURL&)url {
  // The context surrounding a tap -- text on the page that wasn't tapped --
  // cannot be sent if (a) the page is https and (b) the user hasn't opted in
  // (they are undecided or disabled).
  if (url.SchemeIsCryptographic()) {
    // HTTPS means tap context can only be extracted if the user is enabled.
    return self.preferenceState == TouchToSearch::ENABLED;
  }
  // Non-HTTPS means tap context can be extracted if the user isn't disabled.
  return self.preferenceState != TouchToSearch::DISABLED;
}

- (BOOL)canSendPageURLs {
  // Never send URLs for disabled users.
  if (self.preferenceState == TouchToSearch::DISABLED)
    return NO;
  // Page URLs can only be sent if the user has otherwise given permission
  // to share their URLs with Google -- concretely, if they have sync enabled.
  return _syncService && _syncService->IsSyncEnabled();
}

- (BOOL)canPreloadSearchResults {
  // Disabled users cannot preload.
  if (self.preferenceState == TouchToSearch::DISABLED)
    return NO;

  // Only preload the search tab if the user has otherwise enabled preloading.
  // TODO(crbug.com/546216): Factor this and code in
  // chrome/browser/ui/preload_controller into a generic "preload watcher".
  PrefService* prefs = _browserState->GetPrefs();
  if (!prefs->GetBoolean(prefs::kNetworkPredictionEnabled))
    return NO;

  bool preloading_wifiOnly =
      prefs->GetBoolean(prefs::kNetworkPredictionWifiOnly);
  bool cellular = net::NetworkChangeNotifier::IsConnectionCellular(
      net::NetworkChangeNotifier::GetConnectionType());
  return !(preloading_wifiOnly && cellular);
}

#pragma mark - Private methods

- (BOOL)areContextualSearchQueriesSupported {
  TemplateURLService* templateUrlService =
      ios::TemplateURLServiceFactory::GetForBrowserState(_browserState);
  if (!templateUrlService)
    return NO;

  const TemplateURL* defaultURL =
      templateUrlService->GetDefaultSearchProvider();

  // Contextual search is supported if the template URL has a non-empty
  // contextual search URL.
  return !defaultURL->contextual_search_url().empty();
}

- (BOOL)isVoiceOverEnabled {
  return UIAccessibilityIsVoiceOverRunning();
}

- (void)startObserving {
  DCHECK(!self.observing);
  if (!self.audience)
    return;
  // Listen for foregrounding and VoiceOver change events.
  NSNotificationCenter* defaultCenter = [NSNotificationCenter defaultCenter];
  [defaultCenter addObserver:self
                    selector:@selector(statusMayHaveChangedWithNotification:)
                        name:UIApplicationWillEnterForegroundNotification
                      object:[UIApplication sharedApplication]];
  [defaultCenter addObserver:self
                    selector:@selector(statusMayHaveChangedWithNotification:)
                        name:UIAccessibilityVoiceOverStatusChanged
                      object:nil];

  // Track preference changes to get touch-to-search settings changes.
  _prefObserverBridge.reset(new PrefObserverBridge(self));
  _prefChangeRegistrar.Init(_browserState->GetPrefs());
  _prefObserverBridge->ObserveChangesForPreference(
      prefs::kContextualSearchEnabled, &_prefChangeRegistrar);
  _prefObserverBridge->ObserveChangesForPreference(
      DefaultSearchManager::kDefaultSearchProviderDataPrefName,
      &_prefChangeRegistrar);
  self.observing = YES;
}

- (void)stopObserving {
  if (!self.observing)
    return;
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  _prefChangeRegistrar.RemoveAll();
  self.observing = NO;
}

- (void)maybeNotifyAudienceOfPreferenceState {
  if (self.audience &&
      [self.audience
          respondsToSelector:@selector(
                                 touchToSearchDidChangePreferenceState:)]) {
    [self.audience touchToSearchDidChangePreferenceState:self.preferenceState];
  }
}

- (void)maybeNotifyAudienceAsynchronously {
  if (self.audience) {
    if ([self.audience
            respondsToSelector:@selector(touchToSearchPermissionsUpdated)]) {
      __weak NSObject<TouchToSearchPermissionsChangeAudience>* audience =
          self.audience;
      dispatch_async(dispatch_get_main_queue(), ^{
        [audience touchToSearchPermissionsUpdated];
      });
    }
  } else {
    // If audience is now nil, stop observing.
    [self stopObserving];
  }
}

#pragma mark - PrefObserverBridge methods

- (void)onPreferenceChanged:(const std::string&)preferenceName {
  if (preferenceName == prefs::kContextualSearchEnabled) {
    [self maybeNotifyAudienceOfPreferenceState];
  }

  if (preferenceName == prefs::kContextualSearchEnabled ||
      preferenceName ==
          DefaultSearchManager::kDefaultSearchProviderDataPrefName) {
    [self maybeNotifyAudienceAsynchronously];
  }
}

#pragma mark Notification Center handler

- (void)statusMayHaveChangedWithNotification:(NSNotification*)notification {
  // VoiceOver may have been enabled or disabled, so (if there is an audience
  // object), notify it asynchronously.
  [self maybeNotifyAudienceAsynchronously];
}

@end
