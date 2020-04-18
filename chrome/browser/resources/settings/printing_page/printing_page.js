// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'settings-printing-page',

  properties: {
    /** Preferences state. */
    prefs: {
      type: Object,
      notify: true,
    },

    searchTerm: {
      type: String,
    },

    /** @private {!Map<string, string>} */
    focusConfig_: {
      type: Object,
      value: function() {
        const map = new Map();
        if (settings.routes.CLOUD_PRINTERS) {
          map.set(
              settings.routes.CLOUD_PRINTERS.path,
              '#cloudPrinters .subpage-arrow');
        }
        // <if expr="chromeos">
        if (settings.routes.CUPS_PRINTERS) {
          map.set(
              settings.routes.CUPS_PRINTERS.path,
              '#cupsPrinters .subpage-arrow');
        }
        // </if>
        return map;
      },
    },
  },

  // <if expr="chromeos">
  /** @private */
  onTapCupsPrinters_: function() {
    settings.navigateTo(settings.routes.CUPS_PRINTERS);
  },
  // </if>

  // <if expr="not chromeos">
  onTapLocalPrinters_: function() {
    settings.PrintingBrowserProxyImpl.getInstance().openSystemPrintDialog();
  },
  // </if>

  /** @private */
  onTapCloudPrinters_: function() {
    settings.navigateTo(settings.routes.CLOUD_PRINTERS);
  },
});
