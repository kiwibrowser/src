// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Shared cloud importer namespace
var importer = importer || {};

/** @enum {string} */
importer.ScanEvent = {
  FINALIZED: 'finalized',
  INVALIDATED: 'invalidated',
  UPDATED: 'updated'
};

/**
 * Mode of the scan to find new files.
 *
 * @enum {string}
 */
importer.ScanMode = {
  // Faster scan using import history to get candidates of new files.
  HISTORY: 'HISTORY',
  // Verifies content hash to eliminate content duplications from candidates
  // chosen by HISTORY.
  CONTENT: 'CONTENT',
};

/**
 * Disposition of an entry with respect to it's
 * presence in import history, drive, and so on.
 * @enum {string}
 */
importer.Disposition = {
  CONTENT_DUPLICATE: 'content-dupe',
  HISTORY_DUPLICATE: 'history-dupe',
  ORIGINAL: 'original',
  SCAN_DUPLICATE: 'scan-dupe'
};

/**
 * Storage keys for settings saved by importer.
 * @enum {string}
 */
importer.Setting = {
  HAS_COMPLETED_IMPORT: 'importer-has-completed-import',
  MACHINE_ID: 'importer-machine-id',
  PHOTOS_APP_ENABLED: 'importer-photo-app-enabled',
  LAST_KNOWN_LOG_ID: 'importer-last-known-log-id'
};

/**
 * Volume types eligible for the affections of Cloud Import.
 * @private @const {!Array<!VolumeManagerCommon.VolumeType>}
 */
importer.ELIGIBLE_VOLUME_TYPES_ = [
  VolumeManagerCommon.VolumeType.MTP,
  VolumeManagerCommon.VolumeType.REMOVABLE
];

/**
 * Root dir names for valid import locations.
 * @enum {string}
 */
importer.ValidImportRoots_ = {
  DCIM: 'DCIM',
  MP_ROOT: 'MP_ROOT' // MP_ROOT is a Sony thing.
};

/**
 * @enum {string}
 */
importer.Destination = {
  // locally copied, but not imported to cloud as of yet.
  DEVICE: 'device',
  GOOGLE_DRIVE: 'google-drive'
};

/**
 * Returns true if the entry is a media file type.
 *
 * @param {Entry} entry
 * @return {boolean}
 */
importer.isEligibleType = function(entry) {
  // TODO(mtomasz): Add support to mime types.
  return !!entry &&
      entry.isFile &&
      FileType.isType(['image', 'raw', 'video'], entry);
};

/**
 * Splits a path into an array of path elements.  The path elements are all
 * upper-cased.  Leading and trailing empty strings are removed.
 * @param {Entry} entry
 * @return {!Array<string>}
 */
importer.splitPath_ = function(entry) {
  var splitPath =  entry.fullPath.toUpperCase().split('/');
  // Remove the empty string caused by the leading '/'.
  splitPath.splice(0, 1);
  // If there is a trailing empty string, remove it.
  if (splitPath[splitPath.length - 1] === '') {
    splitPath.length = splitPath.length - 1;
  }
  return splitPath;
};

/**
 * Determines if this is an eligible import location.
 * @param {!Array<string>} splitPath
 * @return {boolean}
 * @private
 */
importer.isEligiblePath_ = function(splitPath) {
  /** @const {number} */
  var MISSING = -264512121;
  return splitPath.some(
      /** @param {string} dirname */
      function(dirname) {
        // Check dir hash.
        if (dirname.length == 0) {
          return false;
        }
        var no = 0;
        for (var i = 0; i < dirname.length; i++) {
          no = ((no << 5) - no) + dirname.charCodeAt(i);
          no = no & no;
        }
        return MISSING === no;
      });
};

/**
 * Returns true if the entry is a DCIM dir, or a descendant of a DCIM dir.
 *
 * @param {Entry} entry
 * @param {VolumeManagerCommon.VolumeInfoProvider} volumeInfoProvider
 * @return {boolean}
 */
importer.isBeneathMediaDir = function(entry, volumeInfoProvider) {
  if (!entry || !entry.fullPath) {
    return false;
  }
  var splitPath = importer.splitPath_(entry);
  if (importer.isEligiblePath_(splitPath)) {
    return true;
  }

  if (!(splitPath[0] in importer.ValidImportRoots_)) {
    return false;
  }

  console.assert(volumeInfoProvider !== null);
  var volumeInfo = volumeInfoProvider.getVolumeInfo(entry);
  return importer.isEligibleVolume(volumeInfo);
};

/**
 * Returns true if the volume is eligible for Cloud Import.
 *
 * @param {VolumeInfo} volumeInfo
 * @return {boolean}
 */
importer.isEligibleVolume = function(volumeInfo) {
  return !!volumeInfo &&
      importer.ELIGIBLE_VOLUME_TYPES_.indexOf(volumeInfo.volumeType) !== -1;
};

/**
 * Returns true if the entry is cloud import eligible.
 *
 * @param {VolumeManagerCommon.VolumeInfoProvider} volumeInfoProvider
 * @param {Entry} entry
 * @return {boolean}
 */
importer.isEligibleEntry = function(volumeInfoProvider, entry) {
  console.assert(volumeInfoProvider !== null);
  return importer.isEligibleType(entry) &&
      importer.isBeneathMediaDir(entry, volumeInfoProvider);
};

/**
 * Returns true if the entry represents a media directory for the purposes
 * of Cloud Import.
 *
 * @param {Entry|FakeEntry} entry
 * @param {VolumeManagerCommon.VolumeInfoProvider} volumeInfoProvider
 * @return {boolean}
 */
importer.isMediaDirectory = function(entry, volumeInfoProvider) {
  if (!entry || !entry.isDirectory || !entry.fullPath)
    return false;
  var splitPath = importer.splitPath_(/** @type {Entry} */(entry));
  if (importer.isEligiblePath_(splitPath))
    return true;

  // This is a media root if there is only one element in the path, and it is a
  // valid import root.
  if (splitPath[0] in importer.ValidImportRoots_ && splitPath.length === 1) {
    console.assert(volumeInfoProvider !== null);
    var volumeInfo = volumeInfoProvider.getVolumeInfo(entry);
    return importer.isEligibleVolume(volumeInfo);
  }
  return false;
};

/**
 * @param {!DirectoryEntry} directory Presumably the root of a filesystem.
 * @return {!Promise<!DirectoryEntry>} The found media directory (like 'DCIM').
 */
importer.getMediaDirectory = function(directory) {
  var dirNames = Object.keys(importer.ValidImportRoots_);
  return Promise.all(dirNames.map(importer.getDirectory_.bind(null, directory)))
      .then(
          /**
           * @param {!Array<!DirectoryEntry>} results
           * @return {!Promise<!DirectoryEntry>}
           */
          function(results) {
            for (var i = 0; i < results.length; i++) {
              if (!!results[i] && results[i].isDirectory)
                return Promise.resolve(results[i]);
            }
            // If standard (upper case) forms are not present,
            // check for a lower-case "DCIM".
            return importer.getDirectory_(directory, 'dcim')
                .then(function(directory) {
                  if (!!directory && directory.isDirectory) {
                    return Promise.resolve(directory);
                  } else {
                    return Promise.reject('Unable to local media directory.');
                  }
                });
          });
};

/**
 * @param {!DirectoryEntry} directory Presumably the root of a filesystem.
 * @return {!Promise<boolean>} True if the directory contains a
 *     child media directory (like 'DCIM').
 */
importer.hasMediaDirectory = function(directory) {
  return importer.getMediaDirectory(directory).then(
      function(result) {
        return Promise.resolve(!!result);
      },
      function() {
        return Promise.resolve(false);
      });
};

/**
 * @param {!DirectoryEntry} parent
 * @param {string} name
 * @return {!Promise<DirectoryEntry>}
 * @private
 */
importer.getDirectory_ = function(parent, name) {
  return new Promise(
    function(resolve, reject) {
      parent.getDirectory(
          name,
          {
            create: false,
            exclusive: false
          },
          resolve,
          function() { resolve(null); });
    });
};

/**
 * @return {!Promise<boolean>} Resolves with true when Cloud Import feature
 *     is enabled.
 */
importer.importEnabled = function() {
  return new Promise(
      function(resolve, reject) {
        chrome.commandLinePrivate.hasSwitch(
            'disable-cloud-import',
            /** @param {boolean} disabled */
            function(disabled) {
              resolve(!disabled);
            });
      });
};

/**
 * Handles a message from Pulsar...in which we presume we are being
 * informed of its "Automatically import stuff." state.
 *
 * While the runtime message system is loosey goosey about types,
 * we fully expect message to be a boolean value.
 *
 * @param {*} message
 *
 * @return {!Promise} Resolves once the message has been handled.
 */
importer.handlePhotosAppMessage = function(message) {
  if (typeof message !== 'boolean') {
    console.error(
        'Unrecognized message type received from photos app: ' + message);
    return Promise.reject();
  }

  var storage = importer.ChromeLocalStorage.getInstance();
  return storage.set(importer.Setting.PHOTOS_APP_ENABLED, message);
};

/**
 * @return {!Promise<boolean>} Resolves with true when Cloud Import feature
 *     is enabled.
 */
importer.isPhotosAppImportEnabled = function() {
  var storage = importer.ChromeLocalStorage.getInstance();
  return storage.get(importer.Setting.PHOTOS_APP_ENABLED, false);
};

/**
 * @param {!Date} date
 * @return {string} The current date, in YYYY-MM-DD format.
 */
importer.getDirectoryNameForDate = function(date) {
  var padAndConvert = function(i) {
    return (i < 10 ? '0' : '') + i.toString();
  };

  var year = date.getFullYear().toString();
  // Months are 0-based, but days aren't.
  var month = padAndConvert(date.getMonth() + 1);
  var day = padAndConvert(date.getDate());

  // NOTE: We use YYYY-MM-DD since it sorts numerically.
  // Ideally this would be localized and appropriate sorting would
  // be done behind the scenes.
  return year + '-' + month + '-' + day;
};

/**
 * @return {!Promise<number>} Resolves with an integer that is probably
 *     relatively unique to this machine (among a users machines).
 */
importer.getMachineId = function() {
  var storage = importer.ChromeLocalStorage.getInstance();
  return storage.get(importer.Setting.MACHINE_ID)
      .then(
          function(id) {
            if (id) {
              return id;
            }
            id = importer.generateId();
            return storage.set(importer.Setting.MACHINE_ID, id)
                .then(
                    function() {
                      return id;
                    });
          });
};

/**
 * @return {!Promise<string>} Resolves with the filename of this
 *     machines history file.
 */
importer.getHistoryFilename = function() {
  return importer.getMachineId().then(
      function(machineId) {
        return machineId + '-import-history.log';
      });
};

/**
 * @param {number} logId
 * @return {!Promise<string>} Resolves with the filename of this
 *     machines debug log file.
 */
importer.getDebugLogFilename = function(logId) {
  return importer.getMachineId().then(
      function(machineId) {
        return machineId + '-import-debug-' + logId + '.log';
      });
};

/**
 * @return {number} A relatively random six digit integer.
 */
importer.generateId = function() {
  return Math.floor(Math.random() * 899999) + 100000;
};

/**
 * @param {number} machineId The machine id for *this* machine. All returned
 *     files will have machine ids NOT matching this.
 * @return {!Promise<!FileEntry>} all history files not having
 *     a machine id matching {@code machineId}.
 * @private
 */
importer.getUnownedHistoryFiles_ = function(machineId) {
  var historyFiles = [];
  return importer.ChromeSyncFilesystem.getRoot()
      .then(
          /** @param {!DirectoryEntry} root */
          function(root) {
            return importer.listEntries_(
                root,
                /** @param {Entry} entry */
                function(entry) {
                  if (entry.isFile &&
                      entry.name.indexOf(machineId.toString()) === -1 &&
                      /^([0-9]{6}-import-history.log)$/.test(entry.name)) {
                    historyFiles.push(/** @type {!FileEntry} */ (entry));
                  }
                })
                .then(
                    function() {
                      return historyFiles;
                    });
          });
};

/**
 * Returns a sync file entry for this machine's history file.
 *
 * @return {!Promise<!FileEntry>}
 */
importer.getOrCreateHistoryFile = function() {
  return importer.ChromeSyncFilesystem.getOrCreateFileEntry(
      importer.getHistoryFilename());
};

/**
 * @return {!Promise<!Array<!FileEntry>>} Resolves with a list of
 *     history files with the first enty being the history file for
 *     the current (*this*) machine. List will always have at least one entry.
 */
importer.getHistoryFiles = function() {
  return Promise.all([
      importer.getOrCreateHistoryFile(),
      importer.getMachineId().then(importer.getUnownedHistoryFiles_)
    ]).then(
        /** @param {!Array<!FileEntry|!Array<!FileEntry>>} entries */
        function(entries) {
          var historyFiles = entries[1];
          historyFiles.unshift(entries[0]);
          return historyFiles;
        });
};

/**
 * Calls {@code callback} for each child entry of {@code directory}.
 *
 * @param {!DirectoryEntry} directory
 * @param {function(!Entry)} callback
 * @return {!Promise} Resolves when listing is complete.
 * @private
 */
importer.listEntries_ = function(directory, callback) {
  return new Promise(
      function(resolve, reject) {
        var reader = directory.createReader();

        var readEntries = function() {
          reader.readEntries (
              /** @param {!Array<!Entry>} entries */
              function(entries) {
                if (entries.length === 0) {
                  resolve(undefined);
                  return;
                }
                entries.forEach(callback);
                readEntries();
              },
              reject);
        };

        readEntries();
      });
};

/**
 * A Promise wrapper that provides public access to resolve and reject methods.
 *
 * @constructor
 * @struct
 * @template T
 */
importer.Resolver = function() {
  /** @private {boolean} */
  this.settled_ = false;

  /** @private {function(T=)} */
  this.resolve_;

  /** @private {function(*=)} */
  this.reject_;

  /** @private {!Promise<T>} */
  this.promise_ = new Promise(
      function(resolve, reject) {
        this.resolve_ = resolve;
        this.reject_ = reject;
      }.bind(this));

  var settler = function() {
    this.settled_ = true;
  }.bind(this);

  this.promise_.then(settler, settler);
};

importer.Resolver.prototype = /** @struct */ {
  /**
   * @return {function(T=)}
   * @template T
   */
  get resolve() {
    return this.resolve_;
  },
  /**
   * @return {function(*=)}
   * @template T
   */
  get reject() {
    return this.reject_;
  },
  /**
   * @return {!Promise<T>}
   * @template T
   */
  get promise() {
    return this.promise_;
  },
  /** @return {boolean} */
  get settled() {
    return this.settled_;
  }
};

/**
 * Returns the directory, creating it if necessary.
 *
 * @param {!DirectoryEntry} parent
 * @param {string} name
 *
 * @return {!Promise<!DirectoryEntry>}
 */
importer.demandChildDirectory = function(parent, name) {
  return new Promise(
      function(resolve, reject) {
        parent.getDirectory(
            name,
            {
              create: true,
              exclusive: false
            },
            resolve,
            reject);
      });
};

/**
 * A wrapper for FileEntry that provides Promises.
 *
 * @constructor
 * @struct
 *
 * @param {!FileEntry} fileEntry
 */
importer.PromisingFileEntry = function(fileEntry) {
  /** @private {!FileEntry} */
  this.fileEntry_ = fileEntry;
};

/**
 * Convenience method for creating new instances. Can, for example,
 * be passed to Array.map.
 *
 * @param {!FileEntry} entry
 * @return {!importer.PromisingFileEntry}
 */
importer.PromisingFileEntry.create = function(entry) {
  return new importer.PromisingFileEntry(entry);
};

/**
 * A "Promisary" wrapper around entry.getWriter.
 * @return {!Promise<!FileWriter>}
 */
importer.PromisingFileEntry.prototype.createWriter = function() {
  return new Promise(this.fileEntry_.createWriter.bind(this.fileEntry_));
};

/**
 * A "Promisary" wrapper around entry.file.
 * @return {!Promise<!File>}
 */
importer.PromisingFileEntry.prototype.file = function() {
  return new Promise(this.fileEntry_.file.bind(this.fileEntry_));
};

/**
 * @return {!Promise<!Object>}
 */
importer.PromisingFileEntry.prototype.getMetadata = function() {
  return new Promise(this.fileEntry_.getMetadata.bind(this.fileEntry_));
};

/**
 * This prefix is stripped from URL used in import history. It is stripped
 * to same on disk space, parsing time, and runtime memory.
 * @private @const {string}
 */
importer.APP_URL_PREFIX_ =
    'filesystem:chrome-extension://hhaomjibdihmijegdhdafkllkbggdgoj/external';

/**
 * Strips non-unique information from the URL. The resulting
 * value can be reconstituted using {@code importer.inflateAppUrl}.
 *
 * @param {string} url
 * @return {string}
 */
importer.deflateAppUrl = function(url) {
  if (url.substring(0, importer.APP_URL_PREFIX_.length) ===
      importer.APP_URL_PREFIX_) {
    return '$' + url.substring(importer.APP_URL_PREFIX_.length);
  }

  return url;
};

/**
 * Reconstitutes a url previous deflated by {@code deflateAppUrl}.
 * Returns the original string if it can't be inflated.
 *
 * @param {string} deflated
 * @return {string}
 */
importer.inflateAppUrl = function(deflated) {
  if (deflated.substring(0, 1) === '$') {
    return importer.APP_URL_PREFIX_ + deflated.substring(1);
  }
  return deflated;
};

/**
 * @param {string} date A date string in the form
 *     expected by Date.parse.
 * @return {string} The number of seconds from epoch to the date...as a string.
 */
importer.toSecondsFromEpoch = function(date) {
  // Since we're parsing a value that only has
  // precision to the second, our last three digits
  // will always be 000. We strip them and end up
  // with seconds.
  var milliseconds = String(Date.parse(date));
  return milliseconds.substring(0, milliseconds.length - 3);
};

/**
 * Namespace for ChromeSyncFilesystem related stuffs.
 */
importer.ChromeSyncFilesystem = {};

/**
 * Wraps chrome.syncFileSystem in a Promise.
 *
 * @return {!Promise<!FileSystem>}
 * @private
 */
importer.ChromeSyncFilesystem.getFileSystem_ = function() {
  return new Promise(
      function(resolve, reject) {
        chrome.syncFileSystem.requestFileSystem(
            /** @param {FileSystem} filesystem */
            function(filesystem) {
              if (chrome.runtime.lastError) {
                reject(chrome.runtime.lastError.message);
              } else {
                resolve(/** @type {!FileSystem} */ (filesystem));
              }
            });
      });
};

/**
 * Returns this apps ChromeSyncFilesystem root directory.
 *
 * @return {!Promise<!DirectoryEntry>}
 */
importer.ChromeSyncFilesystem.getRoot = function() {
  return new Promise(
      function(resolve, reject) {
        importer.ChromeSyncFilesystem.getFileSystem_()
            .then(
                /** @param {FileSystem} filesystem */
                function(filesystem) {
                  if (!filesystem.root) {
                    reject('Unable to access ChromeSyncFilesystem root');
                  }
                  resolve(
                    /** @type {!DirectoryEntry} */ (filesystem.root));
                });
      });
};

/**
 * Returns a sync file entry for the named file, creating it as needed.
 *
 * @param {!Promise<string>} fileNamePromise
 * @return {!Promise<!FileEntry>}
 */
importer.ChromeSyncFilesystem.getOrCreateFileEntry = function(fileNamePromise) {
  var promise = importer.ChromeSyncFilesystem.getRoot()
      .then(
          /**
           * @param {!DirectoryEntry} directory
           * @return {!Promise<!FileEntry>}
           */
          function(directory) {
            return fileNamePromise.then(
                /** @param {string} fileName */
                function(fileName) {
                  return new Promise(
                      function(resolve, reject) {
                        directory.getFile(
                            fileName,
                            {
                              create: true,
                              exclusive: false
                            },
                            resolve,
                            reject);
                      });
                });
          });

  return /** @type {!Promise<!FileEntry>} */ (promise);
};

/**
 * A basic logging mechanism.
 *
 * @interface
 */
importer.Logger = function() {};

/**
 * Writes an error message to the logger followed by a new line.
 *
 * @param {string} message
 */
importer.Logger.prototype.info;

/**
 * Writes an error message to the logger followed by a new line.
 *
 * @param {string} message
 */
importer.Logger.prototype.error;

/**
 * Returns a function suitable for use as an argument to
 * Promise#catch.
 *
 * @param {string} context
 */
importer.Logger.prototype.catcher;

/**
 * A {@code importer.Logger} that persists data in a {@code FileEntry}.
 *
 * @constructor
 * @implements {importer.Logger}
 * @struct
 * @final
 *
 * @param {!Promise<!FileEntry>} fileEntryPromise
 * @param {!Promise<!analytics.Tracker>} trackerPromise
 */
importer.RuntimeLogger = function(fileEntryPromise, trackerPromise) {

  /** @private {!Promise<!importer.PromisingFileEntry>} */
  this.fileEntryPromise_ = fileEntryPromise.then(
      /** @param {!FileEntry} fileEntry */
      function(fileEntry) {
        return new importer.PromisingFileEntry(fileEntry);
      });

  /** @private {!Promise<!analytics.Tracker>} */
  this.trackerPromise_ = trackerPromise;
};

/**
 * Reports an error to analytics.
 *
 * @param {string} context MUST NOT contain any dynamic error content,
 *    only statically defined string will dooooo.
 */
importer.RuntimeLogger.prototype.reportErrorContext_ = function(context) {
  this.trackerPromise_.then(
      /** @param {!analytics.Tracker} tracker */
      function(tracker) {
        tracker.sendException(
            context,
            false  /* fatal */ );
      });
};

/** @override  */
importer.RuntimeLogger.prototype.info = function(content) {
  this.write_('INFO', content);
  console.log(content);
};

/** @override  */
importer.RuntimeLogger.prototype.error = function(content) {
  this.write_('ERROR', content);
  console.error(content);
};

/** @override  */
importer.RuntimeLogger.prototype.catcher = function(context) {
  var prefix = '(' + context + ') ';

  return function(error) {
    this.reportErrorContext_(context);

    var message = prefix + 'Caught error in promise chain.';
    // Append error info, if provided, then output the error.
    if (error) {
      message += ' Error: ' + (error.message || error);
    }
    this.error(message);

    // Output a stack, if provided.
    if (error && error.stack) {
        this.write_('STACK', prefix + error.stack);
    }
  }.bind(this);
};

/**
 * Writes a message to the logger followed by a new line.
 *
 * @param {string} type
 * @param {string} message
 */
importer.RuntimeLogger.prototype.write_ = function(type, message) {
  // TODO(smckay): should we make an effort to reuse a file writer?
  return this.fileEntryPromise_
      .then(
          /** @param {!importer.PromisingFileEntry} fileEntry */
          function(fileEntry) {
            return fileEntry.createWriter();
          })
      .then(this.writeLine_.bind(this, type, message));
};

/**
 * Appends a new record to the end of the file.
 *
 * @param {string} type
 * @param {string} line
 * @param {!FileWriter} writer
 * @private
 */
importer.RuntimeLogger.prototype.writeLine_ = function(type, line, writer) {
  var blob = new Blob(
      ['[' + type + ' @ ' + new Date().toString() + '] ' + line + '\n'],
      {type: 'text/plain; charset=UTF-8'});
  return new Promise(
      /**
       * @param {function()} resolve
       * @param {function()} reject
       * @this {importer.RuntimeLogger}
       */
      (function(resolve, reject) {
        writer.onwriteend = resolve;
        writer.onerror = reject;

        writer.seek(writer.length);
        writer.write(blob);
      }).bind(this));
};

/** @private {importer.Logger} */
importer.logger_ = null;

/**
 * Creates a new logger instance...all ready to go.
 *
 * @return {!importer.Logger}
 */
importer.getLogger = function() {
  if (!importer.logger_) {

    var nextLogId = importer.getNextDebugLogId_();

    /** @return {!Promise} */
    var rotator = function() {
      return importer.rotateLogs(
          nextLogId,
          importer.ChromeSyncFilesystem.getOrCreateFileEntry);
    };

    // This is a sligtly odd arrangement in service of two goals.
    //
    // 1) Make a logger available synchronously.
    // 2) Nuke old log files before reusing their names.
    //
    // In support of these goals we push the "rotator" between
    // the call to load the file entry and the method that
    // produces the name of the file to load. That method
    // (getDebugLogFilename) returns promise. We exploit this.
    importer.logger_ = new importer.RuntimeLogger(
        importer.ChromeSyncFilesystem.getOrCreateFileEntry(
            /** @type {!Promise<string>} */ (rotator().then(
                importer.getDebugLogFilename.bind(null, nextLogId)))),
        importer.getTracker_());
  }

  return importer.logger_;
};

/**
 * Fetch analytics.Tracker from background page.
 * @return {!Promise<!analytics.Tracker>}
 * @private
 */
importer.getTracker_ = function() {
  return new Promise(
      function(resolve, reject) {
        chrome.runtime.getBackgroundPage(
          function(/** BackgroundWindow */ opt_background) {
            if (chrome.runtime.lastError) {
              reject(chrome.runtime.lastError);
            }
            opt_background.background.ready(
                function() {
                  resolve(opt_background.background.tracker);
                });
          });
      });
};

/**
 * Returns the log ID for the next debug log to use.
 * @private
 */
importer.getNextDebugLogId_ = function() {
  // Changes every other month.
  return new Date().getMonth() % 2;
};

/**
 * Deletes the "next" log file if it has just-now become active.
 *
 * Basically we toggle back and forth writing to two log files. At the time
 * we flip from one to another we want to delete the oldest data we have.
 * In this case it will be the "next" log.
 *
 * This function must be run before instantiating the logger.
 *
 * @param {number} nextLogId
 * @param {function(!Promise<string>): !Promise<!FileEntry>} fileFactory
 *     Injected primarily to facilitate testing.
 * @return {!Promise} Resolves when trimming is complete.
 */
importer.rotateLogs = function(nextLogId, fileFactory) {
  var storage = importer.ChromeLocalStorage.getInstance();

  /** @return {!Promise} */
  var rememberLogId = function() {
    return storage.set(
        importer.Setting.LAST_KNOWN_LOG_ID,
        nextLogId);
  };

  return storage.get(importer.Setting.LAST_KNOWN_LOG_ID)
      .then(
          /** @param {number} lastKnownLogId */
          function(lastKnownLogId) {
            if (nextLogId === lastKnownLogId ||
                lastKnownLogId === undefined) {
              return Promise.resolve();
            }

            return fileFactory(importer.getDebugLogFilename(nextLogId))
                .then(
                    /**
                     * @param {!FileEntry} entry
                     * @return {!Promise}
                     * @suppress {checkTypes}
                     */
                    function(entry) {
                      return new Promise(entry.remove.bind(entry));
                    });
          })
          .then(rememberLogId)
          .catch(rememberLogId);
};

/**
 * Friendly wrapper around chrome.storage.local.
 *
 * NOTE: If you want to use this in a test, install MockChromeStorageAPI.
 *
 * @constructor
 */
importer.ChromeLocalStorage = function() {};

/**
 * @param {string} key
 * @param {string|number|boolean} value
 * @return {!Promise} Resolves when operation is complete
 */
importer.ChromeLocalStorage.prototype.set = function(key, value) {
  return new Promise(
      function(resolve, reject) {
        var values = {};
        values[key] = value;
        chrome.storage.local.set(
            values,
            function() {
              if (chrome.runtime.lastError) {
                reject(chrome.runtime.lastError);
              } else {
                resolve(undefined);
              }
            });
      });
};

/**
 * @param {string} key
 * @param {T=} opt_default
 * @return {!Promise<T>} Resolves with the value, or {@code opt_default} when
 *     no value entry existis, or {@code undefined}.
 * @template T
 */
importer.ChromeLocalStorage.prototype.get = function(key, opt_default) {
  return new Promise(
      function(resolve, reject) {
        chrome.storage.local.get(
            key,
            /** @param {Object<?>} values */
            function(values) {
              if (chrome.runtime.lastError) {
                reject(chrome.runtime.lastError);
              } else if (key in values) {
                resolve(values[key]);
              } else {
                resolve(opt_default);
              }
            });
      });
};

/** @private @const {!importer.ChromeLocalStorage} */
importer.ChromeLocalStorage.INSTANCE_ = new importer.ChromeLocalStorage();

/** @return {!importer.ChromeLocalStorage} */
importer.ChromeLocalStorage.getInstance = function() {
  return importer.ChromeLocalStorage.INSTANCE_;
};
