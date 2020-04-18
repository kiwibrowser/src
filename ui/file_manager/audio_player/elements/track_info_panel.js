// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {
  'use strict';

  Polymer({
    is: 'track-info-panel',

    properties: {
      track: {
        type: Object,
        value: null
      },

      expanded: {
        type: Boolean,
        value: false,
        notify: true,
        reflectToAttribute: true
      },

      artworkAvailable: {
        type: Boolean,
        value: false,
        reflectToAttribute: true
      }
    },
  });
})();  // Anonymous closure
