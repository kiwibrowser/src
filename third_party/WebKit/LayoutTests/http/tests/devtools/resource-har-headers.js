// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests the nondeterministic bits of HAR conversion via the magic of hard-coded values.\n`);
  await TestRunner.loadModule('application_test_runner');
  await TestRunner.loadModule('network_test_runner');

  function visibleNewlines(s) {
    return s.replace(/\r/, '\\r').replace(/\n/, '\\n');
  }

  function setRequestValues(request) {
    request.setRequestHeaders([{name: 'Request', value: 'request-value'}]);
    request.setRequestHeadersText('GET http://example.com/inspector-test.js HTTP/1.1\r\nRequest: headers-text');

    request.responseHeaders = [{name: 'Response', value: 'response-value'}];
    request.responseHeadersText = 'HTTP/1.1 200 OK\r\nResponse: headers-text';

    request.documentURL = 'http://example.com/inspector-test.js';
    request.requestMethod = 'GET';
    request.mimeType = 'text/html';
    request.statusCode = 200;
    request.statusText = 'OK';
    request.resourceSize = 1000;
    request._transferSize = 539;  // 39 = header size at the end of the day
  }

  var testRequest = new SDK.NetworkRequest('testRequest', 'http://example.com/inspector-test.js', 1);
  setRequestValues(testRequest);
  var headersText = testRequest.requestHeadersText();
  var requestResults = {
    'request': {
      'headers': testRequest.requestHeaders(),
      'headersText': visibleNewlines(headersText),
      'headersSize': headersText.length,
    },
    'response': {
      'headers': testRequest.responseHeaders,
      'headersText': visibleNewlines(testRequest.responseHeadersText),
      'headersSize': testRequest.responseHeadersText.length,
      'resourceSize': testRequest.resourceSize,
      'transferSize': testRequest.transferSize
    }
  };
  TestRunner.addObject(requestResults, {}, '', 'Resource:');

  var stillNondeterministic = {
    'startedDateTime': 'formatAsTypeName',
    'time': 'formatAsTypeName',
    'timings': 'formatAsTypeName',
    '_transferSize': 'formatAsTypeName',
    '_error': 'skip'
  };
  var har = await BrowserSDK.HAREntry.build(testRequest);
  TestRunner.addObject(har, stillNondeterministic, '', 'HAR:');
  TestRunner.completeTest();
})();
