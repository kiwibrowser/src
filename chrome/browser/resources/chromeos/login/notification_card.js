// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'notification-card',

  properties: {
    buttonLabel: {type: String, value: ''},

    linkLabel: {type: String, value: ''},

    type: {type: String, value: ''}
  },

  iconNameByType_: function(type) {
    if (type == 'fail')
      return 'warning';
    if (type == 'success')
      return 'done';
    console.error('Unknown type "' + type + '".');
    return '';
  },

  buttonClicked_: function() {
    this.fire('buttonclick');
  },

  linkClicked_: function(e) {
    this.fire('linkclick');
    e.preventDefault();
  },

  get submitButton() {
    return this.$.submitButton;
  }
});
