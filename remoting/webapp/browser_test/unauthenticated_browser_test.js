// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * @suppress {checkTypes,checkVars}
 *
 * Browser test to verify that an unauthenticated state is handled correctly
 * in the following situations:
 * 1. When launching the app.
 * 2. When refreshing the host-list.
 *
 * TODO(jamiewalch): Add a test case for connecting to a host.
 */

'use strict';

/** @constructor */
browserTest.Unauthenticated = function() {
};

browserTest.Unauthenticated.prototype.run = function(data) {
  remoting.MockIdentity.setActive(true);
  remoting.MockHostListApi.setActive(true);
  remoting.MockOAuth2Api.setActive(true);
  remoting.mockIdentity.setAccessToken(
      remoting.MockIdentity.AccessToken.NONE);
  remoting.startDesktopRemoting();

  browserTest.waitFor(
      browserTest.isVisible('auth-button')
  ).then(
      this.restoreSignedInState.bind(this)
  ).then(
      function() {
        console.log('waiting for enabled');
        return browserTest.waitFor(browserTest.isEnabled('get-started-me2me'));
      }
  ).then(
      // Show the Me2Me UI, invalidate the access token again, and refresh the
      // host-list to verify that the user is prompted to sign in again.
      function() {
        console.log('clicking get-started-me2me');
        browserTest.clickOnControl('get-started-me2me');
        remoting.mockIdentity.setAccessToken(
            remoting.MockIdentity.AccessToken.INVALID);
        browserTest.clickOnControl('host-list-reload');
        return browserTest.waitFor(
            browserTest.isVisible('host-list-refresh-failed-button'));
      }
  ).then(
      this.restoreSignedInState.bind(this)
      // TODO(jamiewalch): The sign-in button of the host-list doesn't work
      // for apps v2, because it assumes the v1 sign-in flow. Fix this and
      // extend this test to handle an authentication failure at this point.
  ).then(browserTest.pass, browserTest.fail);
};

// Set a valid access token, then re-authenticate. Note that we're not
// trying to test the sign-in flow here (that's tested elsewhere), we're
// just getting the app back into a signed-in state so that we can continue
// to test its behaviour if the token becomes invalid at various stages.
browserTest.Unauthenticated.prototype.restoreSignedInState = function() {
  remoting.mockIdentity.setAccessToken(
      remoting.MockIdentity.AccessToken.VALID);
  browserTest.clickOnControl('auth-button');
  return Promise.resolve();
};
