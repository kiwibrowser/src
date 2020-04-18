// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('options', function() {
  function Slow() {}
  cr.addSingletonGetter(Slow);

  Slow.prototype = {initialized_: false};

  Slow.initialize = function() {
    $('slow-disable').addEventListener('click', function(event) {
      Slow.disableTracing();
    });
    $('slow-enable').addEventListener('click', function(event) {
      Slow.enableTracing();
    });
    this.initialized_ = true;
  };

  Slow.disableTracing = function() {
    chrome.send('disableTracing');
  };

  Slow.enableTracing = function() {
    chrome.send('enableTracing');
  };

  Slow.tracingPrefChanged = function(enabled) {
    $('slow-disable').hidden = !enabled;
    $('slow-enable').hidden = enabled;
  };

  // Export
  return {Slow: Slow};
});

function load() {
  options.Slow.initialize();
  chrome.send('loadComplete');
}

document.addEventListener('DOMContentLoaded', load);
