// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.fileManagerPrivate = {
  isPiexLoaderEnabled: function(callback) {
    callback(true);
  }
};

var MockModule = cr.ui.define('div');
MockModule.prototype = Object.create(HTMLDivElement.prototype);
MockModule.prototype.constructor = MockModule;

MockModule.prototype.setBeforeMessageCallback = function(callback) {
  this.onBeforeMessageCallback_ = callback;
};

MockModule.prototype.decorate = function() {
  this.onBeforeMessageCallback_ = null;
  setTimeout(function() {
    this.dispatchEvent(new Event('load', {bubbles: true}));
  }.bind(this));
};

MockModule.prototype.postMessage = function(message) {
  setTimeout(function() {
    this.dispatchEvent(new Event('load', {bubbles: true}));
    if (this.onBeforeMessageCallback_)
      this.onBeforeMessageCallback_();

    var e = new CustomEvent('message', {bubbles: true});
    e.data = {id: message.id, thumbnail: 'thumbnail-data', orientation: 1};
    this.dispatchEvent(e);
  }.bind(this));
};

function testUnloadingAfterTimeout(callback) {
  var loadCount = 0;
  var unloadCount = 0;

  var unloadPromiseFulfill = null;
  var unloadPromise = new Promise(function(onFulfill, onReject) {
    unloadPromiseFulfill = onFulfill;
  });

  var mockModule;
  var loader = new PiexLoader(
      function() {
        loadCount++;
        mockModule = new MockModule();
        mockModule.setBeforeMessageCallback(function() {
          // Simulate slow NaCl module response taking more than the idle
          // timeout.
          loader.simulateIdleTimeoutPassedForTests();
        });
        return mockModule;
      },
      function(module) {
        unloadCount++;
        unloadPromiseFulfill();
      },
      60 * 1000);

  reportPromise(
      Promise.all([
        loader.load('http://foobar/test.raw')
            .then(function(data) {
              assertEquals(0, data.id);
              assertEquals('thumbnail-data', data.thumbnail);
              assertEquals(0, unloadCount);
              assertEquals(1, loadCount);
              return loader.load('http://foobar/another.raw')
            })
            .then(function(data) {
              // The NaCl module is not unloaded, as the next request came
              // before the idling timeout passed.
              assertEquals(1, data.id);
              assertEquals('thumbnail-data', data.thumbnail);
              assertEquals(0, unloadCount);
              assertEquals(1, loadCount);
            })
            .then(function() {
              // Simulate idling while no request are in progress. It should
              // unload the NaCl module.
              loader.simulateIdleTimeoutPassedForTests();
              assertEquals(1, unloadCount);
              return loader.load('http://foobar/chocolate.raw')
            })
            .then(function(data) {
              // Following requests should reload the NaCl module.
              assertEquals(2, data.id);
              assertEquals('thumbnail-data', data.thumbnail);
              assertEquals(1, unloadCount);
              assertEquals(2, loadCount);
            }),
        unloadPromise
      ]),
      callback);
};
