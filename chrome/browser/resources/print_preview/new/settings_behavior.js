// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.exportPath('print_preview_new');
/**
 * |key| is the field in the serialized settings state that corresponds to the
 * setting, or an empty string if the setting should not be saved in the
 * serialized state.
 * @typedef {{
 *   value: *,
 *   unavailableValue: *,
 *   valid: boolean,
 *   available: boolean,
 *   key: string,
 * }}
 */
print_preview_new.Setting;

/**
 * @typedef {{
 *   pages: !print_preview_new.Setting,
 *   copies: !print_preview_new.Setting,
 *   collate: !print_preview_new.Setting,
 *   layout: !print_preview_new.Setting,
 *   color: !print_preview_new.Setting,
 *   mediaSize: !print_preview_new.Setting,
 *   margins: !print_preview_new.Setting,
 *   dpi: !print_preview_new.Setting,
 *   fitToPage: !print_preview_new.Setting,
 *   scaling: !print_preview_new.Setting,
 *   duplex: !print_preview_new.Setting,
 *   cssBackground: !print_preview_new.Setting,
 *   selectionOnly: !print_preview_new.Setting,
 *   headerFooter: !print_preview_new.Setting,
 *   rasterize: !print_preview_new.Setting,
 *   vendorItems: !print_preview_new.Setting,
 *   otherOptions: !print_preview_new.Setting,
 *   ranges: !print_preview_new.Setting,
 *   pagesPerSheet: !print_preview_new.Setting,
 * }}
 */
print_preview_new.Settings;

/** @polymerBehavior */
const SettingsBehavior = {
  properties: {
    /** @type {print_preview_new.Settings} */
    settings: {
      type: Object,
      notify: true,
    },
  },

  /**
   * @param {string} settingName Name of the setting to get.
   * @return {print_preview_new.Setting} The setting object.
   */
  getSetting: function(settingName) {
    const setting = /** @type {print_preview_new.Setting} */ (
        this.get(settingName, this.settings));
    assert(!!setting, 'Setting is missing: ' + settingName);
    return setting;
  },

  /**
   * @param {string} settingName Name of the setting to get the value for.
   * @return {*} The value of the setting, accounting for availability.
   */
  getSettingValue: function(settingName) {
    const setting = this.getSetting(settingName);
    return setting.available ? setting.value : setting.unavailableValue;
  },

  /**
   * @param {string} settingName Name of the setting to set
   * @param {boolean | string | number | Array | Object} value The value to set
   *     the setting to.
   */
  setSetting: function(settingName, value) {
    const setting = this.getSetting(settingName);
    this.set(`settings.${settingName}.value`, value);
  },

  /**
   * @param {string} settingName Name of the setting to set
   * @param {boolean} valid Whether the setting value is currently valid.
   */
  setSettingValid: function(settingName, valid) {
    const setting = this.getSetting(settingName);
    // Should not set the setting to invalid if it is not available, as there
    // is no way for the user to change the value in this case.
    if (!valid)
      assert(setting.available, 'Setting is not available: ' + settingName);
    const shouldFireEvent = valid != setting.valid;
    this.set(`settings.${settingName}.valid`, valid);
    if (shouldFireEvent)
      this.fire('setting-valid-changed', valid);
  }
};
