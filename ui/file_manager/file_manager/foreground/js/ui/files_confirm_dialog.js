// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Confirm dialog.
 * @param {!HTMLElement} parentNode
 * @constructor
 * @extends {cr.ui.dialogs.ConfirmDialog}
 */
var FilesConfirmDialog = function(parentNode) {
  cr.ui.dialogs.ConfirmDialog.call(this, parentNode);
};

FilesConfirmDialog.prototype.__proto__ = cr.ui.dialogs.ConfirmDialog.prototype;

/**
 * @protected
 * @override
 */
FilesConfirmDialog.prototype.initDom_ = function() {
  cr.ui.dialogs.ConfirmDialog.prototype.initDom_.call(this);
  this.frame_.classList.add('files-confirm-dialog');
};
