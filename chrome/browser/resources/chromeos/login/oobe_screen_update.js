// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Oobe update screen implementation.
 */

login.createScreen('UpdateScreen', 'update', function() {
  var USER_ACTION_CANCEL_UPDATE_SHORTCUT = 'cancel-update';
  var CONTEXT_KEY_TIME_LEFT_SEC = 'time-left-sec';
  var CONTEXT_KEY_SHOW_TIME_LEFT = 'show-time-left';
  var CONTEXT_KEY_UPDATE_COMPLETED = 'update-completed';
  var CONTEXT_KEY_SHOW_CURTAIN = 'show-curtain';
  var CONTEXT_KEY_SHOW_PROGRESS_MESSAGE = 'show-progress-msg';
  var CONTEXT_KEY_PROGRESS = 'progress';
  var CONTEXT_KEY_PROGRESS_MESSAGE = 'progress-msg';
  var CONTEXT_KEY_CANCEL_UPDATE_SHORTCUT_ENABLED = 'cancel-update-enabled';

  return {
    EXTERNAL_API: [],

    /** @override */
    decorate: function() {
      var self = this;

      this.context.addObserver(
          CONTEXT_KEY_TIME_LEFT_SEC, function(time_left_sec) {
            self.setEstimatedTimeLeft(time_left_sec);
          });
      this.context.addObserver(
          CONTEXT_KEY_SHOW_TIME_LEFT, function(show_time_left) {
            self.showEstimatedTimeLeft(show_time_left);
          });
      this.context.addObserver(
          CONTEXT_KEY_UPDATE_COMPLETED, function(is_completed) {
            self.setUpdateCompleted(is_completed);
          });
      this.context.addObserver(
          CONTEXT_KEY_SHOW_CURTAIN, function(show_curtain) {
            self.showUpdateCurtain(show_curtain);
          });
      this.context.addObserver(
          CONTEXT_KEY_SHOW_PROGRESS_MESSAGE, function(show_progress_msg) {
            self.showProgressMessage(show_progress_msg);
          });
      this.context.addObserver(CONTEXT_KEY_PROGRESS, function(progress) {
        self.setUpdateProgress(progress);
      });
      this.context.addObserver(
          CONTEXT_KEY_PROGRESS_MESSAGE, function(progress_msg) {
            self.setProgressMessage(progress_msg);
          });
      this.context.addObserver(
          CONTEXT_KEY_CANCEL_UPDATE_SHORTCUT_ENABLED, function(enabled) {
            $('update-cancel-hint').hidden = !enabled;
            $('oobe-update-md').cancelAllowed = enabled;
          });
    },

    /**
     * Header text of the screen.
     * @type {string}
     */
    get header() {
      return loadTimeData.getString('updateScreenTitle');
    },

    /**
     * Cancels the screen.
     */
    cancel: function() {
      // It's safe to act on the accelerator even if it's disabled on official
      // builds, since Chrome will just ignore this user action in that case.
      var updateCancelHint = $('update-cancel-hint').firstElementChild;
      var message = loadTimeData.getString('cancelledUpdateMessage');
      updateCancelHint.textContent = message;
      $('oobe-update-md').setCancelHint(message);
      this.send(
          login.Screen.CALLBACK_USER_ACTED, USER_ACTION_CANCEL_UPDATE_SHORTCUT);
    },

    /**
     * Sets update's progress bar value.
     * @param {number} progress Percentage of the progress bar.
     */
    setUpdateProgress: function(progress) {
      $('update-progress-bar').value = progress;
      $('oobe-update-md').progressValue = progress;
    },

    /**
     * Shows or hides downloading ETA message.
     * @param {boolean} visible Are ETA message visible?
     */
    showEstimatedTimeLeft: function(visible) {
      $('progress-message').hidden = visible;
      $('estimated-time-left').hidden = !visible;
      $('oobe-update-md').estimatedTimeLeftShown = visible;
      $('oobe-update-md').progressMessageShown = !visible;
    },

    /**
     * Sets estimated time left until download will complete.
     * @param {number} seconds Time left in seconds.
     */
    setEstimatedTimeLeft: function(seconds) {
      var minutes = Math.ceil(seconds / 60);
      var message = '';
      if (minutes > 60) {
        message = loadTimeData.getString('downloadingTimeLeftLong');
      } else if (minutes > 55) {
        message = loadTimeData.getString('downloadingTimeLeftStatusOneHour');
      } else if (minutes > 20) {
        message = loadTimeData.getStringF(
            'downloadingTimeLeftStatusMinutes', Math.ceil(minutes / 5) * 5);
      } else if (minutes > 1) {
        message = loadTimeData.getStringF(
            'downloadingTimeLeftStatusMinutes', minutes);
      } else {
        message = loadTimeData.getString('downloadingTimeLeftSmall');
      }
      var formattedMessage = loadTimeData.getStringF('downloading', message);
      $('estimated-time-left').textContent = formattedMessage;
      $('oobe-update-md').estimatedTimeLeft = formattedMessage;
    },

    /**
     * Shows or hides info message below progress bar.
     * @param {boolean} visible Are message visible?
     */
    showProgressMessage: function(visible) {
      $('estimated-time-left').hidden = visible;
      $('progress-message').hidden = !visible;
      $('oobe-update-md').estimatedTimeLeftShown = !visible;
      $('oobe-update-md').progressMessageShown = visible;
    },

    /**
     * Sets message below progress bar.
     * @param {string} message Message that should be shown.
     */
    setProgressMessage: function(message) {
      $('progress-message').innerText = message;
      $('oobe-update-md').progressMessage = message;
    },

    /**
     * Marks update completed. Shows "update completed" message.
     * @param {boolean} is_completed True if update process is completed.
     */
    setUpdateCompleted: function(is_completed) {
      $('update-upper-label').hidden = is_completed;
      $('oobe-update-md').updateCompleted = is_completed;
    },

    /**
     * Shows or hides update curtain.
     * @param {boolean} visible Are curtains visible?
     */
    showUpdateCurtain: function(visible) {
      $('update-screen-curtain').hidden = !visible;
      $('update-screen-main').hidden = visible;
      $('oobe-update-md').checkingForUpdate = visible;
    },

    /**
     * This method takes care of switching to material-design OOBE.
     * @private
     */
    setMDMode_: function() {
      var useMDOobe = (loadTimeData.getString('newOobeUI') == 'on');
      $('oobe-update-md').hidden = !useMDOobe;
      $('oobe-update').hidden = useMDOobe;
    },

    /**
     * Event handler that is invoked just before the screen is shown.
     */
    onBeforeShow: function() {
      this.setMDMode_();
    },

    /**
     * Updates localized content of the screen that is not updated via template.
     */
    updateLocalizedContent: function() {
      this.setMDMode_();
    },
  };
});
