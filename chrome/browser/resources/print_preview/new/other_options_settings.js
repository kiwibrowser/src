// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'print-preview-other-options-settings',

  behaviors: [SettingsBehavior],

  properties: {
    disabled: Boolean,
  },

  observers: [
    'onHeaderFooterSettingChange_(settings.headerFooter.value)',
    'onDuplexSettingChange_(settings.duplex.value)',
    'onCssBackgroundSettingChange_(settings.cssBackground.value)',
    'onRasterizeSettingChange_(settings.rasterize.value)',
    'onSelectionOnlySettingChange_(settings.selectionOnly.value)',
  ],

  /** @private {!Map<string, ?number>} */
  timeouts_: new Map(),

  /** @private {!Map<string, boolean>} */
  previousValues_: new Map(),

  /**
   * @param {string} settingName The name of the setting to updated.
   * @param {boolean} newValue The new value for the setting.
   */
  updateSettingWithTimeout_: function(settingName, newValue) {
    const timeout = this.timeouts_.get(settingName);
    if (timeout != null)
      clearTimeout(timeout);

    this.timeouts_.set(settingName, setTimeout(() => {
                         this.timeouts_.delete(settingName);
                         if (this.previousValues_.get(settingName) == newValue)
                           return;
                         this.previousValues_.set(settingName, newValue);
                         this.setSetting(settingName, newValue);

                         // For tests only
                         this.fire('update-checkbox-setting', settingName);
                       }, 100));
  },

  /**
   * @param {boolean} value The new value of the header footer setting.
   * @private
   */
  onHeaderFooterSettingChange_: function(value) {
    this.$.headerFooter.checked = value;
  },

  /**
   * @param {boolean} value The new value of the duplex setting.
   * @private
   */
  onDuplexSettingChange_: function(value) {
    this.$.duplex.checked = value;
  },

  /**
   * @param {boolean} value The new value of the css background setting.
   * @private
   */
  onCssBackgroundSettingChange_: function(value) {
    this.$.cssBackground.checked = value;
  },

  /**
   * @param {boolean} value The new value of the rasterize setting.
   * @private
   */
  onRasterizeSettingChange_: function(value) {
    this.$.rasterize.checked = value;
  },

  /**
   * @param {boolean} value The new value of the selection only setting.
   * @private
   */
  onSelectionOnlySettingChange_: function(value) {
    this.$.selectionOnly.checked = value;
  },

  /** @private */
  onHeaderFooterChange_: function() {
    this.updateSettingWithTimeout_('headerFooter', this.$.headerFooter.checked);
  },

  /** @private */
  onDuplexChange_: function() {
    this.updateSettingWithTimeout_('duplex', this.$.duplex.checked);
  },

  /** @private */
  onCssBackgroundChange_: function() {
    this.updateSettingWithTimeout_(
        'cssBackground', this.$.cssBackground.checked);
  },

  /** @private */
  onRasterizeChange_: function() {
    this.updateSettingWithTimeout_('rasterize', this.$.rasterize.checked);
  },

  /** @private */
  onSelectionOnlyChange_: function() {
    this.updateSettingWithTimeout_(
        'selectionOnly', this.$.selectionOnly.checked);
  },
});
