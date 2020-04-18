// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'setup-succeeded-page',

  properties: {
    /** Overridden from UiPageContainerBehavior. */
    forwardButtonTextId: {
      type: String,
      value: 'done',
    },

    /** Overridden from UiPageContainerBehavior. */
    headerId: {
      type: String,
      value: 'setupSucceededPageHeader',
    },

    /** Overridden from UiPageContainerBehavior. */
    messageId: {
      type: String,
      value: 'setupSucceededPageMessage',
    },
  },

  behaviors: [
    UiPageContainerBehavior,
  ],

  /** @private */
  openSettings_: function() {
    // TODO(jordynass): Open MultiDevice settings when that page is built.
    console.log('Opening MultiDevice Settings');
    // This method is just for testing that the method was called
    this.fire('settings-opened');
  },

  /** @override */
  ready: function() {
    let linkElement = this.$$('#settings-link');
    linkElement.setAttribute('href', '#');
    linkElement.addEventListener('click', () => this.openSettings_());
  }
});
