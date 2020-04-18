// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Watcher for entry lists.
 * It watches entries and remove the item if the entry is removed from file
 * system.
 * @param {!cr.ui.ArrayDataModel} list
 * @constructor
 * @struct
 */
function EntryListWatcher(list) {
  /**
   * Watched list.
   * @type {!cr.ui.ArrayDataModel}
   * @const
   * @private
   */
  this.list_ = list;

  /**
   * Set of watched URL.
   * Key is watched entry URL. Value is always true.
   * @type {!Object<boolean>}
   * @private
   */
  this.watchers_ = {};

  this.list_.addEventListener('splice', this.onSplice_.bind(this));
  chrome.fileManagerPrivate.onDirectoryChanged.addListener(
      this.onDirectoryChanged_.bind(this));

  this.onSplice_(null);
}

/**
 * Obtains entry from ArrayDataModel's item.
 * @param {*} item Item in ArrayDataModel.
 * @return {!FileEntry}
 */
EntryListWatcher.prototype.getEntry = function(item) {
  return /** @type {!FileEntry} */ (item);
};

/**
 * @param {Event} event
 * @private
 */
EntryListWatcher.prototype.onSplice_ = function(event) {
  // TODO(mtomasz, hirono): Remove operations on URLs as they won't work after
  // switching to isolated entries.

  // Mark all existing watchers as candidates to be removed.
  var diff = {};
  for (var url in this.watchers_) {
    diff[url] = -1;
  }

  // Obtains the set of new watcher.
  this.watchers_ = {};
  for (var i = 0; i < this.list_.length; i++) {
    var entry = this.getEntry(this.list_.item(i));
    var parentURL = entry.toURL().replace(/[^\/]+\/?$/, '');
    this.watchers_[parentURL] = true;
  }

  // Mark new watchers to be added, and existing watchers to be kept.
  for (var url in this.watchers_) {
    diff[url] = (diff[url] || 0) + 1;
  }

  // Check the number in diff.
  // -1: watcher exists in the old set, but does not exists in the new set.
  // 0: watcher exists in both sets.
  // 1: watcher does not exists in the old set, but exists in the new set.
  var reportError = function() {
    if (chrome.runtime.lastError)
      console.error(chrome.runtime.lastError.name);
  };
  for (var url in diff) {
    switch (diff[url]) {
      case 1:
        window.webkitResolveLocalFileSystemURL(url, function(entry) {
          reportError();
          chrome.fileManagerPrivate.addFileWatch(entry, reportError);
        });
        break;
      case -1:
        window.webkitResolveLocalFileSystemURL(url, function(entry) {
          reportError();
          chrome.fileManagerPrivate.removeFileWatch(entry, reportError);
        });
        break;
      case 0:
        break;
      default:
        assertNotReached();
        break;
    }
  }
};

/**
 * @param {!FileWatchEvent} event
 * @private
 */
EntryListWatcher.prototype.onDirectoryChanged_ = function(event) {
  // Add '/' to the tail for checking if the each entry's URL is child URL of
  // the URL or not by using entryURL.indexOf(thisUrl) === 0.
  var url = event.entry.toURL().replace(/\/?$/, '/');
  var promiseList = [];
  var removedEntryURL = {};
  for (var i = 0; i < this.list_.length; i++) {
    var entry = this.getEntry(this.list_.item(i));
    // Remove trailing '/' to prevent calling getMetadata in the case where the
    // event.entry and the entry are same.
    var entryURL = entry.toURL().replace(/\/$/, '');
    if (entry.toURL().indexOf(url) !== 0)
      continue;
    // Look for non-existing files by using getMetadata.
    // getMetadata returns NOT_FOUND error for non-existing files.
    promiseList.push(new Promise(entry.getMetadata.bind(entry)).catch(
        function(url) {
          removedEntryURL[url] = true;
        }.bind(null, entry.toURL())));
  }
  Promise.all(promiseList).then(function() {
    var i = 0;
    while (i < this.list_.length) {
      var url = this.getEntry(this.list_.item(i)).toURL();
      if (removedEntryURL[url]) {
        this.list_.splice(i, 1);
      } else {
        i++;
      }
    }
  }.bind(this));
};
