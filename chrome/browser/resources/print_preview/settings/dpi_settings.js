// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview', function() {
  'use strict';

  /**
   * Encapsulates all settings and logic related to the DPI selection UI.
   * @param {!print_preview.ticket_items.Dpi} ticketItem Used to read and write
   *     the DPI ticket item.
   * @constructor
   * @extends {print_preview.SettingsSectionSelect}
   */
  function DpiSettings(ticketItem) {
    print_preview.SettingsSectionSelect.call(this, ticketItem);
  }

  DpiSettings.prototype = {
    __proto__: print_preview.SettingsSectionSelect.prototype,

    /** @override */
    getDefaultDisplayName_: function(option) {
      const hDpi = option.horizontal_dpi || 0;
      const vDpi = option.vertical_dpi || 0;
      if (hDpi > 0 && vDpi > 0 && hDpi != vDpi) {
        return loadTimeData.getStringF(
            'nonIsotropicDpiItemLabel', hDpi.toLocaleString(),
            vDpi.toLocaleString());
      }
      return loadTimeData.getStringF(
          'dpiItemLabel', (hDpi || vDpi).toLocaleString());
    }
  };

  // Export
  return {DpiSettings: DpiSettings};
});
