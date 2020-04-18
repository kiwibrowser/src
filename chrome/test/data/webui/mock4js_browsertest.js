// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Tests for Mock4JS to ensure that expectations pass or fail as
 * expected using the test framework.
 * @author scr@chromium.org (Sheridan Rawlins)
 * @see test_api.js
 */

/**
 * Test fixture for Mock4JS testing.
 * @constructor
 * @extends {testing.Test}
 */
function Mock4JSWebUITest() {}

Mock4JSWebUITest.prototype = {
  __proto__: testing.Test.prototype,

  /** @inheritDoc */
  browsePreload: DUMMY_URL,

  /** @inheritDoc */
  setUp: function() {
    function MockHandler() {}
    MockHandler.prototype = {
      callMe: function() {},
    };
    this.mockHandler = mock(MockHandler);
  },
};

TEST_F('Mock4JSWebUITest', 'CalledExpectPasses', function() {
  this.mockHandler.expects(once()).callMe();
  this.mockHandler.proxy().callMe();
});

TEST_F('Mock4JSWebUITest', 'CalledTwiceExpectTwice', function() {
  this.mockHandler.expects(exactly(2)).callMe();
  var proxy = this.mockHandler.proxy();
  proxy.callMe();
  proxy.callMe();
});

/**
 * Test fixture for Mock4JS testing which is expected to fail.
 * @constructor
 * @extends {Mock4JSWebUITest}
 */
function Mock4JSWebUITestFails() {}

Mock4JSWebUITestFails.prototype = {
  __proto__: Mock4JSWebUITest.prototype,

  /** @inheritDoc */
  testShouldFail: true,
};

TEST_F('Mock4JSWebUITestFails', 'NotCalledExpectFails', function() {
  this.mockHandler.expects(once()).callMe();
});

TEST_F('Mock4JSWebUITestFails', 'CalledTwiceExpectOnceFails', function() {
  this.mockHandler.expects(once()).callMe();
  var proxy = this.mockHandler.proxy();
  proxy.callMe();
  proxy.callMe();
});

TEST_F('Mock4JSWebUITestFails', 'CalledOnceExpectTwiceFails', function() {
  this.mockHandler.expects(exactly(2)).callMe();
  var proxy = this.mockHandler.proxy();
  proxy.callMe();
});
