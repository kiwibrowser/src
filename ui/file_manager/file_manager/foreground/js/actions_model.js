// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * A single action, that can be taken on a set of entries.
 * @interface
 */
function Action() {
}

/**
 * Executes this action on the set of entries.
 */
Action.prototype.execute = function() {
};

/**
 * Checks whether this action can execute on the set of entries.
 *
 * @return {boolean} True if the function can execute, false if not.
 */
Action.prototype.canExecute = function() {
};

/**
 * @return {?string}
 */
Action.prototype.getTitle = function() {
};

/**
 * @typedef {{
 *  alertDialog: FilesAlertDialog,
 *  errorDialog: ErrorDialog,
 *  listContainer: ListContainer,
 *  shareDialog: ShareDialog,
 * }}
 */
var ActionModelUI;

/**
 * @param {!Entry} entry
 * @param {!ActionModelUI} ui
 * @param {!VolumeManagerWrapper} volumeManager
 * @implements {Action}
 * @constructor
 * @struct
 */
function DriveShareAction(entry, volumeManager, ui) {
  /**
   * @private {!Entry}
   * @const
   */
  this.entry_ = entry;

  /**
   * @private {!VolumeManagerWrapper}
   * @const
   */
  this.volumeManager_ = volumeManager;

  /**
   * @private {!ActionModelUI}
   * @const
   */
  this.ui_ = ui;
}

/**
 * @param {!Array<!Entry>} entries
 * @param {!ActionModelUI} ui
 * @param {!VolumeManagerWrapper} volumeManager
 * @return {DriveShareAction}
 */
DriveShareAction.create = function(entries, volumeManager, ui) {
  if (entries.length !== 1)
    return null;
  return new DriveShareAction(entries[0], volumeManager, ui);
};

/**
 * @override
 */
DriveShareAction.prototype.execute = function() {
  // For Team Drives entries, open the Sharing dialog in a new window.
  if (util.isTeamDriveEntry(this.entry_)) {
    chrome.fileManagerPrivate.getEntryProperties(
        [this.entry_], ['shareUrl'], function(results) {
          if (chrome.runtime.lastError) {
            console.error(chrome.runtime.lastError.message);
            return;
          }
          if (results.length != 1) {
            console.error(
                'getEntryProperties for shareUrl should return 1 entry ' +
                '(returned ' + results.length + ')');
            return;
          }
          if (results[0].shareUrl === undefined) {
            console.error('getEntryProperties shareUrl is undefined');
            return;
          }
          util.visitURL(results[0].shareUrl);
        }.bind(this));
    return;
  }
  this.ui_.shareDialog.showEntry(this.entry_, function(result) {
    if (result == ShareDialog.Result.NETWORK_ERROR)
      this.ui_.errorDialog.show(str('SHARE_ERROR'), null, null, null);
  }.bind(this));
};

/**
 * @override
 */
DriveShareAction.prototype.canExecute = function() {
  return this.volumeManager_.getDriveConnectionState().type !==
      VolumeManagerCommon.DriveConnectionType.OFFLINE &&
      !util.isTeamDriveRoot(this.entry_);
};

/**
 * @return {?string}
 */
DriveShareAction.prototype.getTitle = function() {
  return null;
};

/**
 * @param {!Array<!Entry>} entries
 * @param {!MetadataModel} metadataModel
 * @param {!DriveSyncHandler} driveSyncHandler
 * @param {!ActionModelUI} ui
 * @param {boolean} value
 * @param {function()} onExecute
 * @implements {Action}
 * @constructor
 * @struct
 */
function DriveToggleOfflineAction(entries, metadataModel, driveSyncHandler, ui,
    value, onExecute) {
  /**
   * @private {!Array<!Entry>}
   * @const
   */
  this.entries_ = entries;

  /**
   * @private {!MetadataModel}
   * @const
   */
  this.metadataModel_ = metadataModel;

  /**
   * @private {!DriveSyncHandler}
   * @const
   */
  this.driveSyncHandler_ = driveSyncHandler;

  /**
   * @private {!ActionModelUI}
   * @const
   */
  this.ui_ = ui;

  /**
   * @private {boolean}
   * @const
   */
  this.value_ = value;

  /**
   * @private {function()}
   * @const
   */
  this.onExecute_ = onExecute;
}

/**
 * @param {!Array<!Entry>} entries
 * @param {!MetadataModel} metadataModel
 * @param {!DriveSyncHandler} driveSyncHandler
 * @param {!ActionModelUI} ui
 * @param {boolean} value
 * @param {function()} onExecute
 * @return {DriveToggleOfflineAction}
 */
DriveToggleOfflineAction.create = function(entries, metadataModel,
    driveSyncHandler, ui, value, onExecute) {
  var directoryEntries = entries.filter(function(entry) {
    return entry.isDirectory;
  });
  if (directoryEntries.length > 0)
    return null;

  var actionableEntries = entries.filter(function(entry) {
    if (entry.isDirectory)
      return false;
    var metadata = metadataModel.getCache(
        [entry], ['hosted', 'pinned'])[0];
    if (metadata.hosted)
      return false;
    if (metadata.pinned === value)
      return false;
    return true;
  });

  if (actionableEntries.length === 0)
    return null;

  return new DriveToggleOfflineAction(actionableEntries, metadataModel,
      driveSyncHandler, ui, value, onExecute);
};

/**
 * @override
 */
DriveToggleOfflineAction.prototype.execute = function() {
  var entries = this.entries_;
  if (entries.length == 0)
    return;

  var currentEntry;
  var error = false;

  var steps = {
    // Pick an entry and pin it.
    start: function() {
      // Check if all the entries are pinned or not.
      if (entries.length === 0) {
        this.onExecute_();
        return;
      }
      currentEntry = entries.shift();
      chrome.fileManagerPrivate.pinDriveFile(
          currentEntry,
          this.value_,
          steps.entryPinned);
    }.bind(this),

    // Check the result of pinning.
    entryPinned: function() {
      error = !!chrome.runtime.lastError;
      if (error && this.value_) {
        this.metadataModel_.get([currentEntry], ['size']).then(
            function(results) {
              steps.showError(results[0].size);
            });
        return;
      }
      this.metadataModel_.notifyEntriesChanged([currentEntry]);
      this.metadataModel_.get([currentEntry], ['pinned']).then(steps.updateUI);
    }.bind(this),

    // Update the user interface according to the cache state.
    updateUI: function() {
      this.ui_.listContainer.currentView.updateListItemsMetadata(
          'external', [currentEntry]);
      if (!error)
        steps.start();
    }.bind(this),

    // Show an error.
    showError: function(size) {
      this.ui_.alertDialog.showHtml(
          str('DRIVE_OUT_OF_SPACE_HEADER'),
          strf('DRIVE_OUT_OF_SPACE_MESSAGE',
               unescape(currentEntry.name),
               util.bytesToString(size)),
          null, null, null);
    }.bind(this)
  };
  steps.start();

  if (this.value_ && this.driveSyncHandler_.isSyncSuppressed())
    this.driveSyncHandler_.showDisabledMobileSyncNotification();
};

/**
 * @override
 */
DriveToggleOfflineAction.prototype.canExecute = function() {
  return true;
};

/**
 * @return {?string}
 */
DriveToggleOfflineAction.prototype.getTitle = function() {
  return null;
};

/**
 * @param {!Entry} entry
 * @param {!FolderShortcutsDataModel} shortcutsModel
 * @param {function()} onExecute
 * @implements {Action}
 * @constructor
 * @struct
 */
function DriveCreateFolderShortcutAction(entry, shortcutsModel, onExecute) {
  /**
   * @private {!Entry}
   * @const
   */
  this.entry_ = entry;

  /**
   * @private {!FolderShortcutsDataModel}
   * @const
   */
  this.shortcutsModel_ = shortcutsModel;

  /**
   * @private {function()}
   * @const
   */
  this.onExecute_ = onExecute;
}

/**
 * @param {!Array<!Entry>} entries
 * @param {!VolumeManagerWrapper} volumeManager
 * @param {!FolderShortcutsDataModel} shortcutsModel
 * @param {function()} onExecute
 * @return {DriveCreateFolderShortcutAction}
 */
DriveCreateFolderShortcutAction.create = function(entries, volumeManager,
    shortcutsModel, onExecute) {
  if (entries.length !== 1 || entries[0].isFile)
    return null;
  var locationInfo = volumeManager.getLocationInfo(entries[0]);
  if (!locationInfo || locationInfo.isSpecialSearchRoot ||
      locationInfo.isRootEntry) {
    return null;
  }
  return new DriveCreateFolderShortcutAction(
      entries[0], shortcutsModel, onExecute);
};

/**
 * @override
 */
DriveCreateFolderShortcutAction.prototype.execute = function() {
  this.shortcutsModel_.add(this.entry_);
  this.onExecute_();
};

/**
 * @override
 */
DriveCreateFolderShortcutAction.prototype.canExecute = function() {
  return !this.shortcutsModel_.exists(this.entry_);
};

/**
 * @return {?string}
 */
DriveCreateFolderShortcutAction.prototype.getTitle = function() {
  return null;
};

/**
 * @param {!Entry} entry
 * @param {!FolderShortcutsDataModel} shortcutsModel
 * @param {function()} onExecute
 * @implements {Action}
 * @constructor
 * @struct
 */
function DriveRemoveFolderShortcutAction(entry, shortcutsModel, onExecute) {
  /**
   * @private {!Entry}
   * @const
   */
  this.entry_ = entry;

  /**
   * @private {!FolderShortcutsDataModel}
   * @const
   */
  this.shortcutsModel_ = shortcutsModel;

  /**
   * @private {function()}
   * @const
   */
  this.onExecute_ = onExecute;
}

/**
 * @param {!Array<!Entry>} entries
 * @param {!FolderShortcutsDataModel} shortcutsModel
 * @param {function()} onExecute
 * @return {DriveRemoveFolderShortcutAction}
 */
DriveRemoveFolderShortcutAction.create = function(entries, shortcutsModel,
    onExecute) {
  if (entries.length !== 1 || entries[0].isFile ||
      !shortcutsModel.exists(entries[0])) {
    return null;
  }
  return new DriveRemoveFolderShortcutAction(
      entries[0], shortcutsModel, onExecute);
};

/**
 * @override
 */
DriveRemoveFolderShortcutAction.prototype.execute = function() {
  this.shortcutsModel_.remove(this.entry_);
  this.onExecute_();
};

/**
 * @override
 */
DriveRemoveFolderShortcutAction.prototype.canExecute = function() {
  return this.shortcutsModel_.exists(this.entry_);
};

/**
 * @return {?string}
 */
DriveRemoveFolderShortcutAction.prototype.getTitle = function() {
  return null;
};


/**
 * Opens the entry in Drive Web for the user to manage permissions etc.
 *
 * @param {!Entry} entry The entry to open the 'Manage' page for.
 * @param {!ActionModelUI} ui
 * @param {!VolumeManagerWrapper} volumeManager
 * @implements {Action}
 * @constructor
 * @struct
 */
function DriveManageAction(entry, volumeManager, ui) {
  /**
   * The entry to open the 'Manage' page for.
   *
   * @private {!Entry}
   * @const
   */
  this.entry_ = entry;

  /**
   * @private {!VolumeManagerWrapper}
   * @const
   */
  this.volumeManager_ = volumeManager;

  /**
   * @private {!ActionModelUI}
   * @const
   */
  this.ui_ = ui;
}

/**
 * Creates a new DriveManageAction object.
 * |entries| must contain only a single entry.
 *
 * @param {!Array<!Entry>} entries
 * @param {!ActionModelUI} ui
 * @param {!VolumeManagerWrapper} volumeManager
 * @return {DriveManageAction}
 */
DriveManageAction.create = function(entries, volumeManager, ui) {
  if (entries.length !== 1)
    return null;

  return new DriveManageAction(entries[0], volumeManager, ui);
};

/**
 * @override
 */
DriveManageAction.prototype.execute = function() {
  chrome.fileManagerPrivate.getEntryProperties(
      [this.entry_], ['alternateUrl'], function(results) {
        if (chrome.runtime.lastError) {
          console.error(chrome.runtime.lastError.message);
          return;
        }
        if (results.length != 1) {
          console.error(
              'getEntryProperties for alternateUrl should return 1 entry ' +
              '(returned ' + results.length + ')');
          return;
        }
        if (results[0].alternateUrl === undefined) {
          console.error('getEntryProperties alternateUrl is undefined');
          return;
        }
        util.visitURL(results[0].alternateUrl);
      }.bind(this));
};

/**
 * @override
 */
DriveManageAction.prototype.canExecute = function() {
  return this.volumeManager_.getDriveConnectionState().type !==
      VolumeManagerCommon.DriveConnectionType.OFFLINE &&
      !util.isTeamDriveRoot(this.entry_);
};

/**
 * @return {?string}
 */
DriveManageAction.prototype.getTitle = function() {
  return null;
};


/**
 * A custom action set by the FSP API.
 *
 * @param {!Array<!Entry>} entries
 * @param {string} id
 * @param {?string} title
 * @param {function()} onExecute
 * @implements {Action}
 * @constructor
 * @struct
 */
function CustomAction(entries, id, title, onExecute) {
  /**
   * @private {!Array<!Entry>}
   * @const
   */
  this.entries_ = entries;

  /**
   * @private {string}
   * @const
   */
  this.id_ = id;

  /**
   * @private {?string}
   * @const
   */
  this.title_ = title;

  /**
   * @private {function()}
   * @const
   */
  this.onExecute_ = onExecute;
}

/**
 * @override
 */
CustomAction.prototype.execute = function() {
  chrome.fileManagerPrivate.executeCustomAction(this.entries_, this.id_,
      function() {
        if (chrome.runtime.lastError) {
          console.error('Failed to execute a custom action because of: ' +
            chrome.runtime.lastError.message);
        }
        this.onExecute_();
      }.bind(this));
};

/**
 * @override
 */
CustomAction.prototype.canExecute = function() {
  return true;  // Custom actions are always executable.
};

/**
 * @override
 */
CustomAction.prototype.getTitle = function() {
  return this.title_;
};

/**
 * Represents a set of actions for a set of entries. Includes actions set
 * locally in JS, as well as those retrieved from the FSP API.
 *
 * @param {!VolumeManagerWrapper} volumeManager
 * @param {!MetadataModel} metadataModel
 * @param {!FolderShortcutsDataModel} shortcutsModel
 * @param {!DriveSyncHandler} driveSyncHandler
 * @param {!ActionModelUI} ui
 * @param {!Array<!Entry>} entries
 * @constructor
 * @extends {cr.EventTarget}
 * @struct
 */
function ActionsModel(
    volumeManager, metadataModel, shortcutsModel, driveSyncHandler, ui,
    entries) {
  /**
   * @private {!VolumeManagerWrapper}
   * @const
   */
  this.volumeManager_ = volumeManager;

  /**
   * @private {!MetadataModel}
   * @const
   */
  this.metadataModel_ = metadataModel;

  /**
   * @private {!FolderShortcutsDataModel}
   * @const
   */
  this.shortcutsModel_ = shortcutsModel;

  /**
   * @private {!DriveSyncHandler}
   * @const
   */
  this.driveSyncHandler_ = driveSyncHandler;

  /**
   * @private {!ActionModelUI}
   * @const
   */
  this.ui_ = ui;

  /**
   * @private {!Array<!Entry>}
   * @const
   */
  this.entries_ = entries;

  /**
   * @private {!Object<!Action>}
   */
  this.actions_ = {};

  /**
   * @private {?function()}
   */
  this.initializePromiseReject_ = null;

  /**
   * @private {Promise}
   */
  this.initializePromise_ = null;

  /**
   * @private {boolean}
   */
  this.destroyed_ = false;
}

ActionsModel.prototype = {
  __proto__: cr.EventTarget.prototype
};

/**
 * List of common actions, used both internally and externally (custom actions).
 * Keep in sync with file_system_provider.idl.
 * @enum {string}
 */
ActionsModel.CommonActionId = {
  SHARE: 'SHARE',
  SAVE_FOR_OFFLINE: 'SAVE_FOR_OFFLINE',
  OFFLINE_NOT_NECESSARY: 'OFFLINE_NOT_NECESSARY'
};

/**
 * @enum {string}
 */
ActionsModel.InternalActionId = {
  CREATE_FOLDER_SHORTCUT: 'create-folder-shortcut',
  REMOVE_FOLDER_SHORTCUT: 'remove-folder-shortcut',
  MANAGE_IN_DRIVE: 'manage-in-drive'
};

/**
 * Initializes the ActionsModel, including populating the list of available
 * actions for the given entries.
 * @return {!Promise}
 */
ActionsModel.prototype.initialize = function() {
  if (this.initializePromise_)
    return this.initializePromise_;

  this.initializePromise_ = new Promise(function(fulfill, reject) {
    if (this.destroyed_) {
      reject();
      return;
    }
    this.initializePromiseReject_ = reject;

    var volumeInfo = this.entries_.length >= 1 &&
        this.volumeManager_.getVolumeInfo(this.entries_[0]);
    if (!volumeInfo) {
      fulfill({});
      return;
    }
    // All entries need to be on the same volume to execute ActionsModel
    // commands.
    // TODO(sashab): Move this to util.js.
    for (var i = 1; i < this.entries_.length; i++) {
      var volumeInfoToCompare =
          this.volumeManager_.getVolumeInfo(this.entries_[i]);
      if (!volumeInfoToCompare ||
          volumeInfoToCompare.volumeId != volumeInfo.volumeId) {
        fulfill({});
        return;
      }
    }

    var actions = {};
    switch (volumeInfo.volumeType) {
      // For Drive, actions are constructed directly in the Files app code.
      case VolumeManagerCommon.VolumeType.DRIVE:
        var shareAction = DriveShareAction.create(
            this.entries_, this.volumeManager_, this.ui_);
        if (shareAction)
          actions[ActionsModel.CommonActionId.SHARE] = shareAction;

        var saveForOfflineAction = DriveToggleOfflineAction.create(
            this.entries_, this.metadataModel_, this.driveSyncHandler_,
            this.ui_, true, this.invalidate_.bind(this));
        if (saveForOfflineAction) {
          actions[ActionsModel.CommonActionId.SAVE_FOR_OFFLINE] =
              saveForOfflineAction;
        }

        var offlineNotNecessaryAction = DriveToggleOfflineAction.create(
            this.entries_, this.metadataModel_, this.driveSyncHandler_,
            this.ui_, false, this.invalidate_.bind(this));
        if (offlineNotNecessaryAction) {
          actions[ActionsModel.CommonActionId.OFFLINE_NOT_NECESSARY] =
              offlineNotNecessaryAction;
        }

        var createFolderShortcutAction =
            DriveCreateFolderShortcutAction.create(this.entries_,
                this.volumeManager_, this.shortcutsModel_,
                this.invalidate_.bind(this));
        if (createFolderShortcutAction) {
          actions[ActionsModel.InternalActionId.CREATE_FOLDER_SHORTCUT] =
              createFolderShortcutAction;
        }

        var removeFolderShortcutAction =
            DriveRemoveFolderShortcutAction.create(this.entries_,
                this.shortcutsModel_, this.invalidate_.bind(this));
        if (removeFolderShortcutAction) {
          actions[ActionsModel.InternalActionId.REMOVE_FOLDER_SHORTCUT] =
              removeFolderShortcutAction;
        }

        var manageInDriveAction = DriveManageAction.create(
            this.entries_, this.volumeManager_, this.ui_);
        if (manageInDriveAction) {
          actions[ActionsModel.InternalActionId.MANAGE_IN_DRIVE] =
              manageInDriveAction;
        }

        fulfill(actions);
        break;

      // For FSP, fetch custom actions via an API.
      case VolumeManagerCommon.VolumeType.PROVIDED:
        chrome.fileManagerPrivate.getCustomActions(this.entries_,
            function(customActions) {
              if (chrome.runtime.lastError) {
                console.error('Failed to fetch custom actions because of: ' +
                    chrome.runtime.lastError.message);
              } else {
                customActions.forEach(function(action) {
                  actions[action.id] = new CustomAction(
                      this.entries_, action.id, action.title || null,
                      this.invalidate_.bind(this));
                }.bind(this));
              }
              fulfill(actions);
            }.bind(this));
        break;

      default:
        fulfill(actions);
    }
  }.bind(this)).then(function(actions) {
    this.actions_ = actions;
  }.bind(this));

  return this.initializePromise_;
};

/**
 * @return {!Object<!Action>}
 */
ActionsModel.prototype.getActions = function() {
  return this.actions_;
};

/**
 * @param {string} id
 * @return {Action}
 */
ActionsModel.prototype.getAction = function(id) {
  return this.actions_[id] || null;
};

/**
 * Destroys the model and cancels initialization if in progress.
 */
ActionsModel.prototype.destroy = function() {
  this.destroyed_ = true;
  if (this.initializePromiseReject_ !== null) {
    var reject = this.initializePromiseReject_;
    this.initializePromiseReject_ = null;
    reject();
  }
};

/**
 * Invalidates the current actions model by emitting an invalidation event.
 * The model has to be initialized again, as the list of actions might have
 * changed.
 *
 * @private
 */
ActionsModel.prototype.invalidate_ = function() {
  if (this.initializePromiseReject_ !== null) {
    var reject = this.initializePromiseReject_;
    this.initializePromiseReject_ = null;
    this.initializePromise_ = null;
    reject();
  }
  cr.dispatchSimpleEvent(this, 'invalidated', true);
};
