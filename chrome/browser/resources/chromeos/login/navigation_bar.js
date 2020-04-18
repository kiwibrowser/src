// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'navigation-bar',

  properties: {
    backVisible: {type: Boolean, value: false},
    closeVisible: {type: Boolean, value: false},
    refreshVisible: {type: Boolean, value: false},
    disabled: {type: Boolean, value: false}
  },

  onBack_: function() {
    this.fire('back');
  },
  onClose_: function() {
    this.fire('close');
  },
  onRefresh_: function() {
    this.fire('refresh');
  }
});
