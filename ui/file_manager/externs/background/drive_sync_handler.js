// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Handler of the background page for the drive sync events.
 * @constructor
 * @extends {cr.EventTarget}
 * @struct
 */
function DriveSyncHandler() {}

DriveSyncHandler.prototype = /** @struct */ {
  /**
   * @return {boolean} Whether the handler is having syncing items or not.
   */
  get syncing() {}
};

/**
 * Returns whether the drive sync is currently suppressed or not.
 * @return {boolean}
 */
DriveSyncHandler.prototype.isSyncSuppressed = function() {};

/**
 * Shows the notification saying that the drive sync is disabled on cellular
 * network.
 */
DriveSyncHandler.prototype.showDisabledMobileSyncNotification = function() {};
