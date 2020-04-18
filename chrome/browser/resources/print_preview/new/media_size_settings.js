// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'print-preview-media-size-settings',

  behaviors: [SettingsBehavior],

  properties: {
    capability: Object,

    disabled: Boolean,
  },

  observers:
      ['onMediaSizeSettingChange_(settings.mediaSize.value, ' +
       'capability.option)'],

  /**
   * @param {*} value The new value of the media size setting.
   * @private
   */
  onMediaSizeSettingChange_: function(value) {
    const valueToSet = JSON.stringify(value);
    for (const option of this.capability.option) {
      if (JSON.stringify(option) == valueToSet) {
        this.$$('print-preview-settings-select').selectValue(valueToSet);
        return;
      }
    }
  },
});
