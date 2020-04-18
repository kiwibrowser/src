// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Polymer element for displaying a collapsable list of networks.
 */

/**
 * Polymer class definition for 'cr-network-list'.
 */
Polymer({
  is: 'cr-network-list',

  properties: {
    /**
     * The list of network state properties for the items to display.
     * @type {!Array<!CrOnc.NetworkStateProperties>}
     */
    networks: {
      type: Array,
      value: function() {
        return [];
      },
    },

    /**
     * The list of custom items to display after the list of networks.
     * @type {!Array<!CrNetworkList.CustomItemState>}
     */
    customItems: {
      type: Array,
      value: function() {
        return [];
      },
    },

    /** True if action buttons should be shown for the itmes. */
    showButtons: {
      type: Boolean,
      value: false,
      reflectToAttribute: true,
    },

    /**
     * Reflects the iron-list selecteditem property.
     * @type {!CrNetworkList.CrNetworkListItemType}
     */
    selectedItem: {
      type: Object,
      observer: 'selectedItemChanged_',
    },

    /**
     * Contains |networks| + |customItems|.
     * @private {!Array<!CrNetworkList.CrNetworkListItemType>}
     */
    listItems_: {
      type: Array,
      value: function() {
        return [];
      },
    },
  },

  behaviors: [CrScrollableBehavior],

  observers: ['updateListItems_(networks, customItems)'],

  /** @private {boolean} */
  focusRequested_: false,

  focus: function() {
    this.focusRequested_ = true;
    this.focusFirstItem_();
  },

  /** @private */
  updateListItems_: function() {
    this.saveScroll(this.$.networkList);
    this.listItems_ = this.networks.concat(this.customItems);
    this.restoreScroll(this.$.networkList);
    this.updateScrollableContents();
    if (this.focusRequested_) {
      this.async(function() {
        this.focusFirstItem_();
      });
    }
  },

  /** @private */
  focusFirstItem_: function() {
    // Select the first cr-network-list-item if there is one.
    var item = this.$$('cr-network-list-item');
    if (!item)
      return;
    item.focus();
    this.focusRequested_ = false;
  },

  /**
   * Use iron-list selection (which is not the same as focus) to trigger
   * tap (requires selection-enabled) or keyboard selection.
   * @private
   */
  selectedItemChanged_: function() {
    if (this.selectedItem)
      this.onItemAction_(this.selectedItem);
  },

  /**
   * @param {!CrNetworkList.CrNetworkListItemType} item
   * @private
   */
  onItemAction_: function(item) {
    if (item.hasOwnProperty('customItemName')) {
      this.fire('custom-item-selected', item);
    } else {
      this.fire('selected', item);
      this.focusRequested_ = true;
    }
  },
});
