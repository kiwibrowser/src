// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'unrecoverable-cryptohome-error-card',

  onContinueClicked_: function() {
    this.fire('done');
  },
});
