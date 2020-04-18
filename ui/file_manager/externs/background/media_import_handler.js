// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Namespace
var importer;

importer.MediaImportHandler;

/**
 * Note that this isn't an actual FileOperationManager.Task.  It currently uses
 * the FileOperationManager (and thus *spawns* an associated
 * FileOperationManager.CopyTask) but this is a temporary state of affairs.
 *
 * @constructor
 * @struct
 */
importer.MediaImportHandler.ImportTask = function() {};

/** @struct */
importer.MediaImportHandler.ImportTask.prototype = {
  /** @return {!Promise} Resolves when task
      is complete, or cancelled, rejects on error. */
  get whenFinished() {}
};

/**
 * Request cancellation of this task.  An update will be sent to observers once
 * the task is actually cancelled.
 */
importer.MediaImportHandler.ImportTask.prototype.requestCancel = function() {};
