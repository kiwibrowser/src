// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Converts a number in bytes to a string in megabytes split by comma into
 * three digit block.
 * @param {number} bytes The number in bytes.
 * @return {string} Formatted string in megabytes.
 */
function ToMegaByteString(bytes) {
  var mb = Math.floor(bytes / (1 << 20));
  return mb.toString().replace(
      /\d+?(?=(\d{3})+$)/g,  // Digit sequence (\d+) followed (?=) by 3n digits.
      function(three_digit_block) {
        return three_digit_block + ',';
      });
}

/**
 * Updates the Drive related Preferences section.
 * @param {Array} preferences List of dictionaries describing preferences.
 */
function updateDriveRelatedPreferences(preferences) {
  var ul = $('drive-related-preferences');
  updateKeyValueList(ul, preferences);
}

/**
 * Updates the Connection Status section.
 * @param {Object} connStatus Dictionary containing connection status.
 */
function updateConnectionStatus(connStatus) {
  $('connection-status').textContent = connStatus['status'];
  $('has-refresh-token').textContent = connStatus['has-refresh-token'];
  $('has-access-token').textContent = connStatus['has-access-token'];
}

/**
 * Updates the Path Configurations section.
 * @param {Array} paths List of dictionaries describing paths.
 */
function updatePathConfigurations(paths) {
  var ul = $('path-configurations');
  updateKeyValueList(ul, paths);
}

/**
 * Updates the GCache Contents section.
 * @param {Array} gcacheContents List of dictionaries describing metadata
 * of files and directories under the GCache directory.
 * @param {Object} gcacheSummary Dictionary of summary of GCache.
 */
function updateGCacheContents(gcacheContents, gcacheSummary) {
  var tbody = $('gcache-contents');
  for (var i = 0; i < gcacheContents.length; i++) {
    var entry = gcacheContents[i];
    var tr = document.createElement('tr');

    // Add some suffix based on the type.
    var path = entry.path;
    if (entry.is_directory)
      path += '/';
    else if (entry.is_symbolic_link)
      path += '@';

    tr.appendChild(createElementFromText('td', path));
    tr.appendChild(createElementFromText('td', entry.size));
    tr.appendChild(createElementFromText('td', entry.last_modified));
    tr.appendChild(createElementFromText('td', entry.permission));
    tbody.appendChild(tr);
  }

  $('gcache-summary-total-size').textContent =
      ToMegaByteString(gcacheSummary['total_size']);
}

/**
 * Updates the File System Contents section. The function is called from the
 * C++ side repeatedly with contents of a directory.
 * @param {string} directoryContentsAsText Pre-formatted string representation
 * of contents a directory in the file system.
 */
function updateFileSystemContents(directoryContentsAsText) {
  var div = $('file-system-contents');
  div.appendChild(createElementFromText('pre', directoryContentsAsText));
}

/**
 * Updates the Cache Contents section.
 * @param {Object} cacheEntry Dictionary describing a cache entry.
 * The function is called from the C++ side repeatedly.
 */
function updateCacheContents(cacheEntry) {
  var tr = document.createElement('tr');
  tr.appendChild(createElementFromText('td', cacheEntry.local_id));
  tr.appendChild(createElementFromText('td', cacheEntry.md5));
  tr.appendChild(createElementFromText('td', cacheEntry.is_present));
  tr.appendChild(createElementFromText('td', cacheEntry.is_pinned));
  tr.appendChild(createElementFromText('td', cacheEntry.is_dirty));

  $('cache-contents').appendChild(tr);
}

/**
 * Updates the Local Storage summary.
 * @param {Object} localStorageSummary Dictionary describing the status of local
 * stogage.
 */
function updateLocalStorageUsage(localStorageSummary) {
  var freeSpaceInMB = ToMegaByteString(localStorageSummary.free_space);
  $('local-storage-freespace').innerText = freeSpaceInMB;
}

/**
 * Updates the summary about in-flight operations.
 * @param {Array} inFlightOperations List of dictionaries describing the status
 * of in-flight operations.
 */
function updateInFlightOperations(inFlightOperations) {
  var container = $('in-flight-operations-contents');

  // Reset the table. Remove children in reverse order. Otherwides each
  // existingNodes[i] changes as a side effect of removeChild.
  var existingNodes = container.childNodes;
  for (var i = existingNodes.length - 1; i >= 0; i--) {
    var node = existingNodes[i];
    if (node.className == 'in-flight-operation')
      container.removeChild(node);
  }

  // Add in-flight operations.
  for (var i = 0; i < inFlightOperations.length; i++) {
    var operation = inFlightOperations[i];
    var tr = document.createElement('tr');
    tr.className = 'in-flight-operation';
    tr.appendChild(createElementFromText('td', operation.id));
    tr.appendChild(createElementFromText('td', operation.type));
    tr.appendChild(createElementFromText('td', operation.file_path));
    tr.appendChild(createElementFromText('td', operation.state));
    var progress = operation.progress_current + '/' + operation.progress_total;
    if (operation.progress_total > 0) {
      var percent = operation.progress_current / operation.progress_total * 100;
      progress += ' (' + Math.round(percent) + '%)';
    }
    tr.appendChild(createElementFromText('td', progress));

    container.appendChild(tr);
  }
}

/**
 * Updates the summary about about resource.
 * @param {Object} aboutResource Dictionary describing about resource.
 */
function updateAboutResource(aboutResource) {
  var quotaTotalInMb = ToMegaByteString(aboutResource['account-quota-total']);
  var quotaUsedInMb = ToMegaByteString(aboutResource['account-quota-used']);

  $('account-quota-info').textContent =
      quotaUsedInMb + ' / ' + quotaTotalInMb + ' (MB)';
  $('account-largest-changestamp-remote').textContent =
      aboutResource['account-largest-changestamp-remote'];
  $('root-resource-id').textContent = aboutResource['root-resource-id'];
}

/**
 * Updates the summary about app list.
 * @param {Object} appList Dictionary describing app list.
 */
function updateAppList(appList) {
  $('app-list-etag').textContent = appList['etag'];

  var itemContainer = $('app-list-items');
  for (var i = 0; i < appList['items'].length; i++) {
    var app = appList['items'][i];
    var tr = document.createElement('tr');
    tr.className = 'installed-app';
    tr.appendChild(createElementFromText('td', app.name));
    tr.appendChild(createElementFromText('td', app.application_id));
    tr.appendChild(createElementFromText('td', app.object_type));
    tr.appendChild(createElementFromText('td', app.supports_create));

    itemContainer.appendChild(tr);
  }
}

/**
 * Updates the local cache information about account metadata.
 * @param {Object} localMetadata Dictionary describing account metadata.
 */
function updateLocalMetadata(localMetadata) {
  var startPageToken = localMetadata['account-start-page-token-local'];

  $('account-largest-changestamp-local').textContent = startPageToken +
      (startPageToken ? ' (loaded)' : ' (not loaded)') +
      (localMetadata['account-metadata-refreshing'] ? ' (refreshing)' : '');
}

/**
 * Updates the summary about delta update status.
 * @param {Object} deltaUpdateStatus Dictionary describing delta update status.
 */
function updateDeltaUpdateStatus(deltaUpdateStatus) {
  $('push-notification-enabled').textContent =
      deltaUpdateStatus['push-notification-enabled'];
  $('last-update-check-time').textContent =
      deltaUpdateStatus['last-update-check-time'];
  $('last-update-check-error').textContent =
      deltaUpdateStatus['last-update-check-error'];
}

/**
 * Updates the event log section.
 * @param {Array} log Array of events.
 */
function updateEventLog(log) {
  var ul = $('event-log');
  updateKeyValueList(ul, log);
}

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
 * Updates <ul> element with the given key-value list.
 * @param {HTMLElement} ul <ul> element to be modified.
 * @param {Array} list List of dictionaries containing 'key', 'value' (optional)
 * and 'class' (optional). For each element <li> element with specified class is
 * created.
 */
function updateKeyValueList(ul, list) {
  for (var i = 0; i < list.length; i++) {
    var item = list[i];
    var text = item.key;
    if (item.value != '')
      text += ': ' + item.value;

    var li = createElementFromText('li', text);
    if (item.class)
      li.classList.add(item.class);
    ul.appendChild(li);
  }
}

/**
 * Updates the text next to the 'reset' button to update the status.
 * @param {boolean} success whether or not resetting has succeeded.
 */
function updateResetStatus(success) {
  $('reset-status-text').textContent = (success ? 'success' : 'failed');
}

document.addEventListener('DOMContentLoaded', function() {
  chrome.send('pageLoaded');

  // Update the table of contents.
  var toc = $('toc');
  var sections = document.getElementsByTagName('h2');
  for (var i = 0; i < sections.length; i++) {
    var section = sections[i];
    var a = createElementFromText('a', section.textContent);
    a.href = '#' + section.id;
    var li = document.createElement('li');
    li.appendChild(a);
    toc.appendChild(li);
  }

  $('button-clear-access-token').addEventListener('click', function() {
    chrome.send('clearAccessToken');
  });

  $('button-clear-refresh-token').addEventListener('click', function() {
    chrome.send('clearRefreshToken');
  });

  $('button-reset-drive-filesystem').addEventListener('click', function() {
    $('reset-status-text').textContent = 'resetting...';
    chrome.send('resetDriveFileSystem');
  });

  $('button-show-file-entries').addEventListener('click', function() {
    var button = $('button-show-file-entries');
    button.parentNode.removeChild(button);
    chrome.send('listFileEntries');
  });

  window.setInterval(function() {
    chrome.send('periodicUpdate');
  }, 1000);
});
