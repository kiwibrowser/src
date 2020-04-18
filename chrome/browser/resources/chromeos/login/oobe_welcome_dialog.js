// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

{
  var LONG_TOUCH_TIME_MS = 1000;

  function LongTouchDetector(element, callback) {
    this.callback_ = callback;

    element.addEventListener('touchstart', this.onTouchStart_.bind(this));
    element.addEventListener('touchend', this.killTimer_.bind(this));
    element.addEventListener('touchcancel', this.killTimer_.bind(this));

    element.addEventListener('mousedown', this.onTouchStart_.bind(this));
    element.addEventListener('mouseup', this.killTimer_.bind(this));
    element.addEventListener('mouseleave', this.killTimer_.bind(this));
  }

  LongTouchDetector.prototype = {
    /**
     * This is timeout ID used to kill window timeout that fires "detected"
     * callback if touch event was cancelled.
     *
     * @private {number|null}
     */
    timeoutId_: null,

    /**
     *  window.setTimeout() callback.
     *
     * @private
     */
    onTimeout_: function() {
      this.killTimer_();
      this.callback_();
    },

    /**
     * @private
     */
    onTouchStart_: function() {
      this.killTimer_();
      this.timeoutId_ = window.setTimeout(
          this.onTimeout_.bind(this, this.attempt_), LONG_TOUCH_TIME_MS);
    },

    /**
     * @private
     */
    killTimer_: function() {
      if (this.timeoutId_ === null)
        return;

      window.clearTimeout(this.timeoutId_);
      this.timeoutId_ = null;
    },
  };

  Polymer({
    is: 'oobe-welcome-dialog',

    behaviors: [I18nBehavior],

    properties: {
      /**
       * Currently selected system language (display name).
       */
      currentLanguage: {
        type: String,
        value: '',
      },

      /**
       * Controls visibility of "Timezone" button.
       */
      timezoneButtonVisible: {
        type: Boolean,
        value: false,
      },

      /**
       * Controls displaying of "Enable debugging features" link.
       */
      debuggingLinkVisible: Boolean,

      /**
       * True when in tablet mode.
       */
      isInTabletMode: Boolean,

      /**
       * True when scree orientation is portraight.
       */
      isInPortraitMode: Boolean,
    },

    /**
     * @private {LongTouchDetector}
     */
    titleLongTouchDetector_: null,

    /**
     * This is stored ID of currently focused element to restore id on returns
     * to this dialog from Language / Timezone Selection dialogs.
     */
    focusedElement_: 'languageSelectionButton',

    onLanguageClicked_: function() {
      this.focusedElement_ = 'languageSelectionButton';
      this.fire('language-button-clicked');
    },

    onAccessibilityClicked_: function() {
      this.focusedElement_ = 'accessibilitySettingsButton';
      this.fire('accessibility-button-clicked');
    },

    onTimezoneClicked_: function() {
      this.focusedElement_ = 'timezoneSettingsButton';
      this.fire('timezone-button-clicked');
    },

    onNextClicked_: function() {
      this.focusedElement_ = 'welcomeNextButton';
      this.fire('next-button-clicked');
    },

    onDebuggingLinkClicked_: function() {
      chrome.send(
          'login.NetworkScreen.userActed', ['connect-debugging-features']);
    },

    /*
     * This is called from titleLongTouchDetector_ when long touch is detected.
     *
     * @private
     */
    onTitleLongTouch_: function() {
      this.fire('launch-advanced-options');
    },

    attached: function() {
      this.titleLongTouchDetector_ = new LongTouchDetector(
          this.$.title, this.onTitleLongTouch_.bind(this));
      this.focus();
    },

    focus: function() {
      this.onWindowResize();
      var focusedElement = this.$[this.focusedElement_];
      if (focusedElement)
        focusedElement.focus();
    },

    /**
     * This is called from oobe_welcome when this dialog is shown.
     */
    show: function() {
      this.focus();
    },

    /**
     * This function formats message for labels.
     * @param String label i18n string ID.
     * @param String parameter i18n string parameter.
     * @private
     */
    formatMessage_: function(label, parameter) {
      return loadTimeData.getStringF(label, parameter);
    },

    /**
     * Window-resize event listener (delivered through the display_manager).
     */
    onWindowResize: function() {
      this.isInPortraitMode = window.innerHeight > window.innerWidth;
    },
  });
}
