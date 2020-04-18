// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// require cr.js
// require cr/event_target.js
// require cr/ui.js
// require cr/ui/tabs.js
// require cr/ui/tree.js
// require cr/util.js

(function() {
'use strict';

/**
 * @param {Object} object Object to be checked.
 * @return {boolean} true if |object| is {}.
 * @private
 */
function isEmptyObject_(object) {
  for (var i in object)
    return false;
  return true;
}

/**
 * Copy properties from |source| to |destination|.
 * @param {Object} source Source of the copy.
 * @param {Object} destination Destination of the copy.
 * @return {Object} |destination|.
 * @private
 */
function copyAttributes_(source, destination) {
  for (var i in source)
    destination[i] = source[i];
  return destination;
}

/**
 * Apply localization to |element| with i18n_template.js if available.
 * @param {Element} element Element to be localized.
 * @private
 */
function localize_(element) {
  if (window.i18nTemplate && window.loadTimeData)
    i18nTemplate.process(element, loadTimeData);
}

/**
 * Returns 'N/A' (Not Available) text if |value| is undefined.
 * @param {Object} value Object to print.
 * @return {string} 'N/A' or ''.
 * @private
 */
function checkIfAvailable_(value) {
  return value === undefined ? 'N/A' : '';
}

/**
 * Returns |value| itself if |value| is not undefined,
 * else returns 'N/A' text.
 * @param {?string} value String to print.
 * @return {string} 'N/A' or |value|.
 * @private
 */
function stringToText_(value) {
  return checkIfAvailable_(value) || value;
}

/**
 * Separates |value| into segments.
 * The length of first segment is at most |maxLength|.
 * Length of other following segments are just |maxLength|.
 * e.g. separateBackward_('abcdefghijk', 4) == ['abc','defg','hijk'];
 * @param {string} value String to be separated.
 * @param {number} maxLength Max length of segments.
 * @return {Array<string>} Array of segments.
 * @private
 */
function separateBackward_(value, maxLength) {
  var result = [];
  while (value.length > maxLength) {
    result.unshift(value.slice(-3));
    value = value.slice(0, -3);
  }
  result.unshift(value);
  return result;
}

/**
 * Returns formatted string from number as number of bytes.
 * e.g. numBytesToText(123456789) = '123.45 MB (123,456,789 B)'.
 * If |value| is undefined, this function returns 'N/A'.
 * @param {?number} value Number to print.
 * @return {string} 'N/A' or formatted |value|.
 * @private
 */
function numBytesToText_(value) {
  var result = checkIfAvailable_(value);
  if (result)
    return result;

  var segments = separateBackward_(value.toString(), 3);
  result = segments.join(',') + ' B';

  if (segments.length > 1) {
    var UNIT = [' B', ' KB', ' MB', ' GB', ' TB', ' PB'];
    result = segments[0] + '.' + segments[1].slice(0, 2) +
        UNIT[Math.min(segments.length, UNIT.length) - 1] + ' (' + result + ')';
  }

  return result;
}

/**
 * Return formatted date |value| if |value| is not undefined.
 * If |value| is undefined, this function returns 'N/A'.
 * @param {?number} value Number of milliseconds since
 *   UNIX epoch time (0:00, Jan 1, 1970, UTC).
 * @return {string} Formatted text of date or 'N/A'.
 * @private
 */
function dateToText(value) {
  var result = checkIfAvailable_(value);
  if (result)
    return result;

  var time = new Date(value);
  var now = new Date();
  var delta = Date.now() - value;

  var SECOND = 1000;
  var MINUTE = 60 * SECOND;
  var HOUR = 60 * MINUTE;
  var DAY = 23 * HOUR;
  var WEEK = 7 * DAY;

  var SHOW_SECOND = 5 * MINUTE;
  var SHOW_MINUTE = 5 * HOUR;
  var SHOW_HOUR = 3 * DAY;
  var SHOW_DAY = 2 * WEEK;
  var SHOW_WEEK = 3 * 30 * DAY;

  if (delta < 0) {
    result = 'access from future ';
  } else if (delta < SHOW_SECOND) {
    result = Math.ceil(delta / SECOND) + ' sec ago ';
  } else if (delta < SHOW_MINUTE) {
    result = Math.ceil(delta / MINUTE) + ' min ago ';
  } else if (delta < SHOW_HOUR) {
    result = Math.ceil(delta / HOUR) + ' hr ago ';
  } else if (delta < SHOW_WEEK) {
    result = Math.ceil(delta / DAY) + ' day ago ';
  }

  result += '(' + time.toString() + ')';
  return result;
}

/**
 * Available disk space.
 * @type {number|undefined}
 */
var availableSpace = undefined;

/**
 * Root of the quota data tree,
 * holding userdata as |treeViewObject.detail|.
 * @type {cr.ui.Tree}
 */
var treeViewObject = undefined;

/**
 * Key-value styled statistics data.
 * This WebUI does not touch contents, just show.
 * The value is hold as |statistics[key].detail|.
 * @type {Object<string,Element>}
 */
var statistics = {};

/**
 * Initialize and return |treeViewObject|.
 * @return {cr.ui.Tree} Initialized |treeViewObject|.
 */
function getTreeViewObject() {
  if (!treeViewObject) {
    treeViewObject = $('tree-view');
    cr.ui.decorate(treeViewObject, cr.ui.Tree);
    treeViewObject.detail = {payload: {}, children: {}};
    treeViewObject.addEventListener('change', updateDescription);
  }
  return treeViewObject;
}

/**
 * Initialize and return a tree item, that represents specified storage type.
 * @param {!string} type Storage type.
 * @return {cr.ui.TreeItem} Initialized |storageObject|.
 */
function getStorageObject(type) {
  var treeViewObject = getTreeViewObject();
  var storageObject = treeViewObject.detail.children[type];
  if (!storageObject) {
    storageObject =
        new cr.ui.TreeItem({label: type, detail: {payload: {}, children: {}}});
    storageObject.mayHaveChildren_ = true;
    treeViewObject.detail.children[type] = storageObject;
    treeViewObject.add(storageObject);
  }
  return storageObject;
}

/**
 * Initialize and return a tree item, that represents specified
 *  storage type and hostname.
 * @param {!string} type Storage type.
 * @param {!string} host Hostname.
 * @return {cr.ui.TreeItem} Initialized |hostObject|.
 */
function getHostObject(type, host) {
  var storageObject = getStorageObject(type);
  var hostObject = storageObject.detail.children[host];
  if (!hostObject) {
    hostObject =
        new cr.ui.TreeItem({label: host, detail: {payload: {}, children: {}}});
    hostObject.mayHaveChildren_ = true;
    storageObject.detail.children[host] = hostObject;
    storageObject.add(hostObject);
  }
  return hostObject;
}

/**
 * Initialize and return a tree item, that represents specified
 * storage type, hostname and origin url.
 * @param {!string} type Storage type.
 * @param {!string} host Hostname.
 * @param {!string} origin Origin URL.
 * @return {cr.ui.TreeItem} Initialized |originObject|.
 */
function getOriginObject(type, host, origin) {
  var hostObject = getHostObject(type, host);
  var originObject = hostObject.detail.children[origin];
  if (!originObject) {
    originObject = new cr.ui.TreeItem(
        {label: origin, detail: {payload: {}, children: {}}});
    originObject.mayHaveChildren_ = false;
    hostObject.detail.children[origin] = originObject;
    hostObject.add(originObject);
  }
  return originObject;
}

/**
 * Event Handler for |cr.quota.onAvailableSpaceUpdated|.
 * |event.detail| contains |availableSpace|.
 * |availableSpace| represents total available disk space.
 * @param {CustomEvent} event AvailableSpaceUpdated event.
 */
function handleAvailableSpace(event) {
  /**
   * @type {string}
   */
  availableSpace = event.detail;
  $('diskspace-entry').innerHTML = numBytesToText_(availableSpace);
}

/**
 * Event Handler for |cr.quota.onGlobalInfoUpdated|.
 * |event.detail| contains a record which has:
 *   |type|:
 *     Storage type, that is either 'temporary' or 'persistent'.
 *   |usage|:
 *     Total storage usage of all hosts.
 *   |unlimitedUsage|:
 *     Total storage usage of unlimited-quota origins.
 *   |quota|:
 *     Total quota of the storage.
 *
 *  |usage|, |unlimitedUsage| and |quota| can be missing,
 *  and some additional fields can be included.
 * @param {CustomEvent} event GlobalInfoUpdated event.
 */
function handleGlobalInfo(event) {
  /**
   * @type {{
   *         type: {!string},
   *         usage: {?number},
   *         unlimitedUsage: {?number}
   *         quota: {?string}
   *       }}
   */
  var data = event.detail;
  var storageObject = getStorageObject(data.type);
  copyAttributes_(data, storageObject.detail.payload);
  storageObject.reveal();
  if (getTreeViewObject().selectedItem == storageObject)
    updateDescription();
}

/**
 * Event Handler for |cr.quota.onPerHostInfoUpdated|.
 * |event.detail| contains records which have:
 *   |host|:
 *     Hostname of the entry. (e.g. 'example.com')
 *   |type|:
 *     Storage type. 'temporary' or 'persistent'
 *   |usage|:
 *     Total storage usage of the host.
 *   |quota|:
 *     Per-host quota.
 *
 * |usage| and |quota| can be missing,
 * and some additional fields can be included.
 * @param {CustomEvent} event PerHostInfoUpdated event.
 */
function handlePerHostInfo(event) {
  /**
   * @type {Array<{
   *         host: {!string},
   *         type: {!string},
   *         usage: {?number},
   *         quota: {?number}
   *       }}
   */
  var dataArray = event.detail;

  for (var i = 0; i < dataArray.length; ++i) {
    var data = dataArray[i];
    var hostObject = getHostObject(data.type, data.host);
    copyAttributes_(data, hostObject.detail.payload);
    hostObject.reveal();
    if (getTreeViewObject().selectedItem == hostObject)
      updateDescription();
  }
}

/**
 * Event Handler for |cr.quota.onPerOriginInfoUpdated|.
 * |event.detail| contains records which have:
 *   |origin|:
 *     Origin URL of the entry.
 *   |type|:
 *     Storage type of the entry. 'temporary' or 'persistent'.
 *   |host|:
 *     Hostname of the entry.
 *   |inUse|:
 *     true if the origin is in use.
 *   |usedCount|:
 *     Used count of the storage from the origin.
 *   |lastAccessTime|:
 *     Last storage access time from the origin.
 *     Number of milliseconds since UNIX epoch (Jan 1, 1970, 0:00:00 UTC).
 *   |lastModifiedTime|:
 *     Last modified time of the storage from the origin.
 *     Number of milliseconds since UNIX epoch.
 *
 * |inUse|, |usedCount|, |lastAccessTime| and |lastModifiedTime| can be missing,
 * and some additional fields can be included.
 * @param {CustomEvent} event PerOriginInfoUpdated event.
 */
function handlePerOriginInfo(event) {
  /**
   * @type {Array<{
   *         origin: {!string},
   *         type: {!string},
   *         host: {!string},
   *         inUse: {?boolean},
   *         usedCount: {?number},
   *         lastAccessTime: {?number}
   *         lastModifiedTime: {?number}
   *       }>}
   */
  var dataArray = event.detail;

  for (var i = 0; i < dataArray.length; ++i) {
    var data = dataArray[i];
    var originObject = getOriginObject(data.type, data.host, data.origin);
    copyAttributes_(data, originObject.detail.payload);
    originObject.reveal();
    if (getTreeViewObject().selectedItem == originObject)
      updateDescription();
  }
}

/**
 * Event Handler for |cr.quota.onStatisticsUpdated|.
 * |event.detail| contains misc statistics data as dictionary.
 * @param {CustomEvent} event StatisticsUpdated event.
 */
function handleStatistics(event) {
  /**
   * @type {Object<string>}
   */
  var data = event.detail;
  for (var key in data) {
    var entry = statistics[key];
    if (!entry) {
      entry = cr.doc.createElement('tr');
      $('stat-entries').appendChild(entry);
      statistics[key] = entry;
    }
    entry.detail = data[key];
    entry.innerHTML = '<td>' + stringToText_(key) + '</td>' +
        '<td>' + stringToText_(entry.detail) + '</td>';
    localize_(entry);
  }
}

/**
 * Update description on 'tree-item-description' field with
 * selected item in tree view.
 */
function updateDescription() {
  var item = getTreeViewObject().selectedItem;
  var tbody = $('tree-item-description');
  tbody.innerHTML = '';

  if (item) {
    var keyAndLabel = [
      ['type', 'Storage Type'], ['host', 'Host Name'], ['origin', 'Origin URL'],
      ['usage', 'Total Storage Usage', numBytesToText_],
      ['unlimitedUsage', 'Usage of Unlimited Origins', numBytesToText_],
      ['quota', 'Quota', numBytesToText_], ['inUse', 'Origin is in use?'],
      ['usedCount', 'Used count'],
      ['lastAccessTime', 'Last Access Time', dateToText],
      ['lastModifiedTime', 'Last Modified Time', dateToText]
    ];
    for (var i = 0; i < keyAndLabel.length; ++i) {
      var key = keyAndLabel[i][0];
      var label = keyAndLabel[i][1];
      var entry = item.detail.payload[key];
      if (entry === undefined)
        continue;

      var normalize = keyAndLabel[i][2] || stringToText_;

      var row = cr.doc.createElement('tr');
      row.innerHTML = '<td>' + label + '</td>' +
          '<td>' + normalize(entry) + '</td>';
      localize_(row);
      tbody.appendChild(row);
    }
  }
}

/**
 * Dump |treeViewObject| or subtree to a object.
 * @param {?{cr.ui.Tree|cr.ui.TreeItem}} opt_treeitem
 * @return {Object} Dump result object from |treeViewObject|.
 */
function dumpTreeToObj(opt_treeitem) {
  var treeitem = opt_treeitem || getTreeViewObject();
  var res = {};
  res.payload = treeitem.detail.payload;
  res.children = [];
  for (var i in treeitem.detail.children) {
    var child = treeitem.detail.children[i];
    res.children.push(dumpTreeToObj(child));
  }

  if (isEmptyObject_(res.payload))
    delete res.payload;

  if (res.children.length == 0)
    delete res.children;
  return res;
}

/**
 * Dump |statistics| to a object.
 * @return {Object} Dump result object from |statistics|.
 */
function dumpStatisticsToObj() {
  var result = {};
  for (var key in statistics)
    result[key] = statistics[key].detail;
  return result;
}

/**
 * Event handler for 'dump-button' 'click'ed.
 * Dump and show all data from WebUI page to 'dump-field' element.
 */
function dump() {
  var separator = '========\n';

  $('dump-field').textContent = separator + 'Summary\n' + separator +
      JSON.stringify({availableSpace: availableSpace}, null, 2) + '\n' +
      separator + 'Usage And Quota\n' + separator +
      JSON.stringify(dumpTreeToObj(), null, 2) + '\n' + separator +
      'Misc Statistics\n' + separator +
      JSON.stringify(dumpStatisticsToObj(), null, 2);
}

function onLoad() {
  cr.ui.decorate('tabbox', cr.ui.TabBox);

  cr.quota.onAvailableSpaceUpdated.addEventListener(
      'update', handleAvailableSpace);
  cr.quota.onGlobalInfoUpdated.addEventListener('update', handleGlobalInfo);
  cr.quota.onPerHostInfoUpdated.addEventListener('update', handlePerHostInfo);
  cr.quota.onPerOriginInfoUpdated.addEventListener(
      'update', handlePerOriginInfo);
  cr.quota.onStatisticsUpdated.addEventListener('update', handleStatistics);
  cr.quota.requestInfo();

  $('refresh-button').addEventListener('click', cr.quota.requestInfo, false);
  $('dump-button').addEventListener('click', dump, false);
}

cr.doc.addEventListener('DOMContentLoaded', onLoad, false);
})();
