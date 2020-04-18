// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('welcome', function() {
  'use strict';

  function onAccept(e) {
    chrome.send('handleActivateSignIn');
  }

  function onDecline(e) {
    chrome.send('handleUserDecline');
    e.preventDefault();
  }

  function initialize() {
    $('accept-button').addEventListener('click', onAccept);
    $('decline-button').addEventListener('click', onDecline);

    var logo = document.querySelector('.logo-icon');
    logo.onclick = function(e) {
      logo.animate(
          {
            transform: ['none', 'rotate(-10turn)'],
          },
          /** @type {!KeyframeEffectOptions} */ ({
            duration: 500,
            easing: 'cubic-bezier(1, 0, 0, 1)',
          }));
    };
  }

  return {initialize: initialize};
});

document.addEventListener('DOMContentLoaded', welcome.initialize);
