// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Progress center at the background page.
 * @constructor
 * @struct
 */
var ProgressCenter = function() {};

/**
 * Updates the item in the progress center.
 * If the item has a new ID, the item is added to the item list.
 *
 * @param {ProgressCenterItem} item Updated item.
 */
ProgressCenter.prototype.updateItem = function(item) {};

/**
 * Requests to cancel the progress item.
 * @param {string} id Progress ID to be requested to cancel.
 */
ProgressCenter.prototype.requestCancel = function(id) {};

/**
 * Adds a panel UI to the notification center.
 * @param {ProgressCenterPanel} panel Panel UI.
 */
ProgressCenter.prototype.addPanel = function(panel) {};

/**
 * Removes a panel UI from the notification center.
 * @param {ProgressCenterPanel} panel Panel UI.
 */
ProgressCenter.prototype.removePanel = function(panel) {};

/**
 * Obtains item by ID.
 * @param {string} id ID of progress item.
 * @return {ProgressCenterItem} Progress center item having the specified
 *     ID. Null if the item is not found.
 */
ProgressCenter.prototype.getItemById = function(id) {};
