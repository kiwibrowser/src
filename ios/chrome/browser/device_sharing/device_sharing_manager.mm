// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/device_sharing/device_sharing_manager.h"

#include <memory>

#import "components/handoff/handoff_manager.h"
#include "components/handoff/pref_names_ios.h"
#include "components/prefs/ios/pref_observer_bridge.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface DeviceSharingManager ()<PrefObserverDelegate> {
  ios::ChromeBrowserState* _browserState;  // weak

  // Bridge to listen to pref changes to the active browser state.
  std::unique_ptr<PrefObserverBridge> _browserStatePrefObserverBridge;

  // Registrar for pref change notifications to the active browser state.
  std::unique_ptr<PrefChangeRegistrar> _browserStatePrefChangeRegistrar;

  // Responsible for maintaining all state related to the Handoff feature.
  HandoffManager* _handoffManager;
}

// If handoff is enabled for the active browser state, then this method ensures
// that a handoff manager is active. Destroys the handoff manager otherwise.
- (void)updateHandoffManager;

// Returns an auto-released instance of |HandoffManager|. Factory method
// required for unit testing.
+ (HandoffManager*)createHandoffManager;

@end

@implementation DeviceSharingManager

- (void)updateBrowserState:(ios::ChromeBrowserState*)state {
  DCHECK(!state || !state->IsOffTheRecord());
  if (_browserState == state) {
    return;
  }

  _browserState = state;
  if (!_browserState) {
    _browserStatePrefObserverBridge.reset();
    _browserStatePrefChangeRegistrar.reset();
  } else {
    // Add a listener to changes to the preferences related to device sharing.
    _browserStatePrefChangeRegistrar.reset(new PrefChangeRegistrar);
    _browserStatePrefChangeRegistrar->Init(_browserState->GetPrefs());
    _browserStatePrefObserverBridge.reset(new PrefObserverBridge(self));
    _browserStatePrefObserverBridge->ObserveChangesForPreference(
        prefs::kIosHandoffToOtherDevices,
        _browserStatePrefChangeRegistrar.get());
  }
  [self updateHandoffManager];
  [self updateActiveURL:GURL()];
}

- (void)updateActiveURL:(const GURL&)activeURL {
  [_handoffManager updateActiveURL:activeURL];
}

- (void)updateHandoffManager {
  BOOL handoffEnabled =
      _browserState &&
      _browserState->GetPrefs()->GetBoolean(prefs::kIosHandoffToOtherDevices);
  if (!handoffEnabled) {
    _handoffManager = nil;
    return;
  }

  if (!_handoffManager)
    _handoffManager = [[self class] createHandoffManager];
}

+ (HandoffManager*)createHandoffManager {
  return [[HandoffManager alloc] init];
}

#pragma mark - PrefObserverDelegate

- (void)onPreferenceChanged:(const std::string&)preferenceName {
  if (preferenceName == prefs::kIosHandoffToOtherDevices) {
    [self updateHandoffManager];
  }
}

@end

@implementation DeviceSharingManager (TestingOnly)

- (HandoffManager*)handoffManager {
  return _handoffManager;
}

@end
