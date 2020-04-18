// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var GalleryUtil = {};

/**
 * Obtains the entry set from the entries passed from onLaunched events.
 * If an single entry is specified, the function returns all entries in the same
 * directory. Otherwise the function returns the passed entries.
 *
 * The function also filters non-image items and hidden items.
 *
 * @param {!Array<!FileEntry>} originalEntries Entries passed from onLaunched
 *     events.
 * @return {!Promise} Promise to be fulfilled with entry array.
 */
GalleryUtil.createEntrySet = function(originalEntries) {
  var entriesPromise;
  if (originalEntries.length === 1) {
    var parentPromise =
        new Promise(originalEntries[0].getParent.bind(originalEntries[0]));
    entriesPromise = parentPromise.then(function(parent) {
      var reader = parent.createReader();
      var readEntries = function() {
        return new Promise(reader.readEntries.bind(reader)).then(
            function(entries) {
              if (entries.length === 0)
                return [];
              return readEntries().then(function(nextEntries) {
                return entries.concat(nextEntries);
              });
            });
      };
      return readEntries();
    }).then(function(entries) {
      return entries.filter(function(entry) {
        return originalEntries[0].toURL() === entry.toURL() ||
            entry.name[0] !== '.';
      });
    });
  } else {
    entriesPromise = Promise.resolve(originalEntries);
  }

  return entriesPromise.then(function(entries) {
    return entries.filter(function(entry) {
      // Currently the gallery doesn't support mime types, so checking by
      // file extensions is enough.
      return FileType.isImage(entry) || FileType.isRaw(entry);
    }).sort(function(a, b) {
      return util.compareName(a, b);
    });
  });
};

/**
 * Returns true if entry is on MTP volume.
 * @param {!Entry} entry An entry.
 * @param {!VolumeManagerWrapper} volumeManager Volume manager.
 * @return True if entry is on MTP volume.
 */
GalleryUtil.isOnMTPVolume = function(entry, volumeManager) {
  var volumeInfo = volumeManager.getVolumeInfo(entry);
  return volumeInfo &&
      volumeInfo.volumeType === VolumeManagerCommon.VolumeType.MTP;
};

/**
 * Decorates an element to handle mouse focus specific logic. The element
 * becomes to have using-mouse class when it is focused by mouse.
 * @param {!HTMLElement} element
 */
GalleryUtil.decorateMouseFocusHandling = function(element) {
  element.addEventListener('mousedown',
      element.classList.toggle.bind(element.classList, 'using-mouse', true));
  element.addEventListener('blur',
      element.classList.toggle.bind(element.classList, 'using-mouse', false));
};
