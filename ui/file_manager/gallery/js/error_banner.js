// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @param {Element} container Content container.
 * @constructor
 */
function ErrorBanner(container) {
  this.container_ = container;
  this.errorBanner_ = this.container_.querySelector('.error-banner');
}

/**
 * Shows an error message.
 * @param {string} message Message.
 */
ErrorBanner.prototype.show = function(message) {
  this.errorBanner_.textContent = str(message);
  this.container_.setAttribute('error', true);
};

/**
 * Hides an error message.
 */
ErrorBanner.prototype.clear = function() {
  this.container_.removeAttribute('error');
};
