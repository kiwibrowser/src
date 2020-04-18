// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

'use strict';

var testData = [{
    userAgent: 'Mozilla/5.0 (X11; CrOS x86_64 6457.107.0) AppleWebKit/537.36 ' +
      '(KHTML, like Gecko) Chrome/40.0.2214.115 Safari/537.36,gzip(gfe)',
    osName: 'ChromeOS',
    osVersion: '6457.107.0',
    cpu: 'x86_64',
    chromeVersion: '40.0.2214.115'
  }, {
    userAgent: 'Mozilla/5.0 (X11; CrOS i686 6812.88.0) AppleWebKit/537.36 ' +
      '(KHTML, like Gecko) Chrome/42.0.2311.153 Safari/537.36,gzip(gfe)',
    osName: 'ChromeOS',
    osVersion: '6812.88.0',
    cpu: 'i686',
    chromeVersion: '42.0.2311.153'
  }, {
    userAgent: 'Mozilla/5.0 (X11; CrOS armv7l 6946.52.0) AppleWebKit/537.36 ' +
      '(KHTML, like Gecko) Chrome/43.0.2357.73 Safari/537.36,gzip(gfe)',
    osName: 'ChromeOS',
    osVersion: '6946.52.0',
    cpu: 'armv7l',
    chromeVersion: '43.0.2357.73'
  }, {
    userAgent: 'Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 ' +
        '(KHTML, like Gecko) Chrome/45.0.2414.0 Safari/537.36,gzip(gfe)',
    osName: 'Linux',
    osVersion: '',
    cpu: 'x86_64',
    chromeVersion: '45.0.2414.0'
  },{
    userAgent: 'Mozilla/5.0 (Windows NT 6.1) AppleWebKit/537.36 ' +
        '(KHTML, like Gecko) Chrome/43.0.2357.81 Safari/537.36,gzip(gfe)',
    osName: 'Windows',
    osVersion: '6.1',
    cpu: '',
    chromeVersion: '43.0.2357.81'
  },{
    userAgent: 'Mozilla/5.0 (Windows NT 6.3; WOW64) AppleWebKit/537.36 ' +
      '(KHTML, like Gecko) Chrome/42.0.2311.152 Safari/537.36,gzip(gfe)',
    osName: 'Windows',
    osVersion: '6.3',
    cpu: '',
    chromeVersion: '42.0.2311.152'
  },{
    userAgent: 'Mozilla/5.0 (Windows NT 6.3; Win64; x64) AppleWebKit/537.36 ' +
        '(KHTML, like Gecko) Chrome/43.0.2357.81 Safari/537.36,gzip(gfe)',
    osName: 'Windows',
    osVersion: '6.3',
    cpu: '',
    chromeVersion: '43.0.2357.81'
  }, {
    userAgent: 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 '+
      '(KHTML, like Gecko) Chrome/43.0.2357.81 Safari/537.36,gzip(gfe)',
    osName: 'Windows',
    osVersion: '10.0',
    cpu: '',
    chromeVersion: '43.0.2357.81'
  },{
    userAgent: 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_9_5) AppleWebKit' +
      '/537.36 (KHTML, like Gecko) Chrome/43.0.2357.81 Safari/537.36,gzip(gfe)',
    osName: 'Mac',
    osVersion: '10.9.5',
    cpu: 'Intel',
    chromeVersion: '43.0.2357.81'
  },{
    userAgent: 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_1) AppleWebKit' +
      '/537.36 (KHTML, like Gecko) Chrome/43.0.2357.81 Safari/537.36,gzip(gfe)',
    osName: 'Mac',
    osVersion: '10.10.1',
    cpu: 'Intel',
    chromeVersion: '43.0.2357.81'
  },{
    userAgent: 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_2) AppleWebKit' +
      '/537.36 (KHTML, like Gecko) Chrome/43.0.2357.81 Safari/537.36,gzip(gfe)',
    osName: 'Mac',
    osVersion: '10.10.2',
    cpu: 'Intel',
    chromeVersion: '43.0.2357.81'
  },{
    userAgent: 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_3) AppleWebKit' +
      '/537.36 (KHTML, like Gecko) Chrome/43.0.2357.81 Safari/537.36,gzip(gfe)',
    osName: 'Mac',
    osVersion: '10.10.3',
    cpu: 'Intel',
    chromeVersion: '43.0.2357.81'
  }
];

QUnit.module('platform');

function forEachUserAgent(/** function(Object<string>, string) */ callback) {
  testData.forEach(function(/** Object<string>*/ testCase) {
    var message = 'userAgent: ' + testCase['userAgent']
    var userAgentStub = sinon.stub(remoting, 'getUserAgent');
    userAgentStub.returns(testCase['userAgent']);
    var result = remoting.getSystemInfo();
    callback(testCase, message);
    userAgentStub.restore();
  });
}

QUnit.test('OS name, OS version, chrome version and cpu detection',
           function(assert) {
  forEachUserAgent(
      function(/** Object<string> */ testCase, /** string */ message) {
        var result = remoting.getSystemInfo();
        assert.equal(result.osName, testCase['osName'], message);
        assert.equal(result.osVersion, testCase['osVersion'], message);
        assert.equal(result.cpu, testCase['cpu'], message);
        assert.equal(result.chromeVersion, testCase['chromeVersion'], message);
  });
});

QUnit.test('platform is Mac', function(assert) {
  forEachUserAgent(
      function(/** Object<string> */ testCase, /** string */ message) {
        assert.equal(remoting.platformIsMac(),
                     testCase['osName'] === 'Mac', message);
  });
});

QUnit.test('platform is Windows', function(assert) {
  forEachUserAgent(
      function(/** Object<string> */ testCase, /** string */ message) {
        assert.equal(remoting.platformIsWindows(),
                     testCase['osName'] === 'Windows', message);
  });
});

QUnit.test('platform is Linux', function(assert) {
  forEachUserAgent(
      function(/** Object<string> */ testCase, /** string */ message) {
        assert.equal(remoting.platformIsLinux(),
                     testCase['osName'] === 'Linux', message);
  });
});

QUnit.test('platform is ChromeOS', function(assert) {
  forEachUserAgent(
    function(/** Object<string> */ testCase, /** string */ message) {
      assert.equal(remoting.platformIsChromeOS(),
                   testCase['osName'] === 'ChromeOS', message);
  });
});

})();
