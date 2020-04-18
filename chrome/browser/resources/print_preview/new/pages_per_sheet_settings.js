// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'print-preview-pages-per-sheet-settings',

  behaviors: [SettingsBehavior, print_preview_new.SelectBehavior],

  properties: {
    disabled: Boolean,
  },

  observers: ['onPagesPerSheetSettingChange_(settings.pagesPerSheet.value)'],

  /**
   * @param {*} value The new value of the pages per sheet setting.
   * @private
   */
  onPagesPerSheetSettingChange_: function(value) {
    this.$$('select').value = /** @type {number} */ (value).toString();
  },

  /** @param {string} value The new select value. */
  onProcessSelectChange: function(value) {
    this.setSetting('pagesPerSheet', parseInt(value, 10));
  },
});
