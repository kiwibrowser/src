// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('chrome.sync.user_events', function() {
  function write() {
    chrome.sync.writeUserEvent(
      $('event-time-usec-input').value,
      $('navigation-id-input').value);
  }

  function onLoad() {
    $('create-event-button').addEventListener('click', write);
  }

  return {
    onLoad: onLoad
  };
});

document.addEventListener(
    'DOMContentLoaded',
    chrome.sync.user_events.onLoad,
    false);
