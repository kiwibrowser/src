// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'gaia-button',

  properties: {
    disabled: {type: Boolean, value: false, reflectToAttribute: true},

    type: {
      type: String,
      value: '',
      reflectToAttribute: true,
      observer: 'typeChanged_'
    }
  },

  focus: function() {
    this.$.button.focus();
  },

  focusedChanged_: function() {
    if (this.type == 'link' || this.type == 'dialog')
      return;
    this.$.button.raised = this.$.button.focused;
  },

  typeChanged_: function() {
    if (this.type == 'link')
      this.$.button.setAttribute('noink', '');
    else
      this.$.button.removeAttribute('noink');
  },

  onClick_: function(e) {
    if (this.disabled)
      e.stopPropagation();
  }
});

Polymer({
  is: 'gaia-icon-button',

  properties: {
    disabled: {type: Boolean, value: false, reflectToAttribute: true},

    icon: String,

    ariaLabel: String
  },

  focus: function() {
    this.$.iconButton.focus();
  },

  onClick_: function(e) {
    if (this.disabled)
      e.stopPropagation();
  }
});
