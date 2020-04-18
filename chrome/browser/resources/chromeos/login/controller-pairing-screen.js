// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer((function() {
  /** @const */ var ICON_COLORS = [
    '#F0B9CB', '#F0ACC3', '#F098B6', '#F084A9', '#F06D99', '#F05287', '#F0467F',
    '#F03473', '#F01E65', '#F00051'
  ];
  return {
    is: 'pairing-device-list',

    properties: {
      devices: Array,

      selected: {type: String, notify: true},

      connecting: {type: Boolean, reflectToAttribute: true}
    },

    getStyleForDeviceIcon_: function(deviceName) {
      return 'color: ' + this.colorByName_(deviceName);
    },

    /* Returns pseudo-random color depending of hash of the |name|. */
    colorByName_: function(name) {
      var hash = 0;
      for (var i = 0; i < name.length; ++i)
        hash = (name.charCodeAt(i) + 31 * hash) | 0;
      return ICON_COLORS[hash % ICON_COLORS.length];
    }
  };
})());

Polymer({
  is: 'controller-pairing-page',

  behaviors: [Polymer.NeonAnimatableBehavior],

  properties: {
    animationConfig: {
      value: function() {
        return {
          'entry': [{name: 'fade-in-animation', node: this}],

          'exit': [{name: 'fade-out-animation', node: this}]
        };
      }
    }
  }
});

Polymer((function() {
  'use strict';

  // Keep these constants synced with corresponding constants defined in
  // controller_pairing_screen_actor.{h,cc}.
  /** @const */ var CONTEXT_KEY_CONTROLS_DISABLED = 'controlsDisabled';
  /** @const */ var CONTEXT_KEY_SELECTED_DEVICE = 'selectedDevice';
  /** @const */ var CONTEXT_KEY_ACCOUNT_ID = 'accountId';

  /** @const */ var ACTION_ENROLL = 'enroll';

  /** @const */ var PAGE_AUTHENTICATION = 'authentication';

  return {
    is: 'controller-pairing-screen',

    behaviors: [login.OobeScreenBehavior],

    properties:
        {selectedDevice: {type: String, observer: 'selectedDeviceChanged_'}},

    observers: ['deviceListChanged_(C.devices)'],

    ready: function() {
      /**
       * Workaround for
       * https://github.com/PolymerElements/neon-animation/issues/32
       * TODO(dzhioev): Remove when fixed in Polymer.
       */
      var pages = this.$.pages;
      delete pages._squelchNextFinishEvent;
      Object.defineProperty(pages, '_squelchNextFinishEvent', {
        get: function() {
          return false;
        }
      });
    },

    /** @override */
    initialize: function() {
      ['code', 'controlsDisabled', 'devices', 'enrollmentDomain', 'page']
          .forEach(this.registerBoundContextField, this);
      this.context.set(CONTEXT_KEY_CONTROLS_DISABLED, true);
      this.commitContextChanges();
    },

    i18n: function(args) {
      return loadTimeData.getStringF.apply(loadTimeData, args);
    },

    deviceListChanged_: function() {
      this.selectedDevice = this.context.get(CONTEXT_KEY_SELECTED_DEVICE, null);
    },

    selectedDeviceChanged_: function(selectedDevice) {
      this.context.set(
          CONTEXT_KEY_SELECTED_DEVICE, selectedDevice ? selectedDevice : '');
      this.commitContextChanges();
    },

    helpButtonClicked_: function() {
      console.error('Help is not implemented yet.');
    },

    getHostEnrollmentStepTitle_: function(domain) {
      return this.i18n(
          ['loginControllerPairingScreenEnrollmentInProgress', domain]);
    },

    getSuccessMessage_: function(selectedDevice) {
      return this.i18n(
          ['loginControllerPairingScreenSuccessText', selectedDevice]);
    }
  };
})());
