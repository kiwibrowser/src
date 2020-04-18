// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Root class of the background page.
 * @constructor
 * @implements {FileBrowserBackgroundFull}
 * @extends {BackgroundBase}
 * @struct
 */
function FileBrowserBackgroundImpl() {
  BackgroundBase.call(this);

  /** @type {!analytics.Tracker} */
  this.tracker = metrics.getTracker();

  /**
   * Progress center of the background page.
   * @type {!ProgressCenter}
   */
  this.progressCenter = new ProgressCenter();

  /**
   * File operation manager.
   * @type {FileOperationManager}
   */
  this.fileOperationManager = null;

  /**
   * Class providing loading of import history, used in
   * cloud import.
   *
   * @type {!importer.HistoryLoader}
   */
  this.historyLoader = new importer.RuntimeHistoryLoader(this.tracker);

  /**
   * Event handler for progress center.
   * @private {FileOperationHandler}
   */
  this.fileOperationHandler_ = null;

  /**
   * Event handler for C++ sides notifications.
   * @private {!DeviceHandler}
   */
  this.deviceHandler_ = new DeviceHandler();

  // Handle device navigation requests.
  this.deviceHandler_.addEventListener(
      DeviceHandler.VOLUME_NAVIGATION_REQUESTED,
      this.handleViewEvent_.bind(this));

  /**
   * Drive sync handler.
   * @type {!DriveSyncHandler}
   */
  this.driveSyncHandler = new DriveSyncHandler(this.progressCenter);

  /**
   * @type {!importer.DispositionChecker.CheckerFunction}
   */
  this.dispositionChecker_ = importer.DispositionChecker.createChecker(
      this.historyLoader, this.tracker);

  /**
   * Provides support for scaning media devices as part of Cloud Import.
   * @type {!importer.MediaScanner}
   */
  this.mediaScanner = new importer.DefaultMediaScanner(
      importer.createMetadataHashcode,
      this.dispositionChecker_,
      importer.DefaultDirectoryWatcher.create);

  /**
   * Handles importing of user media (e.g. photos, videos) from removable
   * devices.
   * @type {!importer.MediaImportHandler}
   */
  this.mediaImportHandler = new importer.MediaImportHandler(
      this.progressCenter, this.historyLoader, this.dispositionChecker_,
      this.tracker, this.driveSyncHandler);

  /**
   * String assets.
   * @type {Object<string>}
   */
  this.stringData = null;

  /**
   * Provides drive search to app launcher.
   * @private {!LauncherSearch}
   */
  this.launcherSearch_ = new LauncherSearch();

  // Initialize handlers.
  chrome.fileBrowserHandler.onExecute.addListener(this.onExecute_.bind(this));
  chrome.runtime.onMessageExternal.addListener(
      this.onExternalMessageReceived_.bind(this));
  chrome.contextMenus.onClicked.addListener(
      this.onContextMenuClicked_.bind(this));

  // Initializa string and volume manager related stuffs.
  this.initializationPromise_.then(function(strings) {
    this.stringData = strings;
    this.initContextMenu_();

    volumeManagerFactory.getInstance().then(function(volumeManager) {
      volumeManager.addEventListener(
          VolumeManagerCommon.VOLUME_ALREADY_MOUNTED,
          this.handleViewEvent_.bind(this));
    }.bind(this));

    this.fileOperationManager = new FileOperationManager();
    this.fileOperationHandler_ = new FileOperationHandler(
        this.fileOperationManager, this.progressCenter);
  }.bind(this));

  // Handle newly mounted FSP file systems. Workaround for crbug.com/456648.
  // TODO(mtomasz): Replace this hack with a proper solution.
  chrome.fileManagerPrivate.onMountCompleted.addListener(
      this.onMountCompleted_.bind(this));

  launcher.queue.run(function(callback) {
    this.initializationPromise_.then(callback);
  }.bind(this));
}

FileBrowserBackgroundImpl.prototype.__proto__ = BackgroundBase.prototype;

/**
 * Register callback to be invoked after initialization.
 * If the initialization is already done, the callback is invoked immediately.
 *
 * @param {function()} callback Initialize callback to be registered.
 */
FileBrowserBackgroundImpl.prototype.ready = function(callback) {
  this.initializationPromise_.then(callback);
};

/**
 * Opens the volume root (or opt directoryPath) in main UI.
 *
 * @param {!Event} event An event with the volumeId or
 *     devicePath.
 * @private
 */
FileBrowserBackgroundImpl.prototype.handleViewEvent_ = function(event) {
  util.doIfPrimaryContext(() => {
    this.handleViewEventInternal_(event);
  });
};

/**
 * @param {!Event} event An event with the volumeId or
 *     devicePath.
 * @private
 */
FileBrowserBackgroundImpl.prototype.handleViewEventInternal_ = function(event) {
  volumeManagerFactory.getInstance()
      .then(
          (/**
           * Retrieves the root file entry of the volume on the requested
           * device.
           * @param {!VolumeManager} volumeManager
           */
          function(volumeManager) {
            if (event.devicePath) {
              var volume = volumeManager.volumeInfoList.findByDevicePath(
                  event.devicePath);
              if (volume) {
                this.navigateToVolumeRoot_(volume, event.filePath);
              } else {
                console.error('Got view event with invalid volume id.');
              }
            } else if (event.volumeId) {
              if (event.type === VolumeManagerCommon.VOLUME_ALREADY_MOUNTED)
                this.navigateToVolumeInFocusedWindowWhenReady_(
                    event.volumeId, event.filePath);
              else
                this.navigateToVolumeWhenReady_(event.volumeId, event.filePath);
            } else {
              console.error('Got view event with no actionable destination.');
            }
          }).bind(this));
};

/**
 * Retrieves the root file entry of the volume on the requested device.
 *
 * @param {!string} volumeId ID of the volume to navigate to.
 * @return {!Promise<VolumeInfo>}
 * @private
 */
FileBrowserBackgroundImpl.prototype.retrieveVolumeInfo_ = function(volumeId) {
  return volumeManagerFactory.getInstance().then(
      (/**
        * @param {!VolumeManager} volumeManager
        */
       function(volumeManager) {
         return volumeManager.volumeInfoList.whenVolumeInfoReady(volumeId)
             .catch(function(e) {
               console.error(
                   'Unable to find volume for id: ' + volumeId +
                   '. Error: ' + e.message);
             });
       }).bind(this));
};

/**
 * Opens the volume root (or opt directoryPath) in main UI.
 *
 * @param {!string} volumeId ID of the volume to navigate to.
 * @param {!string=} opt_directoryPath Optional path to be opened.
 * @private
 */
FileBrowserBackgroundImpl.prototype.navigateToVolumeWhenReady_ = function(
    volumeId, opt_directoryPath) {
  this.retrieveVolumeInfo_(volumeId).then(function(volume) {
    this.navigateToVolumeRoot_(volume, opt_directoryPath);
  }.bind(this));
};

/**
 * Opens the volume root (or opt directoryPath) in the main UI of the focused
 * window.
 *
 * @param {!string} volumeId ID of the volume to navigate to.
 * @param {!string=} opt_directoryPath Optional path to be opened.
 * @private
 */
FileBrowserBackgroundImpl.prototype.navigateToVolumeInFocusedWindowWhenReady_ =
    function(volumeId, opt_directoryPath) {
  this.retrieveVolumeInfo_(volumeId).then(function(volume) {
    this.navigateToVolumeInFocusedWindow_(volume, opt_directoryPath);
  }.bind(this));
};

/**
 * If a path was specified, retrieve that directory entry,
 * otherwise return the root entry of the volume.
 *
 * @param {!VolumeInfo} volume
 * @param {string=} opt_directoryPath Optional directory path to be opened.
 * @return {!Promise<!DirectoryEntry>}
 * @private
 */
FileBrowserBackgroundImpl.prototype.retrieveEntryInVolume_ = function(
    volume, opt_directoryPath) {
  return volume.resolveDisplayRoot().then(function(root) {
    if (opt_directoryPath) {
      return new Promise(
          root.getDirectory.bind(root, opt_directoryPath, {create: false}));
    } else {
      return Promise.resolve(root);
    }
  });
};

/**
 * Opens the volume root (or opt directoryPath) in main UI.
 *
 * @param {!VolumeInfo} volume
 * @param {string=} opt_directoryPath Optional directory path to be opened.
 * @private
 */
FileBrowserBackgroundImpl.prototype.navigateToVolumeRoot_ = function(
    volume, opt_directoryPath) {
  this.retrieveEntryInVolume_(volume, opt_directoryPath)
      .then(
          /**
           * Launches app opened on {@code directory}.
           * @param {DirectoryEntry} directory
           */
          function(directory) {
            launcher.launchFileManager(
                {currentDirectoryURL: directory.toURL()},
                /* App ID */ undefined, LaunchType.FOCUS_SAME_OR_CREATE);
          });
};

/**
 * Opens the volume root (or opt directoryPath) in main UI of the focused
 * window.
 *
 * @param {!VolumeInfo} volume
 * @param {string=} opt_directoryPath Optional directory path to be opened.
 * @private
 */
FileBrowserBackgroundImpl.prototype.navigateToVolumeInFocusedWindow_ = function(
    volume, opt_directoryPath) {
  this.retrieveEntryInVolume_(volume, opt_directoryPath)
      .then(function(directoryEntry) {
        if (directoryEntry)
          volumeManagerFactory.getInstance().then(function(volumeManager) {
            volumeManager.dispatchEvent(
                VolumeManagerCommon.createArchiveOpenedEvent(directoryEntry));
          }.bind(this));
      });
};

/**
 * Prefix for the dialog ID.
 * @type {!string}
 * @const
 */
var DIALOG_ID_PREFIX = 'dialog#';

/**
 * Value of the next file manager dialog ID.
 * @type {number}
 */
var nextFileManagerDialogID = 0;

/**
 * Registers dialog window to the background page.
 *
 * @param {!Window} dialogWindow Window of the dialog.
 */
function registerDialog(dialogWindow) {
  var id = DIALOG_ID_PREFIX + (nextFileManagerDialogID++);
  window.background.dialogs[id] = dialogWindow;
  dialogWindow.addEventListener('pagehide', function() {
    delete window.background.dialogs[id];
  });
}

/**
 * Executes a file browser task.
 *
 * @param {string} action Task id.
 * @param {Object} details Details object.
 * @private
 */
FileBrowserBackgroundImpl.prototype.onExecute_ = function(action, details) {
  var appState = {
    params: {action: action},
    // It is not allowed to call getParent() here, since there may be
    // no permissions to access it at this stage. Therefore we are passing
    // the selectionURL only, and the currentDirectory will be resolved
    // later.
    selectionURL: details.entries[0].toURL()
  };

  // Every other action opens a Files app window.
  // For mounted devices just focus any Files app window. The mounted
  // volume will appear on the navigation list.
  launcher.launchFileManager(
      appState,
      /* App ID */ undefined,
      LaunchType.FOCUS_SAME_OR_CREATE);
};

/**
 * Launches the app.
 * @private
 * @override
 */
FileBrowserBackgroundImpl.prototype.onLaunched_ = function() {
  metrics.startInterval('Load.BackgroundLaunch');
  this.initializationPromise_.then(function() {
    if (nextFileManagerWindowID == 0) {
      // The app just launched. Remove window state records that are not needed
      // any more.
      chrome.storage.local.get(function(items) {
        for (var key in items) {
          if (items.hasOwnProperty(key)) {
            if (key.match(FILES_ID_PATTERN))
              chrome.storage.local.remove(key);
          }
        }
      });
    }
    launcher.launchFileManager(
        null, undefined, LaunchType.FOCUS_ANY_OR_CREATE,
        function() { metrics.recordInterval('Load.BackgroundLaunch'); });
  });
};

/** @const {!string} */
var GPLUS_PHOTOS_APP_ID = 'efjnaogkjbogokcnohkmnjdojkikgobo';

/**
 * Handles a message received via chrome.runtime.sendMessageExternal.
 *
 * @param {*} message
 * @param {MessageSender} sender
 */
FileBrowserBackgroundImpl.prototype.onExternalMessageReceived_ =
    function(message, sender) {
  if ('id' in sender && sender.id === GPLUS_PHOTOS_APP_ID) {
    importer.handlePhotosAppMessage(message);
  }
};

/**
 * Restarted the app, restore windows.
 * @private
 * @override
 */
FileBrowserBackgroundImpl.prototype.onRestarted_ = function() {
  // Reopen file manager windows.
  chrome.storage.local.get(function(items) {
    for (var key in items) {
      if (items.hasOwnProperty(key)) {
        var match = key.match(FILES_ID_PATTERN);
        if (match) {
          metrics.startInterval('Load.BackgroundRestart');
          var id = Number(match[1]);
          try {
            var appState = /** @type {Object} */ (JSON.parse(items[key]));
            launcher.launchFileManager(appState, id, undefined, function() {
              metrics.recordInterval('Load.BackgroundRestart');
            });
          } catch (e) {
            console.error('Corrupt launch data for ' + id);
          }
        }
      }
    }
  });
};

/**
 * Handles clicks on a custom item on the launcher context menu.
 * @param {!Object} info Event details.
 * @private
 */
FileBrowserBackgroundImpl.prototype.onContextMenuClicked_ = function(info) {
  if (info.menuItemId == 'new-window') {
    // Find the focused window (if any) and use it's current url for the
    // new window. If not found, then launch with the default url.
    this.findFocusedWindow_().then(function(key) {
      if (!key) {
        launcher.launchFileManager(appState);
        return;
      }
      var appState = {
        // Do not clone the selection url, only the current directory.
        currentDirectoryURL: window.appWindows[key].
            contentWindow.appState.currentDirectoryURL
      };
      launcher.launchFileManager(appState);
    }).catch(function(error) {
      console.error(error.stack || error);
    });
  }
};

/**
 * Looks for a focused window.
 *
 * @return {!Promise<?string>} Promise fulfilled with a key of the focused
 *     window, or null if not found.
 * @private
 */
FileBrowserBackgroundImpl.prototype.findFocusedWindow_ = function() {
  return new Promise(function(fulfill, reject) {
    for (var key in window.appWindows) {
      try {
        if (window.appWindows[key].contentWindow.isFocused()) {
          fulfill(key);
          return;
        }
      } catch (ignore) {
        // The isFocused method may not be defined during initialization.
        // Therefore, wrapped with a try-catch block.
      }
    }
    fulfill(null);
  });
};

/**
 * Handles mounted FSP volumes and fires the Files app. This is a quick fix for
 * crbug.com/456648.
 * @param {!Object} event Event details.
 * @private
 */
FileBrowserBackgroundImpl.prototype.onMountCompleted_ = function(event) {
  util.doIfPrimaryContext(() => {
    this.onMountCompletedInternal_(event);
  });
};

/**
 * @param {!Object} event Event details.
 * @private
 */
FileBrowserBackgroundImpl.prototype.onMountCompletedInternal_ = function(
    event) {
  // If there is no focused window, then create a new one opened on the
  // mounted volume.
  this.findFocusedWindow_()
      .then(function(key) {
        let statusOK = event.status === 'success' ||
            event.status === 'error_path_already_mounted';
        let volumeTypeOK = event.volumeMetadata.volumeType ===
                VolumeManagerCommon.VolumeType.PROVIDED &&
            event.volumeMetadata.source === VolumeManagerCommon.Source.FILE;
        if (key === null && event.eventType === 'mount' && statusOK &&
            event.volumeMetadata.mountContext === 'user' && volumeTypeOK) {
          this.navigateToVolumeWhenReady_(event.volumeMetadata.volumeId);
        }
      }.bind(this))
      .catch(function(error) {
        console.error(error.stack || error);
      });
};

/**
 * Initializes the context menu. Recreates if already exists.
 * @private
 */
FileBrowserBackgroundImpl.prototype.initContextMenu_ = function() {
  try {
    // According to the spec [1], the callback is optional. But no callback
    // causes an error for some reason, so we call it with null-callback to
    // prevent the error. http://crbug.com/353877
    // Also, we read the runtime.lastError here not to output the message on the
    // console as an unchecked error.
    // - [1] https://developer.chrome.com/extensions/contextMenus#method-remove
    chrome.contextMenus.remove('new-window', function() {
      var ignore = chrome.runtime.lastError;
    });
  } catch (ignore) {
    // There is no way to detect if the context menu is already added, therefore
    // try to recreate it every time.
  }
  chrome.contextMenus.create({
    id: 'new-window',
    contexts: ['launcher'],
    title: str('NEW_WINDOW_BUTTON_LABEL')
  });
};

/**
 * Singleton instance of Background object.
 * @type {!FileBrowserBackgroundImpl}
 */
window.background = new FileBrowserBackgroundImpl();

/**
 * Lastly, end recording of the background page Load.BackgroundScript metric.
 * NOTE: This call must come after the call to metrics.clearUserId.
 */
metrics.recordInterval('Load.BackgroundScript');
