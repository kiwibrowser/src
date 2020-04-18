// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'cr-expand-button' is a chrome-specific wrapper around a button that toggles
 * between an opened (expanded) and closed state.
 */
Polymer({
  is: 'cr-expand-button',

  properties: {
    /**
     * If true, the button is in the expanded state and will show the
     * 'expand-less' icon. If false, the button shows the 'expand-more' icon.
     */
    expanded: {type: Boolean, value: false, notify: true},

    /**
     * If true, the button will be disabled and grayed out.
     */
    disabled: {type: Boolean, value: false, reflectToAttribute: true},

    /** A11y text descriptor for this control. */
    alt: String,

    tabIndex: {
      type: Number,
      value: 0,
    },
  },

  /** @private */
  iconName_: function(expanded) {
    return expanded ? 'icon-expand-less' : 'icon-expand-more';
  },

  /**
   * @param {!Event} event
   * @private
   */
  toggleExpand_: function(event) {
    this.expanded = !this.expanded;
    event.stopPropagation();
  },

  /** @private */
  getAriaPressed_: function(expanded) {
    return expanded ? 'true' : 'false';
  },
});
