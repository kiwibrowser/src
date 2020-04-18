// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

'use strict';

/** @type {remoting.WindowShape} */
var windowShape;
/** @type {HTMLElement} */
var elementToPosition;
/** @type {sinon.TestStub} */
var currentWindowStub;
/** @type {sinon.TestStub} */
var isAppsV2Stub;

QUnit.module('WindowShape', {
  beforeEach: function() {
    windowShape = new remoting.WindowShape();
    elementToPosition =
        /** @type {HTMLElement} */ (document.createElement('div'));
    sinon.stub(elementToPosition, 'getBoundingClientRect')
         .returns({left: -50, top: -50, width: 50, height: 50});

    isAppsV2Stub = sinon.stub(base, 'isAppsV2', function() { return true; });
    currentWindowStub = sinon.stub(chrome.app.window, 'current', function() {
      return {
        setShape: base.doNothing
      };
    });
  },
  afterEach: function() {
    windowShape = null;
    elementToPosition = null;
    currentWindowStub.restore();
    isAppsV2Stub.restore();
  }
});

QUnit.test('centerToDesktop() handles no desktop window',
  function(assert) {
    var originalInnerWidth = window.innerWidth;
    var originalInnerHeight = window.innerHeight;
    window.innerHeight = 100;
    window.innerWidth = 100;

    windowShape.centerToDesktop(elementToPosition);
    assert.equal(elementToPosition.style.left, '25px');
    assert.equal(elementToPosition.style.top, '25px');

    window.innerWidth = originalInnerWidth;
    window.innerHeight = originalInnerHeight;
});

QUnit.test('centerToDesktop() handles single desktop window',
  function(assert) {
    windowShape.setDesktopRects([{left: 0, width: 100, top: 0, height: 100}]);
    windowShape.centerToDesktop(elementToPosition);
    assert.equal(elementToPosition.style.left, '25px');
    assert.equal(elementToPosition.style.top, '25px');
});

QUnit.test('centerToDesktop() handles multiple desktop window',
  function(assert) {
    windowShape.setDesktopRects([
      {left: 0, width: 10, top: 0, height: 10},
      {left: 90, width: 10, top: 90, height: 10}
    ]);

    windowShape.centerToDesktop(elementToPosition);
    assert.equal(elementToPosition.style.left, '25px');
    assert.equal(elementToPosition.style.top, '25px');
});

})();
