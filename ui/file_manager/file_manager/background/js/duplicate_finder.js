// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Namespace
var importer = importer || {};

/**
 * A duplicate finder for Google Drive.
 *
 * @constructor
 * @struct
 *
 * @param {!analytics.Tracker} tracker
 */
importer.DriveDuplicateFinder = function(tracker) {

  /** @private {!analytics.Tracker} */
  this.tracker_ = tracker;

  /** @private {Promise<string>} */
  this.driveIdPromise_ = null;

  /**
   * An bounded cache of most recently calculated file content hashcodes.
   * @private {!LRUCache<!Promise<string>>}
   */
  this.hashCache_ = new LRUCache(
      importer.DriveDuplicateFinder.MAX_CACHED_HASHCODES_);
};

/**
 * @param {!FileEntry} entry
 * @return {!Promise<boolean>}
 */
importer.DriveDuplicateFinder.prototype.isDuplicate = function(entry) {
  return this.computeHash_(entry)
      .then(this.findByHash_.bind(this))
      .then(
          /**
           * @param {!Array<string>} urls
           * @return {boolean}
           */
          function(urls) {
            return urls.length > 0;
          });
};

/** @private @const {number} */
importer.DriveDuplicateFinder.HASH_EVENT_THRESHOLD_ = 5000;

/** @private @const {number} */
importer.DriveDuplicateFinder.SEARCH_EVENT_THRESHOLD_ = 1000;

/** @private @const {number} */
importer.DriveDuplicateFinder.MAX_CACHED_HASHCODES_ = 10000;

/**
 * Computes the content hash for the given file entry.
 * @param {!FileEntry} entry
 * @return {!Promise<string>} The computed hash.
 * @private
 */
importer.DriveDuplicateFinder.prototype.computeHash_ = function(entry) {
  return importer.createMetadataHashcode(entry).then(function(hashcode) {
    // Cache key is the concatination of metadata hashcode and URL.
    var cacheKey = hashcode + '|' + entry.toURL();
    if (this.hashCache_.hasKey(cacheKey)) {
      return this.hashCache_.get(cacheKey);
    }

    var hashPromise = new Promise(
        (/** @this {importer.DriveDuplicateFinder} */
        function(resolve, reject) {
          var startTime = new Date().getTime();
          chrome.fileManagerPrivate.computeChecksum(
              entry,
              (/**
               * @param {string|undefined} result The content hash.
               * @this {importer.DriveDuplicateFinder}
               */
              function(result) {
                var elapsedTime = new Date().getTime() - startTime;
                // Send the timing to GA only if it is sorta exceptionally long.
                // A one second, CPU intensive operation, is pretty long.
                if (elapsedTime >=
                    importer.DriveDuplicateFinder.HASH_EVENT_THRESHOLD_) {
                  console.info(
                      'Content hash computation took ' + elapsedTime + ' ms.');
                  this.tracker_.sendTiming(
                     metrics.Categories.ACQUISITION,
                     metrics.timing.Variables.COMPUTE_HASH,
                     elapsedTime);
                }
                if (chrome.runtime.lastError) {
                  reject(chrome.runtime.lastError);
                } else {
                  resolve(result);
                }
              }).bind(this));
        }).bind(this));

    this.hashCache_.put(cacheKey, hashPromise);
    return hashPromise;
  }.bind(this));
};

/**
 * Finds files with content hashes matching the given hash.
 * @param {string} hash The content hash of the file to find.
 * @return {!Promise<!Array<string>>} The URLs of the found files.  If there are
 *     no matches, the list will be empty.
 * @private
 */
importer.DriveDuplicateFinder.prototype.findByHash_ = function(hash) {
  return /** @type {!Promise<!Array<string>>} */ (
      this.getDriveId_()
          .then(this.searchFilesByHash_.bind(this, hash)));
};

/**
 * @return {!Promise<string>} ID of the user's Drive volume.
 * @private
 */
importer.DriveDuplicateFinder.prototype.getDriveId_ = function() {
  if (!this.driveIdPromise_) {
    this.driveIdPromise_ = volumeManagerFactory.getInstance()
        .then(
            /**
             * @param {!VolumeManager} volumeManager
             * @return {string} ID of the user's Drive volume.
             */
            function(volumeManager) {
              return volumeManager.getCurrentProfileVolumeInfo(
                  VolumeManagerCommon.VolumeType.DRIVE).volumeId;
            });
  }
  return this.driveIdPromise_;
};

/**
 * A promise-based wrapper for chrome.fileManagerPrivate.searchFilesByHashes.
 * @param {string} hash The content hash to search for.
 * @param {string} volumeId The volume to search.
 * @return {!Promise<!Array<string>>} A list of file URLs.
 * @private
 */
importer.DriveDuplicateFinder.prototype.searchFilesByHash_ =
    function(hash, volumeId) {
  return new Promise(
      (/** @this {importer.DriveDuplicateFinder} */
      function(resolve, reject) {
        var startTime = new Date().getTime();
        chrome.fileManagerPrivate.searchFilesByHashes(
            volumeId,
            [hash],
            (/**
             * @param {!Object<string, !Array<string>>|undefined} urls
             * @this {importer.DriveDuplicateFinder}
             */
            function(urls) {
              var elapsedTime = new Date().getTime() - startTime;
              // Send the timing to GA only if it is sorta exceptionally long.
              if (elapsedTime >=
                  importer.DriveDuplicateFinder.SEARCH_EVENT_THRESHOLD_) {
                this.tracker_.sendTiming(
                   metrics.Categories.ACQUISITION,
                   metrics.timing.Variables.SEARCH_BY_HASH,
                   elapsedTime);
              }
              if (chrome.runtime.lastError) {
                reject(chrome.runtime.lastError);
              } else {
                resolve(urls[hash]);
              }
            }).bind(this));
      }).bind(this));
};

/**
 * A class that aggregates history/content-dupe checking
 * into a single "Disposition" value. Should now be the
 * primary source for duplicate checking (with the exception
 * of in-scan deduplication, where duplicate results that
 * are within the scan are ignored).
 *
 * @constructor
 *
 * @param {!importer.HistoryLoader} historyLoader
 * @param {!importer.DriveDuplicateFinder} contentMatcher
 */
importer.DispositionChecker = function(historyLoader, contentMatcher) {
  /** @private {!importer.HistoryLoader} */
  this.historyLoader_ = historyLoader;

  /** @private {!importer.DriveDuplicateFinder} */
  this.contentMatcher_ = contentMatcher;
};

/**
 * Type for a function to return content disposition of an entry.
 *
 * @typedef {function(!FileEntry, !importer.Destination,
 *                   !importer.ScanMode):
 *     !Promise<!importer.Disposition>}
 */
importer.DispositionChecker.CheckerFunction;

/**
 * @type {!importer.DispositionChecker.CheckerFunction}
 */
importer.DispositionChecker.prototype.getDisposition =
    function(entry, destination, mode) {
  if (destination !== importer.Destination.GOOGLE_DRIVE) {
    return Promise.reject('Unsupported destination: ' + destination);
  }

  return new Promise(
      (/** @this {importer.DispositionChecker} */
      function(resolve, reject) {
        this.hasHistoryDuplicate_(entry, destination)
            .then(
                (/**
                 * @param {boolean} duplicate
                 * @this {importer.DispositionChecker}
                 */
                function(duplicate) {
                  if (duplicate) {
                    resolve(importer.Disposition.HISTORY_DUPLICATE);
                    return;
                  }
                  if (mode == importer.ScanMode.HISTORY) {
                    resolve(importer.Disposition.ORIGINAL);
                    return;
                  }
                  this.contentMatcher_.isDuplicate(entry)
                      .then(
                          /** @param {boolean} duplicate */
                          function(duplicate) {
                            if (duplicate) {
                              resolve(
                                  importer.Disposition.CONTENT_DUPLICATE);
                            } else {
                              resolve(importer.Disposition.ORIGINAL);
                            }
                          });
                }).bind(this));
            }).bind(this));
};

/**
 * @param {!FileEntry} entry
 * @param {!importer.Destination} destination
 * @return {!Promise<boolean>} True if there is a history-entry-duplicate
 *     for the file.
 * @private
 */
importer.DispositionChecker.prototype.hasHistoryDuplicate_ =
    function(entry, destination) {
  return this.historyLoader_.getHistory()
      .then(
          (/**
           * @param {!importer.ImportHistory} history
           * @return {!Promise}
           * @this {importer.DispositionChecker}
           */
          function(history) {
            return Promise.all([
              history.wasCopied(entry, destination),
              history.wasImported(entry, destination)
            ]).then(
                /**
                 * @param {!Array<boolean>} results
                 * @return {boolean}
                 */
                function(results) {
                  return results[0] || results[1];
                });
          }).bind(this));
};

/**
 * Factory for a function that returns an entry's disposition.
 *
 * @param {!importer.HistoryLoader} historyLoader
 * @param {!analytics.Tracker} tracker
 *
 * @return {!importer.DispositionChecker.CheckerFunction}
 */
importer.DispositionChecker.createChecker =
    function(historyLoader, tracker) {
  var checker = new importer.DispositionChecker(
      historyLoader,
      new importer.DriveDuplicateFinder(tracker));
  return checker.getDisposition.bind(checker);
};
