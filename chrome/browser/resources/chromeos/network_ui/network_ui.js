// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var networkUI = {};

/** @typedef {CrOnc.NetworkStateProperties|CrOnc.DeviceStateProperties} */
networkUI.StateProperties;

var NetworkUI = (function() {
  'use strict';

  CrOncStrings = {
    OncTypeCellular: loadTimeData.getString('OncTypeCellular'),
    OncTypeEthernet: loadTimeData.getString('OncTypeEthernet'),
    OncTypeTether: loadTimeData.getString('OncTypeTether'),
    OncTypeVPN: loadTimeData.getString('OncTypeVPN'),
    OncTypeWiFi: loadTimeData.getString('OncTypeWiFi'),
    OncTypeWiMAX: loadTimeData.getString('OncTypeWiMAX'),
    networkListItemConnected:
        loadTimeData.getString('networkListItemConnected'),
    networkListItemConnecting:
        loadTimeData.getString('networkListItemConnecting'),
    networkListItemConnectingTo:
        loadTimeData.getString('networkListItemConnectingTo'),
    networkListItemInitializing:
        loadTimeData.getString('networkListItemInitializing'),
    networkListItemScanning: loadTimeData.getString('networkListItemScanning'),
    networkListItemNotConnected:
        loadTimeData.getString('networkListItemNotConnected'),
    networkListItemNoNetwork:
        loadTimeData.getString('networkListItemNoNetwork'),
    vpnNameTemplate: loadTimeData.getString('vpnNameTemplate'),
  };

  // Properties to display in the network state table. Each entry can be either
  // a single state field or an array of state fields. If more than one is
  // specified then the first non empty value is used.
  var NETWORK_STATE_FIELDS = [
    'GUID', 'Name', 'Type', 'ConnectionState', 'connectable', 'ErrorState',
    'WiFi.Security', ['Cellular.NetworkTechnology', 'EAP.EAP'],
    'Cellular.ActivationState', 'Cellular.RoamingState', 'WiFi.Frequency',
    'WiFi.SignalStrength'
  ];

  var FAVORITE_STATE_FIELDS =
      ['GUID', 'Name', 'Type', 'profile_path', 'visible', 'Source'];

  var DEVICE_STATE_FIELDS = ['Type', 'State'];

  /**
   * Creates and returns a typed HTMLTableCellElement.
   *
   * @return {!HTMLTableCellElement} A new td element.
   */
  var createTableCellElement = function() {
    return /** @type {!HTMLTableCellElement} */ (document.createElement('td'));
  };

  /**
   * Creates and returns a typed HTMLTableRowElement.
   *
   * @return {!HTMLTableRowElement} A new tr element.
   */
  var createTableRowElement = function() {
    return /** @type {!HTMLTableRowElement} */ (document.createElement('tr'));
  };

  /**
   * Returns the ONC data property for |state| associated with a key. Used
   * to access properties in the state by |key| which may may refer to a
   * nested property, e.g. 'WiFi.Security'. If any part of a nested key is
   * missing, this will return undefined.
   *
   * @param {!networkUI.StateProperties} state
   * @param {string} key The ONC key for the property.
   * @return {*} The value associated with the property or undefined if the
   *     key (any part of it) is not defined.
   */
  var getOncProperty = function(state, key) {
    var dict = /** @type {!Object} */ (state);
    var keys = key.split('.');
    while (keys.length > 1) {
      var k = keys.shift();
      dict = dict[k];
      if (!dict || typeof dict != 'object')
        return undefined;
    }
    return dict[keys.shift()];
  };

  /**
   * Creates a cell with a button for expanding a network state table row.
   *
   * @param {!networkUI.StateProperties} state
   * @return {!HTMLTableCellElement} The created td element that displays the
   *     given value.
   */
  var createStateTableExpandButton = function(state) {
    var cell = createTableCellElement();
    cell.className = 'state-table-expand-button-cell';
    var button = document.createElement('button');
    button.addEventListener('click', function(event) {
      toggleExpandRow(/** @type {!HTMLElement} */ (event.target), state);
    });
    button.className = 'state-table-expand-button';
    button.textContent = '+';
    cell.appendChild(button);
    return cell;
  };

  /**
   * Creates a cell with an icon representing the network state.
   *
   * @param {!networkUI.StateProperties} state
   * @return {!HTMLTableCellElement} The created td element that displays the
   *     icon.
   */
  var createStateTableIcon = function(state) {
    var cell = createTableCellElement();
    cell.className = 'state-table-icon-cell';
    var icon = /** @type {!CrNetworkIconElement} */ (
        document.createElement('cr-network-icon'));
    icon.isListItem = true;
    icon.networkState = {
      GUID: '',
      Type: state.Type,
    };
    cell.appendChild(icon);
    return cell;
  };

  /**
   * Creates a cell in the network state table.
   *
   * @param {*} value Content in the cell.
   * @return {!HTMLTableCellElement} The created td element that displays the
   *     given value.
   */
  var createStateTableCell = function(value) {
    var cell = createTableCellElement();
    cell.textContent = value || '';
    return cell;
  };

  /**
   * Creates a row in the network state table.
   *
   * @param {Array} stateFields The state fields to use for the row.
   * @param {!networkUI.StateProperties} state
   * @return {!HTMLTableRowElement} The created tr element that contains the
   *     network state information.
   */
  var createStateTableRow = function(stateFields, state) {
    var row = createTableRowElement();
    row.className = 'state-table-row';
    row.appendChild(createStateTableExpandButton(state));
    row.appendChild(createStateTableIcon(state));
    for (var i = 0; i < stateFields.length; ++i) {
      var field = stateFields[i];
      var value;
      if (typeof field == 'string') {
        value = getOncProperty(state, field);
      } else {
        for (var j = 0; j < field.length; ++j) {
          value = getOncProperty(state, field[j]);
          if (value != undefined)
            break;
        }
      }
      if (field == 'GUID')
        value = value.slice(0, 8);
      row.appendChild(createStateTableCell(value));
    }
    return row;
  };

  /**
   * Creates a table for networks or favorites.
   *
   * @param {string} tablename The name of the table to be created.
   * @param {!Array<string>} stateFields The list of fields for the table.
   * @param {!Array<!networkUI.StateProperties>} states
   */
  var createStateTable = function(tablename, stateFields, states) {
    var table = $(tablename);
    var oldRows = table.querySelectorAll('.state-table-row');
    for (var i = 0; i < oldRows.length; ++i)
      table.removeChild(oldRows[i]);
    states.forEach(function(state) {
      table.appendChild(createStateTableRow(stateFields, state));
    });
  };

  /**
   * Returns a valid HTMLElement id from |guid|.
   *
   * @param {string} guid A GUID which may start with a digit.
   * @return {string} A valid HTMLElement id.
   */
  var idFromGuid = function(guid) {
    return '_' + guid.replace(/[{}]/g, '');
  };

  /**
   * Returns a valid HTMLElement id from |type|. Note: |type| may be a Shill
   * type or an ONC type, so strip _ and convert to lowercase to unify them.
   *
   * @param {string} type A Shill or ONC network type
   * @return {string} A valid HTMLElement id.
   */
  var idFromType = function(type) {
    return '_' + type.replace(/[{}_]/g, '').toLowerCase();
  };

  /**
   * This callback function is triggered when visible networks are received.
   *
   * @param {!Array<!CrOnc.NetworkStateProperties>} states
   */
  var onVisibleNetworksReceived = function(states) {
    createStateTable('network-state-table', NETWORK_STATE_FIELDS, states);
  };

  /**
   * This callback function is triggered when favorite networks are received.
   *
   * @param {!Array<!CrOnc.NetworkStateProperties>} states
   */
  var onFavoriteNetworksReceived = function(states) {
    createStateTable('favorite-state-table', FAVORITE_STATE_FIELDS, states);
  };

  /**
   * This callback function is triggered when device states are received.
   *
   * @param {!Array<!CrOnc.DeviceStateProperties>} states
   */
  var onDeviceStatesReceived = function(states) {
    createStateTable('device-state-table', DEVICE_STATE_FIELDS, states);
  };

  /**
   * Toggles the button state and add or remove a row displaying the complete
   * state information for a row.
   *
   * @param {!HTMLElement} btn The button that was clicked.
   * @param {!networkUI.StateProperties} state
   */
  var toggleExpandRow = function(btn, state) {
    var cell = btn.parentNode;
    var row = /** @type {!HTMLTableRowElement} */ (cell.parentNode);
    if (btn.textContent == '-') {
      btn.textContent = '+';
      row.parentNode.removeChild(row.nextSibling);
    } else {
      btn.textContent = '-';
      var expandedRow = createExpandedRow(state, row);
      row.parentNode.insertBefore(expandedRow, row.nextSibling);
    }
  };

  /**
   * Creates the expanded row for displaying the complete state as JSON.
   *
   * @param {!networkUI.StateProperties} state
   * @param {!HTMLTableRowElement} baseRow The unexpanded row associated with
   *     the new row.
   * @return {!HTMLTableRowElement} The created tr element for the expanded row.
   */
  var createExpandedRow = function(state, baseRow) {
    assert(state);
    var guid = state.GUID || '';
    var expandedRow = createTableRowElement();
    expandedRow.className = 'state-table-row';
    var emptyCell = createTableCellElement();
    emptyCell.style.border = 'none';
    expandedRow.appendChild(emptyCell);
    var detailCell = createTableCellElement();
    detailCell.id = guid ? idFromGuid(guid) : idFromType(state.Type);
    detailCell.className = 'state-table-expanded-cell';
    detailCell.colSpan = baseRow.childNodes.length - 1;
    expandedRow.appendChild(detailCell);
    var selected = $('get-property-format').selectedIndex;
    var selectedId = $('get-property-format').options[selected].value;
    if (guid)
      handleNetworkDetail(guid, selectedId, detailCell);
    else
      handleDeviceDetail(state, selectedId, detailCell);
    return expandedRow;
  };

  /**
   * Requests network details and calls showDetail with the result.
   * @param {string} guid
   * @param {string} selectedId
   * @param {!HTMLTableCellElement} detailCell
   */
  var handleNetworkDetail = function(guid, selectedId, detailCell) {
    if (selectedId == 'shill') {
      chrome.send('getShillNetworkProperties', [guid]);
    } else if (selectedId == 'state') {
      chrome.networkingPrivate.getState(guid, function(properties) {
        showDetail(detailCell, properties, chrome.runtime.lastError);
      });
    } else if (selectedId == 'managed') {
      chrome.networkingPrivate.getManagedProperties(guid, function(properties) {
        showDetail(detailCell, properties, chrome.runtime.lastError);
      });
    } else {
      chrome.networkingPrivate.getProperties(guid, function(properties) {
        showDetail(detailCell, properties, chrome.runtime.lastError);
      });
    }
  };

  /**
   * Requests network details and calls showDetail with the result.
   * @param {!networkUI.StateProperties} state
   * @param {string} selectedId
   * @param {!HTMLTableCellElement} detailCell
   */
  var handleDeviceDetail = function(state, selectedId, detailCell) {
    if (selectedId == 'shill') {
      chrome.send('getShillDeviceProperties', [state.Type]);
    } else {
      showDetail(detailCell, state);
    }
  };

  /**
   * @param {!HTMLTableCellElement} detailCell
   * @param {!CrOnc.NetworkStateProperties|!CrOnc.DeviceStateProperties|
   *     !chrome.networkingPrivate.ManagedProperties|
   *     !chrome.networkingPrivate.NetworkProperties} state
   * @param {!Object=} error
   */
  var showDetail = function(detailCell, state, error) {
    if (error && error.message)
      detailCell.textContent = error.message;
    else
      detailCell.textContent = JSON.stringify(state, null, '\t');
  };

  /**
   * Callback invoked by Chrome after a getShillNetworkProperties call.
   *
   * @param {Array} args The requested Shill properties. Will contain
   *     just the 'GUID' and 'ShillError' properties if the call failed.
   */
  var getShillNetworkPropertiesResult = function(args) {
    var properties = args.shift();
    var guid = properties['GUID'];
    if (!guid) {
      console.error('No GUID in getShillNetworkPropertiesResult');
      return;
    }

    var detailCell = document.querySelector('td#' + idFromGuid(guid));
    if (!detailCell) {
      console.error('No cell for GUID: ' + guid);
      return;
    }

    if (properties['ShillError'])
      detailCell.textContent = properties['ShillError'];
    else
      detailCell.textContent = JSON.stringify(properties, null, '\t');

  };

  /**
   * Callback invoked by Chrome after a getShillDeviceProperties call.
   *
   * @param {Array} args The requested Shill properties. Will contain
   *     just the 'Type' and 'ShillError' properties if the call failed.
   */
  var getShillDevicePropertiesResult = function(args) {
    var properties = args.shift();
    var type = properties['Type'];
    if (!type) {
      console.error('No Type in getShillDevicePropertiesResult');
      return;
    }

    var detailCell = document.querySelector('td#' + idFromType(type));
    if (!detailCell) {
      console.error('No cell for Type: ' + type);
      return;
    }

    if (properties['ShillError'])
      detailCell.textContent = properties['ShillError'];
    else
      detailCell.textContent = JSON.stringify(properties, null, '\t');

  };

  /**
   * Requests an update of all network info.
   */
  var requestNetworks = function() {
    chrome.networkingPrivate.getNetworks(
        {
          'networkType': chrome.networkingPrivate.NetworkType.ALL,
          'visible': true
        },
        onVisibleNetworksReceived);
    chrome.networkingPrivate.getNetworks(
        {
          'networkType': chrome.networkingPrivate.NetworkType.ALL,
          'configured': true
        },
        onFavoriteNetworksReceived);
    chrome.networkingPrivate.getDeviceStates(onDeviceStatesReceived);
  };

  /**
   * Requests the global policy dictionary and updates the page.
   */
  var requestGlobalPolicy = function() {
    chrome.networkingPrivate.getGlobalPolicy(function(policy) {
      document.querySelector('#global-policy').textContent =
          JSON.stringify(policy, null, '\t');
    });
  };

  /**
   * Sets refresh rate if the interval is found in the url.
   */
  var setRefresh = function() {
    var interval = parseQueryParams(window.location)['refresh'];
    if (interval && interval != '')
      setInterval(requestNetworks, parseInt(interval, 10) * 1000);
  };

  /**
   * Gets network information from WebUI and sets custom items.
   */
  document.addEventListener('DOMContentLoaded', function() {
    let select = document.querySelector('cr-network-select');
    select.customItems = [
      {customItemName: 'Add WiFi', polymerIcon: 'cr:add', customData: 'WiFi'},
      {customItemName: 'Add VPN', polymerIcon: 'cr:add', customData: 'VPN'}
    ];
    $('refresh').onclick = requestNetworks;
    setRefresh();
    requestNetworks();
    requestGlobalPolicy();
  });

  document.addEventListener('custom-item-selected', function(event) {
    chrome.send('addNetwork', [event.detail.customData]);
  });

  return {
    getShillNetworkPropertiesResult: getShillNetworkPropertiesResult,
    getShillDevicePropertiesResult: getShillDevicePropertiesResult
  };
})();
