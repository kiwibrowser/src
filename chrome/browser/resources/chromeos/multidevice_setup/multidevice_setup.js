// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('multidevice_setup', function() {
  /** @enum {string} */
  const PageName = {
    FAILURE: 'setup-failed-page',
    SUCCESS: 'setup-succeeded-page',
    START: 'start-setup-page',
  };

  const MultiDeviceSetup = Polymer({
    is: 'multidevice-setup',

    properties: {
      /**
       * Element name of the currently visible page.
       *
       * @private {!PageName}
       */
      visiblePageName_: {
        type: String,
        value: PageName.START,
        // This notification is for testing purporses only
        notify: true,
      },

      /**
       * DOM Element corresponding to the visible page.
       *
       * @private {!StartSetupPageElement|!SetupSucceededPageElement|
       *           !SetupFailedPageElement}
       */
      visiblePage_: Object,

      /**
       * Array of objects representing all available MultiDevice hosts. Each
       * object contains the name of the device type (e.g. "Pixel XL") and its
       * public key.
       *
       * @private {Array<{name: string, publicKey: string}>}
       */
      devices_: Array,

      /**
       * Public key for the currently selected host device.
       *
       * Undefined if the no list of potential hosts has been received from mojo
       * service.
       *
       * @private {string|undefined}
       */
      selectedPublicKey_: String,
    },

    listeners: {
      'backward-navigation-requested': 'onBackwardNavigationRequested_',
      'forward-navigation-requested': 'onForwardNavigationRequested_',
    },

    ready: function() {
      this.mojoService_.getEligibleBetterTogetherHosts()
          .then((responseParams) => {
            if (responseParams['devices'].length >= 1)
              this.devices_ = responseParams['devices'];
            else
              console.warn('Potential host list is empty.');
          })
          .catch((error) => {
            console.warn('Potential host list was not retrieved. ' + error);
          });
    },

    /** @private */
    onBackwardNavigationRequested_: function() {
      this.closeUi_();
    },

    /** @private */
    onForwardNavigationRequested_: function() {
      switch (this.visiblePageName_) {
        case PageName.FAILURE:
          this.visiblePageName_ = PageName.START;
          break;
        case PageName.SUCCESS:
          this.closeUi_();
          break;
        case PageName.START:
          this.attemptToSetHost_();
          break;
      }
    },

    /** @private */
    closeUi_: function() {
      // TODO(jordynass): Implement closing UI.
      console.log('Closing WebUI');
      // This method is just for testing that the method was called.
      this.fire('ui-closed');
    },

    /**
     * Tries to set the device corresponding to the selected public key as the
     * user's Better Together host and navigates the UI to the success or
     * failure page depending on the response.
     *
     * @private
     */
    attemptToSetHost_: function() {
      this.mojoService_.setBetterTogetherHost(this.selectedPublicKey_)
          .then((responseParams) => {
            if (responseParams['responseCode'] ==
                multidevice_setup.SetBetterTogetherHostResponseCode.SUCCESS) {
              this.visiblePageName_ = PageName.SUCCESS;
            } else {
              this.visiblePageName_ = PageName.FAILURE;
            }
          })
          .catch((error) => {
            console.warn('Mojo service failure. ' + error);
          });
    },

    /**
     * Fake Mojo Service to substitute for MultiDeviceSetup mojo service.
     *
     * @private
     */
    // TODO(jordynass): Once mojo API is complete, make this into a real
    // multidevice setup mojo service object and annotate it with that type.
    mojoService_: multidevice_setup.FakeMojoService,
  });

  return {
    MultiDeviceSetup: MultiDeviceSetup,
  };
});
