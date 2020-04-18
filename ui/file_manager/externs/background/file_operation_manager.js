// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @constructor
 * @struct
 * @extends {cr.EventTarget}
 */
function FileOperationManager() {}

/**
 * Adds an event listener for the tasks.
 * @param {string} type The name of the event.
 * @param {EventListenerType} handler The handler for the event.  This is called
 *     when the event is dispatched.
 * @override
 */
FileOperationManager.prototype.addEventListener = function(type, handler) {};

/**
 * Removes an event listener for the tasks.
 * @param {string} type The name of the event.
 * @param {EventListenerType} handler The handler to be removed.
 * @override
 */
FileOperationManager.prototype.removeEventListener = function(type, handler) {};

/**
 * Says if there are any tasks in the queue.
 * @return {boolean} True, if there are any tasks.
 */
FileOperationManager.prototype.hasQueuedTasks = function() {};

/**
 * Requests the specified task to be canceled.
 * @param {string} taskId ID of task to be canceled.
 */
FileOperationManager.prototype.requestTaskCancel = function(taskId) {};

/**
 * Filters the entry in the same directory
 *
 * @param {Array<Entry>} sourceEntries Entries of the source files.
 * @param {DirectoryEntry|FakeEntry} targetEntry The destination entry of the
 *     target directory.
 * @param {boolean} isMove True if the operation is "move", otherwise (i.e.
 *     if the operation is "copy") false.
 * @return {Promise} Promise fulfilled with the filtered entry. This is not
 *     rejected.
 */
FileOperationManager.prototype.filterSameDirectoryEntry = function(
    sourceEntries, targetEntry, isMove) {};

/**
 * Kick off pasting.
 *
 * @param {Array<Entry>} sourceEntries Entries of the source files.
 * @param {DirectoryEntry} targetEntry The destination entry of the target
 *     directory.
 * @param {boolean} isMove True if the operation is "move", otherwise (i.e.
 *     if the operation is "copy") false.
 * @param {string=} opt_taskId If the corresponding item has already created
 *     at another places, we need to specify the ID of the item. If the
 *     item is not created, FileOperationManager generates new ID.
 */
FileOperationManager.prototype.paste = function(
    sourceEntries, targetEntry, isMove, opt_taskId) {};

/**
 * Schedules the files deletion.
 *
 * @param {Array<Entry>} entries The entries.
 */
FileOperationManager.prototype.deleteEntries = function(entries) {};

/**
 * Creates a zip file for the selection of files.
 *
 * @param {!Array<!Entry>} selectionEntries The selected entries.
 * @param {!DirectoryEntry} dirEntry The directory containing the selection.
 */
FileOperationManager.prototype.zipSelection = function(
    selectionEntries, dirEntry) {};

/**
 * Generates new task ID.
 *
 * @return {string} New task ID.
 */
FileOperationManager.prototype.generateTaskId = function() {};
