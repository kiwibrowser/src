// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Scrollable thumbnail ribbon at the bottom of the Gallery in the Slide mode.
 *
 * @param {!Document} document Document.
 * @param {!Window} targetWindow A window which this ribbon is attached to.
 * @param {!cr.ui.ArrayDataModel} dataModel Data model.
 * @param {!cr.ui.ListSelectionModel} selectionModel Selection model.
 * @param {!ThumbnailModel} thumbnailModel
 * @extends {HTMLDivElement}
 * @constructor
 * @struct
 */
function Ribbon(
    document, targetWindow, dataModel, selectionModel, thumbnailModel) {
  if (this instanceof Ribbon) {
    return Ribbon.call(/** @type {Ribbon} */ (document.createElement('div')),
        document, targetWindow, dataModel, selectionModel, thumbnailModel);
  }

  this.__proto__ = Ribbon.prototype;
  this.className = 'ribbon';
  this.setAttribute('role', 'listbox');
  this.tabIndex = 0;

  /**
   * @private {!Window}
   * @const
   */
  this.targetWindow_ = targetWindow;

  /**
   * @private {!cr.ui.ArrayDataModel}
   * @const
   */
  this.dataModel_ = dataModel;

  /**
   * @private {!cr.ui.ListSelectionModel}
   * @const
   */
  this.selectionModel_ = selectionModel;

  /**
   * @private {!ThumbnailModel}
   * @const
   */
  this.thumbnailModel_ = thumbnailModel;

  /**
   * @type {!Object}
   * @private
   */
  this.renderCache_ = {};

  /**
   * @type {number}
   * @private
   */
  this.firstVisibleIndex_ = 0;

  /**
   * @type {number}
   * @private
   */
  this.lastVisibleIndex_ = -1;

  /**
   * @type {?function(!Event)}
   * @private
   */
  this.onContentBound_ = null;

  /**
   * @type {?function(!Event)}
   * @private
   */
  this.onSpliceBound_ = null;

  /**
   * @type {?function(!Event)}
   * @private
   */
  this.onSelectionBound_ = null;

  /**
   * @type {?number}
   * @private
   */
  this.removeTimeout_ = null;

  /**
   * @private {number}
   */
  this.thumbnailElementId_ = 0;

  this.targetWindow_.addEventListener(
      'resize', this.onWindowResize_.bind(this));

  return this;
}

/**
 * Inherit from HTMLDivElement.
 */
Ribbon.prototype.__proto__ = HTMLDivElement.prototype;

/**
 * Margin of thumbnails.
 * @const {number}
 */
Ribbon.MARGIN = 2; // px

/**
 * Width of thumbnail on the ribbon.
 * @const {number}
 */
Ribbon.THUMBNAIL_WIDTH = 71; // px

/**
 * Height of thumbnail on the ribbon.
 * @const {number}
 */
Ribbon.THUMBNAIL_HEIGHT = 40; // px

/**
 * Returns number of items in the viewport.
 * @return {number} Number of items in the viewport.
 */
Ribbon.prototype.getItemCount_ = function() {
  return Math.ceil(this.targetWindow_.innerWidth /
      (Ribbon.THUMBNAIL_WIDTH + Ribbon.MARGIN * 2));
};

/**
 * Handles resize event of target window.
 */
Ribbon.prototype.onWindowResize_ = function() {
  this.redraw();
};

/**
 * Force redraw the ribbon.
 */
Ribbon.prototype.redraw = function() {
  this.onSelection_();
};

/**
 * Clear all cached data to force full redraw on the next selection change.
 */
Ribbon.prototype.reset = function() {
  this.renderCache_ = {};
  this.firstVisibleIndex_ = 0;
  this.lastVisibleIndex_ = -1;  // Zero thumbnails
};

/**
 * Enable the ribbon.
 */
Ribbon.prototype.enable = function() {
  this.onContentBound_ = this.onContentChange_.bind(this);
  this.dataModel_.addEventListener('content', this.onContentBound_);

  this.onSpliceBound_ = this.onSplice_.bind(this);
  this.dataModel_.addEventListener('splice', this.onSpliceBound_);

  this.onSelectionBound_ = this.onSelection_.bind(this);
  this.selectionModel_.addEventListener('change', this.onSelectionBound_);

  this.reset();
  this.redraw();
};

/**
 * Disable ribbon.
 */
Ribbon.prototype.disable = function() {
  this.dataModel_.removeEventListener('content', this.onContentBound_);
  this.dataModel_.removeEventListener('splice', this.onSpliceBound_);
  this.selectionModel_.removeEventListener('change', this.onSelectionBound_);

  this.removeVanishing_();
  this.textContent = '';
};

/**
 * Data model splice handler.
 * @param {!Event} event Event.
 * @private
 */
Ribbon.prototype.onSplice_ = function(event) {
  if (event.removed.length === 0 && event.added.length === 0)
    return;

  if (event.removed.length > 0 && event.added.length > 0) {
    console.error('Replacing is not implemented.');
    return;
  }

  if (event.added.length > 0) {
    for (var i = 0; i < event.added.length; i++) {
      var index = this.dataModel_.indexOf(event.added[i]);
      if (index === -1)
        continue;
      var element = this.renderThumbnail_(index);
      var nextItem = this.dataModel_.item(index + 1);
      var nextElement =
          nextItem && this.renderCache_[nextItem.getEntry().toURL()];
      this.insertBefore(element, nextElement);
    }
    return;
  }

  var persistentNodes = this.querySelectorAll('.ribbon-image:not([vanishing])');
  if (this.lastVisibleIndex_ < this.dataModel_.length) { // Not at the end.
    var lastNode = persistentNodes[persistentNodes.length - 1];
    if (lastNode.nextSibling) {
      // Pull back a vanishing node from the right.
      lastNode.nextSibling.removeAttribute('vanishing');
    } else {
      // Push a new item at the right end.
      this.appendChild(this.renderThumbnail_(this.lastVisibleIndex_));
    }
  } else {
    // No items to the right, move the window to the left.
    this.lastVisibleIndex_--;
    if (this.firstVisibleIndex_) {
      this.firstVisibleIndex_--;
      var firstNode = persistentNodes[0];
      if (firstNode.previousSibling) {
        // Pull back a vanishing node from the left.
        firstNode.previousSibling.removeAttribute('vanishing');
      } else {
        // Push a new item at the left end.
        if (this.firstVisibleIndex_ < this.dataModel_.length) {
          var newThumbnail = this.renderThumbnail_(this.firstVisibleIndex_);
          newThumbnail.style.marginLeft = -(this.clientHeight - 2) + 'px';
          this.insertBefore(newThumbnail, this.firstChild);
          setTimeout(function() {
            newThumbnail.style.marginLeft = '0';
          }, 0);
        }
      }
    }
  }

  var removed = false;
  for (var i = 0; i < event.removed.length; i++) {
    var removedDom = this.renderCache_[event.removed[i].getEntry().toURL()];
    if (removedDom) {
      removedDom.removeAttribute('selected');
      removedDom.setAttribute('vanishing', 'smooth');
      removed = true;
    }
  }

  if (removed)
    this.scheduleRemove_();

  this.onSelection_();
};

/**
 * Selection change handler.
 * @private
 */
Ribbon.prototype.onSelection_ = function() {
  var indexes = this.selectionModel_.selectedIndexes;
  if (indexes.length === 0)
    return;  // Ignore temporary empty selection.
  var selectedIndex = indexes[0];

  var length = this.dataModel_.length;
  var fullItems = Math.min(this.getItemCount_(), length);
  var right = Math.floor((fullItems - 1) / 2);

  var lastIndex = selectedIndex + right;
  lastIndex = Math.max(lastIndex, fullItems - 1);
  lastIndex = Math.min(lastIndex, length - 1);

  var firstIndex = lastIndex - fullItems + 1;

  if (this.firstVisibleIndex_ !== firstIndex ||
      this.lastVisibleIndex_ !== lastIndex) {

    if (this.lastVisibleIndex_ === -1) {
      this.firstVisibleIndex_ = firstIndex;
      this.lastVisibleIndex_ = lastIndex;
    }

    this.removeVanishing_();

    this.textContent = '';
    var startIndex = Math.min(firstIndex, this.firstVisibleIndex_);
    // All the items except the first one treated equally.
    for (var index = startIndex + 1;
         index <= Math.max(lastIndex, this.lastVisibleIndex_);
         ++index) {
      // Only add items that are in either old or the new viewport.
      if (this.lastVisibleIndex_ < index && index < firstIndex ||
          lastIndex < index && index < this.firstVisibleIndex_)
        continue;

      var box = this.renderThumbnail_(index);
      box.style.marginLeft = Ribbon.MARGIN + 'px';
      this.appendChild(box);

      if (index < firstIndex || index > lastIndex) {
        // If the node is not in the new viewport we only need it while
        // the animation is playing out.
        box.setAttribute('vanishing', 'slide');
      }
    }

    var slideCount = this.childNodes.length + 1 - fullItems;
    var margin = Ribbon.THUMBNAIL_WIDTH * slideCount;
    var startBox = this.renderThumbnail_(startIndex);

    if (startIndex === firstIndex) {
      // Sliding to the right.
      startBox.style.marginLeft = -margin + 'px';

      if (this.firstChild)
        this.insertBefore(startBox, this.firstChild);
      else
        this.appendChild(startBox);

      setTimeout(function() {
        startBox.style.marginLeft = Ribbon.MARGIN + 'px';
      }, 0);
    } else {
      // Sliding to the left. Start item will become invisible and should be
      // removed afterwards.
      startBox.setAttribute('vanishing', 'slide');
      startBox.style.marginLeft = Ribbon.MARGIN + 'px';

      if (this.firstChild)
        this.insertBefore(startBox, this.firstChild);
      else
        this.appendChild(startBox);

      setTimeout(function() {
        startBox.style.marginLeft = -margin + 'px';
      }, 0);
    }

    this.firstVisibleIndex_ = firstIndex;
    this.lastVisibleIndex_ = lastIndex;

    this.scheduleRemove_();
  }

  ImageUtil.setClass(
      this,
      'fade-left',
      firstIndex > 0 && selectedIndex !== firstIndex);
  ImageUtil.setClass(
      this,
      'fade-right',
      lastIndex < length - 1 && selectedIndex !== lastIndex);

  var oldSelected = this.querySelector('[selected]');
  if (oldSelected)
    oldSelected.removeAttribute('selected');

  var newSelected =
      this.renderCache_[this.dataModel_.item(selectedIndex).getEntry().toURL()];
  if (newSelected) {
    newSelected.setAttribute('selected', true);
    this.setAttribute('aria-activedescendant', newSelected.id);
    this.focus();
  }
};

/**
 * Schedule the removal of thumbnails marked as vanishing.
 * @private
 */
Ribbon.prototype.scheduleRemove_ = function() {
  if (this.removeTimeout_)
    clearTimeout(this.removeTimeout_);

  this.removeTimeout_ = setTimeout(function() {
    this.removeTimeout_ = null;
    this.removeVanishing_();
  }.bind(this), 200);
};

/**
 * Remove all thumbnails marked as vanishing.
 * @private
 */
Ribbon.prototype.removeVanishing_ = function() {
  if (this.removeTimeout_) {
    clearTimeout(this.removeTimeout_);
    this.removeTimeout_ = 0;
  }
  var vanishingNodes = this.querySelectorAll('[vanishing]');
  for (var i = 0; i != vanishingNodes.length; i++) {
    vanishingNodes[i].removeAttribute('vanishing');
    this.removeChild(vanishingNodes[i]);
  }
};

/**
 * Create a DOM element for a thumbnail.
 *
 * @param {number} index Item index.
 * @return {!Element} Newly created element.
 * @private
 */
Ribbon.prototype.renderThumbnail_ = function(index) {
  var item = assertInstanceof(this.dataModel_.item(index), GalleryItem);
  var url = item.getEntry().toURL();

  var cached = this.renderCache_[url];
  if (cached) {
    var img = cached.querySelector('img');
    if (img)
      img.classList.add('cached');
    return cached;
  }

  var thumbnail = assertInstanceof(this.ownerDocument.createElement('div'),
      HTMLDivElement);
  thumbnail.id = `thumbnail-${this.thumbnailElementId_++}`;
  thumbnail.className = 'ribbon-image';
  thumbnail.setAttribute('role', 'listitem');
  thumbnail.addEventListener('click', function() {
    var index = this.dataModel_.indexOf(item);
    this.selectionModel_.unselectAll();
    this.selectionModel_.setIndexSelected(index, true);
  }.bind(this));

  util.createChild(thumbnail, 'indicator loading');
  util.createChild(thumbnail, 'image-wrapper');
  util.createChild(thumbnail, 'selection-frame');

  this.setThumbnailImage_(thumbnail, item);

  // TODO: Implement LRU eviction.
  // Never evict the thumbnails that are currently in the DOM because we rely
  // on this cache to find them by URL.
  this.renderCache_[url] = thumbnail;
  return thumbnail;
};

/**
 * Set the thumbnail image.
 *
 * @param {!Element} thumbnail Thumbnail element.
 * @param {!GalleryItem} item Gallery item.
 * @private
 */
Ribbon.prototype.setThumbnailImage_ = function(thumbnail, item) {
  thumbnail.setAttribute('title', item.getFileName());

  if (!item.getThumbnailMetadataItem())
    return;

  var hideIndicator = function() {
    thumbnail.querySelector('.indicator').classList.toggle('loading', false);
  };

  this.thumbnailModel_.get([item.getEntry()]).then(function(metadataList) {
    var loader = new ThumbnailLoader(
        item.getEntry(),
        ThumbnailLoader.LoaderType.IMAGE,
        metadataList[0]);
    // Pass 0.35 as auto fill threshold. This value allows to fill 4:3 and 3:2
    // photos in 16:9 box (ratio factors for them are ~1.34 and ~1.18
    // respectively).
    loader.load(
        thumbnail.querySelector('.image-wrapper'),
        ThumbnailLoader.FillMode.AUTO,
        ThumbnailLoader.OptimizationMode.NEVER_DISCARD,
        hideIndicator /* opt_onSuccess */,
        undefined /* opt_onError */,
        undefined /* opt_onGeneric */,
        0.35 /* opt_autoFillThreshold */,
        Ribbon.THUMBNAIL_WIDTH /* opt_boxWidth */,
        Ribbon.THUMBNAIL_HEIGHT /* opt_boxHeight */);
  });
};

/**
 * Content change handler.
 *
 * @param {!Event} event Event.
 * @private
 */
Ribbon.prototype.onContentChange_ = function(event) {
  var url = event.item.getEntry().toURL();
  if (event.oldEntry.toURL() !== url)
    this.remapCache_(event.oldEntry.toURL(), url);

  var thumbnail = this.renderCache_[url];
  if (thumbnail && event.item)
    this.setThumbnailImage_(thumbnail, event.item);
};

/**
 * Update the thumbnail element cache.
 *
 * @param {string} oldUrl Old url.
 * @param {string} newUrl New url.
 * @private
 */
Ribbon.prototype.remapCache_ = function(oldUrl, newUrl) {
  if (oldUrl != newUrl && (oldUrl in this.renderCache_)) {
    this.renderCache_[newUrl] = this.renderCache_[oldUrl];
    delete this.renderCache_[oldUrl];
  }
};
