// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * FileGrid constructor.
 *
 * Represents grid for the Grid View in the File Manager.
 * @constructor
 * @extends {cr.ui.Grid}
 */

function FileGrid() {
  throw new Error('Use FileGrid.decorate');
}

/**
 * Inherits from cr.ui.Grid.
 */
FileGrid.prototype = {
  __proto__: cr.ui.Grid.prototype,

  get dataModel() {
    if (!this.dataModelDescriptor_) {
      // We get the property descriptor for dataModel from cr.ui.List, because
      // cr.ui.Grid doesn't have its own descriptor.
      this.dataModelDescriptor_ =
          Object.getOwnPropertyDescriptor(cr.ui.List.prototype, 'dataModel');
    }
    return this.dataModelDescriptor_.get.call(this);
  },

  set dataModel(model) {
    // The setter for dataModel is overridden to remove/add the 'splice'
    // listener for the current data model.
    if (this.dataModel)
      this.dataModel.removeEventListener('splice', this.onSplice_.bind(this));
    this.dataModelDescriptor_.set.call(this, model);
    if (this.dataModel) {
      this.dataModel.addEventListener('splice', this.onSplice_.bind(this));
      this.classList.toggle('image-dominant', this.dataModel.isImageDominant());
    }
  }
};

/**
 * Decorates an HTML element to be a FileGrid.
 * @param {!Element} element The grid to decorate.
 * @param {!MetadataModel} metadataModel File system metadata.
 * @param {VolumeManagerWrapper} volumeManager Volume manager instance.
 * @param {!importer.HistoryLoader} historyLoader
 */
FileGrid.decorate = function(
    element, metadataModel, volumeManager, historyLoader) {
  cr.ui.Grid.decorate(element);
  var self = /** @type {!FileGrid} */ (element);
  self.__proto__ = FileGrid.prototype;
  self.metadataModel_ = metadataModel;
  self.volumeManager_ = volumeManager;
  self.historyLoader_ = historyLoader;

  /** @private {ListThumbnailLoader} */
  self.listThumbnailLoader_ = null;

  /** @private {number} */
  self.beginIndex_ = 0;

  /** @private {number} */
  self.endIndex_ = 0;

  /**
   * Reflects the visibility of import status in the UI.  Assumption: import
   * status is only enabled in import-eligible locations.  See
   * ImportController#onDirectoryChanged.  For this reason, the code in this
   * class checks if import status is visible, and if so, assumes that all the
   * files are in an import-eligible location.
   * TODO(kenobi): Clean this up once import status is queryable from metadata.
   *
   * @private {boolean}
   */
  self.importStatusVisible_ = true;

  /** @private {function(!Event)} */
  self.onThumbnailLoadedBound_ = self.onThumbnailLoaded_.bind(self);

  self.itemConstructor = function(entry) {
    var item = self.ownerDocument.createElement('li');
    item.__proto__ = FileGrid.Item.prototype;
    item = /** @type {!FileGrid.Item} */ (item);
    self.decorateThumbnail_(item, entry);
    return item;
  };

  self.relayoutRateLimiter_ =
      new AsyncUtil.RateLimiter(self.relayoutImmediately_.bind(self));

  var style = window.getComputedStyle(self);
  /**
   * @private {number}
   * @const
   */
  self.paddingStart_ = parseFloat(
      isRTL() ? style.paddingRight : style.paddingLeft);
  /**
   * @private {number}
   * @const
   */
  self.paddingTop_ = parseFloat(style.paddingTop);
};

/**
 * Grid size.
 * @const {number}
 */
FileGrid.GridSize = 180; // px

/**
 * Sets list thumbnail loader.
 * @param {ListThumbnailLoader} listThumbnailLoader A list thumbnail loader.
 */
FileGrid.prototype.setListThumbnailLoader = function(listThumbnailLoader) {
  if (this.listThumbnailLoader_) {
    this.listThumbnailLoader_.removeEventListener(
        'thumbnailLoaded', this.onThumbnailLoadedBound_);
  }

  this.listThumbnailLoader_ = listThumbnailLoader;

  if (this.listThumbnailLoader_) {
    this.listThumbnailLoader_.addEventListener(
        'thumbnailLoaded', this.onThumbnailLoadedBound_);
    this.listThumbnailLoader_.setHighPriorityRange(
        this.beginIndex_, this.endIndex_);
  }
};

/**
 * Returns the element containing the thumbnail of a certain list item as
 * background image.
 * @param {number} index The index of the item containing the desired thumbnail.
 * @return {?Element} The element containing the thumbnail, or null, if an error
 *     occurred.
 */
FileGrid.prototype.getThumbnail = function(index) {
  var listItem = this.getListItemByIndex(index);
  if (!listItem) {
    return null;
  }
  var container = listItem.querySelector('.img-container');
  if (!container) {
    return null;
  }
  return container.querySelector('.thumbnail');
};

/**
 * Handles thumbnail loaded event.
 * @param {!Event} event An event.
 * @private
 */
FileGrid.prototype.onThumbnailLoaded_ = function(event) {
  var listItem = this.getListItemByIndex(event.index);
  var entry = listItem && this.dataModel.item(listItem.listIndex);
  if (entry) {
    var box = listItem.querySelector('.img-container');
    if (box) {
      var mimeType = this.metadataModel_.getCache(
          [entry], ['contentMimeType'])[0].contentMimeType;
      if (!event.dataUrl) {
        FileGrid.clearThumbnailImage_(
            assertInstanceof(box, HTMLDivElement));
        FileGrid.setGenericThumbnail_(
            assertInstanceof(box, HTMLDivElement), entry);
      } else {
        FileGrid.setThumbnailImage_(
            assertInstanceof(box, HTMLDivElement),
            entry,
            assert(event.dataUrl),
            assert(event.width),
            assert(event.height),
            /* should animate */ true,
            mimeType);
      }
    }
    listItem.classList.toggle('thumbnail-loaded', !!event.dataUrl);
  }
};

/**
 * @override
 */
FileGrid.prototype.mergeItems = function(beginIndex, endIndex) {
  cr.ui.List.prototype.mergeItems.call(this, beginIndex, endIndex);

  var afterFiller = this.afterFiller_;
  var columns = this.columns;

  for (var item = this.beforeFiller_.nextSibling; item != afterFiller;) {
    var next = item.nextSibling;
    if (isSpacer(item)) {
      // Spacer found on a place it mustn't be.
      this.removeChild(item);
      item = next;
      continue;
    }
    var index = item.listIndex;
    var nextIndex = index + 1;

    // Invisible pinned item could be outside of the
    // [beginIndex, endIndex). Ignore it.
    if (index >= beginIndex && nextIndex < endIndex &&
        (nextIndex < this.dataModel.getFolderCount()
            ? nextIndex % columns == 0
            : (nextIndex - this.dataModel.getFolderCount()) % columns == 0)) {
      var isFolderSpacer = nextIndex === this.dataModel.getFolderCount();
      if (isSpacer(next)) {
        // Leave the spacer on its place.
        next.classList.toggle('folder-spacer', isFolderSpacer);
        item = next.nextSibling;
      } else {
        // Insert spacer.
        var spacer = this.ownerDocument.createElement('div');
        spacer.className = 'spacer';
        spacer.classList.toggle('folder-spacer', isFolderSpacer);
        this.insertBefore(spacer, next);
        item = next;
      }
    } else
      item = next;
  }

  function isSpacer(child) {
    return child.classList.contains('spacer') &&
           child != afterFiller;  // Must not be removed.
  }

  // Make sure that grid item's selected attribute is updated just after the
  // mergeItems operation is done. This prevents shadow of selected grid items
  // from being animated unintentionally by redraw.
  for (var i = beginIndex; i < endIndex; i++) {
    var item = this.getListItemByIndex(i);
    if (!item)
      continue;
    var isSelected = this.selectionModel.getIndexSelected(i);
    if (item.selected != isSelected)
      item.selected = isSelected;
  }

  // Keep these values to set range when a new list thumbnail loader is set.
  this.beginIndex_ = beginIndex;
  this.endIndex_ = endIndex;
  if (this.listThumbnailLoader_ !== null)
    this.listThumbnailLoader_.setHighPriorityRange(beginIndex, endIndex);
};

/**
 * @override
 */
FileGrid.prototype.getItemTop = function(index) {
  if (index < this.dataModel.getFolderCount())
    return Math.floor(index / this.columns) * this.getFolderItemHeight_();

  var folderRows = this.getFolderRowCount();
  var indexInFiles = index - this.dataModel.getFolderCount();
  return folderRows * this.getFolderItemHeight_() +
      (folderRows > 0 ? this.getSeparatorHeight_() : 0) +
      Math.floor(indexInFiles / this.columns) * this.getFileItemHeight_();
};

/**
 * @override
 */
FileGrid.prototype.getItemRow = function(index) {
  if (index < this.dataModel.getFolderCount())
    return Math.floor(index / this.columns);

  var folderRows = this.getFolderRowCount();
  var indexInFiles = index - this.dataModel.getFolderCount();
  return folderRows + Math.floor(indexInFiles / this.columns);
};

/**
 * Returns the column of an item which has given index.
 * @param {number} index The item index.
 */
FileGrid.prototype.getItemColumn = function(index) {
  if (index < this.dataModel.getFolderCount())
    return index % this.columns;

  var indexInFiles = index - this.dataModel.getFolderCount();
  return indexInFiles % this.columns;
};

/**
 * Return the item index which is placed at the given position.
 * If there is no item in the given position, returns -1.
 * @param {number} row The row index.
 * @param {number} column The column index.
 */
FileGrid.prototype.getItemIndex = function(row, column) {
  if (row < 0 || column < 0 || column >= this.columns)
    return -1;
  var folderCount = this.dataModel.getFolderCount();
  var folderRows = this.getFolderRowCount();
  if (row < folderRows) {
    var index = row * this.columns + column;
    return index < folderCount ? index : -1;
  }
  var index = folderCount + (row - folderRows) * this.columns + column;
  return index < this.dataModel.length ? index : -1;
};

/**
 * @override
 */
FileGrid.prototype.getFirstItemInRow = function(row) {
  var folderRows = this.getFolderRowCount();
  if (row < folderRows)
    return row * this.columns;

  return this.dataModel.getFolderCount() + (row - folderRows) * this.columns;
};

/**
 * @override
 */
FileGrid.prototype.scrollIndexIntoView = function(index) {
  var dataModel = this.dataModel;
  if (!dataModel || index < 0 || index >= dataModel.length)
    return;

  var itemHeight = index < this.dataModel.getFolderCount() ?
      this.getFolderItemHeight_() : this.getFileItemHeight_();
  var scrollTop = this.scrollTop;
  var top = this.getItemTop(index);
  var clientHeight = this.clientHeight;

  var computedStyle = window.getComputedStyle(this);
  var paddingY = parseInt(computedStyle.paddingTop, 10) +
                 parseInt(computedStyle.paddingBottom, 10);
  var availableHeight = clientHeight - paddingY;

  var self = this;
  // Function to adjust the tops of viewport and row.
  var scrollToAdjustTop = function() {
      self.scrollTop = top;
  };
  // Function to adjust the bottoms of viewport and row.
  var scrollToAdjustBottom = function() {
      self.scrollTop = top + itemHeight - availableHeight;
  };

  // Check if the entire of given indexed row can be shown in the viewport.
  if (itemHeight <= availableHeight) {
    if (top < scrollTop)
      scrollToAdjustTop();
    else if (scrollTop + availableHeight < top + itemHeight)
      scrollToAdjustBottom();
  } else {
    if (scrollTop < top)
      scrollToAdjustTop();
    else if (top + itemHeight < scrollTop + availableHeight)
      scrollToAdjustBottom();
  }
};

/**
 * @override
 */
FileGrid.prototype.getItemsInViewPort = function(scrollTop, clientHeight) {
  var beginRow = this.getRowForListOffset_(scrollTop);
  var endRow = this.getRowForListOffset_(scrollTop + clientHeight - 1) + 1;
  var beginIndex = this.getFirstItemInRow(beginRow);
  var endIndex = Math.min(this.getFirstItemInRow(endRow),
                          this.dataModel.length);
  var result = {
    first: beginIndex,
    length: endIndex - beginIndex,
    last: endIndex - 1
  };
  return result;
};

/**
 * @override
 */
FileGrid.prototype.getAfterFillerHeight = function(lastIndex) {
  var folderRows = this.getFolderRowCount();
  var fileRows = this.getFileRowCount();
  var row = this.getItemRow(lastIndex - 1);
  if (row < folderRows) {
    var fillerHeight = (folderRows - 1 - row) * this.getFolderItemHeight_() +
                       fileRows * this.getFileItemHeight_();
    if (fileRows > 0)
      fillerHeight += this.getSeparatorHeight_();
    return fillerHeight;
  }
  var rowInFiles = row - folderRows;
  return (fileRows - 1 - rowInFiles) * this.getFileItemHeight_();
};

/**
 * Returns the number of rows in folders section.
 * @return {number}
 */
FileGrid.prototype.getFolderRowCount = function() {
  return Math.ceil(this.dataModel.getFolderCount() / this.columns);
};

/**
 * Returns the number of rows in files section.
 * @return {number}
 */
FileGrid.prototype.getFileRowCount = function() {
  return Math.ceil(this.dataModel.getFileCount() / this.columns);
};

/**
 * Returns the height of folder items in grid view.
 * @return {number} The height of folder items.
 */
FileGrid.prototype.getFolderItemHeight_ = function() {
  return 44;  // TODO(fukino): Read from DOM and cache it.
};

/**
 * Returns the height of file items in grid view.
 * @return {number} The height of file items.
 */
FileGrid.prototype.getFileItemHeight_ = function() {
  return 184;  // TODO(fukino): Read from DOM and cache it.
};

/**
 * Returns the width of grid items.
 * @return {number}
 */
FileGrid.prototype.getItemWidth_ = function() {
  return 184;  // TODO(fukino): Read from DOM and cache it.
};

/**
 * Returns the margin top of grid items.
 * @return {number};
 */
FileGrid.prototype.getItemMarginTop_ = function() {
  return 4;  // TODO(fukino): Read from DOM and cache it.
};

/**
 * Returns the margin left of grid items.
 * @return {number}
 */
FileGrid.prototype.getItemMarginLeft_ = function() {
  return 4;  // TODO(fukino): Read from DOM and cache it.
};

/**
 * Returns the height of the separator which separates folders and files.
 * @return {number} The height of the separator.
 */
FileGrid.prototype.getSeparatorHeight_ = function() {
  return 5;  // TODO(fukino): Read from DOM and cache it.
};

/**
 * Returns index of a row which contains the given y-position(offset).
 * @param {number} offset The offset from the top of grid.
 * @return {number} Row index corresponding to the given offset.
 * @private
 */
FileGrid.prototype.getRowForListOffset_ = function(offset) {
  var innerOffset = Math.max(0, offset - this.paddingTop_);
  var folderRows = this.getFolderRowCount();
  if (innerOffset < folderRows * this.getFolderItemHeight_())
    return Math.floor(innerOffset / this.getFolderItemHeight_());

  var offsetInFiles = innerOffset - folderRows * this.getFolderItemHeight_();
  if (folderRows > 0)
    offsetInFiles = Math.max(0, offsetInFiles - this.getSeparatorHeight_());
  return folderRows + Math.floor(offsetInFiles / this.getFileItemHeight_());
};

/**
 * @override
 */
FileGrid.prototype.createSelectionController = function(sm) {
  return new FileGridSelectionController(assert(sm), this);
};

/**
 * Updates items to reflect metadata changes.
 * @param {string} type Type of metadata changed.
 * @param {Array<Entry>} entries Entries whose metadata changed.
 */
FileGrid.prototype.updateListItemsMetadata = function(type, entries) {
  var urls = util.entriesToURLs(entries);
  var boxes = /** @type {!NodeList<!HTMLElement>} */(
      this.querySelectorAll('.img-container'));
  for (var i = 0; i < boxes.length; i++) {
    var box = boxes[i];
    var listItem = this.getListItemAncestor(box);
    var entry = listItem && this.dataModel.item(listItem.listIndex);
    if (!entry || urls.indexOf(entry.toURL()) === -1)
      continue;

    this.decorateThumbnailBox_(assert(listItem), entry);
    this.updateSharedStatus_(assert(listItem), entry);
  }
};

/**
 * Redraws the UI. Skips multiple consecutive calls.
 */
FileGrid.prototype.relayout = function() {
  this.relayoutRateLimiter_.run();
};

/**
 * Redraws the UI immediately.
 * @private
 */
FileGrid.prototype.relayoutImmediately_ = function() {
  this.startBatchUpdates();
  this.columns = 0;
  this.redraw();
  this.endBatchUpdates();
  cr.dispatchSimpleEvent(this, 'relayout');
};

/**
 * Decorates thumbnail.
 * @param {cr.ui.ListItem} li List item.
 * @param {!Entry} entry Entry to render a thumbnail for.
 * @private
 */
FileGrid.prototype.decorateThumbnail_ = function(li, entry) {
  li.className = 'thumbnail-item';
  if (entry)
    filelist.decorateListItem(li, entry, this.metadataModel_);

  var frame = li.ownerDocument.createElement('div');
  frame.className = 'thumbnail-frame';
  li.appendChild(frame);

  var box = li.ownerDocument.createElement('div');
  box.className = 'img-container';
  frame.appendChild(box);
  if (entry)
    this.decorateThumbnailBox_(assertInstanceof(li, HTMLLIElement), entry);

  var shield = li.ownerDocument.createElement('div');
  shield.className = 'shield';
  frame.appendChild(shield);

  var isDirectory = entry && entry.isDirectory;
  if (!isDirectory) {
    var activeCheckmark = li.ownerDocument.createElement('div');
    activeCheckmark.className = 'checkmark active';
    frame.appendChild(activeCheckmark);
    var inactiveCheckmark = li.ownerDocument.createElement('div');
    inactiveCheckmark.className = 'checkmark inactive';
    frame.appendChild(inactiveCheckmark);
  }

  var badge = li.ownerDocument.createElement('div');
  badge.className = 'badge';
  frame.appendChild(badge);

  var bottom = li.ownerDocument.createElement('div');
  bottom.className = 'thumbnail-bottom';
  var mimeType = this.metadataModel_.getCache(
      [entry], ['contentMimeType'])[0].contentMimeType;
  var detailIcon = filelist.renderFileTypeIcon(
      li.ownerDocument, entry, mimeType);
  if (isDirectory) {
    var checkmark = li.ownerDocument.createElement('div');
    checkmark.className = 'detail-checkmark';
    detailIcon.appendChild(checkmark);
  }
  bottom.appendChild(detailIcon);
  bottom.appendChild(filelist.renderFileNameLabel(li.ownerDocument, entry));
  frame.appendChild(bottom);

  this.updateSharedStatus_(li, entry);
};

/**
 * Decorates the box containing a centered thumbnail image.
 *
 * @param {!HTMLLIElement} li List item which contains the box to be decorated.
 * @param {Entry} entry Entry which thumbnail is generating for.
 * @private
 */
FileGrid.prototype.decorateThumbnailBox_ = function(li, entry) {
  var box = assertInstanceof(li.querySelector('.img-container'),
                             HTMLDivElement);
  if (this.importStatusVisible_ &&
      importer.isEligibleType(entry)) {
    this.historyLoader_.getHistory().then(
        FileGrid.applyHistoryBadges_.bind(
            null,
            /** @type {!FileEntry} */ (entry),
            box));
  }

  if (entry.isDirectory) {
    FileGrid.setGenericThumbnail_(box, entry);
    return;
  }

  // Set thumbnail if it's already in cache, and the thumbnail data is not
  // empty.
  var thumbnailData = this.listThumbnailLoader_ ?
      this.listThumbnailLoader_.getThumbnailFromCache(entry) : null;
  if (thumbnailData && thumbnailData.dataUrl) {
    var mimeType = this.metadataModel_.getCache(
        [entry], ['contentMimeType'])[0].contentMimeType;
    FileGrid.setThumbnailImage_(
        box,
        entry,
        thumbnailData.dataUrl,
        thumbnailData.width,
        thumbnailData.height,
        /* should not animate */ false,
        mimeType);
    li.classList.toggle('thumbnail-loaded', true);
  } else {
    FileGrid.setGenericThumbnail_(box, entry);
    li.classList.toggle('thumbnail-loaded', false);
  }
  var mimeType = this.metadataModel_.getCache(
      [entry], ['contentMimeType'])[0].contentMimeType;
  li.classList.toggle('can-hide-filename',
                      FileType.isImage(entry, mimeType) ||
                      FileType.isRaw(entry, mimeType));
};

/**
 * Added 'shared' class to icon and placeholder of a folder item.
 * @param {!HTMLLIElement} li The grid item.
 * @param {!Entry} entry File entry for the grid item.
 * @private
 */
FileGrid.prototype.updateSharedStatus_ = function(li, entry) {
  if (!entry.isDirectory)
    return;

  var shared = !!this.metadataModel_.getCache([entry], ['shared'])[0].shared;
  var box = li.querySelector('.img-container');
  if (box)
    box.classList.toggle('shared', shared);
  var icon = li.querySelector('.detail-icon');
  if (icon)
    icon.classList.toggle('shared', shared);
};

/**
 * Sets the visibility of the cloud import status column.
 * @param {boolean} visible
 */
FileGrid.prototype.setImportStatusVisible = function(visible) {
  this.importStatusVisible_ = visible;
};

/**
 * Handles the splice event of the data model to change the view based on
 * whether image files is dominant or not in the directory.
 * @private
 */
FileGrid.prototype.onSplice_ = function() {
  this.classList.toggle('image-dominant', this.dataModel.isImageDominant());
};

/**
 * Sets thumbnail image to the box.
 * @param {!HTMLDivElement} box A div element to hold thumbnails.
 * @param {!Entry} entry An entry of the thumbnail.
 * @param {string} dataUrl Data url of thumbnail.
 * @param {number} width Width of thumbnail.
 * @param {number} height Height of thumbnail.
 * @param {boolean} shouldAnimate Whether the thumbnail is shown with animation
 *     or not.
 * @param {string=} opt_mimeType Optional mime type for the image.
 * @private
 */
FileGrid.setThumbnailImage_ = function(
    box, entry, dataUrl, width, height, shouldAnimate, opt_mimeType) {
  var oldThumbnails = box.querySelectorAll('.thumbnail');

  var thumbnail = box.ownerDocument.createElement('div');
  thumbnail.classList.add('thumbnail');

  // If the image is JPEG or the thumbnail is larger than the grid size, resize
  // it to cover the thumbnail box.
  var type = FileType.getType(entry, opt_mimeType);
  if ((type.type === 'image' && type.subtype === 'JPEG') ||
      width > FileGrid.GridSize || height > FileGrid.GridSize)
    thumbnail.style.backgroundSize = 'cover';

  thumbnail.style.backgroundImage = 'url(' + dataUrl + ')';
  thumbnail.addEventListener('animationend', function() {
    // Remove animation css once animation is completed in order not to animate
    // again when an item is attached to the dom again.
    thumbnail.classList.remove('animate');

    for (var i = 0; i < oldThumbnails.length; i++) {
      if (box.contains(oldThumbnails[i]))
        box.removeChild(oldThumbnails[i]);
    }
  });
  if (shouldAnimate)
    thumbnail.classList.add('animate');
  box.appendChild(thumbnail);
};

/**
 * Clears thumbnail image from the box.
 * @param {!HTMLDivElement} box A div element to hold thumbnails.
 * @private
 */
FileGrid.clearThumbnailImage_ = function(box) {
  var oldThumbnails = box.querySelectorAll('.thumbnail');
  for (var i = 0; i < oldThumbnails.length; i++) {
    box.removeChild(oldThumbnails[i]);
  }
  return;
};

/**
 * Sets a generic thumbnail on the box.
 * @param {!HTMLDivElement} box A div element to hold thumbnails.
 * @param {!Entry} entry An entry of the thumbnail.
 * @private
 */
FileGrid.setGenericThumbnail_ = function(box, entry) {
  if (entry.isDirectory) {
    box.setAttribute('generic-thumbnail', 'folder');
  } else {
    var mediaType = FileType.getMediaType(entry);
    box.setAttribute('generic-thumbnail', mediaType);
  }
};

/**
 * Applies cloud import history badges as appropriate for the Entry.
 *
 * @param {!FileEntry} entry
 * @param {Element} box Box to decorate.
 * @param {!importer.ImportHistory} history
 *
 * @private
 */
FileGrid.applyHistoryBadges_ = function(entry, box, history) {
  history.wasImported(entry, importer.Destination.GOOGLE_DRIVE)
      .then(
          function(imported) {
            if (imported) {
              // TODO(smckay): update badges when history changes
              // "box" is currently the sibling of the elemement
              // we want to style. So rather than employing
              // a possibly-fragile sibling selector we just
              // plop the imported class on the parent of both.
              box.parentElement.classList.add('imported');
            } else {
              history.wasCopied(entry, importer.Destination.GOOGLE_DRIVE)
                  .then(
                      function(copied) {
                        if (copied) {
                          // TODO(smckay): update badges when history changes
                          // "box" is currently the sibling of the elemement
                          // we want to style. So rather than employing
                          // a possibly-fragile sibling selector we just
                          // plop the imported class on the parent of both.
                          box.parentElement.classList.add('copied');
                        }
                      });
            }
          });
};

/**
 * Item for the Grid View.
 * @constructor
 * @extends {cr.ui.ListItem}
 */
FileGrid.Item = function() {
  throw new Error();
};

/**
 * Inherits from cr.ui.ListItem.
 */
FileGrid.Item.prototype.__proto__ = cr.ui.ListItem.prototype;

Object.defineProperty(FileGrid.Item.prototype, 'label', {
  /**
   * @this {FileGrid.Item}
   * @return {string} Label of the item.
   */
  get: function() {
    return this.querySelector('filename-label').textContent;
  }
});

/**
 * @override
 */
FileGrid.Item.prototype.decorate = function() {
  cr.ui.ListItem.prototype.decorate.apply(this);
  // Override the default role 'listitem' to 'option' to match the parent's
  // role (listbox).
  this.setAttribute('role', 'option');
  var nameId = this.id + '-entry-name';
  this.querySelector('.entry-name').setAttribute('id', nameId);
  this.querySelector('.img-container').setAttribute('aria-labelledby', nameId);
  this.setAttribute('aria-labelledby', nameId);
};

/**
 * Returns whether the drag event is inside a file entry in the list (and not
 * the background padding area).
 * @param {MouseEvent} event Drag start event.
 * @return {boolean} True if the mouse is over an element in the list, False if
 *                   it is in the background.
 */
FileGrid.prototype.hasDragHitElement = function(event) {
  var pos = DragSelector.getScrolledPosition(this, event);
  return this.getHitElements(pos.x, pos.y).length !== 0;
};

/**
 * Obtains if the drag selection should be start or not by referring the mouse
 * event.
 * @param {MouseEvent} event Drag start event.
 * @return {boolean} True if the mouse is hit to the background of the list.
 */
FileGrid.prototype.shouldStartDragSelection = function(event) {
  // Start dragging area if the drag starts outside of the contents of the grid.
  return !this.hasDragHitElement(event);
};

/**
 * Returns the index of row corresponding to the given y position.
 *
 * If the reverse is false, this returns index of the first row in which bottom
 * of grid items is greater than or equal to y. Otherwise, this returns index of
 * the last row in which top of grid items is less than or equal to y.
 * @param {number} y
 * @param {boolean} reverse
 * @return {number}
 * @private
 */
FileGrid.prototype.getHitRowIndex_ = function(y, reverse) {
  var folderRows = this.getFolderRowCount();
  var folderHeight = this.getFolderItemHeight_();
  var fileHeight = this.getFileItemHeight_();

  if (y < folderHeight * folderRows) {
    var shift = reverse ? -this.getItemMarginTop_() : 0;
    return Math.floor((y + shift) / folderHeight);
  }
  var yInFiles = y - folderHeight * folderRows;
  if (folderRows > 0)
    yInFiles = Math.max(0, yInFiles - this.getSeparatorHeight_());
  var shift = reverse ? -this.getItemMarginTop_() : 0;
  return folderRows + Math.floor((yInFiles + shift) / fileHeight);
};

/**
 * Returns the index of column corresponding to the given x position.
 *
 * If the reverse is false, this returns index of the first column in which
 * left of grid items is greater than or equal to x. Otherwise, this returns
 * index of the last column in which right of grid items is less than or equal
 * to x.
 * @param {number} x
 * @param {boolean} reverse
 * @return {number}
 * @private
 */
FileGrid.prototype.getHitColumnIndex_ = function(x, reverse) {
  var itemWidth = this.getItemWidth_();
  var shift = reverse ? -this.getItemMarginLeft_() : 0;
  return Math.floor((x + shift) / itemWidth);
};

/**
 * Obtains the index list of elements that are hit by the point or the
 * rectangle.
 *
 * We should match its argument interface with FileList.getHitElements.
 *
 * @param {number} x X coordinate value.
 * @param {number} y Y coordinate value.
 * @param {number=} opt_width Width of the coordinate.
 * @param {number=} opt_height Height of the coordinate.
 * @return {Array<number>} Index list of hit elements.
 */
FileGrid.prototype.getHitElements = function(x, y, opt_width, opt_height) {
  var currentSelection = [];
  var startXWithPadding = isRTL() ? this.clientWidth - (x + opt_width) : x;
  var startX = Math.max(0, startXWithPadding - this.paddingStart_);
  var endX = startX + (opt_width ? opt_width - 1 : 0);
  var top = Math.max(0, y - this.paddingTop_);
  var bottom = top + (opt_height ? opt_height - 1 : 0);

  var firstRow = this.getHitRowIndex_(top, false);
  var lastRow = this.getHitRowIndex_(bottom, true);
  var firstColumn = this.getHitColumnIndex_(startX, false);
  var lastColumn = this.getHitColumnIndex_(endX, true);

  for (var row = firstRow; row <= lastRow; row++) {
    for (var col = firstColumn; col <= lastColumn; col++) {
      var index = this.getItemIndex(row, col);
      if (0 <= index && index < this.dataModel.length)
        currentSelection.push(index);
    }
  }
  return currentSelection;
};

/**
 * Selection controller for the file grid.
 * @param {!cr.ui.ListSelectionModel} selectionModel The selection model to
 *     interact with.
 * @param {!cr.ui.Grid} grid The grid to interact with.
 * @constructor
 * @extends {cr.ui.GridSelectionController}
 * @struct
 */
function FileGridSelectionController(selectionModel, grid) {
  cr.ui.GridSelectionController.call(this, selectionModel, grid);

  /**
   * Whether to allow touch-specific interaction.
   * @private {boolean}
   */
  this.enableTouchMode_ = false;
  util.isTouchModeEnabled().then(function(enabled) {
    this.enableTouchMode_ = enabled;
  }.bind(this));

  /**
   * @type {!FileTapHandler}
   * @const
   */
  this.tapHandler_ = new FileTapHandler();
}

FileGridSelectionController.prototype = /** @struct */ {
  __proto__: cr.ui.GridSelectionController.prototype
};

/** @override */
FileGridSelectionController.prototype.handlePointerDownUp = function(e, index) {
  filelist.handlePointerDownUp.call(this, e, index);
};

/** @override */
FileGridSelectionController.prototype.handleTouchEvents = function(e, index) {
  if (!this.enableTouchMode_)
    return;
  if (this.tapHandler_.handleTouchEvents(
          e, index, filelist.handleTap.bind(this)))
    filelist.focusParentList(e);
};

/** @override */
FileGridSelectionController.prototype.handleKeyDown = function(e) {
  filelist.handleKeyDown.call(this, e);
};

/** @override */
FileGridSelectionController.prototype.getIndexBelow = function(index) {
  if (this.isAccessibilityEnabled())
    return this.getIndexAfter(index);
  if (index === this.getLastIndex())
    return -1;

  var grid = /** @type {!FileGrid} */ (this.grid_);
  var row = grid.getItemRow(index);
  var col = grid.getItemColumn(index);
  var nextIndex = grid.getItemIndex(row + 1, col);
  if (nextIndex === -1) {
    return row + 1 < grid.getFolderRowCount() ?
        grid.dataModel.getFolderCount() - 1 :
        grid.dataModel.length - 1;
  }
  return nextIndex;
};

/** @override */
FileGridSelectionController.prototype.getIndexAbove = function(index) {
  if (this.isAccessibilityEnabled())
    return this.getIndexBefore(index);
  if (index == 0)
    return -1;

  var grid = /** @type {!FileGrid} */ (this.grid_);
  var row = grid.getItemRow(index);
  if (row - 1 < 0)
    return 0;
  var col = grid.getItemColumn(index);
  var nextIndex = grid.getItemIndex(row - 1, col);
  if (nextIndex === -1) {
    return row - 1 < grid.getFolderRowCount() ?
        grid.dataModel.getFolderCount() - 1 :
        grid.dataModel.length - 1;
  }
  return nextIndex;
};
