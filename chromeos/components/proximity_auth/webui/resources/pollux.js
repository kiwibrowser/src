/* Copyright 2017 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

Pollux = {
  controller_: null,

  /**
   * Initializes the debug UI.
   */
  init: function() {
    Pollux.controller_ = new PolluxController();
  }
};

/**
 * Interface with the native WebUI component for Pollux events.
 */
PolluxInterface = {
  /**
   * Called when a new challenge is created.
   */
  onChallengeCreated: function(challenge, eid, sessionKey) {
    if (Pollux.controller_) {
      Pollux.controller_.add(log);
    }
  },
};

class PolluxController {
  constructor() {
    this.masterKeyInput_ = document.getElementById('master-key-input');
    this.challengeButton_ = document.getElementById('challenge-button');
    this.challengeInput_ = document.getElementById('challenge-input');
    this.eidInput_ = document.getElementById('eid-input');
    this.sessionKeyInput_ = document.getElementById('session-key-input');
    this.assertionButton = document.getElementById('assertion-button');
    this.authStateElement_ = document.getElementById('authenticator-state');

    this.challengeButton_.onclick = this.createNewChallenge_.bind(this);
    this.assertionButton_.onclick = this.startAssertion_.bind(this);
  }
}

document.addEventListener('DOMContentLoaded', function() {
  WebUI.onWebContentsInitialized();
  Logs.init();
});
