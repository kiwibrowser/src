// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_APP_TESTS_HOOK_H_
#define IOS_CHROME_APP_TESTS_HOOK_H_

namespace tests_hook {

// Returns true if ContentSuggestions should be disabled to allow other tests to
// run unimpeded.
bool DisableContentSuggestions();

// Returns true if contextual search should be disabled to allow other tests
// to run unimpeded.
bool DisableContextualSearch();

// Returns true if the first_run path should be disabled to allow other tests to
// run unimpeded.
bool DisableFirstRun();

// Returns true if the geolocation should be disabled to avoid the user location
// prompt displaying for the omnibox.
bool DisableGeolocation();

// Returns true if the signin recall promo should be disabled to allow other
// tests to run unimpeded.
bool DisableSigninRecallPromo();

// Returns true if the update service should be disabled so that the update
// infobar won't be shown during testing.
bool DisableUpdateService();

// TODO(crbug.com/800266): Removes this hook.
// Returns true if the first phase of the UI refresh will be displayed,
// overriding the flag value.
bool ForceUIRefreshPhase1();

// Global integration tests setup.  This is not used by EarlGrey-based
// integration tests.
void SetUpTestsIfPresent();

// Runs the integration tests.  This is not used by EarlGrey-based integration
// tests.
void RunTestsIfPresent();

}  // namespace tests_hook

#endif  // IOS_CHROME_APP_TESTS_HOOK_H_
