// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * A link row is a UI element similar to a button, though usually wider than a
 * button (taking up the whole 'row'). The name link comes from the intended use
 * of this element to take the user to another page in the app or to an external
 * page (somewhat like an HTML link).
 * Note: the ripple handling was taken from Polymer v1 paper-icon-button-light.
 */
Polymer({
  is: 'cr-link-row',

  behaviors: [Polymer.PaperRippleBehavior],

  properties: {
    iconClass: String,

    label: String,

    subLabel: {
      type: String,
      /* Value used for noSubLabel attribute. */
      value: '',
    },

    disabled: {
      type: Boolean,
      reflectToAttribute: true,
    },
  },

  listeners: {
    'down': '_rippleDown',
    'up': '_rippleUp',
    'focus': '_rippleDown',
    'blur': '_rippleUp',
  },

  _rippleDown: function() {
    this.getRipple().uiDownAction();
  },

  _rippleUp: function() {
    this.getRipple().uiUpAction();
  },

  _createRipple: function() {
    this._rippleContainer = this.$.icon;
    var ripple = Polymer.PaperRippleBehavior._createRipple();
    ripple.id = 'ink';
    ripple.setAttribute('recenters', '');
    ripple.classList.add('circle');
    return ripple;
  },
});
