// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Externs generated from namespace: fileManagerPrivate */

/**
 * @typedef {{
 *   taskId: string,
 *   title: string,
 *   iconUrl: string,
 *   isDefault: boolean
 * }}
 */
var FileTask;

/**
 * @typedef {{
 *   icon16x16Url: (string|undefined),
 *   icon32x32Url: (string|undefined)
 * }}
 */
var IconSet;

/**
 * @typedef {{
 *   size: (number|undefined),
 *   modificationTime: (number|undefined),
 *   modificationByMeTime: (number|undefined),
 *   thumbnailUrl: (string|undefined),
 *   croppedThumbnailUrl: (string|undefined),
 *   externalFileUrl: (string|undefined),
 *   alternateUrl: (string|undefined),
 *   imageWidth: (number|undefined),
 *   imageHeight: (number|undefined),
 *   imageRotation: (number|undefined),
 *   pinned: (boolean|undefined),
 *   present: (boolean|undefined),
 *   hosted: (boolean|undefined),
 *   dirty: (boolean|undefined),
 *   availableOffline: (boolean|undefined),
 *   availableWhenMetered: (boolean|undefined),
 *   customIconUrl: (string|undefined),
 *   contentMimeType: (string|undefined),
 *   sharedWithMe: (boolean|undefined),
 *   shared: (boolean|undefined)
 * }}
 */
var EntryProperties;

/**
 * @typedef {{
 *   totalSize: number,
 *   remainingSize: number
 * }}
 */
var MountPointSizeStats;

/**
 * @typedef {{
 *   profileId: string,
 *   displayName: string,
 *   isCurrentProfile: boolean
 * }}
 */
var ProfileInfo;

/**
 * @typedef {{
 *   volumeId: string,
 *   fileSystemId: (string|undefined),
 *   iconSet: IconSet,
 *   source: string,
 *   volumeLabel: (string|undefined),
 *   profile: ProfileInfo,
 *   sourcePath: (string|undefined),
 *   volumeType: string,
 *   deviceType: (string|undefined),
 *   devicePath: (string|undefined),
 *   isParentDevice: (boolean|undefined),
 *   isReadOnly: boolean,
 *   isReadOnlyRemovableDevice: boolean,
 *   hasMedia: boolean,
 *   configurable: boolean,
 *   watchable: boolean,
 *   mountCondition: (string|undefined),
 *   mountContext: (string|undefined),
 *   diskFileSystemType: (string|undefined)
 * }}
 */
var VolumeMetadata;

/**
 * @typedef {{
 *   eventType: string,
 *   status: string,
 *   volumeMetadata: VolumeMetadata,
 *   shouldNotify: boolean
 * }}
 */
var MountCompletedEvent;

/**
 * @typedef {{
 *   fileUrl: string,
 *   transferState: string,
 *   transferType: string,
 *   processed: number,
 *   total: number,
 *   num_total_jobs: number
 * }}
 */
var FileTransferStatus;

/**
 * @typedef {{
 *   type: string,
 *   fileUrl: string
 * }}
 */
var DriveSyncErrorEvent;

/**
 * @typedef {{
 *   type: string,
 *   sourceUrl: (string|undefined),
 *   destinationUrl: (string|undefined),
 *   size: (number|undefined),
 *   error: (string|undefined)
 * }}
 */
var CopyProgressStatus;

/**
 * @typedef {{
 *   url: string,
 *   changes: Array
 * }}
 */
var FileChange;

/**
 * @typedef {{
 *   eventType: string,
 *   entry: Object,
 *   changedFiles: (Array|undefined)
 * }}
 */
var FileWatchEvent;

/**
 * @typedef {{
 *   driveEnabled: boolean,
 *   cellularDisabled: boolean,
 *   hostedFilesDisabled: boolean,
 *   searchSuggestEnabled: boolean,
 *   use24hourClock: boolean,
 *   allowRedeemOffers: boolean,
 *   timezone: string
 * }}
 */
var Preferences;

/**
 * @typedef {{
 *   cellularDisabled: (boolean|undefined),
 *   hostedFilesDisabled: (boolean|undefined)
 * }}
 */
var PreferencesChange;

/**
 * @typedef {{
 *   query: string,
 *   nextFeed: string
 * }}
 */
var SearchParams;

/**
 * @typedef {{
 *   query: string,
 *   types: string,
 *   maxResults: number
 * }}
 */
var SearchMetadataParams;

/**
 * @typedef {{
 *   entry: Entry,
 *   highlightedBaseName: string
 * }}
 */
var SearchResult;

/**
 * @typedef {{
 *   type: string,
 *   reason: (string|undefined)
 * }}
 */
var DriveConnectionState;

/**
 * @typedef {{
 *   type: string,
 *   devicePath: string
 * }}
 */
var DeviceEvent;

/**
 * @typedef {{
 *   providerId: string,
 *   iconSet: IconSet,
 *   name: string,
 *   configurable: boolean,
 *   watchable: boolean,
 *   multipleMounts: boolean,
 *   source: string
 * }}
 */
var Provider;

/**
 * @typedef {{
 *   id: string,
 *   title: (string|undefined)
 * }}
 */
var EntryAction;

/**
 * @const
 */
chrome.fileManagerPrivate = {};

/**
 * Logout the current user for navigating to the re-authentication screen for
 * the Google account.
 */
chrome.fileManagerPrivate.logoutUserForReauthentication = function() {};

/**
 * Cancels file selection.
 */
chrome.fileManagerPrivate.cancelDialog = function() {};

/**
 * Executes file browser task over selected files. |taskId| The unique
 * identifier of task to execute. |entries| Array of file entries |callback|
 * @param {string} taskId
 * @param {!Array<!Entry>} entries
 * @param {function((boolean|undefined))} callback |result| Result of the task
 *     execution.
 */
chrome.fileManagerPrivate.executeTask = function(taskId, entries, callback) {};

/**
 * Sets the default task for the supplied MIME types and path extensions.
 * Lists of MIME types and entries may contain duplicates.
 * |taskId| The unique identifier of task to mark as default. |entries| Array
 * of selected file entries to extract path extensions from. |mimeTypes| Array
 * of selected file MIME types. |callback|
 * @param {string} taskId
 * @param {!Array<!Entry>} entries
 * @param {!Array<string>} mimeTypes
 * @param {!function()} callback Callback that does not take arguments.
 */
chrome.fileManagerPrivate.setDefaultTask = function(taskId, entries, mimeTypes,
    callback) {};

/**
 * Gets the list of tasks that can be performed over selected files. |entries|
 * Array of selected entries |callback|
 * @param {!Array<!Entry>} entries
 * @param {function((!Array<!FileTask>|undefined))} callback |tasks| The list of
 *     matched file entries for this task.
 */
chrome.fileManagerPrivate.getFileTasks = function(entries, callback) {};

/**
 * Gets localized strings and initialization data. |callback|
 * @param {function((!Object|undefined))} callback |result| Hash containing the
 *     string assets.
 */
chrome.fileManagerPrivate.getStrings = function(callback) {};

/**
 * Adds file watch. |entry| Entry of file to watch |callback|
 * @param {!Entry} entry
 * @param {function((boolean|undefined))} callback |success| True when file
 *     watch is successfully added.
 */
chrome.fileManagerPrivate.addFileWatch = function(entry, callback) {};

/**
 * Removes file watch. |entry| Entry of watched file to remove |callback|
 * @param {!Entry} entry
 * @param {function((boolean|undefined))} callback |success| True when file
 *     watch is successfully
 * removed.
 */
chrome.fileManagerPrivate.removeFileWatch = function(entry, callback) {};

/**
 * Enables the extenal file scheme necessary to initiate drags to the browser
 * window for files on the external backend.
 */
chrome.fileManagerPrivate.enableExternalFileScheme = function() {};

/**
 * Requests R/W access to the specified entries as |entryUrls|. Note, that only
 * files backed by external file system backend will be granted the access.
 * @param {!Array<string>} entryUrls
 * @param {function()} callback Completion callback.
 */
chrome.fileManagerPrivate.grantAccess = function(entryUrls, callback) {};

/**
 * Selects multiple files. |selectedPaths| Array of selected paths
 * |shouldReturnLocalPath| true if paths need to be resolved to local paths.
 * |callback|
 * @param {!Array<string>} selectedPaths
 * @param {boolean} shouldReturnLocalPath
 * @param {function()} callback Callback that does not take arguments.
 */
chrome.fileManagerPrivate.selectFiles = function(selectedPaths,
    shouldReturnLocalPath, callback) {};

/**
 * Selects a file. |selectedPath| A selected path |index| Index of Filter
 * |forOpening| true if paths are selected for opening. false if for saving.
 * |shouldReturnLocalPath| true if paths need to be resolved to local paths.
 * |callback|
 * @param {string} selectedPath
 * @param {number} index
 * @param {boolean} forOpening
 * @param {boolean} shouldReturnLocalPath
 * @param {function()} callback Callback that does not take arguments.
 */
chrome.fileManagerPrivate.selectFile = function(selectedPath, index, forOpening,
    shouldReturnLocalPath, callback) {};

/**
 * Requests additional properties for files. |entries| list of entries of files
 * |callback|
 * @param {!Array<!Entry>} entries
 * @param {!Array<string>} names
 * @param {function((!Array<!EntryProperties>|undefined))} callback
 *     |entryProperties| A dictionary containing properties of the requested
 *     entries.
 */
chrome.fileManagerPrivate.getEntryProperties = function(entries, names,
    callback) {};

/**
 * Pins/unpins a Drive file in the cache. |entry| Entry of a file to pin/unpin.
 * |pin| Pass true to pin the file. |callback| Completion callback.
 * $(ref:runtime.lastError) will be set if     there was an error.
 * @param {!Entry} entry
 * @param {boolean} pin
 * @param {function()} callback Callback that does not take arguments.
 */
chrome.fileManagerPrivate.pinDriveFile = function(entry, pin, callback) {};

/**
 * If |entry| is a Drive file, ensures the file is downloaded to the cache.
 * Otherwise, finishes immediately in success. For example, when the file is
 * under Downloads, MTP, removeable media, or provided by extensions for
 * other cloud storage services than Google Drive, this does nothing.
 * This is a workaround to avoid intermittent and duplicated downloading of
 * a Drive file by current implementation of Drive integration when an
 * extension reads a file sequentially but intermittently.
 * @param {!Entry} entry A regular file entry to be read.
 * @param {function()} callback Callback called after having the file in cache.
 *     runtime.lastError will be set if there was an error.
 */
chrome.fileManagerPrivate.ensureFileDownloaded = function(entry, callback) {};

/**
 * Resolves file entries in the isolated file system and returns corresponding
 * entries in the external file system mounted to Chrome OS file manager
 * backend. If resolving entry fails, the entry will be just ignored and the
 * corresponding entry does not appear in the result.
 * @param {!Array<!Entry>} entries
 * @param {function((!Array<!Entry>|undefined))} callback Completion callback
 *     with resolved entries.
 */
chrome.fileManagerPrivate.resolveIsolatedEntries = function(entries,
    callback) {};

/**
 * Mount a resource or a file. |source| Mount point source. For compressed
 * files it is relative file path     within external file system |callback|
 * @param {string} source
 * @param {function((string|undefined))} callback Callback with source path of
 *     the mount.
 */
chrome.fileManagerPrivate.addMount = function(source, callback) {};

/**
 * Unmounts a mounted resource. |volumeId| An ID of the volume.
 * @param {string} volumeId
 */
chrome.fileManagerPrivate.removeMount = function(volumeId) {};

/**
 * Marks a cache file of Drive as mounted or unmounted.
 * Does nothing if the file is not under Drive directory.
 * @param {string} sourcePath Mounted source file. Relative file path within
 *     external file system.
 * @param {boolean} isMounted Mark as mounted if true. Mark as unmounted
 *     otherwise.
 * @param {function()} callback Completion callback. runtime.lastError will be
 *     set if there was an error.
 */
chrome.fileManagerPrivate.markCacheAsMounted = function(
    sourcePath, isMounted, callback) {};

/**
 * Get the list of mounted volumes. |callback|
 * @param {function((!Array<!VolumeMetadata>|undefined))} callback Callback with
 *     the list of VolumeMetadata representing mounted volumes.
 */
chrome.fileManagerPrivate.getVolumeMetadataList = function(callback) {};

/**
 * Cancels ongoing file transfers for selected files. |entries| Array of files
 * for which ongoing transfer should be canceled.
 * @param {!Array<!FileEntry>} entries
 * @param {function()} callback
 */
chrome.fileManagerPrivate.cancelFileTransfers = function(entries, callback) {};

/**
 * Cancels all ongoing file transfers.
 * @param {function()} callback
 */
chrome.fileManagerPrivate.cancelAllFileTransfers = function(callback) {};

/**
 * Starts to copy an entry. If the source is a directory, the copy is done
 * recursively. |entry| Entry of the source entry to be copied. |parent| Entry
 * of the destination directory. |newName| Name of the new entry. It must not
 * contain '/'. |callback| Completion callback.
 * @param {!Entry} entry
 * @param {!DirectoryEntry} parentEntry
 * @param {string} newName
 * @param {function((number|undefined))} callback |copyId| ID of the copy task.
 *     Can be used to identify the progress, and to cancel the task.
 */
chrome.fileManagerPrivate.startCopy = function(entry, parentEntry, newName,
    callback) {};

/**
 * Cancels the running copy task. |copyId| ID of the copy task to be cancelled.
 * |callback| Completion callback of the cancel.
 * @param {number} copyId
 * @param {function()} callback Callback that does not take arguments.
 */
chrome.fileManagerPrivate.cancelCopy = function(copyId, callback) {};

/**
 * Retrieves total and remaining size of a mount point. |volumeId| ID of the
 * volume to be checked. |callback|
 * @param {string} volumeId
 * @param {function((!MountPointSizeStats|undefined))} callback Name/value pairs
 *     of size stats. Will be undefined if stats could not be determined.
 */
chrome.fileManagerPrivate.getSizeStats = function(volumeId, callback) {};

/**
 * Formats a mounted volume. |volumeId| ID of the volume to be formatted.
 * @param {string} volumeId
 */
chrome.fileManagerPrivate.formatVolume = function(volumeId) {};

/**
 * Renames a mounted volume. |volumeId| ID of the volume to be renamed to
 * |newName|.
 * @param {string} volumeId
 * @param {string} newName
 */
chrome.fileManagerPrivate.renameVolume = function(volumeId, newName) {};

/**
 * Retrieves file manager preferences. |callback|
 * @param {function((!Preferences|undefined))} callback
 */
chrome.fileManagerPrivate.getPreferences = function(callback) {};

/**
 * Sets file manager preferences. |changeInfo|
 * @param {PreferencesChange} changeInfo
 */
chrome.fileManagerPrivate.setPreferences = function(changeInfo) {};

/**
 * Performs drive content search. |searchParams| |callback|
 * @param {SearchParams} searchParams
 * @param {function((!Array<Entry>|undefined), (string|undefined))} callback
 * Entries and ID of the feed that contains next chunk of the search result.
 * Should be sent to the next searchDrive request to perform incremental search.
 */
chrome.fileManagerPrivate.searchDrive = function(searchParams, callback) {};

/**
 * Performs drive metadata search. |searchParams| |callback|
 * @param {SearchMetadataParams} searchParams
 * @param {function((!Array<!SearchResult>|undefined))} callback
 */
chrome.fileManagerPrivate.searchDriveMetadata = function(searchParams,
    callback) {};

/**
 * Search for files in the given volume, whose content hash matches the list of
 * given hashes.
 * @param {string} volumeId
 * @param {!Array<string>} hashes
 * @param {function((!Object<string, !Array<string>>|undefined))} callback
 */
chrome.fileManagerPrivate.searchFilesByHashes = function(volumeId, hashes,
    callback) {};

/**
 * Create a zip file for the selected files. |parentEntry| Entry of the
 * directory containing the selected files. |entries| Selected entries.
 * The files must be under the directory specified by |parentEntry|. |destName|
 * Name of the destination zip file. The zip file will be created under the
 * directory specified by |parentEntry|.
 * @param {!Array<!Entry>} entries
 * @param {!DirectoryEntry} parentEntry
 * @param {string} destName
 * @param {function((boolean|undefined))} callback
 */
chrome.fileManagerPrivate.zipSelection = function(entries, parentEntry,
    destName, callback) {};

/**
 * Retrieves the state of the current drive connection. |callback|
 * @param {function((!DriveConnectionState|undefined))} callback
 */
chrome.fileManagerPrivate.getDriveConnectionState = function(callback) {};

/**
 * Checks whether the path name length fits in the limit of the filesystem.
 * |parentEntry| The parent directory entry. |name| The name of the file.
 * |callback| Called back when the check is finished.
 * @param {!DirectoryEntry} parentEntry
 * @param {string} name
 * @param {function((boolean|undefined))} callback |result| true if the length
 *     is in the valid range, false otherwise.
 */
chrome.fileManagerPrivate.validatePathNameLength = function(
    parentEntry, name, callback) {};

/**
 * Changes the zoom factor of the Files app. |operation| Zooming mode.
 * @param {string} operation
 */
chrome.fileManagerPrivate.zoom = function(operation) {};

/**
 * Requests a Drive API OAuth2 access token. |refresh| Whether the token should
 * be refetched instead of using the cached     one. |callback|
 * @param {boolean} refresh
 * @param {function((string|undefined))} callback |accessToken| OAuth2 access
 *     token, or an empty string if failed to fetch.
 */
chrome.fileManagerPrivate.requestAccessToken = function(refresh, callback) {};

/**
 * Requests a Webstore API OAuth2 access token. |callback|
 * @param {function((string|undefined))} callback |accessToken| OAuth2 access
 *     token, or an empty string if failed to fetch.
 */
chrome.fileManagerPrivate.requestWebStoreAccessToken = function(callback) {};

/**
 * Requests a share dialog url for the specified file.
 * @param {!Entry} entry
 * @param {function((string|undefined))} callback Callback with the result url.
 */
chrome.fileManagerPrivate.getShareUrl = function(entry, callback) {};

/**
 * Requests a download url to download the file contents.
 * @param {!Entry} entry
 * @param {function((string|undefined))} callback Callback with the result url.
 */
chrome.fileManagerPrivate.getDownloadUrl = function(entry, callback) {};

/**
 * Requests to share drive files.
 * @param {!Entry} entry
 * @param {string} shareType
 * @param {function()} callback Callback that does not take arguments.
 */
chrome.fileManagerPrivate.requestDriveShare = function(entry, shareType,
    callback) {};

/**
 * Requests to install a webstore item. |item_id| The id of the item to
 * install. |silentInstallation| False to show installation prompt. True not to
 * show. |callback|
 * @param {string} itemId
 * @param {boolean} silentInstallation
 * @param {function()} callback Callback that does not take arguments.
 */
chrome.fileManagerPrivate.installWebstoreItem = function(itemId,
    silentInstallation, callback) {};

/**
 * Obtains a list of profiles that are logged-in.
 * @param {function((!Array<!ProfileInfo>|undefined), (string|undefined),
 *     (string|undefined))} callback Callback with list of profile information,
 *     |runningProfile| ID of the profile that runs the application instance.
 *     |showingProfile| ID of the profile that shows the application window.
 */
chrome.fileManagerPrivate.getProfiles = function(callback) {};

/**
 * Opens inspector window. |type| InspectionType which specifies how to open
 * inspector.
 * @param {string} type
 */
chrome.fileManagerPrivate.openInspector = function(type) {};

/**
 * Opens settings sub page. |sub_page| Name of a sub page.
 * @param {string} sub_page
 */
chrome.fileManagerPrivate.openSettingsSubpage = function(sub_page) {};

/**
 * Computes an MD5 checksum for the given file.
 * @param {!Entry} entry
 * @param {function((string|undefined))} callback
 */
chrome.fileManagerPrivate.computeChecksum = function(entry, callback) {};

/**
 * Gets the MIME type of a file.
 * @param {!Entry} entry
 * @param {function((string|undefined))} callback Callback that MIME type of the
 *     file is passed.
 */
chrome.fileManagerPrivate.getMimeType = function(entry, callback) {};

/**
 * Gets a flag indicating whether user metrics reporting is enabled.
 * @param {function((boolean|undefined))} callback
 */
chrome.fileManagerPrivate.isUMAEnabled = function(callback) {};

/**
 * Sets a tag on a file or a directory. Only Drive files are supported.
 * @param {!Entry} entry
 * @param {string} visibility 'private' or 'public'
 * @param {string} key
 * @param {string} value
 * @param {function()} callback
 */
chrome.fileManagerPrivate.setEntryTag = function(entry, visibility, key,
    value, callback) {};

/**
 * Gets a flag indicating whether PiexLoader is enabled.
 * @param {function((boolean|undefined))} callback
 */
chrome.fileManagerPrivate.isPiexLoaderEnabled = function(callback) {};

/**
 * Returns list of available providers.
 * @param {function((!Array<!Provider>|undefined))} callback
 */
chrome.fileManagerPrivate.getProviders = function(callback) {};

/**
 * Requests adding a new provided file system. If not possible, then an error
 * via chrome.runtime.lastError is returned.
 * @param {string} providerId
 * @param {function()} callback
 */
chrome.fileManagerPrivate.addProvidedFileSystem =
    function(providerId, callback) {};

/**
 * Requests configuring an existing file system. If not possible, then returns
 * an error via chrome.runtime.lastError.
 * @param {string} volumeId
 * @param {function()} callback
 */
chrome.fileManagerPrivate.configureVolume = function(volumeId, callback) {};

/**
 * Requests fetching list of actions for the specified set of entries. If not
 * possible, then returns an error via chrome.runtime.lastError.
 * @param {!Array<!Entry>} entries
 * @param {function((!Array<!EntryAction>|undefined))} callback
 */
chrome.fileManagerPrivate.getCustomActions = function(entries, callback) {};

/**
 * Get the total size of a directory. |entry| Entry of the target directory.
 * |callback|
 * @param {!DirectoryEntry} entry
 * @param {function(number)} callback
 */
chrome.fileManagerPrivate.getDirectorySize = function(entry, callback) {};

/**
 * Gets recently modified files across file systems.
 * @param {string} restriction
 * @param {function((!Array<!FileEntry>))} callback
 */
chrome.fileManagerPrivate.getRecentFiles = function(restriction, callback) {};

/**
 * Executes the action on the specified set of entries. If not possible, then
 * returns an error via chrome.runtime.lastError.
 * @param {!Array<!Entry>} entries
 * @param {string} actionId
 * @param {function()} callback
 */
chrome.fileManagerPrivate.executeCustomAction = function(
    entries, actionId, callback) {};

/**
 * Returns true if crostini is enabled.
 * @param {function(boolean)} callback
 */
chrome.fileManagerPrivate.isCrostiniEnabled = function(callback) {};

/**
 * Starts and mounts crostini container.
 * @param {function()} callback Callback called after the crostini container
 *     is started and mounted.
 *     chrome.runtime.lastError will be set if there was an error.
 */
chrome.fileManagerPrivate.mountCrostiniContainer = function(callback) {};

/** @type {!ChromeEvent} */
chrome.fileManagerPrivate.onMountCompleted;

/** @type {!ChromeEvent} */
chrome.fileManagerPrivate.onFileTransfersUpdated;

/** @type {!ChromeEvent} */
chrome.fileManagerPrivate.onCopyProgress;

/** @type {!ChromeEvent} */
chrome.fileManagerPrivate.onDirectoryChanged;

/** @type {!ChromeEvent} */
chrome.fileManagerPrivate.onPreferencesChanged;

/** @type {!ChromeEvent} */
chrome.fileManagerPrivate.onDriveConnectionStatusChanged;

/** @type {!ChromeEvent} */
chrome.fileManagerPrivate.onDeviceChanged;

/** @type {!ChromeEvent} */
chrome.fileManagerPrivate.onDriveSyncError;

/** @type {!ChromeEvent} */
chrome.fileManagerPrivate.onAppsUpdated;

/** @enum {string} */
chrome.fileManagerPrivate.Verb = {
  OPEN_WITH: 'open_with',
  ADD_TO: 'add_to',
  PACK_WITH: 'pack_with',
  SHARE_WITH: 'share_with',
};

/** @enum {string} */
chrome.fileManagerPrivate.SourceRestriction = {
  ANY_SOURCE: 'any_source',
  NATIVE_SOURCE: 'native_source',
  NATIVE_OR_DRIVE_SOURCE: 'native_or_drive_source',
};
