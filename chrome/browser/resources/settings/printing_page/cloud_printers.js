// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'settings-cloud-printers' is a component for showing Google
 * Cloud Printer settings subpage (chrome://settings/cloudPrinters).
 */
// TODO(xdai): Rename it to 'settings-cloud-printers-page'.
Polymer({
  is: 'settings-cloud-printers',

  properties: {
    prefs: {
      type: Object,
      notify: true,
    },
  },
});
