// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Javascript for AdapterPage, served from chrome://bluetooth-internals/.
 */

cr.define('adapter_page', function() {
  /** @const */ var Page = cr.ui.pageManager.Page;

  var PROPERTY_NAMES = {
    address: 'Address',
    name: 'Name',
    initialized: 'Initialized',
    present: 'Present',
    powered: 'Powered',
    discoverable: 'Discoverable',
    discovering: 'Discovering',
  };

  /**
   * Page that contains an ObjectFieldSet that displays the latest AdapterInfo.
   * @constructor
   * @extends {cr.ui.pageManager.Page}
   */
  function AdapterPage() {
    Page.call(this, 'adapter', 'Adapter', 'adapter');

    this.adapterFieldSet = new object_fieldset.ObjectFieldSet();
    this.adapterFieldSet.setPropertyDisplayNames(PROPERTY_NAMES);
    this.pageDiv.appendChild(this.adapterFieldSet);

    this.refreshBtn_ = $('adapter-refresh-btn');
    this.refreshBtn_.addEventListener('click', function() {
      this.refreshBtn_.disabled = true;
      this.pageDiv.dispatchEvent(new CustomEvent('refreshpressed'));
    }.bind(this));
  }

  AdapterPage.prototype = {
    __proto__: Page.prototype,

    /**
     * Sets the information to display in fieldset.
     * @param {!bluetooth.mojom.AdapterInfo} info
     */
    setAdapterInfo: function(info) {
      this.adapterFieldSet.setObject(info);
      this.refreshBtn_.disabled = false;
    },

    /**
     * Redraws the fieldset displaying the adapter info.
     */
    redraw: function() {
      this.adapterFieldSet.redraw();
      this.refreshBtn_.disabled = false;
    },
  };

  return {
    AdapterPage: AdapterPage,
  };
});
