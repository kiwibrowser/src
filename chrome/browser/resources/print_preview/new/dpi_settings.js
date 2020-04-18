// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.exportPath('print_preview_new');

/**
 * @typedef {{
 *   horizontal_dpi: (number | undefined),
 *   vertical_dpi: (number | undefined),
 *   vendor_id: (number | undefined)}}
 */
print_preview_new.DpiOption;

/**
 * @typedef {{
 *   horizontal_dpi: (number | undefined),
 *   name: string,
 *   vertical_dpi: (number | undefined),
 *   vendor_id: (number | undefined)}}
 */
print_preview_new.LabelledDpiOption;

Polymer({
  is: 'print-preview-dpi-settings',

  behaviors: [SettingsBehavior],

  properties: {
    /** @type {{ option: Array<!print_preview_new.SelectOption> }} */
    capability: Object,

    disabled: Boolean,

    /** @private {{ option: Array<!print_preview_new.SelectOption> }} */
    capabilityWithLabels_: {
      type: Object,
      computed: 'computeCapabilityWithLabels_(capability)',
    },
  },

  observers: [
    'onDpiSettingChange_(settings.dpi.value, capabilityWithLabels_.option)',
  ],

  /**
   * Adds default labels for each option.
   * @return {{option: Array<!print_preview_new.SelectOption>}}
   * @private
   */
  computeCapabilityWithLabels_: function() {
    if (!this.capability || !this.capability.option)
      return this.capability;
    const result =
        /** @type {{option: Array<!print_preview_new.SelectOption>}} */ (
            JSON.parse(JSON.stringify(this.capability)));
    this.capability.option.forEach((option, index) => {
      const dpiOption = /** @type {print_preview_new.DpiOption} */ (option);
      const hDpi = dpiOption.horizontal_dpi || 0;
      const vDpi = dpiOption.vertical_dpi || 0;
      if (hDpi > 0 && vDpi > 0 && hDpi != vDpi) {
        result.option[index].name = loadTimeData.getStringF(
            'nonIsotropicDpiItemLabel', hDpi.toLocaleString(),
            vDpi.toLocaleString());
      } else {
        result.option[index].name = loadTimeData.getStringF(
            'dpiItemLabel', (hDpi || vDpi).toLocaleString());
      }
    });
    return result;
  },

  /**
   * @param {!print_preview_new.SelectOption} value The new value of the dpi
   *     setting.
   * @private
   */
  onDpiSettingChange_: function(value) {
    const dpiValue = /** @type {print_preview_new.DpiOption} */ (value);
    for (const option of assert(this.capabilityWithLabels_.option)) {
      const dpiOption =
          /** @type {print_preview_new.LabelledDpiOption} */ (option);
      if (dpiValue.horizontal_dpi == dpiOption.horizontal_dpi &&
          dpiValue.vertical_dpi == dpiOption.vertical_dpi &&
          dpiValue.vendor_id == dpiOption.vendor_id) {
        this.$$('print-preview-settings-select')
            .selectValue(JSON.stringify(option));
        return;
      }
    }
  },
});
