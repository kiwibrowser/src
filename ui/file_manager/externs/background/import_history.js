// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Namespace
var importer;

/**
 * A persistent data store for Cloud Import history information.
 *
 * @interface
 */
importer.ImportHistory = function() {};

/**
 * @return {!Promise<!importer.ImportHistory>} Resolves when history
 *     has been fully loaded.
 */
importer.ImportHistory.prototype.whenReady;

/**
 * @param {!FileEntry} entry
 * @param {!importer.Destination} destination
 * @return {!Promise<boolean>} Resolves with true if the FileEntry
 *     was previously copied to the device.
 */
importer.ImportHistory.prototype.wasCopied;

/**
 * @param {!FileEntry} entry
 * @param {!importer.Destination} destination
 * @return {!Promise<boolean>} Resolves with true if the FileEntry
 *     was previously imported to the specified destination.
 */
importer.ImportHistory.prototype.wasImported;

/**
 * @param {!FileEntry} entry
 * @param {!importer.Destination} destination
 * @param {string} destinationUrl
 */
importer.ImportHistory.prototype.markCopied;

/**
 * List urls of all files that are marked as copied, but not marked as synced.
 * @param {!importer.Destination} destination
 * @return {!Promise<!Array<string>>}
 */
importer.ImportHistory.prototype.listUnimportedUrls;

/**
 * @param {!FileEntry} entry
 * @param {!importer.Destination} destination
 * @return {!Promise<?>} Resolves when the operation is completed.
 */
importer.ImportHistory.prototype.markImported;

/**
 * @param {string} destinationUrl
 * @return {!Promise<?>} Resolves when the operation is completed.
 */
importer.ImportHistory.prototype.markImportedByUrl;

/**
 * Adds an observer, which will be notified when cloud import history changes.
 *
 * @param {!importer.ImportHistory.Observer} observer
 */
importer.ImportHistory.prototype.addObserver;

/**
 * Remove a previously registered observer.
 *
 * @param {!importer.ImportHistory.Observer} observer
 */
importer.ImportHistory.prototype.removeObserver;

/**
 * @typedef {{
 *   state: !importer.ImportHistoryState,
 *   entry: !FileEntry,
 *   destination: !importer.Destination,
 *   destinationUrl: (string|undefined)
 * }}
 */
importer.ImportHistory.ChangedEvent;

/** @typedef {function(!importer.ImportHistory.ChangedEvent)} */
importer.ImportHistory.Observer;

/**
 * Provider of lazy loaded importer.ImportHistory. This is the main
 * access point for a fully prepared {@code importer.ImportHistory} object.
 *
 * @interface
 */
importer.HistoryLoader = function() {};

/**
 * Instantiates an {@code importer.ImportHistory} object and manages any
 * necessary ongoing maintenance of the object with respect to
 * its external dependencies.
 *
 * @see importer.SynchronizedHistoryLoader for an example.
 *
 * @return {!Promise<!importer.ImportHistory>} Resolves when history instance
 *     is ready.
 */
importer.HistoryLoader.prototype.getHistory;

/**
 * Adds a listener to be notified when history is fully loaded for the first
 * time. If history is already loaded...will be called immediately.
 *
 * @param {function(!importer.ImportHistory)} listener
 */
importer.HistoryLoader.prototype.addHistoryLoadedListener;
