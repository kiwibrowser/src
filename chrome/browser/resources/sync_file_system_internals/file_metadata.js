// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * WebUI to monitor File Metadata per Extension ID.
 */
var FileMetadata = (function() {
  'use strict';

  var FileMetadata = {};

  /**
   * Gets extension data so the select drop down can be filled.
   */
  function getExtensions() {
    chrome.send('getExtensions');
  }

  /**
   * Renders result of getFileMetadata as a table.
   * @param {Array} list of dictionaries containing 'extensionName',
   *     'extensionID', 'status'.
   */
  FileMetadata.onGetExtensions = function(extensionStatuses) {
    var select = $('extensions-select');

    // Record existing drop down extension ID. If it's still there after the
    // refresh then keep it as the selected value.
    var oldSelectedExtension = getSelectedExtensionId();

    select.textContent = '';
    for (var i = 0; i < extensionStatuses.length; i++) {
      var originEntry = extensionStatuses[i];
      var tr = document.createElement('tr');
      var title = originEntry.extensionName + ' [' + originEntry.status + ']';
      select.options.add(new Option(title, originEntry.extensionID));

      // If option was the previously only selected, make it selected again.
      if (originEntry.extensionID != oldSelectedExtension)
        continue;
      select.options[select.options.length - 1].selected = true;
    }

    // After drop down has been loaded with options, file metadata can be loaded
    getFileMetadata();
  };

  /**
   * @return {string} extension ID that's currently selected in drop down box.
   */
  function getSelectedExtensionId() {
    var dropDown = $('extensions-select').options;
    if (dropDown.selectedIndex >= 0)
      return dropDown[dropDown.selectedIndex].value;

    return null;
  }

  /**
   * Get File Metadata depending on which extension is selected from the drop
   * down if any.
   */
  function getFileMetadata() {
    var dropDown = $('extensions-select');
    if (dropDown.options.length === 0) {
      $('file-metadata-header').textContent = '';
      $('file-metadata-entries').textContent = 'No file metadata available.';
      return;
    }

    var selectedExtensionId = getSelectedExtensionId();
    chrome.send('getFileMetadata', [selectedExtensionId]);
  }

  /**
   * Renders result of getFileMetadata as a table.
   */
  FileMetadata.onGetFileMetadata = function(fileMetadataMap) {
    var header = $('file-metadata-header');
    // Only draw the header if it hasn't been drawn yet
    if (header.children.length === 0) {
      var tr = document.createElement('tr');
      tr.appendChild(createElementFromText('td', 'Type'));
      tr.appendChild(createElementFromText('td', 'Status'));
      tr.appendChild(createElementFromText('td', 'Path', {width: '250px'}));
      tr.appendChild(createElementFromText('td', 'Details'));
      header.appendChild(tr);
    }

    // Add row entries.
    var itemContainer = $('file-metadata-entries');
    itemContainer.textContent = '';
    for (var i = 0; i < fileMetadataMap.length; i++) {
      var metadatEntry = fileMetadataMap[i];
      var tr = document.createElement('tr');
      tr.appendChild(createFileIconCell(metadatEntry.type));
      tr.appendChild(createElementFromText('td', metadatEntry.status));
      tr.appendChild(createElementFromText('td', metadatEntry.path));
      tr.appendChild(createElementFromDictionary('td', metadatEntry.details));
      itemContainer.appendChild(tr);
    }
  };

  /**
   * @param {string} file type string.
   * @return {HTMLElement} TD with file or folder icon depending on type.
   */
  function createFileIconCell(type) {
    var img = document.createElement('div');
    var lowerType = type.toLowerCase();
    if (lowerType == 'file') {
      img.style.content =
          cr.icon.getImage('chrome://theme/IDR_DEFAULT_FAVICON');
    } else if (lowerType == 'folder') {
      img.style.content = cr.icon.getImage('chrome://theme/IDR_FOLDER_CLOSED');
      img.className = 'folder-image';
    }

    var imgWrapper = document.createElement('div');
    imgWrapper.appendChild(img);

    var td = document.createElement('td');
    td.className = 'file-icon-cell';
    td.appendChild(imgWrapper);
    td.appendChild(document.createTextNode(type));
    return td;
  }

  function main() {
    getExtensions();
    $('refresh-metadata-button').addEventListener('click', getExtensions);
    $('extensions-select').addEventListener('change', getFileMetadata);
  }

  document.addEventListener('DOMContentLoaded', main);
  return FileMetadata;
})();
