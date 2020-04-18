// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Progress center at the background page.
 * @constructor
 * @struct
 */
var ProgressCenter = function() {
  /**
   * Current items managed by the progress center.
   * @type {Array<!ProgressCenterItem>}
   * @private
   */
  this.items_ = [];

  /**
   * Map of progress ID and notification ID.
   * @type {Object<string>}
   * @private
   */
  this.notifications_ = new ProgressCenter.Notifications_(
      this.requestCancel.bind(this),
      this.onNotificationDismissed_.bind(this));

  /**
   * List of panel UI managed by the progress center.
   * @type {Array<ProgressCenterPanel>}
   * @private
   */
  this.panels_ = [];
};

/**
 * Notifications created by progress center.
 * @param {function(string)} cancelCallback Callback to notify the progress
 *     center of cancel operation.
 * @param {function(string)} dismissCallback Callback to notify the progress
 *     center that a notification is dismissed.
 * @constructor
 * @struct
 * @private
 */
ProgressCenter.Notifications_ = function(cancelCallback, dismissCallback) {
  /**
   * ID set of notifications that is progressing now.
   * @type {Object<ProgressCenter.Notifications_.NotificationState_>}
   * @private
   */
  this.ids_ = {};

  /**
   * Async queue.
   * @type {AsyncUtil.Queue}
   * @private
   */
  this.queue_ = new AsyncUtil.Queue();

  /**
   * Callback to notify the progress center of cancel operation.
   * @type {function(string)}
   * @private
   */
  this.cancelCallback_ = cancelCallback;

  /**
   * Callback to notify the progress center that a notification is dismissed.
   * @private {function(string)}
   */
  this.dismissCallback_ = dismissCallback;

  chrome.notifications.onButtonClicked.addListener(
      this.onButtonClicked_.bind(this));
  chrome.notifications.onClosed.addListener(this.onClosed_.bind(this));
};

/**
 * State of notification.
 * @enum {string}
 * @const
 * @private
 */
ProgressCenter.Notifications_.NotificationState_ = {
  VISIBLE: 'visible',
  DISMISSED: 'dismissed'
};

/**
 * Updates the notification according to the item.
 * @param {ProgressCenterItem} item Item to contain new information.
 * @param {boolean} newItemAcceptable Whether to accept new item or not.
 */
ProgressCenter.Notifications_.prototype.updateItem = function(
    item, newItemAcceptable) {
  var NotificationState = ProgressCenter.Notifications_.NotificationState_;
  var newlyAdded = !(item.id in this.ids_);

  // If new item is not acceptable, just return.
  if (newlyAdded && !newItemAcceptable)
    return;

  // Update the ID map and return if we does not show a notification for the
  // item.
  if (item.state === ProgressItemState.PROGRESSING ||
      item.state === ProgressItemState.ERROR) {
    if (newlyAdded)
      this.ids_[item.id] = NotificationState.VISIBLE;
    else if (this.ids_[item.id] === NotificationState.DISMISSED)
      return;
  } else {
    // This notification is no longer tracked.
    var previousState = this.ids_[item.id];
    delete this.ids_[item.id];
    // Clear notifications for complete or canceled items.
    if (item.state === ProgressItemState.CANCELED ||
        item.state === ProgressItemState.COMPLETED) {
      if (previousState === NotificationState.VISIBLE) {
        this.queue_.run(function(proceed) {
          chrome.notifications.clear(item.id, proceed);
        });
      }
      return;
    }
  }

  // Create/update the notification with the item.
  this.queue_.run(function(proceed) {
    var params = {
      title: chrome.runtime.getManifest().name,
      iconUrl: chrome.runtime.getURL('/common/images/icon96.png'),
      type: item.state === ProgressItemState.PROGRESSING ? 'progress' : 'basic',
      message: item.message,
      buttons: item.cancelable ? [{title: str('CANCEL_LABEL')}] : undefined,
      progress: item.state === ProgressItemState.PROGRESSING ?
          item.progressRateInPercent : undefined,
      priority: (item.state === ProgressItemState.ERROR || !item.quiet) ? 0 : -1
    };
    if (newlyAdded)
      chrome.notifications.create(item.id, params, proceed);
    else
      chrome.notifications.update(item.id, params, proceed);
  }.bind(this));
};

/**
 * Dismisses error item.
 * @param {string} id Item ID.
 */
ProgressCenter.Notifications_.prototype.dismissErrorItem = function(id) {
  if (!this.ids_[id])
    return;

  delete this.ids_[id];

  this.queue_.run(function(proceed) {
    chrome.notifications.clear(id, proceed);
  });
};

/**
 * Handles cancel button click.
 * @param {string} id Item ID.
 * @private
 */
ProgressCenter.Notifications_.prototype.onButtonClicked_ = function(id) {
  if (id in this.ids_)
    this.cancelCallback_(id);
};

/**
 * Handles notification close.
 * @param {string} id Item ID.
 * @private
 */
ProgressCenter.Notifications_.prototype.onClosed_ = function(id) {
  if (id in this.ids_) {
    this.ids_[id] = ProgressCenter.Notifications_.NotificationState_.DISMISSED;
    this.dismissCallback_(id);
  }
};

/**
 * Updates the item in the progress center.
 * If the item has a new ID, the item is added to the item list.
 *
 * @param {ProgressCenterItem} item Updated item.
 */
ProgressCenter.prototype.updateItem = function(item) {
  // Update item.
  var index = this.getItemIndex_(item.id);
  if (item.state === ProgressItemState.PROGRESSING) {
    if (index === -1)
      this.items_.push(item);
    else
      this.items_[index] = item;
  } else {
    // Error item is not removed until user explicitly dismiss it.
    if (item.state !== ProgressItemState.ERROR && index !== -1)
      this.items_.splice(index, 1);
  }

  // Update panels.
  for (var i = 0; i < this.panels_.length; i++) {
    this.panels_[i].updateItem(item);
  }

  // Update notifications.
  this.notifications_.updateItem(item, !this.panels_.length);
};

/**
 * Requests to cancel the progress item.
 * @param {string} id Progress ID to be requested to cancel.
 */
ProgressCenter.prototype.requestCancel = function(id) {
  var item = this.getItemById(id);
  if (item && item.cancelCallback)
    item.cancelCallback();
};

/**
 * Called when notification is dismissed.
 * @param {string} id Item id.
 * @private
 */
ProgressCenter.prototype.onNotificationDismissed_ = function(id) {
  var item = this.getItemById(id);
  if (item && item.state === ProgressItemState.ERROR)
    this.dismissErrorItem_(id);
};

/**
 * Adds a panel UI to the notification center.
 * @param {ProgressCenterPanel} panel Panel UI.
 */
ProgressCenter.prototype.addPanel = function(panel) {
  if (this.panels_.indexOf(panel) !== -1)
    return;

  // Update the panel list.
  this.panels_.push(panel);

  // Set the current items.
  for (var i = 0; i < this.items_.length; i++)
    panel.updateItem(this.items_[i]);

  // Register the cancel callback.
  panel.cancelCallback = this.requestCancel.bind(this);

  // Register the dismiss error item callback.
  panel.dismissErrorItemCallback = this.dismissErrorItem_.bind(this);
};

/**
 * Removes a panel UI from the notification center.
 * @param {ProgressCenterPanel} panel Panel UI.
 */
ProgressCenter.prototype.removePanel = function(panel) {
  var index = this.panels_.indexOf(panel);
  if (index === -1)
    return;

  this.panels_.splice(index, 1);
  panel.cancelCallback = null;

  // If there is no panel, show the notifications.
  if (this.panels_.length)
    return;
  for (var i = 0; i < this.items_.length; i++)
    this.notifications_.updateItem(this.items_[i], true);
};

/**
 * Obtains item by ID.
 * @param {string} id ID of progress item.
 * @return {ProgressCenterItem} Progress center item having the specified
 *     ID. Null if the item is not found.
 */
ProgressCenter.prototype.getItemById = function(id) {
  return this.items_[this.getItemIndex_(id)];
};

/**
 * Obtains item index that have the specifying ID.
 * @param {string} id Item ID.
 * @return {number} Item index. Returns -1 If the item is not found.
 * @private
 */
ProgressCenter.prototype.getItemIndex_ = function(id) {
  for (var i = 0; i < this.items_.length; i++) {
    if (this.items_[i].id === id)
      return i;
  }
  return -1;
};

/**
 * Requests all panels to dismiss an error item.
 * @param {string} id Item ID.
 * @private
 */
ProgressCenter.prototype.dismissErrorItem_ = function(id) {
  var index = this.getItemIndex_(id);
  if (index > -1)
    this.items_.splice(index, 1);

  this.notifications_.dismissErrorItem(id);

  for (var i = 0; i < this.panels_.length; i++) {
    this.panels_[i].dismissErrorItem(id);
  }
};
