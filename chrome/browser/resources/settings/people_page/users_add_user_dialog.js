// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'settings-users-add-user-dialog' is the dialog shown for adding new allowed
 * users to a ChromeOS device.
 */
(function() {

/**
 * Regular expression for adding a user where the string provided is just
 * the part before the "@".
 * Email alias only, assuming it's a gmail address.
 *     e.g. 'john'
 * @type {!RegExp}
 */
const NAME_ONLY_REGEX =
    new RegExp('^\\s*([\\w\\.!#\\$%&\'\\*\\+-\\/=\\?\\^`\\{\\|\\}~]+)\\s*$');

/**
 * Regular expression for adding a user where the string provided is a full
 * email address.
 *     e.g. 'john@chromium.org'
 * @type {!RegExp}
 */
const EMAIL_REGEX = new RegExp(
    '^\\s*([\\w\\.!#\\$%&\'\\*\\+-\\/=\\?\\^`\\{\\|\\}~]+)@' +
    '([A-Za-z0-9\-]{2,63}\\..+)\\s*$');

Polymer({
  is: 'settings-users-add-user-dialog',

  properties: {
    /** @private */
    isValid_: {
      type: Boolean,
      value: false,
    },
  },

  open: function() {
    this.isValid_ = false;
    this.$.dialog.showModal();
  },

  /** @private */
  onCancelTap_: function() {
    this.$.dialog.cancel();
  },

  /**
   * Validates that the new user entered is valid.
   * @private
   * @return {boolean}
   */
  validate_: function() {
    const input = this.$.addUserInput.value;
    this.isValid_ = NAME_ONLY_REGEX.test(input) || EMAIL_REGEX.test(input);
    return this.isValid_;
  },

  /** @private */
  addUser_: function() {
    // May be submitted by the Enter key even if the input value is invalid.
    if (!this.validate_())
      return;

    const input = this.$.addUserInput.value;

    const nameOnlyMatches = NAME_ONLY_REGEX.exec(input);
    let userEmail;
    if (nameOnlyMatches) {
      userEmail = nameOnlyMatches[1] + '@gmail.com';
    } else {
      const emailMatches = EMAIL_REGEX.exec(input);
      // Assuming the input validated, one of these two must match.
      assert(emailMatches);
      userEmail = emailMatches[1] + '@' + emailMatches[2];
    }

    chrome.usersPrivate.addWhitelistedUser(
        userEmail,
        /* callback */ function(success) {});
    this.$.addUserInput.value = '';
    this.$.dialog.close();
  },
});

})();
