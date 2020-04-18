// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Namespace
var importer;

/**
 * Interface providing access to information about active import processes.
 *
 * @interface
 */
importer.ImportRunner = function() {};

/**
 * Imports all media identified by scanResult.
 *
 * @param {!importer.ScanResult} scanResult
 * @param {!importer.Destination} destination
 * @param {!Promise<!DirectoryEntry>} directoryPromise
 *
 * @return {!importer.MediaImportHandler.ImportTask} The resulting import task.
 */
importer.ImportRunner.prototype.importFromScanResult;
