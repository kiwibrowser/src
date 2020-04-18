// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Polymer element for displaying material design assistant
 * value prop screen.
 */

class HtmlSanitizer {
  /**
   * @param {Set} allowedTags set of whitelisted tags.
   */
  constructor(allowedTags) {
    this.allowedTags = allowedTags;
  }

  /**
   * Sanitize the html snippet.
   * Only allow the tags in allowedTags.
   *
   * @param {string} content the html snippet to be sanitized.
   * @return {string} sanitized html snippet.
   *
   * @public
   */
  sanitizeHtml(content) {
    var doc = document.implementation.createHTMLDocument();
    var div = doc.createElement('div');
    div.innerHTML = content;
    return this.sanitizeNode_(doc, div).innerHTML;
  }

  /**
   * Sanitize the html node.
   *
   * @param {Document} doc document object for sanitize use.
   * @param {Element} node the DOM element to be sanitized.
   * @return {Element} sanitized DOM element.
   *
   * @private
   */
  sanitizeNode_(doc, node) {
    var name = node.nodeName.toLowerCase();
    if (name == '#text') {
      return node;
    }
    if (!this.allowedTags.has(name)) {
      return doc.createTextNode('');
    }

    var copy = doc.createElement(name);
    // Only allow 'href' attribute for tag 'a'.
    if (name == 'a' && node.attributes.length == 1 &&
        node.attributes.item(0).name == 'href') {
      copy.setAttribute('href', node.getAttribute('href'));
    }

    while (node.childNodes.length > 0) {
      var child = node.removeChild(node.childNodes[0]);
      copy.appendChild(this.sanitizeNode_(doc, child));
    }
    return copy;
  }
}

Polymer({
  is: 'assistant-value-prop-md',

  properties: {
    /**
     * Buttons are disabled when the webview content is loading.
     */
    buttonsDisabled: {
      type: Boolean,
      value: true,
    },

    /**
     * System locale.
     */
    locale: {
      type: String,
    },

    /**
     * Default url for locale en_us.
     */
    defaultUrl: {
      type: String,
      value:
          'https://www.gstatic.com/opa-android/oobe/a02187e41eed9e42/v1_omni_en_us.html',
    },

    /**
     * Whether there are more consent contents to show.
     */
    moreContents: {
      type: Boolean,
      value: false,
    },
  },

  /**
   * Whether try to reload with the default url when a 404 error occurred.
   * @type {boolean}
   * @private
   */
  reloadWithDefaultUrl_: false,

  /**
   * Whether an error occurs while the webview is loading.
   * @type {boolean}
   * @private
   */
  loadingError_: false,

  /**
   * Timeout ID for loading animation.
   * @type {number}
   * @private
   */
  animationTimeout_: null,

  /**
   * Timeout ID for loading (will fire an error).
   * @type {number}
   * @private
   */
  loadingTimeout_: null,

  /**
   * The value prop webview object.
   * @type {Object}
   * @private
   */
  valuePropView_: null,

  /**
   * Whether the screen has been initialized.
   * @type {boolean}
   * @private
   */
  initialized_: false,

  /**
   * Whether the response header has been received for the value prop view.
   * @type {boolean}
   * @private
   */
  headerReceived_: false,

  /**
   * Whether the webview has been successfully loaded.
   * @type {boolean}
   * @private
   */
  webViewLoaded_: false,

  /**
   * Whether all the setting zippy has been successfully loaded.
   * @type {boolean}
   * @private
   */
  settingZippyLoaded_: false,

  /**
   * Whether all the consent text strings has been successfully loaded.
   * @type {boolean}
   * @private
   */
  consentStringLoaded_: false,

  /**
   * Sanitizer used to sanitize html snippets.
   * @type {HtmlSanitizer}
   * @private
   */
  sanitizer_:
      new HtmlSanitizer(new Set(['b', 'i', 'br', 'p', 'a', 'ul', 'li', 'div'])),

  /**
   * On-tap event handler for skip button.
   *
   * @private
   */
  onSkipTap_: function() {
    chrome.send('AssistantValuePropScreen.userActed', ['skip-pressed']);
  },

  /**
   * On-tap event handler for retry button.
   *
   * @private
   */
  onRetryTap_: function() {
    this.reloadWebView();
  },

  /**
   * On-tap event handler for more button.
   *
   * @private
   */
  onMoreTap_: function() {
    this.removeClass_('value-prop-loading');
    this.removeClass_('value-prop-loaded');
    this.removeClass_('value-prop-error');
    this.addClass_('value-prop-more');
    this.moreContents = false;
    this.$['next-button'].focus();
  },

  /**
   * On-tap event handler for next button.
   *
   * @private
   */
  onNextTap_: function() {
    chrome.send('AssistantValuePropScreen.userActed', ['next-pressed']);
  },

  /**
   * Add class to the list of classes of root elements.
   * @param {string} className class to add
   *
   * @private
   */
  addClass_: function(className) {
    this.$['value-prop-dialog'].classList.add(className);
  },

  /**
   * Remove class to the list of classes of root elements.
   * @param {string} className class to remove
   *
   * @private
   */
  removeClass_: function(className) {
    this.$['value-prop-dialog'].classList.remove(className);
  },

  /**
   * Reloads value prop webview.
   */
  reloadWebView: function() {
    this.loadingError_ = false;
    this.headerReceived_ = false;
    this.valuePropView_.src =
        'https://www.gstatic.com/opa-android/oobe/a02187e41eed9e42/v1_omni_' +
        this.locale + '.html';

    window.clearTimeout(this.animationTimeout_);
    window.clearTimeout(this.loadingTimeout_);
    this.removeClass_('value-prop-loaded');
    this.removeClass_('value-prop-error');
    this.removeClass_('value-prop-more');
    this.addClass_('value-prop-loading');
    this.buttonsDisabled = true;

    this.animationTimeout_ = window.setTimeout(function() {
      this.addClass_('value-prop-loading-animation');
    }.bind(this), 500);
    this.loadingTimeout_ = window.setTimeout(function() {
      this.onWebViewErrorOccurred();
    }.bind(this), 5000);
  },

  /**
   * Handles event when value prop webview cannot be loaded.
   */
  onWebViewErrorOccurred: function(details) {
    this.loadingError_ = true;
    window.clearTimeout(this.animationTimeout_);
    window.clearTimeout(this.loadingTimeout_);
    this.removeClass_('value-prop-loading-animation');
    this.removeClass_('value-prop-loading');
    this.removeClass_('value-prop-loaded');
    this.removeClass_('value-prop-more');
    this.addClass_('value-prop-error');

    this.buttonsDisabled = false;
    this.$['retry-button'].focus();
  },

  /**
   * Handles event when value prop webview is loaded.
   */
  onWebViewContentLoad: function(details) {
    if (details == null) {
      return;
    }
    if (this.loadingError_ || !this.headerReceived_) {
      return;
    }
    if (this.reloadWithDefaultUrl_) {
      this.valuePropView_.src = this.defaultUrl;
      this.headerReceived_ = false;
      this.reloadWithDefaultUrl_ = false;
      return;
    }

    this.webViewLoaded_ = true;
    if (this.settingZippyLoaded_ && this.consentStringLoaded_) {
      this.onPageLoaded();
    }
  },

  /**
   * Handles event when webview request headers received.
   */
  onWebViewHeadersReceived: function(details) {
    if (details == null) {
      return;
    }
    this.headerReceived_ = true;
    if (details.statusCode == '404') {
      if (details.url != this.defaultUrl) {
        this.reloadWithDefaultUrl_ = true;
        return;
      } else {
        this.onWebViewErrorOccurred();
      }
    } else if (details.statusCode != '200') {
      this.onWebViewErrorOccurred();
    }
  },

  /**
   * Reload the page with the given consent string text data.
   */
  reloadContent: function(data) {
    this.$['intro-text'].textContent = data['valuePropIntro'];
    this.$['identity'].textContent = data['valuePropIdentity'];
    this.$['next-button-text'].textContent = data['valuePropNextButton'];
    this.$['skip-button-text'].textContent = data['valuePropSkipButton'];
    this.$['footer-text'].innerHTML =
        this.sanitizer_.sanitizeHtml(data['valuePropFooter']);

    this.consentStringLoaded_ = true;
    if (this.webViewLoaded_ && this.settingZippyLoaded_) {
      this.onPageLoaded();
    }
  },

  /**
   * Add a setting zippy with the provided data.
   */
  addSettingZippy: function(zippy_data) {
    for (var i in zippy_data) {
      var data = zippy_data[i];
      var zippy = document.createElement('setting-zippy');
      zippy.setAttribute(
          'icon-src',
          'data:text/html;charset=utf-8,' +
              encodeURIComponent(zippy.getWrappedIcon(data['iconUri'])));
      if (i == 0)
        zippy.setAttribute('hide-line', true);

      var title = document.createElement('div');
      title.className = 'zippy-title';
      title.innerHTML = this.sanitizer_.sanitizeHtml(data['title']);
      zippy.appendChild(title);

      var description = document.createElement('div');
      description.className = 'zippy-description';
      description.innerHTML = this.sanitizer_.sanitizeHtml(data['description']);
      zippy.appendChild(description);

      var additional = document.createElement('div');
      additional.className = 'zippy-additional';
      additional.innerHTML =
          this.sanitizer_.sanitizeHtml(data['additionalInfo']);
      zippy.appendChild(additional);

      if (i == 0) {
        this.$['insertion-point-0'].appendChild(zippy);
      } else {
        this.$['insertion-point-1'].appendChild(zippy);
      }
    }

    if (zippy_data.length >= 2)
      this.moreContents = true;

    this.settingZippyLoaded_ = true;
    if (this.webViewLoaded_ && this.consentStringLoaded_) {
      this.onPageLoaded();
    }
  },

  /**
   * Handles event when all the page content has been loaded.
   */
  onPageLoaded: function() {
    window.clearTimeout(this.animationTimeout_);
    window.clearTimeout(this.loadingTimeout_);
    this.removeClass_('value-prop-loading-animation');
    this.removeClass_('value-prop-loading');
    this.removeClass_('value-prop-error');
    this.removeClass_('value-prop-more');
    this.addClass_('value-prop-loaded');

    this.buttonsDisabled = false;
    if (this.moreContents) {
      this.$['more-button'].focus();
    } else {
      this.$['next-button'].focus();
    }
  },

  /**
   * Signal from host to show the screen.
   */
  onShow: function() {
    var requestFilter = {urls: ['<all_urls>'], types: ['main_frame']};
    this.valuePropView_ = this.$['value-prop-view'];
    this.locale =
        loadTimeData.getString('locale').replace('-', '_').toLowerCase();

    if (!this.initialized_) {
      this.valuePropView_.request.onErrorOccurred.addListener(
          this.onWebViewErrorOccurred.bind(this), requestFilter);
      this.valuePropView_.request.onHeadersReceived.addListener(
          this.onWebViewHeadersReceived.bind(this), requestFilter);
      this.valuePropView_.request.onCompleted.addListener(
          this.onWebViewContentLoad.bind(this), requestFilter);

      this.valuePropView_.addContentScripts([{
        name: 'stripLinks',
        matches: ['<all_urls>'],
        js: {
          code: 'document.querySelectorAll(\'a\').forEach(' +
              'function(anchor){anchor.href=\'javascript:void(0)\';})'
        },
        run_at: 'document_end'
      }]);

      this.initialized_ = true;
    }

    this.reloadWebView();
  },
});
