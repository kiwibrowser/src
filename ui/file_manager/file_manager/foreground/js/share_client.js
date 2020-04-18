// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @param {WebView} webView Web View tag.
 * @param {string} url Share Url for an entry.
 * @param {ShareClient.Observer} observer Observer instance.
 * @constructor
 */
function ShareClient(webView, url, observer) {
  this.webView_ = webView;
  this.url_ = url;
  this.observer_ = observer;
  this.loaded_ = false;
  this.loading_ = false;
  this.onMessageBound_ = this.onMessage_.bind(this);
  this.onLoadStopBound_ = this.onLoadStop_.bind(this);
  this.onLoadAbortBound_ = this.onLoadAbort_.bind(this);
}

/**
 * Target origin of the embedded dialog.
 * @type {string}
 * @const
 */
ShareClient.SHARE_TARGET = 'https://drive.google.com';

/**
 * Observes for state changes of the embedded dialog.
 * @interface
 */
ShareClient.Observer = function() {
};

/**
 * Notifies about the embedded dialog being loaded.
 */
ShareClient.Observer.prototype.onLoaded = function() {
};

/**
 * Notifies when the the embedded dialog failed to load.
 */
ShareClient.Observer.prototype.onLoadFailed = function() {
};

/**
 * Notifies about changed dimensions of the embedded dialog.
 * @param {number} width Width in pixels.
 * @param {number} height Height in pixels.
 * @param {function()} callback Completion callback. Call when finished
 *     handling the resize.
 */
ShareClient.Observer.prototype.onResized = function(width, height, callback) {
};

/**
 * Notifies about the embedded dialog being closed.
 */
ShareClient.Observer.prototype.onClosed = function() {
};

/**
 * Handles messages from the embedded dialog.
 * @param {Event} e Message event.
 * @private
 */
ShareClient.prototype.onMessage_ = function(e) {
  if (e.origin != ShareClient.SHARE_TARGET && !window.IN_TEST) {
    // Logs added temporarily to track crbug.com/288783.
    console.debug('Received a message from an illegal origin: ' + e.origin);
    return;
  }

  var data = JSON.parse(e.data);
  // Logs added temporarily to track crbug.com/288783.
  console.debug('Received message: ' + data.type);

  switch (data.type) {
    case 'resize':
      this.observer_.onResized(data.args.width,
                               data.args.height,
                               this.postMessage_.bind(this, 'resizeComplete'));
      break;
    case 'prepareForVisible':
      this.postMessage_('prepareComplete');
      if (!this.loaded_) {
        this.loading_ = false;
        this.loaded_ = true;
        this.observer_.onLoaded();
      }
      break;
    case 'setVisible':
      if (!data.args.visible)
        this.observer_.onClosed();
      break;
  }
};

/**
 * Handles completion of the web view request.
 * @param {Event} e Message event.
 * @private
 */
ShareClient.prototype.onLoadStop_ = function(e) {
  // Logs added temporarily to track crbug.com/288783.
  console.debug('Web View loaded.');

  this.postMessage_('makeBodyVisible');
};

/**
 * Handles termination of the web view request.
 * @param {Event} e Message event.
 * @private
 */
ShareClient.prototype.onLoadAbort_ = function(e) {
  // Logs added temporarily to track crbug.com/288783.
  console.debug('Web View failed to load with error: ' + e.reason + ', url: ' +
      e.url + ' while requested: ' + this.url_);

  this.observer_.onLoadFailed();
};

/**
 * Sends a message to the embedded dialog.
 * @param {string} type Message type.
 * @param {Object=} opt_args Optional arguments.
 * @private
 */
ShareClient.prototype.postMessage_ = function(type, opt_args) {
  // Logs added temporarily to track crbug.com/288783.
  console.debug('Sending message: ' + type);

  var message = {
    type: type,
    args: opt_args
  };
  this.webView_.contentWindow.postMessage(
      JSON.stringify(message),
      !window.IN_TEST ? ShareClient.SHARE_TARGET : '*');
};

/**
 * Loads the embedded dialog. Can be called only one.
 */
ShareClient.prototype.load = function() {
  if (this.loading_ || this.loaded_)
    throw new Error('Already loaded.');
  this.loading_ = true;

  // Logs added temporarily to track crbug.com/288783.
  console.debug('Loading.');

  window.addEventListener('message', this.onMessageBound_);
  this.webView_.addEventListener('loadstop', this.onLoadStopBound_);
  this.webView_.addEventListener('loadabort', this.onLoadAbortBound_);
  this.webView_.setAttribute('src', this.url_);
};

/**
 * Aborts loading of the embedded dialog and performs cleanup.
 */
ShareClient.prototype.abort = function() {
  window.removeEventListener('message', this.onMessageBound_);
  this.webView_.removeEventListener('loadstop', this.onLoadStopBound_);
  this.webView_.removeEventListener(
      'loadabort', this.onLoadAbortBound_);
  this.webView_.stop();
};

/**
 * Cleans the dialog by removing all handlers.
 */
ShareClient.prototype.dispose = function() {
  this.abort();
};
