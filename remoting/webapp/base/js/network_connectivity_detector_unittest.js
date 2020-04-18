// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

'use strict';

/** @type {remoting.NetworkConnectivityDetector} */
var detector;

/** @type {sinon.TestStub} */
var onlineStub;

function setXhrStatus(/** number */ status) {
  remoting.MockXhr.setEmptyResponseFor(
      'GET', remoting.NetworkConnectivityDetector.getUrlForTesting(), status);

}

QUnit.module('NetworkConnectivityDetector', {
  beforeEach: function() {
    remoting.settings = new remoting.Settings();
    remoting.MockXhr.activate();
    onlineStub = sinon.stub(base, 'isOnline');
    detector = remoting.NetworkConnectivityDetector.create();
  },
  afterEach: function() {
    onlineStub.restore();
    base.dispose(detector);
    detector = null;
    remoting.MockXhr.restore();
    remoting.settings = null;
  }
});

QUnit.test('waitForOnline() window.onLine = true', function(assert){
  onlineStub.returns(true);
  setXhrStatus(200);
  return detector.waitForOnline().then(function() {
    assert.ok(true);
  });
});

QUnit.test('waitForOnline() window.onLine = false', function(assert){
  onlineStub.returns(false);
  setXhrStatus(200);
  var promise = detector.waitForOnline().then(function() {
    assert.ok(true);
  });

  Promise.resolve().then(function() {
    onlineStub.returns(true);
    window.dispatchEvent(new CustomEvent('online'));
  });
  return promise;
});

QUnit.test('waitForOnline() use one single XHR for multiple clients',
    function(assert){
  onlineStub.returns(true);

  // We only set one single Xhr response. The next Xhr will fail.
  setXhrStatus(200);

  var promise1 = detector.waitForOnline();
  var promise2 = detector.waitForOnline();
  var promise3 = detector.waitForOnline();
  return Promise.all([promise1, promise2, promise3]).then(function(){
    assert.ok(true);
  });
});

QUnit.test('cancel() rejects the promise', function(assert){
  onlineStub.returns(false);
  setXhrStatus(200);
  var promise = detector.waitForOnline().then(function() {
    assert.ok(true);
  }).then(function(){
    assert.ok(false, 'Expects the promise to reject with Canceled');
  }).catch(function(/** * */ reason) {
    var error = /** @type {remoting.Error} */ (reason);
    assert.ok(error.hasTag(remoting.Error.Tag.CANCELLED));
  });

  detector.cancel();
  Promise.resolve().then(function() {
    onlineStub.returns(true);
    window.dispatchEvent(new CustomEvent('online'));
  });

  return promise;
});

})();
