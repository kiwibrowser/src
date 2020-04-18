// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Controller for QuickView.
 * This should be initialized with |init_| method.
 *
 * @param {!MetadataModel} metadataModel File system metadata.
 * @param {!FileSelectionHandler} selectionHandler
 * @param {!ListContainer} listContainer
 * @param {!cr.ui.MenuButton} selectionMenuButton
 * @param {!QuickViewModel} quickViewModel
 * @param {!TaskController} taskController
 * @param {!cr.ui.ListSelectionModel} fileListSelectionModel
 * @param {!QuickViewUma} quickViewUma
 * @param {!MetadataBoxController} metadataBoxController
 * @param {DialogType} dialogType
 * @param {!VolumeManagerWrapper} volumeManager
 *
 * @constructor
 */
function QuickViewController(
    metadataModel, selectionHandler, listContainer, selectionMenuButton,
    quickViewModel, taskController, fileListSelectionModel, quickViewUma,
    metadataBoxController, dialogType, volumeManager) {
  /**
   * @type {FilesQuickView}
   * @private
   */
  this.quickView_ = null;

  /**
   * @type {!FileSelectionHandler}
   * @private
   */
  this.selectionHandler_ = selectionHandler;

  /**
   * @type {!ListContainer}
   * @private
   */
  this.listContainer_ = listContainer;

  /**
   * @type{!QuickViewModel}
   * @private
   */
  this.quickViewModel_ = quickViewModel;

  /**
   * @type {!QuickViewUma}
   * @private
   */
  this.quickViewUma_ = quickViewUma;

  /**
   * @type {!MetadataModel}
   * @private
   */
  this.metadataModel_ = metadataModel;

  /**
   * @type {!TaskController}
   * @private
   */
  this.taskController_ = taskController;

  /**
   * @type {!cr.ui.ListSelectionModel}
   * @private
   */
  this.fileListSelectionModel_ = fileListSelectionModel;

  /**
   * @type {!MetadataBoxController}
   * @private
   */
  this.metadataBoxController_ = metadataBoxController;

  /**
   * @type {DialogType}
   * @private
   */
  this.dialogType_ = dialogType;

  /**
   * @type {!VolumeManagerWrapper}
   * @private
   */
  this.volumeManager_ = volumeManager;

  /**
   * Current selection of selectionHandler.
   *
   * @type {!Array<!FileEntry>}
   * @private
   */
  this.entries_ = [];

  this.selectionHandler_.addEventListener(
      FileSelectionHandler.EventType.CHANGE,
      this.onFileSelectionChanged_.bind(this));
  this.listContainer_.element.addEventListener(
      'keydown', this.onKeyDownToOpen_.bind(this));
  this.listContainer_.element.addEventListener('command', function(event) {
    if (event.command.id === 'get-info')
      this.display_(QuickViewUma.WayToOpen.CONTEXT_MENU);
  }.bind(this));
  selectionMenuButton.addEventListener('command', function(event) {
    if (event.command.id === 'get-info')
      this.display_(QuickViewUma.WayToOpen.SELECTION_MENU);
  }.bind(this));
}

/**
 * List of local volume types.
 *
 * In this context, "local" means that files in that volume are directly
 * accessible from the Chrome browser process by Linux VFS paths. In this
 * regard, media views are NOT local even though files in media views are
 * actually stored in the local disk.
 *
 * Due to access control of WebView, non-local files can not be previewed
 * with Quick View unless thumbnails are provided (which is the case with
 * Drive).
 *
 * @type {!Array<!VolumeManagerCommon.VolumeType>}
 * @const
 * @private
 */
QuickViewController.LOCAL_VOLUME_TYPES_ = [
  VolumeManagerCommon.VolumeType.ARCHIVE,
  VolumeManagerCommon.VolumeType.DOWNLOADS,
  VolumeManagerCommon.VolumeType.REMOVABLE,
  VolumeManagerCommon.VolumeType.ANDROID_FILES,
];

/**
 * List of unsupported image subtypes
 * @type {!Array<string>}
 * @const
 * @private
 */
QuickViewController.UNSUPPORTED_IMAGE_SUBTYPES_ = [
  'TIFF',
];

/**
 * Initialize the controller with quick view which will be lazily loaded.
 * @param {!FilesQuickView} quickView
 * @private
 */
QuickViewController.prototype.init_ = function(quickView) {
  this.quickView_ = quickView;
  this.metadataBoxController_.init(quickView);
  document.body.addEventListener(
      'keydown', this.onQuickViewKeyDown_.bind(this));
  quickView.addEventListener('close', function() {
    this.listContainer_.focus();
  }.bind(this));
  quickView.onOpenInNewButtonTap = this.onOpenInNewButtonTap_.bind(this);

  var toolTip = this.quickView_.$$('files-tooltip');
  var elems = this.quickView_.$$('#toolbar').querySelectorAll('[has-tooltip]');
  toolTip.addTargets(elems);
};

/**
 * Craete quick view element.
 * @return Promise<!FilesQuickView>
 * @private
 */
QuickViewController.prototype.createQuickView_ = function() {
  return new Promise(function(resolve, reject) {
    Polymer.Base.importHref(constants.FILES_QUICK_VIEW_HTML, function() {
      var quickView = document.querySelector('#quick-view');
      i18nTemplate.process(quickView, loadTimeData);
      resolve(quickView);
    }, reject);
  });
};

/**
 * Handles open-in-new button tap.
 *
 * @param {!Event} event A button click event.
 * @private
 */
QuickViewController.prototype.onOpenInNewButtonTap_ = function(event) {
  this.taskController_.executeDefaultTask();
  this.quickView_.close();
};

/**
 * Handles key event on listContainer if it's relevant to quick view.
 *
 * @param {!Event} event A keyboard event.
 * @private
 */
QuickViewController.prototype.onKeyDownToOpen_ = function(event) {
  if (this.entries_.length == 0)
    return;
  if (event.key === ' ') {
    event.preventDefault();
    event.stopImmediatePropagation();
    this.display_(QuickViewUma.WayToOpen.SPACE_KEY);
  }
};

/**
 * Handles key event on quick view.
 *
 * @param {!Event} event A keyboard event.
 * @private
 */
QuickViewController.prototype.onQuickViewKeyDown_ = function(event) {
  if (this.quickView_.isOpened()) {
    switch (event.key) {
      case ' ':
      case 'Escape':
        event.preventDefault();
        // Prevent the open dialog from closing.
        event.stopImmediatePropagation();
        this.quickView_.close();
        break;
      case 'ArrowRight':
      case 'ArrowDown':
        var index = this.fileListSelectionModel_.selectedIndex + 1;
        if (index >= this.fileListSelectionModel_.length)
          index = 0;
        this.fileListSelectionModel_.selectedIndex = index;
        break;
      case 'ArrowLeft':
      case 'ArrowUp':
        var index = this.fileListSelectionModel_.selectedIndex - 1;
        if (index < 0)
          index = this.fileListSelectionModel_.length - 1;
        this.fileListSelectionModel_.selectedIndex = index;
        break;
    }
  }
};

/**
 * Display quick view.
 *
 * @param {QuickViewUma.WayToOpen=} opt_wayToOpen in which way opening of
 *     quick view was triggered. Can be omitted if quick view is already open.
 * @private
 */
QuickViewController.prototype.display_ = function(opt_wayToOpen) {
  this.updateQuickView_().then(function() {
    if (!this.quickView_.isOpened()) {
      this.quickView_.open();
      this.quickViewUma_.onOpened(this.entries_[0], assert(opt_wayToOpen));
    }
  }.bind(this));
};

/**
 * Update quick view on file selection change.
 *
 * @param {!Event} event an Event whose target is FileSelectionHandler.
 * @private
 */
QuickViewController.prototype.onFileSelectionChanged_ = function(event) {
  this.entries_ = event.target.selection.entries;
  if (this.quickView_ && this.quickView_.isOpened()) {
    assert(this.entries_.length > 0);
    var entry = this.entries_[0];
    this.quickViewModel_.setSelectedEntry(entry);
    this.display_();
  }
};

/**
 * @param {!FileEntry} entry
 * @return {!Promise<!Array<!FileTask>>}
 * @private
 */
QuickViewController.prototype.getAvailableTasks_ = function(entry) {
  return this.taskController_.getFileTasks().then(function(tasks) {
    return tasks.getTaskItems();
  });
};

/**
 * Update quick view using current entries.
 *
 * @return {!Promise} Promise fulfilled after quick view is updated.
 * @private
 */
QuickViewController.prototype.updateQuickView_ = function() {
  if (!this.quickView_) {
    return this.createQuickView_()
        .then(this.init_.bind(this))
        .then(this.updateQuickView_.bind(this))
        .catch(console.error);
  }
  assert(this.entries_.length > 0);
  // TODO(oka): Support multi-selection.
  this.quickViewModel_.setSelectedEntry(this.entries_[0]);

  var entry =
      (/** @type {!FileEntry} */ (this.quickViewModel_.getSelectedEntry()));
  assert(entry);
  this.quickViewUma_.onEntryChanged(entry);
  return Promise
      .all([
        this.metadataModel_.get([entry], ['thumbnailUrl']),
        this.getAvailableTasks_(entry)
      ])
      .then(function(values) {
        var items = (/**@type{Array<MetadataItem>}*/ (values[0]));
        var tasks = (/**@type{!Array<!FileTask>}*/ (values[1]));
        return this.onMetadataLoaded_(entry, items, tasks);
      }.bind(this))
      .catch(console.error);
};

/**
 * Update quick view using file entry and loaded metadata and tasks.
 *
 * @param {!FileEntry} entry
 * @param {Array<MetadataItem>} items
 * @param {!Array<!FileTask>} tasks
 * @private
 */
QuickViewController.prototype.onMetadataLoaded_ = function(
    entry, items, tasks) {
  return this.getQuickViewParameters_(entry, items, tasks)
      .then(function(params) {
        this.quickView_.type = params.type || '';
        this.quickView_.subtype = params.subtype || '';
        this.quickView_.filePath = params.filePath || '';
        this.quickView_.hasTask = params.hasTask || false;
        this.quickView_.contentUrl = params.contentUrl || '';
        this.quickView_.videoPoster = params.videoPoster || '';
        this.quickView_.audioArtwork = params.audioArtwork || '';
        this.quickView_.autoplay = params.autoplay || false;
        this.quickView_.browsable = params.browsable || false;
      }.bind(this));
};

/**
 * @typedef {{
 *   type: string,
 *   subtype: string,
 *   filePath: string,
 *   contentUrl: (string|undefined),
 *   videoPoster: (string|undefined),
 *   audioArtwork: (string|undefined),
 *   autoplay: (boolean|undefined),
 *   browsable: (boolean|undefined),
 * }}
 */
var QuickViewParams;

/**
 * @param {!FileEntry} entry
 * @param {Array<MetadataItem>} items
 * @param {!Array<!FileTask>} tasks
 * @return !Promise<!QuickViewParams>
 *
 * @private
 */
QuickViewController.prototype.getQuickViewParameters_ = function(
    entry, items, tasks) {
  var item = items[0];
  var typeInfo = FileType.getType(entry);
  var type = typeInfo.type;

  /** @type {!QuickViewParams} */
  var params = {
    type: type,
    subtype: typeInfo.subtype,
    filePath: entry.name,
    hasTask: tasks.length > 0,
  };

  var volumeInfo = this.volumeManager_.getVolumeInfo(entry);
  var localFile =
      volumeInfo &&
      QuickViewController.LOCAL_VOLUME_TYPES_.indexOf(
          volumeInfo.volumeType) >= 0;

  if (!localFile) {
    // For Drive files, display a thumbnail if there is one.
    if (item.thumbnailUrl) {
      return this.loadThumbnailFromDrive_(item.thumbnailUrl)
          .then(function(result) {
            if (result.status === 'success') {
              if (params.type == 'video') {
                params.videoPoster = result.data;
              } else if (params.type == 'image') {
                params.contentUrl = result.data;
              } else {
                // TODO(sashab): Rather than re-use 'image', create a new type
                // here, e.g. 'thumbnail'.
                params.type = 'image';
                params.contentUrl = result.data;
              }
            }
            return params;
          }.bind(this));
    }

    // We ask user to open it with external app.
    return Promise.resolve(params);
  }

  if (type === '.folder') {
    return Promise.resolve(params);
  }
  return new Promise(function(resolve, reject) {
           entry.file(resolve, reject);
         })
      .then(function(file) {
        switch (type) {
          case 'image':
            if (QuickViewController.UNSUPPORTED_IMAGE_SUBTYPES_.indexOf(
                    typeInfo.subtype) !== -1) {
              params.contentUrl = '';
            } else {
              params.contentUrl = URL.createObjectURL(file);
            }
            return params;
          case 'video':
            params.contentUrl = URL.createObjectURL(file);
            params.autoplay = true;
            if (item.thumbnailUrl) {
              params.videoPoster = item.thumbnailUrl;
            }
            return params;
          case 'audio':
            params.contentUrl = URL.createObjectURL(file);
            params.autoplay = true;
            return this.metadataModel_.get([entry], ['contentThumbnailUrl'])
                .then(function(items) {
                  var item = items[0];
                  if (item.contentThumbnailUrl) {
                    params.audioArtwork = item.contentThumbnailUrl;
                  }
                  return params;
                });
          case 'document':
            if (typeInfo.subtype === 'HTML') {
              params.contentUrl = URL.createObjectURL(file);
              return params;
            } else {
              break;
            }
        }
        var browsable = tasks.some(function(task) {
          return ['view-in-browser', 'view-pdf'].includes(
              task.taskId.split('|')[2]);
        });
        params.browsable = browsable;
        params.contentUrl = browsable ? URL.createObjectURL(file) : '';
        if (params.subtype == 'PDF')
          params.contentUrl += '#view=FitH';
        return params;
      }.bind(this))
      .catch(function(e) {
        console.error(e);
        return params;
      });
};

/**
 * Loads a thumbnail from Drive.
 *
 * @param {string} url Thumbnail url
 * @return Promise<{{status: string, data:string, width:number, height:number}}>
 * @private
 */
QuickViewController.prototype.loadThumbnailFromDrive_ = function(url) {
  return new Promise(function(resolve) {
    ImageLoaderClient.getInstance().load(url, resolve);
  });
};
