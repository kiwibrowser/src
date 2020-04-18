// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'settings-account-manager' is the settings subpage containing controls to
 * list, add and delete Secondary Google Accounts.
 */

/**
 * Information for an account managed by Chrome OS AccountManager.
 * @typedef {{
 *   fullName: string,
 *   email: string,
 *   pic: string,
 * }}
 */
let Account;

Polymer({
  is: 'settings-account-manager',

  behaviors: [
    I18nBehavior,
  ],

  properties: {
    /**
     * List of Accounts.
     * @type {!Array<Account>}
     */
    accounts_: {
      type: Array,
      value: function() {
        return [];
      },
    },
  },

  /** @override */
  ready: function() {
    cr.sendWithPromise('getAccounts').then(accounts => {
      this.set('accounts_', accounts);
    });
  },

  /**
   * @param {string} iconUrl
   * @return {string} A CSS image-set for multiple scale factors.
   * @private */
  getIconImageSet_: function(iconUrl) {
    return cr.icon.getImage(iconUrl);
  },
});
