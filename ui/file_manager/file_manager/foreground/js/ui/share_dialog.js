// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @param {HTMLElement} parentNode Node to be parent for this dialog.
 * @constructor
 * @extends {FileManagerDialogBase}
 * @implements {ShareClient.Observer}
 */
function ShareDialog(parentNode) {
  this.queue_ = new AsyncUtil.Queue();
  this.onQueueTaskFinished_ = null;
  this.shareClient_ = null;
  this.webViewWrapper_ = null;
  this.webView_ = null;
  this.failureTimeout_ = null;
  this.callback_ = null;
  this.overrideURLForTesting_ = null;

  FileManagerDialogBase.call(this, parentNode);
}

/**
 * Timeout for loading the share dialog before giving up.
 * @type {number}
 * @const
 */
ShareDialog.FAILURE_TIMEOUT = 20000;

/**
 * Polling interval for detecting the end of resizing animation.
 * @type {number}
 * @const
 */
ShareDialog.WEBVIEW_CHECKSIZE_INTERVAL = 66;

/**
 * The result of opening the dialog.
 * @enum {string}
 * @const
 */
ShareDialog.Result = {
  // The dialog is closed normally. This includes user cancel.
  SUCCESS: 'success',
  // The dialog is closed by network error.
  NETWORK_ERROR: 'networkError',
  // The dialog is not opened because it is already showing.
  ALREADY_SHOWING: 'alreadyShowing'
};
Object.freeze(ShareDialog.Result);

/**
 * Wraps a Web View element and adds authorization headers to it.
 * @param {string} urlPattern Pattern of urls to be authorized.
 * @param {WebView} webView Web View element to be wrapped.
 * @constructor
 */
ShareDialog.WebViewAuthorizer = function(urlPattern, webView) {
  this.urlPattern_ = urlPattern;
  this.webView_ = webView;
  this.initialized_ = false;
  this.accessToken_ = null;
};

/**
 * Initializes the web view by installing hooks injecting the authorization
 * headers.
 * @param {function()} callback Completion callback.
 */
ShareDialog.WebViewAuthorizer.prototype.initialize = function(callback) {
  if (this.initialized_) {
    callback();
    return;
  }

  var registerInjectionHooks = function() {
    this.webView_.removeEventListener('loadstop', registerInjectionHooks);
    this.webView_.request.onBeforeSendHeaders.addListener(
        this.authorizeRequest_.bind(this),
        /** @type {!RequestFilter} */ ({urls: [this.urlPattern_]}),
        ['blocking', 'requestHeaders']);
    this.initialized_ = true;
    callback();
  }.bind(this);

  this.webView_.addEventListener('loadstop', registerInjectionHooks);
  this.webView_.setAttribute('src', 'data:text/html,');
};

/**
 * Authorizes the web view by fetching the freshest access tokens.
 * @param {function()} callback Completion callback.
 */
ShareDialog.WebViewAuthorizer.prototype.authorize = function(callback) {
  // Fetch or update the access token.
  chrome.fileManagerPrivate.requestAccessToken(false,  // force_refresh
      function(inAccessToken) {
        this.accessToken_ = inAccessToken;
        callback();
      }.bind(this));
};

/**
 * Injects headers into the passed request.
 * @param {!Object} e Request event.
 * @return {!BlockingResponse} Modified headers.
 * @private
 */
ShareDialog.WebViewAuthorizer.prototype.authorizeRequest_ = function(e) {
  e.requestHeaders.push({
    name: 'Authorization',
    value: 'Bearer ' + this.accessToken_
  });
  return /** @type {!BlockingResponse} */ ({requestHeaders: e.requestHeaders});
};

ShareDialog.prototype = {
  __proto__: FileManagerDialogBase.prototype
};

/**
 * Sets an override URLs for testing. It will be used instead of the sharing URL
 * fetched from Drive. Note, that the domain still has to match
 * ShareClient.SHARE_TARGET, as well as the hostname access enabled in the
 * manifest (if different).
 *
 * @param {?string} url
 */
ShareDialog.prototype.setOverrideURLForTesting = function(url) {
  this.overrideURLForTesting_ = url;
};

/**
 * One-time initialization of DOM.
 * @protected
 */
ShareDialog.prototype.initDom_ = function() {
  FileManagerDialogBase.prototype.initDom_.call(this);
  this.frame_.classList.add('share-dialog-frame');

  this.webViewWrapper_ = this.document_.createElement('div');
  this.webViewWrapper_.className = 'share-dialog-webview-wrapper';
  this.cancelButton_.hidden = true;
  this.okButton_.hidden = true;
  this.closeButton_.hidden = true;
  this.frame_.insertBefore(this.webViewWrapper_,
                           this.frame_.querySelector('.cr-dialog-buttons'));
};

/**
 * @override
 */
ShareDialog.prototype.onResized = function(width, height, callback) {
  if (!width || !height)
    return;

  this.webViewWrapper_.style.width = width + 'px';
  this.webViewWrapper_.style.height = height + 'px';

  // Wait sending 'resizeComplete' event until the size of the WebView
  // stabilizes. This is a workaround for crbug.com/693416.
  // TODO(yamaguchi): Detect animation end by the absolute size to distinguish
  // it from frame drops.
  /**
   * @param {number} previousWidth Width in pixels.
   * @param {number} previousHeight Height in pixels.
   */
  var checkSize = function(previousWidth, previousHeight) {
    this.webView_.executeScript({
      code: "[document.documentElement.clientWidth," +
            " document.documentElement.clientHeight];"
    }, function(results) {
      var newWidth = results[0][0];
      var newHeight = results[0][1];
      if (newWidth === previousWidth && newHeight === previousHeight) {
        callback();
      } else {
        setTimeout(checkSize.bind(null, newWidth, newHeight),
            ShareDialog.WEBVIEW_CHECKSIZE_INTERVAL);
      }
    }.bind(this));
  }.bind(this);

  setTimeout(checkSize.bind(null, -1, -1),
      ShareDialog.WEBVIEW_CHECKSIZE_INTERVAL);
};

/**
 * @override
 */
ShareDialog.prototype.onClosed = function() {
  this.hide();
};

/**
 * @override
 */
ShareDialog.prototype.onLoaded = function() {
  if (this.failureTimeout_) {
    clearTimeout(this.failureTimeout_);
    this.failureTimeout_ = null;
  }

  // Logs added temporarily to track crbug.com/288783.
  console.debug('Loaded.');

  this.okButton_.hidden = false;
  this.webViewWrapper_.classList.add('loaded');
  this.webView_.focus();
};

/**
 * @override
 */
ShareDialog.prototype.onLoadFailed = function() {
  this.hideWithResult(ShareDialog.Result.NETWORK_ERROR);
};

/**
 * @param {Function=} opt_onHide Called when the dialog is hidden.
 * @override
 */
ShareDialog.prototype.hide = function(opt_onHide) {
  this.hideWithResult(ShareDialog.Result.SUCCESS, opt_onHide);
};

/**
 * Hide the dialog with the result and the callback.
 * @param {ShareDialog.Result} result Result passed to the closing callback.
 * @param {Function=} opt_onHide Callback called at the end of hiding.
 */
ShareDialog.prototype.hideWithResult = function(result, opt_onHide) {
  if (!this.isShowing())
    return;

  if (this.shareClient_) {
    this.shareClient_.dispose();
    this.shareClient_ = null;
  }

  this.webViewWrapper_.textContent = '';
  if (this.failureTimeout_) {
    clearTimeout(this.failureTimeout_);
    this.failureTimeout_ = null;
  }

  FileManagerDialogBase.prototype.hide.call(
      this,
      function() {
        if (opt_onHide)
          opt_onHide();
        this.callback_(result);
        this.callback_ = null;
      }.bind(this));
};

/**
 * Shows the dialog.
 * @param {!Entry} entry Entry to share.
 * @param {function(ShareDialog.Result)} callback Callback to be called when the
 *     showing task is completed. The argument is whether to succeed or not.
 *     Note that cancel is regarded as success.
 */
ShareDialog.prototype.showEntry = function(entry, callback) {
  // If the dialog is already showing, return the error.
  if (this.isShowing()) {
    callback(ShareDialog.Result.ALREADY_SHOWING);
    return;
  }

  // Initialize the variables.
  this.callback_ = callback;
  this.webViewWrapper_.style.width = '';
  this.webViewWrapper_.style.height = '';
  this.webViewWrapper_.classList.remove('loaded');

  // If the embedded share dialog is not started within some time, then
  // give up and show an error message.
  this.failureTimeout_ = setTimeout(function() {
    this.hideWithResult(ShareDialog.Result.NETWORK_ERROR);

    // Logs added temporarily to track crbug.com/288783.
    console.debug('Timeout. Web View points at: ' + this.webView_.src);
  }.bind(this), ShareDialog.FAILURE_TIMEOUT);

  // TODO(mtomasz): Move to initDom_() once and reuse <webview> once it gets
  // fixed. See: crbug.com/260622.
  this.webView_ = /** @type {WebView} */ (util.createChild(
      this.webViewWrapper_, 'share-dialog-webview', 'webview'));
  this.webViewAuthorizer_ = new ShareDialog.WebViewAuthorizer(
      !window.IN_TEST ? (ShareClient.SHARE_TARGET + '/*') : '<all_urls>',
      this.webView_);
  this.webView_.addEventListener('newwindow', function(e) {
    e = /** @type {NewWindowEvent} */ (e);
    // Discard the window object and reopen in an external window.
    e.window.discard();
    util.visitURL(e.targetUrl);
  });
  var show = FileManagerDialogBase.prototype.showBlankDialog.call(this);
  if (!show) {
    // The code shoundn't get here, since already-showing was handled before.
    console.error('ShareDialog can\'t be shown.');
    return;
  }

  // Initialize and authorize the Web View tag asynchronously.
  var group = new AsyncUtil.Group();

  var shareUrl;
  if (this.overrideURLForTesting_) {
    console.debug('Using an override URL for testing: ' +
        this.overrideURLForTesting_);
    shareUrl = this.overrideURLForTesting_;
  } else {
    // Fetches an url to the sharing dialog.
    group.add(function(inCallback) {
      chrome.fileManagerPrivate.getShareUrl(
          entry,
          function(inShareUrl) {
            if (!chrome.runtime.lastError)
              shareUrl = inShareUrl;
            else
              console.error(chrome.runtime.lastError.message);
            inCallback();
          });
    });
  }

  group.add(this.webViewAuthorizer_.initialize.bind(this.webViewAuthorizer_));
  group.add(this.webViewAuthorizer_.authorize.bind(this.webViewAuthorizer_));

  // Loads the share widget once all the previous async calls are finished.
  group.run(function() {
    // If the url is not obtained, return the network error.
    if (!shareUrl) {
      // Logs added temporarily to track crbug.com/288783.
      console.debug('The share URL is not available.');

      this.hideWithResult(ShareDialog.Result.NETWORK_ERROR);
      return;
    }
    // Already inactive, therefore ignore.
    if (!this.isShowing())
      return;
    this.shareClient_ = new ShareClient(this.webView_,
                                        shareUrl,
                                        this);
    this.shareClient_.load();
  }.bind(this));
};

/**
 * Tells whether the share dialog is showing or not.
 *
 * @return {boolean} True since the show method is called and until the closing
 *     callback is invoked.
 */
ShareDialog.prototype.isShowing = function() {
  return !!this.callback_;
};
