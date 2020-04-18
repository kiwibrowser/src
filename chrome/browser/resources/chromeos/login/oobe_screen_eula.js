// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Oobe eula screen implementation.
 */

login.createScreen('EulaScreen', 'eula', function() {
  var CONTEXT_KEY_USAGE_STATS_ENABLED = 'usageStatsEnabled';
  var CLEAR_ANCHORS_CONTENT_SCRIPT = {
    code: 'A=Array.from(document.getElementsByTagName("a"));' +
        'for(var i = 0; i < A.length; ++i) {' +
        '  const el = A[i];' +
        '  let e = document.createElement("span");' +
        '  e.textContent=el.textContent;' +
        '  el.parentNode.replaceChild(e,el);' +
        '}'
  };

  /**
   * Load text/html contents from the given url into the given webview. The
   * contents is loaded via XHR and is sent to webview via data url so that it
   * is properly sandboxed.
   *
   * @param {!WebView} webview Webview element to host the terms.
   * @param {!string} url URL to load terms contents.
   */
  function loadUrlToWebview(webview, url) {
    assert(webview.tagName === 'WEBVIEW');

    var onError = function() {
      webview.src = 'about:blank';
    };

    var setContents = function(contents) {
      webview.src =
          'data:text/html;charset=utf-8,' + encodeURIComponent(contents);
    };

    var xhr = new XMLHttpRequest();
    xhr.open('GET', url);
    xhr.setRequestHeader('Accept', 'text/html');
    xhr.onreadystatechange = function() {
      if (xhr.readyState != 4)
        return;
      if (xhr.status != 200) {
        onError();
        return;
      }

      var contentType = xhr.getResponseHeader('Content-Type');
      if (contentType && !/text\/html/.test(contentType)) {
        onError();
        return;
      }

      setContents(xhr.response);
    };

    try {
      xhr.send();
    } catch (e) {
      onError();
    }
  }

  // A class to wrap online contents loading for a webview. A PendingLoad is
  // constructed with a target webview, an url to load, a load timeout and an
  // error callback. It attaches to a "pendingLoad" property of |webview| after
  // starting the load by setting |webview|.src. If the load times out or fails,
  // the error callback is invoked. If there exists a "pendingLoad" before the
  // load, the existing one is stopped (note the error callback is not invoked
  // in this case).
  class PendingLoad {
    constructor(webview, url, timeout, opt_errorCallback) {
      assert(webview.tagName === 'WEBVIEW');
      assert(/^https?:\/\//.test(url));

      this.webview_ = webview;
      this.url_ = url;
      this.timeout_ = timeout;
      this.errorCallback_ = opt_errorCallback;

      this.loadTimer_ = 0;
      this.boundOnLoadCompleted_ = this.onLoadCompleted_.bind(this);
      this.boundOnLoadError_ = this.onLoadError_.bind(this);
    }

    start() {
      if (this.webview_[PendingLoad.ATTACHED_PROPERTY_NAME])
        this.webview_[PendingLoad.ATTACHED_PROPERTY_NAME].stop();
      this.webview_[PendingLoad.ATTACHED_PROPERTY_NAME] = this;

      this.webview_.addEventListener('loadabort', this.boundOnLoadError_);
      this.webview_.request.onCompleted.addListener(
          this.boundOnLoadCompleted_,
          {urls: ['<all_urls>'], types: ['main_frame']});
      this.webview_.request.onErrorOccurred.addListener(
          this.boundOnLoadError_,
          {urls: ['<all_urls>'], types: ['main_frame']});
      this.loadTimer_ =
          window.setTimeout(this.boundOnLoadError_, this.timeout_);

      this.webview_.src = this.url_;
    }

    stop() {
      assert(this.webview_[PendingLoad.ATTACHED_PROPERTY_NAME] === this);
      delete this.webview_[PendingLoad.ATTACHED_PROPERTY_NAME];

      this.webview_.removeEventListener('loadabort', this.boundOnLoadError_);
      this.webview_.request.onCompleted.removeListener(
          this.boundOnLoadCompleted_);
      this.webview_.request.onErrorOccurred.removeListener(
          this.boundOnLoadError_);
      window.clearTimeout(this.loadTimer_);
    }

    onLoadCompleted_(details) {
      if (details.url != this.url_)
        return;

      // Http errors such as 4xx, 5xx hit here instead of 'onErrorOccurred'.
      if (details.statusCode != 200) {
        this.onLoadError_();
        return;
      }

      this.stop();
    }

    onLoadError_(opt_details) {
      // A 'loadabort' or 'onErrorOccurred' event from a previous load could
      // be invoked even though a new navigation has started. Filter out those
      // by checking the url.
      if (opt_details && opt_details.url != this.url_)
        return;

      this.stop();
      if (this.errorCallback_)
        this.errorCallback_();
    }
  }
  /**
   * A property name used to attach to a Webview.
   * type{string}
   */
  PendingLoad.ATTACHED_PROPERTY_NAME = 'pendingLoad';

  return {
    /**
     * Tracks OEM Eula url so that it could be properly reloaded.
     * @type {?string}
     */
    oemEulaUrl_: null,

    /** @override */
    decorate: function() {
      $('eula-chrome-credits-link').hidden = true;
      $('eula-chromeos-credits-link').hidden = true;
      $('stats-help-link').addEventListener('click', function(event) {
        chrome.send('eulaOnLearnMore');
      });
      $('installation-settings-link')
          .addEventListener('click', function(event) {
            chrome.send('eulaOnInstallationSettingsPopupOpened');
            $('popup-overlay').hidden = false;
            $('installation-settings-ok-button').focus();
          });
      $('installation-settings-ok-button')
          .addEventListener('click', function(event) {
            $('popup-overlay').hidden = true;
          });

      $('cros-eula-frame')
          .addEventListener('contentload', this.onFrameLoad.bind(this));

      var self = this;
      $('usage-stats').addEventListener('click', function(event) {
        self.onUsageStatsClicked_($('usage-stats').checked);
        event.stopPropagation();
      });
      $('oobe-eula-md').screen = this;
    },

    /**
     * Event handler for $('usage-stats') click event.
     * @param {boolean} value $('usage-stats').checked value.
     */
    onUsageStatsClicked_: function(value) {
      this.context.set(CONTEXT_KEY_USAGE_STATS_ENABLED, value);
      this.commitContextChanges();
    },

    /**
     * Event handler that is invoked when 'chrome://terms' is loaded.
     */
    onFrameLoad: function() {
      $('accept-button').disabled = false;
      $('eula').classList.remove('eula-loading');
      // Initially, the back button is focused and the accept button is
      // disabled.
      // Move the focus to the accept button now but only if the user has not
      // moved the focus anywhere in the meantime.
      if (!$('back-button').blurred)
        $('accept-button').focus();
    },

    /**
     * Event handler that is invoked just before the screen is shown.
     * @param {object} data Screen init payload.
     */
    onBeforeShow: function() {
      $('eula').classList.add('eula-loading');
      $('accept-button').disabled = true;
      this.updateLocalizedContent();
    },

    /**
     * Header text of the screen.
     * @type {string}
     */
    get header() {
      return loadTimeData.getString('eulaScreenTitle');
    },

    /**
     * Buttons in oobe wizard's button strip.
     * @type {Array} Array of Buttons.
     */
    get buttons() {
      var buttons = [];

      var backButton = this.declareButton('back-button');
      backButton.textContent = loadTimeData.getString('back');
      buttons.push(backButton);

      var acceptButton = this.declareButton('accept-button');
      acceptButton.disabled = true;
      acceptButton.classList.add('preserve-disabled-state');
      acceptButton.textContent = loadTimeData.getString('acceptAgreement');
      acceptButton.addEventListener('click', function(e) {
        $('eula').classList.add('loading');  // Mark EULA screen busy.
        Oobe.clearErrors();
        e.stopPropagation();
      });
      buttons.push(acceptButton);

      return buttons;
    },

    /**
     * Returns a control which should receive an initial focus.
     */
    get defaultControl() {
      if (loadTimeData.getString('newOobeUI') == 'on')
        return $('oobe-eula-md');

      return $('accept-button').disabled ? $('back-button') :
                                           $('accept-button');
    },

    enableKeyboardFlow: function() {
      $('eula-chrome-credits-link').hidden = false;
      $('eula-chromeos-credits-link').hidden = false;
      $('eula-chrome-credits-link').addEventListener('click', function(event) {
        chrome.send('eulaOnChromeCredits');
      });
      $('eula-chromeos-credits-link')
          .addEventListener('click', function(event) {
            chrome.send('eulaOnChromeOSCredits');
          });
    },

    /**
     * This method takes care of switching to material-design OOBE.
     * @private
     */
    setMDMode_: function() {
      var useMDOobe = (loadTimeData.getString('newOobeUI') == 'on');
      $('oobe-eula-md').hidden = !useMDOobe;
      $('oobe-eula').hidden = useMDOobe;
    },

    /**
     * Updates localized content of the screen that is not updated via template.
     */
    updateLocalizedContent: function() {
      this.setMDMode_();

      // Reload the terms contents.
      if (!$('oobe-eula-md').hidden)
        $('oobe-eula-md').updateLocalizedContent();

      if (!$('oobe-eula').hidden) {
        this.loadEulaToWebview_($('cros-eula-frame'));
        if (this.oemEulaUrl_)
          loadUrlToWebview($('oem-eula-frame'), this.oemEulaUrl_);
      }
    },

    /**
     * Sets url for OEM Eula. Oem Eula UI is hidden if the url is null or empty.
     * @param {?string} oemEulaUrl The URL for OEM Eula.
     */
    setOemEulaUrl: function(oemEulaUrl) {
      this.oemEulaUrl_ = oemEulaUrl;

      if (this.oemEulaUrl_) {
        loadUrlToWebview($('oem-eula-frame'), this.oemEulaUrl_);
        $('eulas').classList.remove('one-column');
      } else {
        $('eulas').classList.add('one-column');
      }
    },

    /**
     * Sets TPM password.
     * @param {text} password TPM password to be shown.
     */
    setTpmPassword: function(password) {
      if (loadTimeData.getString('newOobeUI') == 'on') {
        $('oobe-eula-md').password = password;
      } else {
        $('tpm-busy').hidden = true;
        if (password.length) {
          $('tpm-password').textContent = password;
          $('tpm-password').hidden = false;
        } else {
          $('tpm-desc').hidden = true;
          $('tpm-desc-powerwash').hidden = false;
        }
      }
    },

    /**
     * Load Eula into the given webview. Online version is attempted first with
     * a timeout. If it fails to load, fallback to chrome://terms. The loaded
     * terms contents is then set to the webview via data url. Webview is
     * used as a sandbox for both online and local contents. Data url is
     * used for chrome://terms so that webview never needs to have the
     * privileged webui bindings.
     *
     * @param {!WebView} webview Webview element to host the terms.
     */
    loadEulaToWebview_: function(webview) {
      assert(webview.tagName === 'WEBVIEW');

      /**
       * Timeout to load online Eula.
       * @type {number}
       */
      var ONLINE_EULA_LOAD_TIMEOUT_IN_MS = 7000;

      /**
       * URL to use when online page is not available.
       * @type {string}
       */
      var TERMS_URL = 'chrome://terms';

      var loadBundledEula = function() {
        loadUrlToWebview(webview, TERMS_URL);
      };

      webview.addContentScripts([{
        name: 'clearAnchors',
        matches: ['<all_urls>'],
        js: CLEAR_ANCHORS_CONTENT_SCRIPT,
      }]);
      webview.addEventListener('contentload', () => {
        webview.executeScript(CLEAR_ANCHORS_CONTENT_SCRIPT);
      });

      var onlineEulaUrl = loadTimeData.getString('eulaOnlineUrl');
      if (!onlineEulaUrl) {
        loadBundledEula();
        return;
      }

      // Load online Eula with a timeout to fallback to the offline version.
      var pendingLoad = new PendingLoad(
          webview, onlineEulaUrl, ONLINE_EULA_LOAD_TIMEOUT_IN_MS,
          loadBundledEula);
      pendingLoad.start();
    },

    /**
     * Called when focus is returned.
     */
    onFocusReturned: function() {
      if ($('oobe-eula') && !$('oobe-eula').hidden) {
        $('oobe-eula').focus();
      } else if ($('oobe-eula-md') && !$('oobe-eula-md').hidden) {
        $('oobe-eula-md').focus();
      }
    },
  };
});
