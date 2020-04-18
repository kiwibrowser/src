// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Controller of metadata box.
 * This should be initialized with |init| method.
 *
 * @param{!MetadataModel} metadataModel
 * @param{!QuickViewModel} quickViewModel
 * @param{!FileMetadataFormatter} fileMetadataFormatter
 *
 * @constructor
 */
function MetadataBoxController(
    metadataModel, quickViewModel, fileMetadataFormatter) {
  /**
   * @type {!MetadataModel}
   * @private
   */
  this.metadataModel_ = metadataModel;

  /**
   * @type {!QuickViewModel}
   * @private
   */
  this.quickViewModel_ = quickViewModel;

 /**
  * @type {FilesMetadataBox} metadataBox
  * @private
  */
  this.metadataBox_ = null;

  /**
   * @type {FilesQuickView} quickView
   * @private
   */
  this.quickView_ = null;

  /**
   * @type {!FileMetadataFormatter}
   * @private
   */
  this.fileMetadataFormatter_ = fileMetadataFormatter;

  /**
   * @type {Entry}
   * @private
   */
  this.previousEntry_ = null;

  /**
   * @type {boolean}
   * @private
   */
  this.isDirectorySizeLoading_ = false;

  /**
   * @type {?function(!DirectoryEntry)}
   * @private
   */
  this.onDirectorySizeLoaded_ = null;
}

/**
 * Initialize the controller with quick view which will be lazily loaded.
 * @param{!FilesQuickView} quickView
 */
MetadataBoxController.prototype.init = function(quickView) {
  // TODO(oka): Add storage to persist the value of
  // quickViewModel_.metadataBoxActive.
  this.fileMetadataFormatter_.addEventListener(
      'date-time-format-changed', this.updateView_.bind(this));

  quickView.addEventListener(
      'metadata-box-active-changed', this.updateView_.bind(this));

  this.quickViewModel_.addEventListener(
      'selected-entry-changed', this.updateView_.bind(this));

  this.metadataBox_ = quickView.getFilesMetadataBox();
  this.quickView_ = quickView;
};

/**
 * @const {!Array<string>}
 */
MetadataBoxController.GENERAL_METADATA_NAME = [
  'size',
  'modificationTime',
];

/**
 * Update the view of metadata box.
 *
 * @private
 */
MetadataBoxController.prototype.updateView_ = function() {
  if (!this.quickView_.metadataBoxActive) {
    return;
  }
  var entry = this.quickViewModel_.getSelectedEntry();
  var isSameEntry = util.isSameEntry(entry, this.previousEntry_);
  this.previousEntry_ = entry;
  // Do not clear isSizeLoading and size fields when the entry is not changed.
  this.metadataBox_.clear(isSameEntry);
  if (!entry)
    return;
  this.metadataModel_
      .get([entry], MetadataBoxController.GENERAL_METADATA_NAME.concat([
        'hosted', 'externalFileUrl'
      ]))
      .then(this.onGeneralMetadataLoaded_.bind(this, entry, isSameEntry));
};

/**
 * Update metadata box with general file information.
 * Then retrieve file specific metadata if any.
 *
 * @param {!Entry} entry
 * @param {boolean} isSameEntry if the entry is not changed from the last time.
 * @param {!Array<!MetadataItem>} items
 *
 * @private
 */
MetadataBoxController.prototype.onGeneralMetadataLoaded_ = function(
    entry, isSameEntry, items) {
  var type = FileType.getType(entry).type;
  var item = items[0];

  this.metadataBox_.type = type;
  // For directory, item.size is always -1.
  if (item.size && !entry.isDirectory) {
    this.metadataBox_.size =
        this.fileMetadataFormatter_.formatSize(item.size, item.hosted);
  }
  if (entry.isDirectory) {
    this.setDirectorySize_(
        /** @type {!DirectoryEntry} */ (entry), isSameEntry);
  }
  if (item.modificationTime) {
    this.metadataBox_.modificationTime =
        this.fileMetadataFormatter_.formatModDate(item.modificationTime);
  }

  if (item.externalFileUrl) {
    this.metadataModel_.get([entry], ['contentMimeType']).then(function(items) {
      var item = items[0];
      this.metadataBox_.mediaMimeType = item.contentMimeType;
    }.bind(this));
  } else {
    this.metadataModel_.get([entry], ['mediaMimeType']).then(function(items) {
      var item = items[0];
      this.metadataBox_.mediaMimeType = item.mediaMimeType;
    }.bind(this));
  }

  if (['image', 'video', 'audio'].includes(type)) {
    if (item.externalFileUrl) {
      this.metadataModel_.get([entry], ['imageHeight', 'imageWidth'])
          .then(function(items) {
            var item = items[0];
            this.metadataBox_.imageHeight = item.imageHeight;
            this.metadataBox_.imageWidth = item.imageWidth;
          }.bind(this));
    } else {
      this.metadataModel_
          .get(
              [entry],
              [
                'ifd',
                'imageHeight',
                'imageWidth',
                'mediaAlbum',
                'mediaArtist',
                'mediaDuration',
                'mediaGenre',
                'mediaTitle',
                'mediaTrack',
                'mediaYearRecorded',
              ])
          .then(function(items) {
            var item = items[0];
            this.metadataBox_.ifd = item.ifd || null;
            this.metadataBox_.imageHeight = item.imageHeight || 0;
            this.metadataBox_.imageWidth = item.imageWidth || 0;
            this.metadataBox_.mediaAlbum = item.mediaAlbum || '';
            this.metadataBox_.mediaArtist = item.mediaArtist || '';
            this.metadataBox_.mediaDuration = item.mediaDuration || 0;
            this.metadataBox_.mediaGenre = item.mediaGenre || '';
            this.metadataBox_.mediaTitle = item.mediaTitle || '';
            this.metadataBox_.mediaTrack = item.mediaTrack || '';
            this.metadataBox_.mediaYearRecorded = item.mediaYearRecorded || '';
          }.bind(this));
    }
  }
};

/**
 * Set a current directory's size in metadata box.
 * If previous getDirectorySize is still running, next getDirectorySize is not
 * called at the time. After the previous callback is finished, getDirectorySize
 * that corresponds to the last setDirectorySize_ is called.
 *
 * @param {!DirectoryEntry} entry
 * @param {boolean} isSameEntry
 *     if the entry is not changed from the last time.
 *
 * @private
 */
MetadataBoxController.prototype.setDirectorySize_ = function(
    entry, isSameEntry) {
  if (!entry.isDirectory)
    return;

  if (this.isDirectorySizeLoading_) {
    if (!isSameEntry)
      this.metadataBox_.isSizeLoading = true;

    // Only retain the last setDirectorySize_ request.
    this.onDirectorySizeLoaded_ = function(lastEntry) {
      this.setDirectorySize_(entry, util.isSameEntry(entry, lastEntry));
    }.bind(this);
    return;
  }

  // false if the entry is same. true if the entry is changed.
  this.metadataBox_.isSizeLoading = !isSameEntry;
  this.isDirectorySizeLoading_ = true;
  chrome.fileManagerPrivate.getDirectorySize(entry, function(size) {
    this.isDirectorySizeLoading_ = false;
    if (this.onDirectorySizeLoaded_)
      setTimeout(this.onDirectorySizeLoaded_.bind(null, entry));

    if (this.quickViewModel_.getSelectedEntry() != entry)
      return;

    if (chrome.runtime.lastError) {
      this.metadataBox_.isSizeLoading = false;
      return;
    }

    this.metadataBox_.isSizeLoading = false;
    this.metadataBox_.size = this.fileMetadataFormatter_.formatSize(size, true);
  }.bind(this));
};
