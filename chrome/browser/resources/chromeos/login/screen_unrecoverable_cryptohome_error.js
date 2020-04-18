// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

login.createScreen(
    'UnrecoverableCryptohomeErrorScreen', 'unrecoverable-cryptohome-error',
    function() {
      return {
        EXTERNAL_API: ['show', 'resumeAfterFeedbackUI'],

        /** @override */
        decorate: function() {
          this.card_ = $('unrecoverable-cryptohome-error-card');
          this.throbber_ = $('unrecoverable-cryptohome-error-busy');

          this.card_.addEventListener('done', function(e) {
            this.setLoading_(true);
            $('oobe').hidden = true;  // Hide while showing the feedback UI.
            chrome.send('sendFeedbackAndResyncUserData');
          }.bind(this));
        },

        /**
         * Sets whether to show the loading throbber.
         * @param {boolean} loading
         */
        setLoading_: function(loading) {
          this.card_.hidden = loading;
          this.throbber_.hidden = !loading;
        },

        /**
         * Show the unrecoverable cryptohome error screen to ask user permission
         * to collect a feedback report.
         */
        show: function() {
          this.setLoading_(false);

          Oobe.getInstance().headerHidden = true;
          Oobe.showScreen({id: SCREEN_UNRECOVERABLE_CRYPTOHOME_ERROR});
        },

        /**
         * Shows the loading UI after the feedback UI is dismissed.
         */
        resumeAfterFeedbackUI: function() {
          $('oobe').hidden = false;
        }
      };
    });
