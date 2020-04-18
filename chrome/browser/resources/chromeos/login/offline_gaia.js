// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer((function() {
  var DEFAULT_EMAIL_DOMAIN = '@gmail.com';

  var TRANSITION_TYPE = {FORWARD: 0, BACKWARD: 1, NONE: 2};

  return {
    is: 'offline-gaia',

    behaviors: [I18nBehavior],

    properties: {
      disabled: {
        type: Boolean,
        value: false,
      },

      showEnterpriseMessage: {
        type: Boolean,
        value: false,
      },

      domain: {
        type: String,
        observer: 'onDomainChanged_',
      },

      emailDomain: String,

      activeSection: {
        type: String,
        value: 'emailSection',
      },

      animationInProgress: Boolean,

      /**
       * Controls GLIF MM mode.
       */
      glifMode: {
        type: Boolean,
        value: false,
      },
    },

    attached: function() {
      if (this.isRTL_())
        this.setAttribute('rtl', '');
    },

    focus: function() {
      if (this.isEmailSectionActive_())
        this.$$('#emailInput').focus();
      else
        this.$$('#passwordInput').focus();
    },

    back: function() {
      this.switchToEmailCard(true /* animated */);
    },

    onDomainChanged_: function() {
      this.$$('#managedBy').textContent =
          loadTimeData.getStringF('enterpriseInfoMessage', this.domain);
      this.showEnterpriseMessage = !!this.domain.length;
    },

    onAnimationFinish_: function() {
      this.fire('backButton', !this.isEmailSectionActive_());
      this.focus();
    },

    onForgotPasswordClicked_: function() {
      this.disabled = true;
      this.fire('dialogShown');
      this.$$('#forgotPasswordDlg').showModal();
      this.$$('#passwordCard').classList.add('full-disabled');
    },

    onForgotPasswordCloseTap_: function() {
      this.$$('#forgotPasswordDlg').close();
    },

    onDialogOverlayClosed_: function() {
      this.fire('dialogHidden');
      this.disabled = false;
      this.$$('#passwordCard').classList.remove('full-disabled');
    },

    setEmail: function(email) {
      if (email) {
        if (this.emailDomain)
          email = email.replace(this.emailDomain, '');

        this.switchToPasswordCard(email, false /* animated */);
        this.$$('#passwordInput').isInvalid = true;
        this.fire('backButton', true);
      } else {
        this.$$('#emailInput').value = '';
        this.switchToEmailCard(false /* animated */);
      }
    },

    isRTL_: function() {
      return !!document.querySelector('html[dir=rtl]');
    },

    isEmailSectionActive_: function() {
      return this.activeSection == 'emailSection';
    },

    switchToEmailCard(animated) {
      this.$$('#passwordInput').value = '';
      this.$$('#passwordInput').isInvalid = false;
      this.$$('#emailInput').isInvalid = false;
      if (this.isEmailSectionActive_())
        return;

      this.animationInProgress = animated;
      this.activeSection = 'emailSection';
    },

    switchToPasswordCard(email, animated) {
      this.$$('#emailInput').value = email;
      if (email.indexOf('@') === -1) {
        if (this.emailDomain)
          email = email + this.emailDomain;
        else
          email = email + DEFAULT_EMAIL_DOMAIN;
      }
      this.$$('#passwordHeader').email = email;
      if (!this.isEmailSectionActive_())
        return;

      this.animationInProgress = animated;
      this.activeSection = 'passwordSection';
    },

    onSlideAnimationEnd_: function() {
      this.animationInProgress = false;
      this.focus();
    },

    onEmailSubmitted_: function() {
      if (this.$$('#emailInput').checkValidity()) {
        this.switchToPasswordCard(
            this.$$('#emailInput').value, true /* animated */);
      } else {
        this.$$('#emailInput').focus();
      }
    },

    onPasswordSubmitted_: function() {
      if (!this.$$('#passwordInput').checkValidity())
        return;
      var msg = {
        'useOffline': true,
        'email': this.$$('#passwordHeader').email,
        'password': this.$$('#passwordInput').value
      };
      this.$$('#passwordInput').value = '';
      this.fire('authCompleted', msg);
    },

    onBackButtonClicked_: function() {
      if (!this.isEmailSectionActive_()) {
        this.switchToEmailCard(true);
      } else {
        this.fire('offline-gaia-cancel');
      }
    },
  };
})());
