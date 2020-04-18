// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * Mock implementation of remoting.ModalDialogFactory for testing.
 */

/** @suppress {duplicate} */
var remoting = remoting || {};

(function() {

'use strict';

/**
 * @constructor
 * @extends {remoting.InputDialog}
 */
remoting.MockInputDialog = function() {};

/** @override */
remoting.MockInputDialog.prototype.show = function() {
  return Promise.resolve();
};

/**
 * @constructor
 * @extends {remoting.MessageDialog}
 */
remoting.MockMessageDialog = function() {};

/** @override */
remoting.MockMessageDialog.prototype.show = function() {
  return Promise.resolve();
};

/**
 * @constructor
 * @extends {remoting.Html5ModalDialog}
 */
remoting.MockHtml5ModalDialog = function() {};

/** @override */
remoting.MockHtml5ModalDialog.prototype.show = function() {
  return Promise.resolve();
};

/**
 * @constructor
 * @extends {remoting.ConnectingDialog}
 */
remoting.MockConnectingDialog = function() {};

/** @override */
remoting.MockConnectingDialog.prototype.show = function() {};

/** @override */
remoting.MockConnectingDialog.prototype.hide = function() {};

/**
 * @constructor
 * @extends {remoting.ModalDialogFactory}
 */
remoting.MockModalDialogFactory = function() {
  /** @type {remoting.MockConnectingDialog} */
  this.connectingDialog = new remoting.MockConnectingDialog();
  /** @type {remoting.MockMessageDialog} */
  this.messageDialog = new remoting.MockMessageDialog();
  /** @type {remoting.MockInputDialog} */
  this.inputDialog = new remoting.MockInputDialog();
  /** @type {remoting.MockHtml5ModalDialog} */
  this.html5ModalDialog = new remoting.MockHtml5ModalDialog();
};

/** @override */
remoting.MockModalDialogFactory.prototype.createConnectingDialog =
    function(cancelCallback) {
  return this.connectingDialog;
};

/** @override */
remoting.MockModalDialogFactory.prototype.createHtml5ModalDialog =
    function(params) {
  return this.html5ModalDialog;
};

/**
 * @param {remoting.AppMode} mode
 * @param {HTMLElement} primaryButton
 * @param {HTMLElement=} opt_secondaryButton
 * @return {remoting.MessageDialog}
 * @override
 */
remoting.MockModalDialogFactory.prototype.createMessageDialog =
    function(mode, primaryButton, opt_secondaryButton) {
  return this.messageDialog;
};

/** @override */
remoting.MockModalDialogFactory.prototype.createInputDialog =
    function(mode, formElement, inputField, cancelButton) {
  return this.inputDialog;
};

})();
