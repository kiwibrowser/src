// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'viewer-error-screen',
  properties: {
    reloadFn: Function,

    strings: Object,
  },

  show: function() {
    /** @type {!CrDialogElement} */ (this.$.dialog).showModal();
  },

  reload: function() {
    if (this.reloadFn)
      this.reloadFn();
  }
});
