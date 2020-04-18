// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * This class is an extended class, to manage the status of the dialogs.
 *
 * @param {HTMLElement} parentNode Parent node of the dialog.
 * @extends {cr.ui.dialogs.BaseDialog}
 * @constructor
 */
var FileManagerDialogBase = function(parentNode) {
  cr.ui.dialogs.BaseDialog.call(this, parentNode);
};

FileManagerDialogBase.prototype = {
  __proto__: cr.ui.dialogs.BaseDialog.prototype
};

/**
 * The flag if any dialog is shown. True if a dialog is visible, false
 *     otherwise.
 * @type {boolean}
 */
FileManagerDialogBase.shown = false;

/**
 * @param {string} title Title.
 * @param {string} message Message.
 * @param {?function()} onOk Called when the OK button is pressed.
 * @param {?function()} onCancel Called when the cancel button is pressed.
 * @return {boolean} True if the dialog can show successfully. False if the
 *     dialog failed to show due to an existing dialog.
 */
FileManagerDialogBase.prototype.showOkCancelDialog = function(
    title, message, onOk, onCancel) {
  return this.showImpl_(title, message, onOk, onCancel);
};

/**
 * @param {string} title Title.
 * @param {string} message Message.
 * @param {?function()} onOk Called when the OK button is pressed.
 * @param {?function()} onCancel Called when the cancel button is pressed.
 * @return {boolean} True if the dialog can show successfully. False if the
 *     dialog failed to show due to an existing dialog.
 * @private
 */
FileManagerDialogBase.prototype.showImpl_ = function(
    title, message, onOk, onCancel) {
  if (FileManagerDialogBase.shown)
    return false;

  FileManagerDialogBase.shown = true;

  // If a dialog is shown, activate the window.
  var appWindow = chrome.app.window.current();
  if (appWindow)
    appWindow.focus();

  cr.ui.dialogs.BaseDialog.prototype.showWithTitle.call(
      this, title, message, onOk, onCancel, null);

  return true;
};

/**
 * @return {boolean} True if the dialog can show successfully. False if the
 *     dialog failed to show due to an existing dialog.
 */
FileManagerDialogBase.prototype.showBlankDialog = function() {
  return this.showImpl_('', '', null, null);
};

/**
 * @param {string} title Title.
 * @return {boolean} True if the dialog can show successfully. False if the
 *     dialog failed to show due to an existing dialog.
 */
FileManagerDialogBase.prototype.showTitleOnlyDialog = function(title) {
  return this.showImpl_(title, '', null, null);
};

/**
 * @param {string} title Title.
 * @param {string} text Text to be shown in the dialog.
 * @return {boolean} True if the dialog can show successfully. False if the
 *     dialog failed to show due to an existing dialog.
 */
FileManagerDialogBase.prototype.showTitleAndTextDialog = function(title, text) {
  this.buttons.style.display = 'none';
  return this.showImpl_(title, text, null, null);
};

/**
 * @param {Function=} opt_onHide Called when the dialog is hidden.
 */
FileManagerDialogBase.prototype.hide = function(opt_onHide) {
  cr.ui.dialogs.BaseDialog.prototype.hide.call(
      this,
      function() {
        if (opt_onHide)
          opt_onHide();
        FileManagerDialogBase.shown = false;
      });
};
