// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview A helper object used by the welcome page to interact with
 * the browser.
 */

cr.define('welcome', function() {

  /** @interface */
  class WelcomeBrowserProxy {
    handleActivateSignIn() {}
    handleUserDecline() {}
  }

  /** @implements {welcome.WelcomeBrowserProxy} */
  class WelcomeBrowserProxyImpl {
    /** @override */
    handleActivateSignIn() {
      chrome.send('handleActivateSignIn');
    }

    /** @override */
    handleUserDecline() {
      chrome.send('handleUserDecline');
    }
  }

  cr.addSingletonGetter(WelcomeBrowserProxyImpl);

  return {
    WelcomeBrowserProxy: WelcomeBrowserProxy,
    WelcomeBrowserProxyImpl: WelcomeBrowserProxyImpl,
  };
});
