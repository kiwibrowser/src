// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Polymer element for network configuration input fields.
 */
Polymer({
  is: 'network-config-input',

  behaviors: [I18nBehavior],

  properties: {
    label: String,

    disabled: {
      type: Boolean,
      reflectToAttribute: true,
    },

    value: {
      type: String,
      notify: true,
    },

    password: Boolean,

    showPassword: {
      type: Boolean,
      value: false,
    },
  },

  focus: function() {
    this.$$('input').focus();
  },

  /**
   * @return {string}
   * @private
   */
  getInputType_: function() {
    return (this.password && !this.showPassword) ? 'password' : 'text';
  },

  /**
   * @return {string}
   * @private
   */
  getIconClass_: function() {
    return this.showPassword ? 'icon-visibility-off' : 'icon-visibility';
  },

  /**
   * @return {string}
   * @private
   */
  getShowPasswordTitle_: function() {
    return this.showPassword ? this.i18n('hidePassword') :
                               this.i18n('showPassword');
  },

  /** @private */
  onShowPasswordTap_: function() {
    this.showPassword = !this.showPassword;
  },
});
