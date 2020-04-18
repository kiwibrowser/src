// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Javascript for DeviceTable UI, served from chrome://bluetooth-internals/.
 */

cr.define('device_table', function() {
  var COLUMNS = {
    NAME: 0,
    ADDRESS: 1,
    RSSI: 2,
    SERVICES: 3,
    CONNECTION_STATE: 4,
    LINKS: 5,
  };

  /**
   * A table that lists the devices and responds to changes in the given
   * DeviceCollection. Fires events for inspection requests from listed
   * devices.
   * @constructor
   * @extends {HTMLTableElement}
   */
  var DeviceTable = cr.ui.define(function() {
    /** @private {?Array<device_collection.Device>} */
    this.devices_ = null;

    return document.importNode(
        $('table-template').content.children[0], true /* deep */);
  });

  DeviceTable.prototype = {
    __proto__: HTMLTableElement.prototype,

    /**
     * Decorates an element as a UI element class. Caches references to the
     *    table body and headers.
     */
    decorate: function() {
      /** @private */
      this.body_ = this.tBodies[0];
      /** @private */
      this.headers_ = this.tHead.rows[0].cells;
      /** @private {!Map<!bluetooth.mojom.DeviceInfo, boolean>} */
      this.inspectionMap_ = new Map();
    },

    /**
     * Sets the tables device collection.
     * @param {!device_collection.DeviceCollection} deviceCollection
     */
    setDevices: function(deviceCollection) {
      assert(!this.devices_, 'Devices can only be set once.');

      this.devices_ = deviceCollection;
      this.devices_.addEventListener('sorted', this.redraw_.bind(this));
      this.devices_.addEventListener('change', this.handleChange_.bind(this));
      this.devices_.addEventListener('splice', this.handleSplice_.bind(this));

      this.redraw_();
    },

    /**
     * Updates the inspect status of the row matching the given |deviceInfo|.
     * If |isInspecting| is true, the forget link is enabled otherwise it's
     * disabled.
     * @param {!bluetooth.mojom.DeviceInfo} deviceInfo
     * @param {boolean} isInspecting
     */
    setInspecting: function(deviceInfo, isInspecting) {
      this.inspectionMap_.set(deviceInfo, isInspecting);
      this.updateRow_(deviceInfo, this.devices_.indexOf(deviceInfo));
    },

    /**
     * Fires a forget pressed event for the row |index|.
     * @param {number} index
     * @private
     */
    handleForgetClick_: function(index) {
      var event = new CustomEvent('forgetpressed', {
        bubbles: true,
        detail: {
          address: this.devices_.item(index).address,
        }
      });
      this.dispatchEvent(event);
    },

    /**
     * Updates table row on change event of the device collection.
     * @param {!Event} event
     * @private
     */
    handleChange_: function(event) {
      this.updateRow_(this.devices_.item(event.index), event.index);
    },

    /**
     * Fires an inspect pressed event for the row |index|.
     * @param {number} index
     * @private
     */
    handleInspectClick_: function(index) {
      var event = new CustomEvent('inspectpressed', {
        bubbles: true,
        detail: {
          address: this.devices_.item(index).address,
        }
      });
      this.dispatchEvent(event);
    },

    /**
     * Updates table row on splice event of the device collection.
     * @param {!Event} event
     * @private
     */
    handleSplice_: function(event) {
      event.removed.forEach(function() {
        this.body_.deleteRow(event.index);
      }, this);

      event.added.forEach(function(device, index) {
        this.insertRow_(device, event.index + index);
      }, this);
    },

    /**
     * Inserts a new row at |index| and updates it with info from |device|.
     * @param {!bluetooth.mojom.DeviceInfo} device
     * @param {?number} index
     * @private
     */
    insertRow_: function(device, index) {
      var row = this.body_.insertRow(index);
      row.id = device.address;

      for (var i = 0; i < this.headers_.length; i++) {
        // Skip the LINKS column. It has no data-field attribute.
        if (i === COLUMNS.LINKS)
          continue;
        row.insertCell();
      }

      // Make two extra cells for the inspect link and connect errors.
      var inspectCell = row.insertCell();

      var inspectLink = document.createElement('a', 'action-link');
      inspectLink.textContent = 'Inspect';
      inspectCell.appendChild(inspectLink);
      inspectLink.addEventListener('click', function() {
        this.handleInspectClick_(row.sectionRowIndex);
      }.bind(this));

      var forgetLink = document.createElement('a', 'action-link');
      forgetLink.textContent = 'Forget';
      inspectCell.appendChild(forgetLink);
      forgetLink.addEventListener('click', function() {
        this.handleForgetClick_(row.sectionRowIndex);
      }.bind(this));

      this.updateRow_(device, row.sectionRowIndex);
    },

    /**
     * Deletes and recreates the table using the cached |devices_|.
     * @private
     */
    redraw_: function() {
      this.removeChild(this.body_);
      this.appendChild(document.createElement('tbody'));
      this.body_ = this.tBodies[0];
      this.body_.classList.add('table-body');

      for (var i = 0; i < this.devices_.length; i++) {
        this.insertRow_(this.devices_.item(i));
      }
    },

    /**
     * Updates the row at |index| with the info from |device|.
     * @param {!bluetooth.mojom.DeviceInfo} device
     * @param {number} index
     * @private
     */
    updateRow_: function(device, index) {
      var row = this.body_.rows[index];
      assert(row, 'Row ' + index + ' is not in the table.');

      row.classList.toggle('removed', device.removed);

      var forgetLink = row.cells[COLUMNS.LINKS].children[1];

      if (this.inspectionMap_.has(device))
        forgetLink.disabled = !this.inspectionMap_.get(device);
      else
        forgetLink.disabled = true;

      // Update the properties based on the header field path.
      for (var i = 0; i < this.headers_.length; i++) {
        // Skip the LINKS column. It has no data-field attribute.
        if (i === COLUMNS.LINKS)
          continue;

        var header = this.headers_[i];
        var propName = header.dataset.field;

        var parts = propName.split('.');
        var obj = device;
        while (obj != null && parts.length > 0) {
          var part = parts.shift();
          obj = obj[part];
        }

        if (propName == 'isGattConnected') {
          obj = obj ? 'Connected' : 'Not Connected';
        }

        var cell = row.cells[i];
        cell.textContent = obj == null ? 'Unknown' : obj;
        cell.dataset.label = header.textContent;
      }
    },
  };

  return {
    DeviceTable: DeviceTable,
  };
});
