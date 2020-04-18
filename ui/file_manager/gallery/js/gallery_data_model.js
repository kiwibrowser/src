// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Data model for gallery.
 *
 * @param {!MetadataModel} metadataModel
 * @param {!EntryListWatcher=} opt_watcher Entry list watcher.
 * @constructor
 * @extends {cr.ui.ArrayDataModel}
 */
function GalleryDataModel(metadataModel, opt_watcher) {
  cr.ui.ArrayDataModel.call(this, []);

  /**
   * File system metadata.
   * @private {!MetadataModel}
   * @const
   */
  this.metadataModel_ = metadataModel;

  /**
   * Directory where the image is saved if the image is located in a read-only
   * volume.
   * @public {DirectoryEntry}
   */
  this.fallbackSaveDirectory = null;

  // Start to watch file system entries.
  var watcher = opt_watcher ? opt_watcher : new EntryListWatcher(this);
  watcher.getEntry = function(item) { return item.getEntry(); };

  this.addEventListener('splice', this.onSplice_.bind(this));
}

/**
 * Maximum number of full size image cache.
 * @type {number}
 * @const
 * @private
 */
GalleryDataModel.MAX_FULL_IMAGE_CACHE_ = 3;

GalleryDataModel.prototype = {
  __proto__: cr.ui.ArrayDataModel.prototype
};

/**
 * Saves new image.
 *
 * @param {!VolumeManagerWrapper} volumeManager Volume manager instance.
 * @param {!GalleryItem} item Original gallery item.
 * @param {!HTMLCanvasElement} canvas Canvas containing new image.
 * @param {boolean} overwrite Set true to overwrite original if it's possible.
 * @return {!Promise} Promise to be fulfilled with when the operation completes.
 */
GalleryDataModel.prototype.saveItem = function(
    volumeManager, item, canvas, overwrite) {
  var oldEntry = item.getEntry();
  var oldLocationInfo = item.getLocationInfo();
  var oldIsOriginal = item.isOriginal();
  return new Promise(function(fulfill, reject) {
    item.saveToFile(
        volumeManager,
        this.metadataModel_,
        assert(this.fallbackSaveDirectory),
        canvas,
        overwrite,
        function(success) {
          if (!success) {
            reject('Failed to save the image.');
            return;
          }

          // Current entry is updated.
          // Dispatch an event.
          var event = new Event('content');
          event.item = item;
          event.oldEntry = oldEntry;
          event.thumbnailChanged = true;
          this.dispatchEvent(event);

          if (!util.isSameEntry(oldEntry, item.getEntry())) {
            Promise.all([
              this.metadataModel_.get(
                  [oldEntry], GalleryItem.PREFETCH_PROPERTY_NAMES),
              new ThumbnailModel(this.metadataModel_).get([oldEntry])
            ]).then(function(itemLists) {
              // New entry is added and the item now tracks it.
              // Add another item for the old entry.
              var anotherItem = new GalleryItem(
                  oldEntry,
                  oldLocationInfo,
                  itemLists[0][0],
                  itemLists[1][0],
                  oldIsOriginal);
              // The item must be added behind the existing item so that it does
              // not change the index of the existing item.
              // TODO(hirono): Update the item index of the selection model
              // correctly.
              this.splice(this.indexOf(item) + 1, 0, anotherItem);
            }.bind(this)).then(fulfill, reject);
          } else {
            fulfill();
          }
        }.bind(this));
  }.bind(this));
};

/**
 * Evicts image caches in the items.
 */
GalleryDataModel.prototype.evictCache = function() {
  // Sort the item by the last accessed date.
  var sorted = this.slice().sort(function(a, b) {
    return b.getLastAccessedDate() - a.getLastAccessedDate();
  });

  // Evict caches.
  var contentCacheCount = 0;
  var screenCacheCount = 0;
  for (var i = 0; i < sorted.length; i++) {
    if (sorted[i].contentImage) {
      if (++contentCacheCount > GalleryDataModel.MAX_FULL_IMAGE_CACHE_) {
        if (sorted[i].contentImage.parentNode) {
          console.error('The content image has a parent node.');
        } else {
          // Force to free the buffer of the canvas by assigning zero size.
          sorted[i].contentImage.width = 0;
          sorted[i].contentImage.height = 0;
          sorted[i].contentImage = null;
        }
      }
    }
  }
};

/**
 * Handles entry delete.
 * @param {!Event} event
 * @private
 */
GalleryDataModel.prototype.onSplice_ = function(event) {
  if (!event.removed || !event.removed.length)
    return;
  var removedURLs = event.removed.map(function(item) {
    return item.getEntry().toURL();
  });
  this.metadataModel_.notifyEntriesRemoved(removedURLs);
};
