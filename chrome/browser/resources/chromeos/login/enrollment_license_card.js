// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Polymer element for displaying enrollment license selection
 card.
 */

Polymer({
  is: 'enrollment-license-card',

  behaviors: [I18nBehavior],

  properties: {
    /**
     * Whether the UI disabled.
     */
    disabled: {
      type: Boolean,
      value: true,
    },

    /**
     * Selected license type
     */
    selected: {
      type: String,
      value: 'sometype',
    },

    /**
     * Array with available license types.
     */
    licenses: {
      type: Array,
      value: function() {
        return [
          {
            id: 'licenseType',
            label: 'perpetualLicenseTypeTitle',
            count: 123,
            disabled: false,
            hidden: false,
          },
        ];
      },
      observer: 'licensesChanged_',
    },
  },

  get submitButton() {
    return this.$.submitButton;
  },

  buttonClicked_: function() {
    this.fire('buttonclick');
  },

  licensesChanged_: function(newValue, oldValue) {
    var firstSelection = '';
    for (var i = 0, item; item = this.licenses[i]; ++i) {
      if (this.isSelectable_(item) && firstSelection == '') {
        firstSelection = item.id;
        break;
      }
    }
    if (firstSelection != '') {
      this.selected = firstSelection;
    } else if (this.licenses[0]) {
      this.selected = this.licenses[0].id;
    } else {
      this.selected = '';
      this.disabled = true;
    }
  },

  isSelectable_: function(item) {
    return item.count > 0 && !item.disabled && !item.hidden;
  },

  formatTitle_: function(item) {
    return this.i18n('licenseCountTemplate', this.i18n(item.label), item.count);
  },

  or_: function(left, right) {
    return left || right;
  },
});
