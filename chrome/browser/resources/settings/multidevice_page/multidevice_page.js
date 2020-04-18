// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * Settings page for managing multidevice features.
 */

Polymer({
  is: 'settings-multidevice-page',

  properties: {
    /** Preferences state. */
    prefs: {
      type: Object,
      notify: true,
    },
  },
});
