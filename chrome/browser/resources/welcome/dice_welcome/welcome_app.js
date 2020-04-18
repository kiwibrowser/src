// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'welcome-app',

  welcomeBrowserProxy_: null,

  /** @override */
  ready: function() {
    this.welcomeBrowserProxy_ = welcome.WelcomeBrowserProxyImpl.getInstance();
  },

  /** @private */
  onAccept_: function() {
    this.welcomeBrowserProxy_.handleActivateSignIn();
  },

  /** @private */
  onDecline_: function() {
    this.welcomeBrowserProxy_.handleUserDecline();
  },

  /** @private */
  onLogoTap_: function() {
    this.$.logo.animate(
        {
          transform: ['none', 'rotate(-10turn)'],
        },
        /** @type {!KeyframeEffectOptions} */ ({
          duration: 500,
          easing: 'cubic-bezier(1, 0, 0, 1)',
        }));
  },
});
