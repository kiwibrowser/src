// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'print-preview-margins-settings',

  behaviors: [SettingsBehavior, print_preview_new.SelectBehavior],

  properties: {
    disabled: Boolean,
  },

  observers: ['onMarginsSettingChange_(settings.margins.value)'],

  /**
   * @param {*} value The new value of the margins setting.
   * @private
   */
  onMarginsSettingChange_: function(value) {
    this.$$('select').value = /** @type {string} */ (value).toString();
  },

  /** @param {string} value The new select value. */
  onProcessSelectChange: function(value) {
    this.setSetting('margins', parseInt(value, 10));
  },
});
