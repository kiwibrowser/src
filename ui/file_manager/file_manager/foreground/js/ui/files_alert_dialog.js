// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Alert dialog.
 * @param {!HTMLElement} parentNode
 * @constructor
 * @extends {cr.ui.dialogs.AlertDialog}
 */
var FilesAlertDialog = function(parentNode) {
  cr.ui.dialogs.AlertDialog.call(this, parentNode);
};

FilesAlertDialog.prototype.__proto__ = cr.ui.dialogs.AlertDialog.prototype;

/**
 * @protected
 * @override
 */
FilesAlertDialog.prototype.initDom_ = function() {
  cr.ui.dialogs.AlertDialog.prototype.initDom_.call(this);
  this.frame_.classList.add('files-alert-dialog');
};
