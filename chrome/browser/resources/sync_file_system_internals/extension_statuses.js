// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Handles the Extension ID -> SyncStatus tab for syncfs-internals.
 */
var ExtensionStatuses = (function() {
  'use strict';

  var ExtensionStatuses = {};

  /**
   * Get initial map of extension statuses (pending batch sync, enabled and
   * disabled).
   */
  function getExtensionStatuses() {
    chrome.send('getExtensionStatuses');
  }

  // TODO(calvinlo): Move to helper file so it doesn't need to be duplicated.
  /**
   * Creates an element named |elementName| containing the content |text|.
   * @param {string} elementName Name of the new element to be created.
   * @param {string} text Text to be contained in the new element.
   * @return {HTMLElement} The newly created HTML element.
   */
  function createElementFromText(elementName, text) {
    var element = document.createElement(elementName);
    element.appendChild(document.createTextNode(text));
    return element;
  }

  /**
   * Handles callback from onGetExtensionStatuses.
   * @param {Array} list of dictionaries containing 'extensionName',
   *     'extensionID, 'status'.
   */
  ExtensionStatuses.onGetExtensionStatuses = function(extensionStatuses) {
    var itemContainer = $('extension-entries');
    itemContainer.textContent = '';

    for (var i = 0; i < extensionStatuses.length; i++) {
      var originEntry = extensionStatuses[i];
      var tr = document.createElement('tr');
      tr.appendChild(createElementFromText('td', originEntry.extensionName));
      tr.appendChild(createElementFromText('td', originEntry.extensionID));
      tr.appendChild(createElementFromText('td', originEntry.status));
      itemContainer.appendChild(tr);
    }
  };

  function main() {
    getExtensionStatuses();
    $('refresh-extensions-statuses')
        .addEventListener('click', getExtensionStatuses);
  }

  document.addEventListener('DOMContentLoaded', main);
  return ExtensionStatuses;
})();
