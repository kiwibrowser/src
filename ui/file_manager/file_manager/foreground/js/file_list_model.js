// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * File list.
 * @param {!MetadataModel} metadataModel
 * @constructor
 * @extends {cr.ui.ArrayDataModel}
 */
function FileListModel(metadataModel) {
  cr.ui.ArrayDataModel.call(this, []);

  /**
   * @private {!MetadataModel}
   * @const
   */
  this.metadataModel_ = metadataModel;

  // Initialize compare functions.
  this.setCompareFunction('name',
      /** @type {function(*, *): number} */ (this.compareName_.bind(this)));
  this.setCompareFunction('modificationTime',
      /** @type {function(*, *): number} */ (this.compareMtime_.bind(this)));
  this.setCompareFunction('size',
      /** @type {function(*, *): number} */ (this.compareSize_.bind(this)));
  this.setCompareFunction('type',
      /** @type {function(*, *): number} */ (this.compareType_.bind(this)));

  /**
   * Whether this file list is sorted in descending order.
   * @type {boolean}
   * @private
   */
  this.isDescendingOrder_ = false;

  /**
   * The number of folders in the list.
   * @private {number}
   */
  this.numFolders_ = 0;

  /**
   * The number of files in the list.
   * @private {number}
   */
  this.numFiles_ = 0;

  /**
   * The number of image files in the list.
   * @private {number}
   */
  this.numImageFiles_ = 0;

  /**
   * Whether to use modificationByMeTime as "Last Modified" time.
   * @private {boolean}
   */
  this.useModificationByMeTime_ = false;
}

/**
 * @param {!Object} fileType Type object returned by FileType.getType().
 * @return {string} Localized string representation of file type.
 */
FileListModel.getFileTypeString = function(fileType) {
  if (fileType.subtype)
    return strf(fileType.name, fileType.subtype);
  else
    return str(fileType.name);
};

FileListModel.prototype = {
  __proto__: cr.ui.ArrayDataModel.prototype
};

/**
 * Sorts data model according to given field and direction and dispathes
 * sorted event.
 * @param {string} field Sort field.
 * @param {string} direction Sort direction.
 * @override
 */
FileListModel.prototype.sort = function(field, direction) {
  this.isDescendingOrder_ = direction === 'desc';
  cr.ui.ArrayDataModel.prototype.sort.call(this, field, direction);
};

/**
 * Called before a sort happens so that you may fetch additional data
 * required for the sort.
 * @param {string} field Sort field.
 * @param {function()} callback The function to invoke when preparation
 *     is complete.
 * @override
 */
FileListModel.prototype.prepareSort = function(field, callback) {
  // Starts the actual sorting immediately as we don't need any preparation to
  // sort the file list and we want to start actual sorting as soon as possible
  // after we get the |this.isDescendingOrder_| value in sort().
  callback();
};

/**
 * Removes and adds items to the model.
 *
 * The implementation is similar to cr.ui.ArrayDataModel.splice(), but this has
 * a Files app specific optimization, which sorts only the new items and merge
 * sorted lists.
 * Note that this implementation assumes following conditions.
 * - The list is always sorted.
 * - FileListModel does't have to do anything in prepareSort().
 *
 * @param {number} index The index of the item to update.
 * @param {number} deleteCount The number of items to remove.
 * @param {...*} var_args The items to add.
 * @return {!Array} An array with the removed items.
 * @override
 */
FileListModel.prototype.splice = function(index, deleteCount, var_args) {
  var insertPos = Math.max(0, Math.min(index, this.indexes_.length));
  deleteCount = Math.min(deleteCount, this.indexes_.length - insertPos);

  for (var i = insertPos; i < insertPos + deleteCount; i++) {
    this.onRemoveEntryFromList_(this.array_[this.indexes_[i]]);
  }
  for (var i = 2; i < arguments.length; i++) {
    this.onAddEntryToList_(arguments[i]);
  }

  // Prepare a comparison function to sort the list.
  var comp = null;
  if (this.sortStatus.field && this.compareFunctions_) {
    var compareFunction = this.compareFunctions_[this.sortStatus.field];
    if (compareFunction) {
      var dirMultiplier = this.sortStatus.direction === 'desc' ? -1 : 1;
      comp = function(a, b) {
        return compareFunction(a, b) * dirMultiplier;
      };
    }
  }

  // Store the given new items in |newItems| and sort it before marge them to
  // the existing list.
  var newItems = [];
  for (var i = 0; i < arguments.length - 2; i++)
    newItems.push(arguments[i + 2]);
  if (comp)
    newItems.sort(comp);

  // Creating a list of existing items.
  // This doesn't include items which should be deleted by this splice() call.
  var deletedItems = [];
  var currentItems = [];
  for (var i = 0; i < this.indexes_.length; i++) {
    var item = this.array_[this.indexes_[i]];
    if (insertPos <= i && i < insertPos + deleteCount) {
      deletedItems.push(item);
    } else {
      currentItems.push(item);
    }
  }

  // Initialize splice permutation with -1s.
  // Values of undeleted items will be filled in following merge step.
  var permutation = new Array(this.indexes_.length);
  for (var i = 0; i < permutation.length; i++)
    permutation[i] = -1;

  // Merge the list of existing item and the list of new items.
  this.indexes_ = [];
  this.array_ = [];
  var p = 0;
  var q = 0;
  while (p < currentItems.length || q < newItems.length) {
    var currentIndex = p + q;
    this.indexes_.push(currentIndex);
    // Determine which should be inserted to the resulting list earlier, the
    // smallest item of unused current items or the smallest item of unused new
    // items.
    var shouldPushCurrentItem;
    if (q === newItems.length) {
      shouldPushCurrentItem = true;
    } else if (p === currentItems.length) {
      shouldPushCurrentItem = false;
    } else {
      if (comp) {
        shouldPushCurrentItem = comp(currentItems[p], newItems[q]) <= 0;
      } else {
        // If the comparater is not defined, new items should be inserted to the
        // insertion position. That is, the current items before insertion
        // position should be pushed to the resulting list earlier.
        shouldPushCurrentItem = p < insertPos;
      }
    }
    if (shouldPushCurrentItem) {
      this.array_.push(currentItems[p]);
      if (p < insertPos) {
        permutation[p] = currentIndex;
      } else {
        permutation[p + deleteCount] = currentIndex;
      }
      p++;
    } else {
      this.array_.push(newItems[q]);
      q++;
    }
  }

  // Calculate the index property of splice event.
  // If no item is inserted, it is simply the insertion/deletion position.
  // If at least one item is inserted, it should be the resulting index of the
  // item which is inserted first.
  var spliceIndex = insertPos;
  if (arguments.length > 2) {
    for (var i = 0; i < this.indexes_.length; i++) {
      if (this.array_[this.indexes_[i]] === arguments[2]) {
        spliceIndex = i;
        break;
      }
    }
  }

  // Dispatch permute/splice event.
  this.dispatchPermutedEvent_(permutation);
  // TODO(arv): Maybe unify splice and change events?
  var spliceEvent = new Event('splice');
  spliceEvent.removed = deletedItems;
  spliceEvent.added = Array.prototype.slice.call(arguments, 2);
  spliceEvent.index = spliceIndex;
  this.dispatchEvent(spliceEvent);

  return deletedItems;
};

/**
 * @override
 */
FileListModel.prototype.replaceItem = function(oldItem, newItem) {
  this.onRemoveEntryFromList_(oldItem);
  this.onAddEntryToList_(newItem);

  cr.ui.ArrayDataModel.prototype.replaceItem.apply(this, arguments);
};

/**
 * Returns the number of files in this file list.
 * @return {number} The number of files.
 */
FileListModel.prototype.getFileCount = function() {
  return this.numFiles_;
};

/**
 * Returns the number of folders in this file list.
 * @return {number} The number of folders.
 */
FileListModel.prototype.getFolderCount = function() {
  return this.numFolders_;
};

/**
 * Returns true if image files are dominant in this file list (i.e. 80% or more
 * files are images).
 * @return {boolean}
 */
FileListModel.prototype.isImageDominant = function() {
  return this.numFiles_ > 0 &&
      this.numImageFiles_ * 10 >= this.numFiles_ * 8;
};

/**
 * Sets whether to use modificationByMeTime as "Last Modified" time.
 * @param {boolean} useModificationByMeTime
 */
FileListModel.prototype.setUseModificationByMeTime = function(
    useModificationByMeTime) {
  this.useModificationByMeTime_ = useModificationByMeTime;
};

/**
 * Updates the statistics about contents when new entry is about to be added.
 * @param {Entry} entry Entry of the new item.
 * @private
 */
FileListModel.prototype.onAddEntryToList_ = function(entry) {
  if (entry.isDirectory)
    this.numFolders_++;
  else
    this.numFiles_++;

  var mimeType = this.metadataModel_.getCache([entry],
      ['contentMimeType'])[0].contentMimeType;
  if (FileType.isImage(entry, mimeType) || FileType.isRaw(entry, mimeType))
    this.numImageFiles_++;
};

/**
 * Updates the statistics about contents when an entry is about to be removed.
 * @param {Entry} entry Entry of the item to be removed.
 * @private
 */
FileListModel.prototype.onRemoveEntryFromList_ = function(entry) {
  if (entry.isDirectory)
    this.numFolders_--;
  else
    this.numFiles_--;

  var mimeType = this.metadataModel_.getCache([entry],
      ['contentMimeType'])[0].contentMimeType;
  if (FileType.isImage(entry, mimeType) || FileType.isRaw(entry, mimeType))
    this.numImageFiles_--;
};

/**
 * Compares entries by name.
 * @param {!Entry} a First entry.
 * @param {!Entry} b Second entry.
 * @return {number} Compare result.
 * @private
 */
FileListModel.prototype.compareName_ = function(a, b) {
  // Directories always precede files.
  if (a.isDirectory !== b.isDirectory)
    return a.isDirectory === this.isDescendingOrder_ ? 1 : -1;

  return util.compareName(a, b);
};

/**
 * Compares entries by mtime first, then by name.
 * @param {Entry} a First entry.
 * @param {Entry} b Second entry.
 * @return {number} Compare result.
 * @private
 */
FileListModel.prototype.compareMtime_ = function(a, b) {
  // Directories always precede files.
  if (a.isDirectory !== b.isDirectory)
    return a.isDirectory === this.isDescendingOrder_ ? 1 : -1;

  var properties = this.metadataModel_.getCache(
      [a, b], ['modificationTime', 'modificationByMeTime']);
  var aTime = this.getMtime_(properties[0]);
  var bTime = this.getMtime_(properties[1]);

  if (aTime > bTime)
    return 1;

  if (aTime < bTime)
    return -1;

  return util.compareName(a, b);
};

/**
 * Returns the modification time from a properties object.
 * "Modification time" can be modificationTime or modificationByMeTime
 * depending on this.useModificationByMeTime_.
 * @param {!Object} properties Properties object.
 * @return {number} Modification time.
 * @private
 */
FileListModel.prototype.getMtime_ = function(properties) {
  if (this.useModificationByMeTime_)
    return properties.modificationByMeTime || properties.modificationTime || 0;
  return properties.modificationTime || 0;
};

/**
 * Compares entries by size first, then by name.
 * @param {Entry} a First entry.
 * @param {Entry} b Second entry.
 * @return {number} Compare result.
 * @private
 */
FileListModel.prototype.compareSize_ = function(a, b) {
  // Directories always precede files.
  if (a.isDirectory !== b.isDirectory)
    return a.isDirectory === this.isDescendingOrder_ ? 1 : -1;

  var properties = this.metadataModel_.getCache([a, b], ['size']);
  var aSize = properties[0].size || 0;
  var bSize = properties[1].size || 0;

  return aSize !== bSize ? aSize - bSize : util.compareName(a, b);
};

/**
 * Compares entries by type first, then by subtype and then by name.
 * @param {Entry} a First entry.
 * @param {Entry} b Second entry.
 * @return {number} Compare result.
 * @private
 */
FileListModel.prototype.compareType_ = function(a, b) {
  // Directories always precede files.
  if (a.isDirectory !== b.isDirectory)
    return a.isDirectory === this.isDescendingOrder_ ? 1 : -1;

  var properties = this.metadataModel_.getCache([a, b], ['contentMimeType']);
  var aType = FileListModel.getFileTypeString(
      FileType.getType(a, properties[0].contentMimeType));
  var bType = FileListModel.getFileTypeString(
      FileType.getType(b, properties[1].contentMimeType));

  var result = util.collator.compare(aType, bType);
  return result !== 0 ? result : util.compareName(a, b);
};
