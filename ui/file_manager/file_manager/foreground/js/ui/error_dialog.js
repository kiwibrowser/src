// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @param {HTMLElement} parentNode Node to be parent for this dialog.
 * @constructor
 * @extends {cr.ui.dialogs.BaseDialog}
 */
function ErrorDialog(parentNode) {
  cr.ui.dialogs.BaseDialog.call(this, parentNode);
}

ErrorDialog.prototype = {
  __proto__: cr.ui.dialogs.BaseDialog.prototype
};

/**
 * One-time initialization of DOM.
 * @protected
 */
ErrorDialog.prototype.initDom_ = function() {
  cr.ui.dialogs.BaseDialog.prototype.initDom_.call(this);
  this.frame_.classList.add('error-dialog-frame');
  var img = this.document_.createElement('div');
  img.className = 'error-dialog-img';
  this.frame_.insertBefore(img, this.text_);
};
