// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Oobe Terms of Service screen implementation.
 */

login.createScreen('TermsOfServiceScreen', 'terms-of-service', function() {
  return {
    EXTERNAL_API:
        ['setDomain', 'setTermsOfServiceLoadError', 'setTermsOfService'],

    /**
     * Updates headings on the screen to indicate that the Terms of Service
     * being shown belong to |domain|.
     * @param {string} domain The domain whose Terms of Service are being shown.
     */
    setDomain: function(domain) {
      $('tos-heading').textContent =
          loadTimeData.getStringF('termsOfServiceScreenHeading', domain);
      $('tos-subheading').textContent =
          loadTimeData.getStringF('termsOfServiceScreenSubheading', domain);
      $('tos-content-heading').textContent =
          loadTimeData.getStringF('termsOfServiceContentHeading', domain);
    },

    /**
     * Displays an error message on the Terms of Service screen. Called when the
     * download of the Terms of Service has failed.
     */
    setTermsOfServiceLoadError: function() {
      this.classList.remove('tos-loading');
      this.classList.add('error');
    },

    /**
     * Displays the given |termsOfService|, enables the accept button and moves
     * the focus to it.
     * @param {string} termsOfService The terms of service, as plain text.
     */
    setTermsOfService: function(termsOfService) {
      this.classList.remove('tos-loading');
      // Load the Terms of Service text as data url in a <webview> to ensure
      // the content from the web does not load within the privileged WebUI
      // process.
      $('tos-content-main').src = 'data:text/html;charset=utf-8,' +
          encodeURIComponent('<style>' +
                             'body {' +
                             '  font-family: Roboto, sans-serif;' +
                             '  font-size: 14px;' +
                             '  margin : 0;' +
                             '  padding : 0;' +
                             '  white-space: pre-wrap;' +
                             '}' +
                             '</style>' +
                             '<body>' + termsOfService + '<body>');
      $('tos-accept-button').disabled = false;
      // Initially, the back button is focused and the accept button is
      // disabled.
      // Move the focus to the accept button now but only if the user has not
      // moved the focus anywhere in the meantime.
      if (!$('tos-back-button').blurred)
        $('tos-accept-button').focus();
    },

    /**
     * Buttons in Oobe wizard's button strip.
     * @type {array} Array of Buttons.
     */
    get buttons() {
      var buttons = [];

      var backButton = this.ownerDocument.createElement('button');
      backButton.id = 'tos-back-button';
      backButton.textContent =
          loadTimeData.getString('termsOfServiceBackButton');
      backButton.addEventListener('click', function(event) {
        $('tos-back-button').disabled = true;
        $('tos-accept-button').disabled = true;
        chrome.send('termsOfServiceBack');
      });
      backButton.addEventListener('blur', function(event) {
        this.blurred = true;
      });
      buttons.push(backButton);

      var acceptButton = this.ownerDocument.createElement('button');
      acceptButton.id = 'tos-accept-button';
      acceptButton.disabled = this.classList.contains('tos-loading');
      acceptButton.classList.add('preserve-disabled-state');
      acceptButton.textContent =
          loadTimeData.getString('termsOfServiceAcceptButton');
      acceptButton.addEventListener('click', function(event) {
        $('tos-back-button').disabled = true;
        $('tos-accept-button').disabled = true;
        chrome.send('termsOfServiceAccept');
      });
      buttons.push(acceptButton);

      return buttons;
    },

    /**
     * Returns the control which should receive initial focus.
     */
    get defaultControl() {
      return $('tos-accept-button').disabled ? $('tos-back-button') :
                                               $('tos-accept-button');
    },

    /**
     * Event handler that is invoked just before the screen is shown.
     * @param {object} data Screen init payload.
     */
    onBeforeShow: function(data) {
      Oobe.getInstance().headerHidden = true;
    }
  };
});
