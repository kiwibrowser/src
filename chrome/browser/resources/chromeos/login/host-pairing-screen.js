// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'host-pairing-page',

  behaviors: [Polymer.NeonAnimatableBehavior]
});

Polymer((function() {
  'use strict';

  /** @const */ var CALLBACK_CONTEXT_READY = 'contextReady';

  return {
    is: 'host-pairing-screen',

    behaviors: [login.OobeScreenBehavior],

    onBeforeShow: function() {
      Oobe.getInstance().headerHidden = true;
    },

    /** @override */
    initialize: function() {
      ['code', 'deviceName', 'enrollmentDomain', 'page', 'enrollmentError']
          .forEach(this.registerBoundContextField, this);
      this.send(CALLBACK_CONTEXT_READY);
    },

    i18n: function(args) {
      return loadTimeData.getStringF.apply(loadTimeData, args);
    },

    getEnrollmentStepTitle_: function(enrollmentDomain) {
      return this.i18n(
          ['loginHostPairingScreenEnrollingTitle', enrollmentDomain]);
    }
  };
})());
