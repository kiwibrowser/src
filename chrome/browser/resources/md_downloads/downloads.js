// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

window.addEventListener('load', function() {
  downloads.Manager.onLoad().then(function() {
    requestIdleCallback(function() {
      chrome.send(
          'metricsHandler:recordTime',
          ['Download.ResultsRenderedTime', window.performance.now()]);
      document.fonts.load('bold 12px Roboto');
    });
  });
});
