// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'settings-passwords-and-forms-page',

  behaviors: [PrefsBehavior],

  properties: {
    /** @private Filter applied to passwords and password exceptions. */
    passwordFilter_: String,

    /** @type {?Map<string, string>} */
    focusConfig: Object,
  },

  /** @override */
  ready: function() {
    this.focusConfig_ = new Map();
    if (settings.routes.AUTOFILL) {
      this.focusConfig_.set(
          settings.routes.AUTOFILL.path, '#autofillManagerButton');
    }
    if (settings.routes.MANAGE_PASSWORDS) {
      this.focusConfig_.set(
          settings.routes.MANAGE_PASSWORDS.path,
          '#passwordManagerButton .subpage-arrow');
    }
  },

  /**
   * Shows the manage autofill sub page.
   * @param {!Event} event
   * @private
   */
  onAutofillTap_: function(event) {
    settings.navigateTo(settings.routes.AUTOFILL);
  },

  /**
   * Shows the manage passwords sub page.
   * @param {!Event} event
   * @private
   */
  onPasswordsTap_: function(event) {
    settings.navigateTo(settings.routes.MANAGE_PASSWORDS);
  },
});
