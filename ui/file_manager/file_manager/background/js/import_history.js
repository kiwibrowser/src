// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Namespace
var importer = importer || {};

/** @enum {string} */
importer.ImportHistoryState = {
  'COPIED': 'copied',
  'IMPORTED': 'imported'
};

/**
 * An dummy {@code ImportHistory} implementation. This class can conveniently
 * be used when cloud import is disabled.
 * @param {boolean} answer The value to answer all {@code wasImported}
 *     queries with.
 *
 * @constructor
 * @implements {importer.HistoryLoader}
 * @implements {importer.ImportHistory}
 */
importer.DummyImportHistory = function(answer) {
  /** @private {boolean} */
  this.answer_ = answer;
};

/** @override */
importer.DummyImportHistory.prototype.whenReady = function() {
  return Promise.resolve(/** @type {!importer.ImportHistory} */ (this));
};

/** @override */
importer.DummyImportHistory.prototype.getHistory = function() {
  return this.whenReady();
};

/** @override */
importer.DummyImportHistory.prototype.wasCopied =
    function(entry, destination) {
  return Promise.resolve(this.answer_);
};

/** @override */
importer.DummyImportHistory.prototype.wasImported =
    function(entry, destination) {
  return Promise.resolve(this.answer_);
};

/** @override */
importer.DummyImportHistory.prototype.markCopied =
    function(entry, destination, destinationUrl) {
  return Promise.resolve();
};

/** @override */
importer.DummyImportHistory.prototype.listUnimportedUrls =
    function(destination) {
  return Promise.resolve([]);
};

/** @override */
importer.DummyImportHistory.prototype.markImported =
    function(entry, destination) {
  return Promise.resolve();
};

/** @override */
importer.DummyImportHistory.prototype.markImportedByUrl =
    function(destinationUrl) {
  return Promise.resolve();
};

/** @override */
importer.DummyImportHistory.prototype.addHistoryLoadedListener =
    function(listener) {};

/** @override */
importer.DummyImportHistory.prototype.addObserver = function(observer) {};

/** @override */
importer.DummyImportHistory.prototype.removeObserver = function(observer) {};

/**
 * @private @enum {number}
 */
importer.RecordType_ = {
  COPY: 0,
  IMPORT: 1
};

/**
 * @typedef {{
 *   sourceUrl: string,
 *   destinationUrl: string
 * }}
 */
importer.Urls;

/**
 * An {@code ImportHistory} implementation that reads from and
 * writes to a storage object.
 *
 * @constructor
 * @implements {importer.ImportHistory}
 * @struct
 *
 * @param {function(!FileEntry): !Promise<string>} hashGenerator
 * @param {!importer.RecordStorage} storage
 */
importer.PersistentImportHistory = function(hashGenerator, storage) {
  /** @private {function(!FileEntry): !Promise<string>} */
  this.createKey_ = hashGenerator;

  /** @private {!importer.RecordStorage} */
  this.storage_ = storage;

  /**
   * An in-memory representation of local copy history.
   * The first value is the "key" (as generated internally
   * from a file entry).
   * @private {!Object<!Object<!importer.Destination, importer.Urls>>}
   */
  this.copiedEntries_ = {};

  /**
   * An in-memory index from destination URL to key.
   *
   * @private {!Object<string>}
   */
  this.copyKeyIndex_ = {};

  /**
   * An in-memory representation of import history.
   * The first value is the "key" (as generated internally
   * from a file entry).
   * @private {!Object<!Array<importer.Destination>>}
   */
  this.importedEntries_ = {};

  /** @private {!Array<!importer.ImportHistory.Observer>} */
  this.observers_ = [];

  /** @private {Promise<!importer.PersistentImportHistory>} */
  this.whenReady_ = this.load_();
};

/**
 * Reloads history from disk. Should be called when the file
 * is changed by an external source.
 *
 * @return {!Promise<!importer.PersistentImportHistory>} Resolves when
 *     history has been refreshed.
 * @private
 */
importer.PersistentImportHistory.prototype.load_ = function() {
  return this.storage_.readAll(this.updateInMemoryRecord_.bind(this))
      .then(
          (/**
           * @return {!importer.PersistentImportHistory}
           * @this {importer.PersistentImportHistory}
           */
          function() {
            return this;
          }).bind(this))
      .catch(importer.getLogger().catcher('import-history-load'));
};

/**
 * @return {!Promise<!importer.ImportHistory>}
 */
importer.PersistentImportHistory.prototype.whenReady = function() {
  return /** @type {!Promise<!importer.ImportHistory>} */ (this.whenReady_);
};

/**
 * Detects record type and expands records to appropriate arguments.
 *
 * @param {!Array<*>} record
 * @this {importer.PersistentImportHistory}
 */
importer.PersistentImportHistory.prototype.updateInMemoryRecord_ =
    function(record) {
  switch (record[0]) {
    case importer.RecordType_.COPY:
      if (record.length !== 5) {
        importer.getLogger().error(
            'Skipping copy record with wrong number of fields: ' +
            record.length);
        break;
      }
      this.updateInMemoryCopyRecord_(
          /** @type {string} */ (
              record[1]),  // key
          /** @type {!importer.Destination} */ (
              record[2]),
          /** @type {string } */ (
              record[3]),  // sourceUrl
          /** @type {string } */ (
              record[4])); // destinationUrl
      return;
    case importer.RecordType_.IMPORT:
      if (record.length !== 3) {
        importer.getLogger().error(
            'Skipping import record with wrong number of fields: ' +
            record.length);
        break;
      }
      this.updateInMemoryImportRecord_(
          /** @type {string } */ (
              record[1]),  // key
          /** @type {!importer.Destination} */ (
              record[2]));
      return;
    default:
      assertNotReached('Ignoring record with unrecognized type: ' + record[0]);
  }
};

/**
 * Adds an import record to the in-memory history model.
 *
 * @param {string} key
 * @param {!importer.Destination} destination
 *
 * @return {boolean} True if a record was created.
 * @private
 */
importer.PersistentImportHistory.prototype.updateInMemoryImportRecord_ =
    function(key, destination) {
  if (!this.importedEntries_.hasOwnProperty(key)) {
    this.importedEntries_[key] = [destination];
    return true;
  } else if (this.importedEntries_[key].indexOf(destination) === -1) {
    this.importedEntries_[key].push(destination);
    return true;
  }
  return false;
};

/**
 * Adds a copy record to the in-memory history model.
 *
 * @param {string} key
 * @param {!importer.Destination} destination
 * @param {string} sourceUrl
 * @param {string} destinationUrl
 *
 * @return {boolean} True if a record was created.
 * @private
 */
importer.PersistentImportHistory.prototype.updateInMemoryCopyRecord_ =
    function(key, destination, sourceUrl, destinationUrl) {
  this.copyKeyIndex_[destinationUrl] = key;
  if (!this.copiedEntries_.hasOwnProperty(key)) {
    this.copiedEntries_[key] = {};
  }
  if (!this.copiedEntries_[key].hasOwnProperty(destination)) {
    this.copiedEntries_[key][destination] = {
      sourceUrl: sourceUrl,
      destinationUrl: destinationUrl
    };
    return true;
  }
  return false;
};

/** @override */
importer.PersistentImportHistory.prototype.wasCopied =
    function(entry, destination) {
  return this.whenReady_
      .then(this.createKey_.bind(this, entry))
      .then(
          (/**
           * @param {string} key
           * @return {boolean}
           * @this {importer.PersistentImportHistory}
           */
          function(key) {
            return key in this.copiedEntries_ &&
                destination in this.copiedEntries_[key];
          }).bind(this))
      .catch(importer.getLogger().catcher('import-history-was-imported'));
};

/** @override */
importer.PersistentImportHistory.prototype.wasImported =
    function(entry, destination) {
  return this.whenReady_
      .then(this.createKey_.bind(this, entry))
      .then(
          (/**
           * @param {string} key
           * @return {boolean}
           * @this {importer.PersistentImportHistory}
           */
          function(key) {
            return this.getDestinations_(key).indexOf(destination) >= 0;
          }).bind(this))
      .catch(importer.getLogger().catcher('import-history-was-imported'));
};

/** @override */
importer.PersistentImportHistory.prototype.markCopied = function(
    entry, destination, destinationUrl) {
  return this.whenReady_.then(this.createKey_.bind(this, entry))
      .then(
          (/**
           * @param {string} key
           * @return {!Promise<?>}
           * @this {importer.ImportHistory}
           */
          function(key) {
            return this.storeRecord_([
                importer.RecordType_.COPY,
                key,
                destination,
                importer.deflateAppUrl(entry.toURL()),
                importer.deflateAppUrl(destinationUrl)]);
          }).bind(this))
      .then(this.notifyObservers_.bind(
          this, importer.ImportHistoryState.COPIED, entry, destination,
          destinationUrl))
      .catch(importer.getLogger().catcher('import-history-mark-copied'));
};

/** @override */
importer.PersistentImportHistory.prototype.listUnimportedUrls =
    function(destination) {
  return this.whenReady_.then(
      function() {
        // TODO(smckay): Merge copy and sync records for simpler
        // unimported file discovery.
        var unimported = [];
        for (var key in this.copiedEntries_) {
          var imported = this.importedEntries_[key];
          for (var destination in this.copiedEntries_[key]) {
            if (!imported || imported.indexOf(destination) === -1) {
              var url = importer.inflateAppUrl(
                  this.copiedEntries_[key][destination].destinationUrl);
              unimported.push(url);
            }
          }
        }
        return unimported;
      }.bind(this))
      .catch(
          importer.getLogger().catcher('import-history-list-unimported-urls'));
};

/** @override */
importer.PersistentImportHistory.prototype.markImported = function(
    entry, destination) {
  return this.whenReady_.then(this.createKey_.bind(this, entry))
      .then(
          (/**
           * @param {string} key
           * @return {!Promise<?>}
           * @this {importer.ImportHistory}
           */
          function(key) {
            return this.storeRecord_([
                importer.RecordType_.IMPORT,
                key,
                destination]);
          }).bind(this))
      .then(this.notifyObservers_.bind(
          this, importer.ImportHistoryState.IMPORTED, entry, destination))
      .catch(importer.getLogger().catcher('import-history-mark-imported'));
};

/** @override */
importer.PersistentImportHistory.prototype.markImportedByUrl =
    function(destinationUrl) {
  var deflatedUrl = importer.deflateAppUrl(destinationUrl);
  var key = this.copyKeyIndex_[deflatedUrl];
  if (!!key) {
    var copyData = this.copiedEntries_[key];

    // We could build an index of this as well, but it seems
    // unnecessary given the fact that there will almost always
    // be just one destination for a file (assumption).
    for (var destination in copyData) {
      if (copyData[destination].destinationUrl === deflatedUrl) {
        return this.storeRecord_([
          importer.RecordType_.IMPORT,
          key,
          destination])
            .then(
                (/** @this {importer.PersistentImportHistory} */
                function() {
                  var sourceUrl = importer.inflateAppUrl(
                      copyData[destination].sourceUrl);
                  // Here we try to create an Entry for the source URL.
                  // This will allow observers to update the UI if the
                  // source entry is in view.
                  util.urlToEntry(sourceUrl).then(
                      (/**
                       * @param {Entry} entry
                       * @this {importer.PersistentImportHistory}
                       */
                      function(entry) {
                        if (entry.isFile) {
                          this.notifyObservers_(
                              importer.ImportHistoryState.IMPORTED,
                              /** @type {!FileEntry} */ (entry), destination);
                        }
                      }).bind(this),
                      function() {
                        console.log(
                            'Unable to find original entry for: ' + sourceUrl);
                        return;
                      })
                      .catch(importer.getLogger().catcher(
                          'notify-listeners-on-import'));
                }).bind(this))
            .catch(importer.getLogger().catcher('mark-imported-by-url'));
      }
    }
  }

  return Promise.reject(
      'Unable to match destination URL to import record > ' + destinationUrl);
};

/** @override */
importer.PersistentImportHistory.prototype.addObserver =
    function(observer) {
  this.observers_.push(observer);
};

/** @override */
importer.PersistentImportHistory.prototype.removeObserver =
    function(observer) {
  var index = this.observers_.indexOf(observer);
  if (index > -1) {
    this.observers_.splice(index, 1);
  } else {
    console.warn('Ignoring request to remove observer that is not registered.');
  }
};

/**
 * @param {!importer.ImportHistoryState} state
 * @param {!FileEntry} entry
 * @param {!importer.Destination} destination
 * @param {string=} opt_destinationUrl
 * @private
 */
importer.PersistentImportHistory.prototype.notifyObservers_ =
    function(state, entry, destination, opt_destinationUrl) {
  this.observers_.forEach(
      (/**
       * @param {!importer.ImportHistory.Observer} observer
       * @this {importer.PersistentImportHistory}
       */
      function(observer) {
        observer({
          state: state,
          entry: entry,
          destination: destination,
          destinationUrl: opt_destinationUrl
        });
      }).bind(this));
};

/**
 * @param {!Array<*>} record
 *
 * @return {!Promise<?>} Resolves once the write has been completed.
 * @private
 */
importer.PersistentImportHistory.prototype.storeRecord_ = function(record) {
  this.updateInMemoryRecord_(record);
  return this.storage_.write(record);
};

/**
 * @param {string} key
 * @return {!Array<string>} The list of previously noted
 *     destinations, or an empty array, if none.
 * @private
 */
importer.PersistentImportHistory.prototype.getDestinations_ = function(key) {
  return key in this.importedEntries_ ? this.importedEntries_[key] : [];
};

/**
 * Class responsible for lazy loading of {@code importer.ImportHistory},
 * and reloading when the underlying data is updated (via sync).
 *
 * @constructor
 * @implements {importer.HistoryLoader}
 * @struct
 *
 * @param {function(): !Promise<!Array<!FileEntry>>} filesProvider
 * @param {!analytics.Tracker} tracker
 */
importer.SynchronizedHistoryLoader = function(filesProvider, tracker) {
  /**
   * @return {!Promise<!Array<!FileEntry>>} History files. Will always
   *     have at least one file (the "primary file"). When other devices
   *     have been used for import, additional files may be present
   *     as well. In all cases the primary file will be used for write
   *     operations and all non-primary files are read-only.
   * @private
   */
  this.getHistoryFiles_ = filesProvider;

  /** @private {!analytics.Tracker} */
  this.tracker_ = tracker;

  /** @private {boolean} */
  this.needsInitialization_ = true;

  /** @private {!importer.Resolver} */
  this.historyResolver_ = new importer.Resolver();
};

/** @override */
importer.SynchronizedHistoryLoader.prototype.getHistory = function() {
  if (this.needsInitialization_) {
    this.needsInitialization_ = false;
    this.getHistoryFiles_()
        .then(
            (/**
             * @param {!Array<!FileEntry>} fileEntries
             * @this {importer.SynchronizedHistoryLoader}
             */
            function(fileEntries) {
              var storage = new importer.FileBasedRecordStorage(
                  fileEntries,
                  this.tracker_);
              var history = new importer.PersistentImportHistory(
                  importer.createMetadataHashcode,
                  storage);
              new importer.DriveSyncWatcher(history);
              history.whenReady().then(
                  (/** @this {importer.SynchronizedHistoryLoader} */
                  function() {
                    this.historyResolver_.resolve(history);
                  }).bind(this));
            }).bind(this))
        .catch(importer.getLogger().catcher('history-load-chain'));
  }

  return this.historyResolver_.promise;
};

/** @override */
importer.SynchronizedHistoryLoader.prototype.addHistoryLoadedListener =
    function(listener) {
  this.historyResolver_.promise.then(listener);
};

/**
 * An simple record storage mechanism.
 *
 * @interface
 */
importer.RecordStorage = function() {};

/**
 * Adds a new record.
 *
 * @param {!Array<*>} record
 * @return {!Promise<?>} Resolves when record is added.
 */
importer.RecordStorage.prototype.write;

/**
 * Reads all records.
 *
 * @param {function(!Array<*>)} recordCallback Callback called once
 *     for each record loaded.
 */
importer.RecordStorage.prototype.readAll;

/**
 * A {@code RecordStore} that persists data in a {@code FileEntry}.
 *
 * @param {!Array<!FileEntry>} fileEntries The first entry is the
 *     "primary" file for read-write, all other are read-only
 *     sources of data (presumably synced from other machines).
 *
 * @constructor
 * @implements {importer.RecordStorage}
 * @struct
 *
 * @param {!analytics.Tracker} tracker
 */
importer.FileBasedRecordStorage = function(fileEntries, tracker) {
  /** @private {!Array<!importer.PromisingFileEntry>} */
  this.inputFiles_ = fileEntries.map(
      importer.PromisingFileEntry.create);

  /** @private {!importer.PromisingFileEntry} */
  this.outputFile_ = this.inputFiles_[0];

  /** @private {!analytics.Tracker} */
  this.tracker_ = tracker;

  /**
   * Serializes all writes and reads on the primary file.
   * @private {!Promise<?>}
   * */
  this.latestOperation_ = Promise.resolve(null);
};

/** @override */
importer.FileBasedRecordStorage.prototype.write = function(record) {
  return this.latestOperation_ = this.latestOperation_
      .then(
          (/**
           * @param {?} ignore
           * @this {importer.FileBasedRecordStorage}
           */
          function(ignore) {
            return this.outputFile_.createWriter();
          }).bind(this))
      .then(this.writeRecord_.bind(this, record))
      .catch(importer.getLogger().catcher('file-record-store-write'));
};

/**
 * Appends a new record to the end of the file.
 *
 * @param {!Object} record
 * @param {!FileWriter} writer
 * @return {!Promise<?>} Resolves when write is complete.
 * @private
 */
importer.FileBasedRecordStorage.prototype.writeRecord_ =
    function(record, writer) {
  var blob = new Blob(
      [JSON.stringify(record) + ',\n'],
      {type: 'text/plain; charset=UTF-8'});

  return new Promise(
      (/**
       * @param {function()} resolve
       * @param {function()} reject
       * @this {importer.FileBasedRecordStorage}
       */
      function(resolve, reject) {
        writer.onwriteend = resolve;
        writer.onerror = reject;

        writer.seek(writer.length);
        writer.write(blob);
      }).bind(this));
};

/** @override */
importer.FileBasedRecordStorage.prototype.readAll = function(recordCallback) {
  var processTiming = this.tracker_.startTiming(
      metrics.Categories.ACQUISITION,
      metrics.timing.Variables.HISTORY_LOAD);

  return this.latestOperation_ = this.latestOperation_
      .then(
          (/**
           * @param {?} ignored
           * @this {importer.FileBasedRecordStorage}
           */
          function(ignored) {
            var filePromises = this.inputFiles_.map(
                /**
                 * @param {!importer.PromisingFileEntry} entry
                 * @this {importer.FileBasedRecordStorage}
                 */
                function(entry) {
                  return entry.file();
                });
            return Promise.all(filePromises);
          }).bind(this))
      .then(
          (/**
           * @return {!Promise<!Array<string>>}
           * @this {importer.FileBasedRecordStorage}
           */
          function(files) {
            var contentPromises = files.map(
                this.readFileAsText_.bind(this));
            return Promise.all(contentPromises);
          }).bind(this),
          (/**
           * @return {string}
           * @this {importer.FileBasedRecordStorage}
           */
          function() {
            console.error('Unable to read from one of history files.');
            return '';
          }).bind(this))
      .then(
          (/**
           * @param {!Array<string>} fileContents
           * @this {importer.FileBasedRecordStorage}
           */
          function(fileContents) {
            var parsePromises = fileContents.map(
                this.parse_.bind(this));
            return Promise.all(parsePromises);
          }).bind(this))
      .then(
          (/** @param {!Array<!Array<*>>} parsedContents */
          function(parsedContents) {
            parsedContents.forEach(
                /** @param {!Array<!Array<*>>} recordSet */
                function(recordSet) {
                  recordSet.forEach(recordCallback);
                });

            processTiming.send();

            var fileCount = this.inputFiles_.length;
            this.tracker_.send(
                metrics.ImportEvents.HISTORY_LOADED
                    .value(fileCount)
                    .dimension(fileCount === 1
                        ? metrics.Dimensions.MACHINE_USE_SINGLE
                        : metrics.Dimensions.MACHINE_USE_MULTIPLE));
          }).bind(this))
      .catch(importer.getLogger().catcher('file-record-store-read-all'));
};

/**
 * Reads the entire entry as a single string value.
 *
 * @param {!File} file
 * @return {!Promise<string>}
 * @private
 */
importer.FileBasedRecordStorage.prototype.readFileAsText_ = function(file) {
  return new Promise(
      function(resolve, reject) {
        var reader = new FileReader();

        reader.onloadend = function() {
          if (reader.error) {
            console.error(reader.error);
            reject();
          } else {
            resolve(reader.result);
          }
        }.bind(this);

        reader.onerror = function(error) {
          console.error(error);
          reject(error);
        }.bind(this);

        reader.readAsText(file);
      }.bind(this))
      .catch(importer.getLogger().catcher(
      'file-record-store-read-file-as-text'));
};

/**
 * Parses the text.
 *
 * @param {string} text
 * @return {!Array<!Array<*>>}
 * @private
 */
importer.FileBasedRecordStorage.prototype.parse_ = function(text) {
  if (text.length === 0) {
    return [];
  } else {
    // Dress up the contents of the file like an array,
    // so the JSON object can parse it using JSON.parse.
    // That means we need to both:
    //   1) Strip the trailing ',\n' from the last record
    //   2) Surround the whole string in brackets.
    // NOTE: JSON.parse is WAY faster than parsing this
    // ourselves in javascript.
    var json = '[' + text.substring(0, text.length - 2) + ']';
    return /** @type {!Array<!Array<*>>} */ (JSON.parse(json));
  }
};

/**
 * This class makes the "drive" badges appear by way of marking entries as
 * imported in history when a previously imported file is fully synced to drive.
 *
 * @constructor
 * @struct
 *
 * @param {!importer.ImportHistory} history
 */
importer.DriveSyncWatcher = function(history) {
  /** @private {!importer.ImportHistory} */
  this.history_ = history;

  this.history_.addObserver(
      this.onHistoryChanged_.bind(this));

  this.history_.whenReady()
      .then(
          function() {
            this.history_.listUnimportedUrls(importer.Destination.GOOGLE_DRIVE)
                .then(this.updateSyncStatus_.bind(
                    this,
                    importer.Destination.GOOGLE_DRIVE));
          }.bind(this))
      .catch(importer.getLogger().catcher('drive-sync-watcher-constructor'));

  // Listener is only registered once the history object is initialized.
  // No need to register synchonously since we don't want to be
  // woken up to respond to events.
  chrome.fileManagerPrivate.onFileTransfersUpdated.addListener(
      this.onFileTransfersUpdated_.bind(this));
  // TODO(smckay): Listen also for errors on onDriveSyncError.
};

/** @const {number} */
importer.DriveSyncWatcher.UPDATE_DELAY_MS = 3500;

/**
 * @param {!importer.Destination} destination
 * @param {!Array<string>} unimportedUrls
 * @private
 */
importer.DriveSyncWatcher.prototype.updateSyncStatus_ =
    function(destination, unimportedUrls) {
  // TODO(smckay): Chunk processing of urls...to ensure we're not
  // blocking interactive tasks. For now, we just defer the update
  // for a few seconds.
  setTimeout(
      function() {
        unimportedUrls.forEach(
            function(url) {
              this.checkSyncStatus_(destination, url);
            }.bind(this));
      }.bind(this),
      importer.DriveSyncWatcher.UPDATE_DELAY_MS);
};

/**
 * @param {!FileTransferStatus} status
 * @private
 */
importer.DriveSyncWatcher.prototype.onFileTransfersUpdated_ =
    function(status) {
  // If the synced file it isn't one we copied,
  // the call to mark by url will just fail...fine by us.
  if (status.transferState === 'completed') {
    this.history_.markImportedByUrl(status.fileUrl);
  }
};

/**
 * @param {!importer.ImportHistory.ChangedEvent} event
 * @private
 */
importer.DriveSyncWatcher.prototype.onHistoryChanged_ = function(event) {
  if (event.state === importer.ImportHistoryState.COPIED) {
    // Check sync status incase the file synced *before* it was able
    // to mark be marked as copied.
    this.checkSyncStatus_(
        event.destination,
        /**@type {string}*/ (event.destinationUrl), event.entry);
  }
};

/**
 * @param {!importer.Destination} destination
 * @param {string} url
 * @param {!FileEntry=} opt_entry Pass this if you have an entry
 *     on hand, else, we'll jump through some extra hoops to
 *     make do without it.
 * @private
 */
importer.DriveSyncWatcher.prototype.checkSyncStatus_ =
    function(destination, url, opt_entry) {
  console.assert(
      destination === importer.Destination.GOOGLE_DRIVE,
      'Unsupported destination: ' + destination);

  this.getSyncStatus_(url)
      .then(
          (/**
           * @param {boolean} synced True if file is synced
           * @this {importer.DriveSyncWatcher}
           */
          function(synced) {
            if (synced) {
              if (opt_entry) {
                this.history_.markImported(opt_entry, destination);
              } else {
                this.history_.markImportedByUrl(url);
              }
            }
          }).bind(this))
      .catch(
          importer.getLogger().catcher(
              'drive-sync-watcher-check-sync-status'));
};

/**
 * @param {string} url
 * @return {!Promise<boolean>} Resolves with true if the
 *     file has been synced to the named destination.
 * @private
 */
importer.DriveSyncWatcher.prototype.getSyncStatus_ = function(url) {
  return util.URLsToEntries([url])
    .then(
        function(results) {
          if (results.entries.length !== 1)
            return Promise.reject();
          return new Promise(
              (/** @this {importer.DriveSyncWatcher} */
              function(resolve, reject) {
                // TODO(smckay): User Metadata Cache...once it is available
                // in the background.
                chrome.fileManagerPrivate.getEntryProperties(
                    [results.entries[0]],
                    ['dirty'],
                    (/**
                     * @param {!Array<!EntryProperties>|undefined}
                     *     propertiesList
                     * @this {importer.DriveSyncWatcher}
                     */
                    function(propertiesList) {
                      console.assert(
                          propertiesList.length === 1,
                          'Got an unexpected number of results.');
                      if (chrome.runtime.lastError) {
                        reject(chrome.runtime.lastError);
                      } else {
                        var data = propertiesList[0];
                        resolve(!data['dirty']);
                      }
                    }).bind(this));
              }).bind(this));
        })
    .catch(importer.getLogger().catcher('drive-sync-watcher-get-sync-status'));
};

/**
 * History loader that provides an ImportHistorty appropriate
 * to user settings (if import history is enabled/disabled).
 *
 * TODO(smckay): Use SynchronizedHistoryLoader directly
 *     once cloud-import feature is enabled by default.
 *
 * @constructor
 * @implements {importer.HistoryLoader}
 * @struct
 *
 * @param {!analytics.Tracker} tracker
 */
importer.RuntimeHistoryLoader = function(tracker) {

  /** @return {!importer.HistoryLoader} */
  this.createRealHistoryLoader_ = function() {
    return new importer.SynchronizedHistoryLoader(
        importer.getHistoryFiles,
        tracker);
  };

  /** @private {boolean} */
  this.needsInitialization_ = true;

  /** @private {!importer.Resolver.<!importer.ImportHistory>} */
  this.historyResolver_ = new importer.Resolver();
};

/** @override */
importer.RuntimeHistoryLoader.prototype.getHistory = function() {
  if (this.needsInitialization_) {
    this.needsInitialization_ = false;
    importer.importEnabled()
        .then(
            (/**
             * @param {boolean} enabled
             * @return {!importer.HistoryLoader}
             * @this {importer.RuntimeHistoryLoader}
             */
            function(enabled) {
              return enabled ?
                  this.createRealHistoryLoader_() :
                  new importer.DummyImportHistory(false);
            }).bind(this))
        .then(
            function(loader) {
              return this.historyResolver_.resolve(loader.getHistory());
            }.bind(this))
        .catch(
            importer.getLogger().catcher(
                'runtime-history-loader-get-history'));
  }

  return this.historyResolver_.promise;
};

/** @override */
importer.RuntimeHistoryLoader.prototype.addHistoryLoadedListener =
    function(listener) {
  this.historyResolver_.promise.then(listener);
};

/**
 * @param {!FileEntry} fileEntry
 * @return {!Promise<string>} Resolves with a "hashcode" consisting of
 *     just the last modified time and the file size.
 */
importer.createMetadataHashcode = function(fileEntry) {
  return new Promise(
             function(resolve, reject) {
               metadataProxy.getEntryMetadata(fileEntry).then(
                   (/**
                    * @param {!Object} metadata
                    * @this {importer.PersistentImportHistory}
                    */
                   function(metadata) {
                     if (!('modificationTime' in metadata)) {
                       reject('File entry missing "modificationTime" field.');
                     } else if (!('size' in metadata)) {
                       reject('File entry missing "size" field.');
                     } else {
                       var secondsSinceEpoch = importer.toSecondsFromEpoch(
                           metadata.modificationTime);
                       resolve(secondsSinceEpoch + '_' + metadata.size);
                     }
                   }).bind(this));
             }.bind(this))
      .catch(importer.getLogger().catcher('importer-common-create-hashcode'));
};
