// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @interface
 * @extends {FileBrowserBackground}
 * @struct
 */
var FileBrowserBackgroundFull = function() {};

/**
 * @type {!DriveSyncHandler}
 */
FileBrowserBackgroundFull.prototype.driveSyncHandler;

/**
 * @type {!ProgressCenter}
 */
FileBrowserBackgroundFull.prototype.progressCenter;

/**
 * String assets.
 * @type {Object<string>}
 */
FileBrowserBackgroundFull.prototype.stringData;

/**
 * @type {FileOperationManager}
 */
FileBrowserBackgroundFull.prototype.fileOperationManager;

/**
 * @type {!importer.ImportRunner}
 */
FileBrowserBackgroundFull.prototype.mediaImportHandler;

/**
 * @type {!importer.MediaScanner}
 */
FileBrowserBackgroundFull.prototype.mediaScanner;

/**
 * @type {!importer.HistoryLoader}
 */
FileBrowserBackgroundFull.prototype.historyLoader;
