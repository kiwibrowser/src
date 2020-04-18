// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('settings', function() {
  /** @interface */
  class ChangePasswordBrowserProxy {
    /** Initialize the change password handler.*/
    initializeChangePasswordHandler() {}

    /**
     * Initiate the change password process. e.g., for Gmail users, it
     * navigates to accounts.google.com; for GSuite users, it navigates to the
     * corresponding change password URLs.
     */
    changePassword() {}
  }

  /**
   * @implements {settings.ChangePasswordBrowserProxy}
   */
  class ChangePasswordBrowserProxyImpl {
    /** @override */
    initializeChangePasswordHandler() {
      chrome.send('initializeChangePasswordHandler');
    }

    /** @override */
    changePassword() {
      chrome.send('changePassword');
    }
  }

  cr.addSingletonGetter(ChangePasswordBrowserProxyImpl);

  return {
    ChangePasswordBrowserProxy: ChangePasswordBrowserProxy,
    ChangePasswordBrowserProxyImpl: ChangePasswordBrowserProxyImpl,
  };
});
