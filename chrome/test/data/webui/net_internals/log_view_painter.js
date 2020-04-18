// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Include test fixture.
GEN_INCLUDE(['net_internals_test.js']);

// Anonymous namespace
(function() {
/**
 * Tests the formatting of log entries to fixed width text.
 */
TEST_F('NetInternalsTest', 'netInternalsLogViewPainterPrintAsText', function() {
  // Add a DOM node to draw the log entries into.
  var div = addNode(document.body, 'div');

  // Helper function to run a particular "test case". This comprises an input
  // and the resulting formatted text expectation.
  function runTestCase(testCase) {
    div.innerHTML = '';
    timeutil.setTimeTickOffset(testCase.tickOffset);

    var baseTime = 0;
    if (typeof testCase.baseTimeTicks != 'undefined')
      baseTime = timeutil.convertTimeTicksToTime(testCase.baseTimeTicks);

    var tablePrinter = createLogEntryTablePrinter(
        testCase.logEntries, baseTime, testCase.logCreationTime);
    tablePrinter.toText(0, div);

    // Strip any trailing newlines, since the whitespace when using innerText
    // can be a bit unpredictable.
    var actualText = div.innerText;
    actualText = actualText.replace(/^\s+|\s+$/g, '');

    expectEquals(testCase.expectedText, actualText);
  }

  runTestCase(painterTestURLRequest());
  runTestCase(painterTestURLRequestIncomplete());
  runTestCase(painterTestURLRequestIncompleteFromLoadedLog());
  runTestCase(painterTestURLRequestIncompleteFromLoadedLogSingleEvent());
  runTestCase(painterTestNetError());
  runTestCase(painterTestQuicError());
  runTestCase(painterTestQuicCryptoHandshakeMessage());
  runTestCase(painterTestHexEncodedBytes());
  runTestCase(painterTestCertVerifierJob());
  runTestCase(painterTestCertVerifyResult());
  runTestCase(painterTestCheckedCert());
  runTestCase(painterTestProxyConfigOneProxyAllSchemes());
  runTestCase(painterTestProxyConfigTwoProxiesAllSchemes());
  runTestCase(painterTestExtraCustomParameter());
  runTestCase(painterTestMissingCustomParameter());
  runTestCase(painterTestInProgressURLRequest());
  runTestCase(painterTestBaseTime());

  testDone();
});

/**
 * Test case for a URLRequest. This includes custom formatting for load flags,
 * request/response HTTP headers, dependent sources, as well as basic
 * indentation and grouping.  Also makes sure that no extra event is logged
 * for finished sources when there's a logCreationTime.
 */
function painterTestURLRequest() {
  var testCase = {};
  testCase.tickOffset = '1337911098446';
  testCase.logCreationTime = 1338864634013;
  testCase.loadFlags =
      LoadFlag.MAIN_FRAME_DEPRECATED | LoadFlag.MAYBE_USER_GESTURE;

  testCase.logEntries = [
    {
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953534778',
      'type': EventType.REQUEST_ALIVE
    },
    {
      'params': {
        'load_flags': testCase.loadFlags,
        'method': 'GET',
        'priority': 4,
        'url': 'http://www.google.com/'
      },
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953534792',
      'type': EventType.URL_REQUEST_START_JOB
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953534800',
      'type': EventType.URL_REQUEST_START_JOB
    },
    {
      'params': {
        'load_flags': testCase.loadFlags,
        'method': 'GET',
        'priority': 4,
        'url': 'http://www.google.com/'
      },
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953534802',
      'type': EventType.URL_REQUEST_START_JOB
    },
    {
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953534809',
      'type': EventType.HTTP_CACHE_GET_BACKEND
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953534810',
      'type': EventType.HTTP_CACHE_GET_BACKEND
    },
    {
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953534811',
      'type': EventType.HTTP_CACHE_OPEN_ENTRY
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953534816',
      'type': EventType.HTTP_CACHE_OPEN_ENTRY
    },
    {
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953534817',
      'type': EventType.HTTP_CACHE_ADD_TO_ENTRY
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953534818',
      'type': EventType.HTTP_CACHE_ADD_TO_ENTRY
    },
    {
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953534823',
      'type': EventType.HTTP_CACHE_READ_INFO
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953534827',
      'type': EventType.HTTP_CACHE_READ_INFO
    },
    {
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953534830',
      'type': EventType.HTTP_STREAM_REQUEST
    },
    {
      'params': {
        'source_dependency':
            {'id': 149, 'type': EventSourceType.HTTP_STREAM_JOB}
      },
      'phase': EventPhase.PHASE_NONE,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953534898',
      'type': EventType.HTTP_STREAM_REQUEST_BOUND_TO_JOB
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953534902',
      'type': EventType.HTTP_STREAM_REQUEST
    },
    {
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953534906',
      'type': EventType.HTTP_TRANSACTION_SEND_REQUEST
    },
    {
      'params': {
        'headers': [
          'Host: www.google.com', 'Connection: keep-alive',
          'User-Agent: Mozilla/5.0', 'Accept: text/html',
          'Accept-Encoding: gzip,deflate', 'Accept-Language: en-US,en;q=0.8',
          'Accept-Charset: ISO-8859-1'
        ],
        'line': 'GET / HTTP/1.1\r\n'
      },
      'phase': EventPhase.PHASE_NONE,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953534910',
      'type': EventType.HTTP_TRANSACTION_SEND_REQUEST_HEADERS
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953534915',
      'type': EventType.HTTP_TRANSACTION_SEND_REQUEST
    },
    {
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953534916',
      'type': EventType.HTTP_TRANSACTION_READ_HEADERS
    },
    {
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953534917',
      'type': EventType.HTTP_STREAM_PARSER_READ_HEADERS
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953534987',
      'type': EventType.HTTP_STREAM_PARSER_READ_HEADERS
    },
    {
      'params': {
        'headers': [
          'HTTP/1.1 200 OK',
          'Date: Tue, 05 Jun 2012 02:50:33 GMT',
          'Expires: -1',
          'Cache-Control: private, max-age=0',
          'Content-Type: text/html; charset=UTF-8',
          'Content-Encoding: gzip',
          'Server: gws',
          'Content-Length: 23798',
        ]
      },
      'phase': EventPhase.PHASE_NONE,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953534989',
      'type': EventType.HTTP_TRANSACTION_READ_RESPONSE_HEADERS
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953534992',
      'type': EventType.HTTP_TRANSACTION_READ_HEADERS
    },
    {
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953534993',
      'type': EventType.HTTP_CACHE_WRITE_INFO
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953535023',
      'type': EventType.HTTP_CACHE_WRITE_INFO
    },
    {
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953535024',
      'type': EventType.HTTP_CACHE_WRITE_DATA
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953535062',
      'type': EventType.HTTP_CACHE_WRITE_DATA
    },
    {
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953535062',
      'type': EventType.HTTP_CACHE_WRITE_INFO
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953535075',
      'type': EventType.HTTP_CACHE_WRITE_INFO
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953535081',
      'type': EventType.URL_REQUEST_START_JOB
    },
    {
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953535537',
      'type': EventType.HTTP_TRANSACTION_READ_BODY
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953535538',
      'type': EventType.HTTP_TRANSACTION_READ_BODY
    },
    {
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953535538',
      'type': EventType.HTTP_CACHE_WRITE_DATA
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953535541',
      'type': EventType.HTTP_CACHE_WRITE_DATA
    },
    {
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953535542',
      'type': EventType.HTTP_TRANSACTION_READ_BODY
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953535545',
      'type': EventType.HTTP_TRANSACTION_READ_BODY
    },
    {
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953535546',
      'type': EventType.HTTP_CACHE_WRITE_DATA
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953535548',
      'type': EventType.HTTP_CACHE_WRITE_DATA
    },
    {
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953535548',
      'type': EventType.HTTP_TRANSACTION_READ_BODY
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953535548',
      'type': EventType.HTTP_TRANSACTION_READ_BODY
    },
    {
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953535549',
      'type': EventType.HTTP_CACHE_WRITE_DATA
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953535549',
      'type': EventType.HTTP_CACHE_WRITE_DATA
    },
    {
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953535550',
      'type': EventType.HTTP_TRANSACTION_READ_BODY
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953535550',
      'type': EventType.HTTP_TRANSACTION_READ_BODY
    },
    {
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953535550',
      'type': EventType.HTTP_CACHE_WRITE_DATA
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953535551',
      'type': EventType.HTTP_CACHE_WRITE_DATA
    },
    {
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953535552',
      'type': EventType.HTTP_TRANSACTION_READ_BODY
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953535553',
      'type': EventType.HTTP_TRANSACTION_READ_BODY
    },
    {
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953535553',
      'type': EventType.HTTP_CACHE_WRITE_DATA
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953535556',
      'type': EventType.HTTP_CACHE_WRITE_DATA
    },
    {
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953535556',
      'type': EventType.HTTP_TRANSACTION_READ_BODY
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953535559',
      'type': EventType.HTTP_TRANSACTION_READ_BODY
    },
    {
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953535559',
      'type': EventType.HTTP_CACHE_WRITE_DATA
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953535559',
      'type': EventType.HTTP_CACHE_WRITE_DATA
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953535567',
      'type': EventType.REQUEST_ALIVE
    }
  ];

  testCase.expectedText =
      't=1338864633224 [st=  0] +REQUEST_ALIVE  [dt=789]\n' +
      't=1338864633238 [st= 14]    URL_REQUEST_START_JOB  [dt=8]\n' +
      '                            --> load_flags = ' +
      testCase.loadFlags.toString() +
      ' (MAIN_FRAME_DEPRECATED | MAYBE_USER_GESTURE)\n' +
      '                            --> method = "GET"\n' +
      '                            --> priority = 4\n' +
      '                            --> url = "http://www.google.com/"\n' +
      't=1338864633248 [st= 24]   +URL_REQUEST_START_JOB  [dt=279]\n' +
      '                            --> load_flags = ' +
      testCase.loadFlags.toString() +
      ' (MAIN_FRAME_DEPRECATED | MAYBE_USER_GESTURE)\n' +
      '                            --> method = "GET"\n' +
      '                            --> priority = 4\n' +
      '                            --> url = "http://www.google.com/"\n' +
      't=1338864633255 [st= 31]      HTTP_CACHE_GET_BACKEND  [dt=1]\n' +
      't=1338864633257 [st= 33]      HTTP_CACHE_OPEN_ENTRY  [dt=5]\n' +
      't=1338864633263 [st= 39]      HTTP_CACHE_ADD_TO_ENTRY  [dt=1]\n' +
      't=1338864633269 [st= 45]      HTTP_CACHE_READ_INFO  [dt=4]\n' +
      't=1338864633276 [st= 52]     +HTTP_STREAM_REQUEST  [dt=72]\n' +
      't=1338864633344 [st=120]        HTTP_STREAM_REQUEST_BOUND_TO_JOB\n' +
      '                                --> source_dependency = 149 ' +
      '(HTTP_STREAM_JOB)\n' +
      't=1338864633348 [st=124]     -HTTP_STREAM_REQUEST\n' +
      't=1338864633352 [st=128]     +HTTP_TRANSACTION_SEND_REQUEST  [dt=9]\n' +
      't=1338864633356 [st=132]        HTTP_TRANSACTION_SEND_REQUEST_HEADERS\n' +
      '                                --> GET / HTTP/1.1\n' +
      '                                    Host: www.google.com\n' +
      '                                    Connection: keep-alive\n' +
      '                                    User-Agent: Mozilla/5.0\n' +
      '                                    Accept: text/html\n' +
      '                                    Accept-Encoding: gzip,deflate\n' +
      '                                    Accept-Language: en-US,en;q=0.8\n' +
      '                                    Accept-Charset: ISO-8859-1\n' +
      't=1338864633361 [st=137]     -HTTP_TRANSACTION_SEND_REQUEST\n' +
      't=1338864633362 [st=138]     +HTTP_TRANSACTION_READ_HEADERS  [dt=76]\n' +
      't=1338864633363 [st=139]        HTTP_STREAM_PARSER_READ_HEADERS  [dt=70]\n' +
      't=1338864633435 [st=211]        HTTP_TRANSACTION_READ_RESPONSE_HEADERS\n' +
      '                                --> HTTP/1.1 200 OK\n' +
      '                                    Date: Tue, 05 Jun 2012 02:50:33 GMT\n' +
      '                                    Expires: -1\n' +
      '                                    Cache-Control: private, max-age=0\n' +
      '                                    Content-Type: text/html; charset=UTF-8\n' +
      '                                    Content-Encoding: gzip\n' +
      '                                    Server: gws\n' +
      '                                    Content-Length: 23798\n' +
      't=1338864633438 [st=214]     -HTTP_TRANSACTION_READ_HEADERS\n' +
      't=1338864633439 [st=215]      HTTP_CACHE_WRITE_INFO  [dt=30]\n' +
      't=1338864633470 [st=246]      HTTP_CACHE_WRITE_DATA  [dt=38]\n' +
      't=1338864633508 [st=284]      HTTP_CACHE_WRITE_INFO  [dt=13]\n' +
      't=1338864633527 [st=303]   -URL_REQUEST_START_JOB\n' +
      't=1338864633983 [st=759]    HTTP_TRANSACTION_READ_BODY  [dt=1]\n' +
      't=1338864633984 [st=760]    HTTP_CACHE_WRITE_DATA  [dt=3]\n' +
      't=1338864633988 [st=764]    HTTP_TRANSACTION_READ_BODY  [dt=3]\n' +
      't=1338864633992 [st=768]    HTTP_CACHE_WRITE_DATA  [dt=2]\n' +
      't=1338864633994 [st=770]    HTTP_TRANSACTION_READ_BODY  [dt=0]\n' +
      't=1338864633995 [st=771]    HTTP_CACHE_WRITE_DATA  [dt=0]\n' +
      't=1338864633996 [st=772]    HTTP_TRANSACTION_READ_BODY  [dt=0]\n' +
      't=1338864633996 [st=772]    HTTP_CACHE_WRITE_DATA  [dt=1]\n' +
      't=1338864633998 [st=774]    HTTP_TRANSACTION_READ_BODY  [dt=1]\n' +
      't=1338864633999 [st=775]    HTTP_CACHE_WRITE_DATA  [dt=3]\n' +
      't=1338864634002 [st=778]    HTTP_TRANSACTION_READ_BODY  [dt=3]\n' +
      't=1338864634005 [st=781]    HTTP_CACHE_WRITE_DATA  [dt=0]\n' +
      't=1338864634013 [st=789] -REQUEST_ALIVE';

  return testCase;
}

/**
 * Test case for a URLRequest that was not completed that did not come from a
 * loaded log file.
 */
function painterTestURLRequestIncomplete() {
  var testCase = {};
  testCase.tickOffset = '1337911098446';

  testCase.logEntries = [
    {
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953534778',
      'type': EventType.REQUEST_ALIVE
    },
    {
      'params': {
        'load_flags': 0,
        'method': 'GET',
        'priority': 4,
        'url': 'http://www.google.com/'
      },
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953534910',
      'type': EventType.URL_REQUEST_START_JOB
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953534970',
      'type': EventType.URL_REQUEST_START_JOB
    },
  ];

  testCase.expectedText = 't=1338864633224 [st=  0] +REQUEST_ALIVE  [dt=?]\n' +
      't=1338864633356 [st=132]    URL_REQUEST_START_JOB  [dt=60]\n' +
      '                            --> load_flags = 0 (NORMAL)\n' +
      '                            --> method = "GET"\n' +
      '                            --> priority = 4\n' +
      '                            --> url = "http://www.google.com/"';

  return testCase;
}

/**
 * Test case for a URLRequest that was not completed that came from a loaded
 * log file.
 */
function painterTestURLRequestIncompleteFromLoadedLog() {
  var testCase = painterTestURLRequestIncomplete();
  testCase.logCreationTime = 1338864634013;
  testCase.expectedText =
      't=1338864633224 [st=  0] +REQUEST_ALIVE  [dt=789+]\n' +
      't=1338864633356 [st=132]    URL_REQUEST_START_JOB  [dt=60]\n' +
      '                            --> load_flags = 0 (NORMAL)\n' +
      '                            --> method = "GET"\n' +
      '                            --> priority = 4\n' +
      '                            --> url = "http://www.google.com/"\n' +
      't=1338864634013 [st=789]';
  return testCase;
}

/**
 * Test case for a URLRequest that was not completed that came from a loaded
 * log file when there's only a begin event.
 */
function painterTestURLRequestIncompleteFromLoadedLogSingleEvent() {
  var testCase = painterTestURLRequestIncomplete();
  testCase.logEntries = [testCase.logEntries[0]];
  testCase.logCreationTime = 1338864634013;
  testCase.expectedText =
      't=1338864633224 [st=  0] +REQUEST_ALIVE  [dt=789+]\n' +
      't=1338864634013 [st=789]';
  return testCase;
}

/**
 * Tests the custom formatting of net_errors across several different event
 * types.
 */
function painterTestNetError() {
  var testCase = {};
  testCase.tickOffset = '1337911098446';
  testCase.loadFlags =
      LoadFlag.MAIN_FRAME_DEPRECATED | LoadFlag.MAYBE_USER_GESTURE;

  testCase.logEntries = [
    {
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 318, 'type': EventSourceType.URL_REQUEST},
      'time': '953675448',
      'type': EventType.REQUEST_ALIVE
    },
    {
      'params': {
        'load_flags': testCase.loadFlags,
        'method': 'GET',
        'priority': 4,
        'url': 'http://www.doesnotexistdomain.com/'
      },
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 318, 'type': EventSourceType.URL_REQUEST},
      'time': '953675455',
      'type': EventType.URL_REQUEST_START_JOB
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 318, 'type': EventSourceType.URL_REQUEST},
      'time': '953675460',
      'type': EventType.URL_REQUEST_START_JOB
    },
    {
      'params': {
        'load_flags': testCase.loadFlags,
        'method': 'GET',
        'priority': 4,
        'url': 'http://www.doesnotexistdomain.com/'
      },
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 318, 'type': EventSourceType.URL_REQUEST},
      'time': '953675460',
      'type': EventType.URL_REQUEST_START_JOB
    },
    {
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 318, 'type': EventSourceType.URL_REQUEST},
      'time': '953675469',
      'type': EventType.HTTP_CACHE_GET_BACKEND
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 318, 'type': EventSourceType.URL_REQUEST},
      'time': '953675469',
      'type': EventType.HTTP_CACHE_GET_BACKEND
    },
    {
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 318, 'type': EventSourceType.URL_REQUEST},
      'time': '953675469',
      'type': EventType.HTTP_CACHE_OPEN_ENTRY
    },
    {
      'params': {'net_error': -2},
      'phase': EventPhase.PHASE_END,
      'source': {'id': 318, 'type': EventSourceType.URL_REQUEST},
      'time': '953675470',
      'type': EventType.HTTP_CACHE_OPEN_ENTRY
    },
    {
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 318, 'type': EventSourceType.URL_REQUEST},
      'time': '953675471',
      'type': EventType.HTTP_CACHE_CREATE_ENTRY
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 318, 'type': EventSourceType.URL_REQUEST},
      'time': '953675473',
      'type': EventType.HTTP_CACHE_CREATE_ENTRY
    },
    {
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 318, 'type': EventSourceType.URL_REQUEST},
      'time': '953675473',
      'type': EventType.HTTP_CACHE_ADD_TO_ENTRY
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 318, 'type': EventSourceType.URL_REQUEST},
      'time': '953675474',
      'type': EventType.HTTP_CACHE_ADD_TO_ENTRY
    },
    {
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 318, 'type': EventSourceType.URL_REQUEST},
      'time': '953675474',
      'type': EventType.HTTP_STREAM_REQUEST
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 318, 'type': EventSourceType.URL_REQUEST},
      'time': '953675699',
      'type': EventType.HTTP_STREAM_REQUEST
    },
    {
      'params': {'net_error': -105},
      'phase': EventPhase.PHASE_END,
      'source': {'id': 318, 'type': EventSourceType.URL_REQUEST},
      'time': '953675705',
      'type': EventType.URL_REQUEST_START_JOB
    },
    {
      'params': {'net_error': -105},
      'phase': EventPhase.PHASE_END,
      'source': {'id': 318, 'type': EventSourceType.URL_REQUEST},
      'time': '953675923',
      'type': EventType.REQUEST_ALIVE
    }
  ];

  testCase.expectedText =
      't=1338864773894 [st=  0] +REQUEST_ALIVE  [dt=475]\n' +
      't=1338864773901 [st=  7]    URL_REQUEST_START_JOB  [dt=5]\n' +
      '                            --> load_flags = ' +
      testCase.loadFlags.toString() +
      ' (MAIN_FRAME_DEPRECATED | MAYBE_USER_GESTURE)\n' +
      '                            --> method = "GET"\n' +
      '                            --> priority = 4\n' +
      '                            --> url = "http://www.doesnotexistdomain.com/"\n' +
      't=1338864773906 [st= 12]   +URL_REQUEST_START_JOB  [dt=245]\n' +
      '                            --> load_flags = ' +
      testCase.loadFlags.toString() +
      ' (MAIN_FRAME_DEPRECATED | MAYBE_USER_GESTURE)\n' +
      '                            --> method = "GET"\n' +
      '                            --> priority = 4\n' +
      '                            --> url = "http://www.doesnotexistdomain.com/"\n' +
      't=1338864773915 [st= 21]      HTTP_CACHE_GET_BACKEND  [dt=0]\n' +
      't=1338864773915 [st= 21]      HTTP_CACHE_OPEN_ENTRY  [dt=1]\n' +
      '                              --> net_error = -2 (ERR_FAILED)\n' +
      't=1338864773917 [st= 23]      HTTP_CACHE_CREATE_ENTRY  [dt=2]\n' +
      't=1338864773919 [st= 25]      HTTP_CACHE_ADD_TO_ENTRY  [dt=1]\n' +
      't=1338864773920 [st= 26]      HTTP_STREAM_REQUEST  [dt=225]\n' +
      't=1338864774151 [st=257]   -URL_REQUEST_START_JOB\n' +
      '                            --> net_error = -105 (ERR_NAME_NOT_RESOLVED)\n' +
      't=1338864774369 [st=475] -REQUEST_ALIVE\n' +
      '                          --> net_error = -105 (ERR_NAME_NOT_RESOLVED)';

  return testCase;
}

/**
 * Tests the custom formatting of QUIC errors across several different event
 * types.
 */
function painterTestQuicError() {
  var testCase = {};
  testCase.tickOffset = '1337911098446';

  testCase.logEntries = [
    {
      'params': {'host': 'www.example.com'},
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 318, 'type': EventSourceType.URL_REQUEST},
      'time': '953675448',
      'type': EventType.QUIC_SESSION
    },
    {
      'params': {
        'details': 'invalid headers',
        'quic_rst_stream_error':
            QuicRstStreamError.QUIC_BAD_APPLICATION_PAYLOAD,
        'stream_id': 1
      },
      'phase': EventPhase.PHASE_NONE,
      'source': {'id': 318, 'type': EventSourceType.URL_REQUEST},
      'time': '953675460',
      'type': EventType.QUIC_SESSION_RST_STREAM_FRAME_RECEIVED
    },
    {
      'params': {
        'quic_error': QuicError.QUIC_NETWORK_IDLE_TIMEOUT,
      },
      'phase': EventPhase.PHASE_NONE,
      'source': {'id': 318, 'type': EventSourceType.URL_REQUEST},
      'time': '953675705',
      'type': EventType.QUIC_SESSION_CONNECTION_CLOSE_FRAME_RECEIVED
    },
    {
      'params': {'quic_error': QuicError.QUIC_NETWORK_IDLE_TIMEOUT},
      'phase': EventPhase.PHASE_END,
      'source': {'id': 318, 'type': EventSourceType.URL_REQUEST},
      'time': '953675923',
      'type': EventType.QUIC_SESSION
    }
  ];

  testCase.expectedText = 't=1338864773894 [st=  0] +QUIC_SESSION  [dt=475]\n' +
      '                          --> host = "www.example.com"\n' +
      't=1338864773906 [st= 12]    QUIC_SESSION_RST_STREAM_FRAME_RECEIVED\n' +
      '                            --> details = "invalid headers"\n' +
      '                            --> quic_rst_stream_error = ' +
      QuicRstStreamError.QUIC_BAD_APPLICATION_PAYLOAD + ' (' +
      'QUIC_BAD_APPLICATION_PAYLOAD)\n' +
      '                            --> stream_id = 1\n' +
      't=1338864774151 [st=257]    QUIC_SESSION_CONNECTION_CLOSE_FRAME_RECEIVED\n' +
      '                            --> quic_error = ' +
      QuicError.QUIC_NETWORK_IDLE_TIMEOUT + ' (QUIC_NETWORK_IDLE_TIMEOUT)\n' +
      't=1338864774369 [st=475] -QUIC_SESSION\n' +
      '                          --> quic_error = ' +
      QuicError.QUIC_NETWORK_IDLE_TIMEOUT + ' (QUIC_NETWORK_IDLE_TIMEOUT)';

  return testCase;
}

/**
 * Tests the custom formatting of QUIC crypto handshake messages.
 */
function painterTestQuicCryptoHandshakeMessage() {
  var testCase = {};
  testCase.tickOffset = '1337911098446';

  testCase.logEntries = [
    {
      'params': {'host': 'www.example.com'},
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 318, 'type': EventSourceType.URL_REQUEST},
      'time': '953675448',
      'type': EventType.QUIC_SESSION
    },
    {
      'params': {
        'quic_crypto_handshake_message': 'REJ <\n' +
            '  STK : 4FDE\n' +
            '  SNO : A228\n' +
            '  PROF: 3045\n' +
            '  SCFG:\n' +
            '    SCFG<\n' +
            '      AEAD: AESG\n' +
            '      SCID: FED7\n' +
            '      PDMD: CHID\n' +
            '      PUBS: 2000\n' +
            '      VERS: 0000\n' +
            '      KEXS: C255,P256\n' +
            '      OBIT: 7883764781F2DFD0\n' +
            '      EXPY: FFEE725200000000\n' +
            '    >\n' +
            '  >'
      },
      'phase': EventPhase.PHASE_NONE,
      'source': {'id': 318, 'type': EventSourceType.URL_REQUEST},
      'time': '953675460',
      'type': EventType.QUIC_SESSION_CRYPTO_HANDSHAKE_MESSAGE_SENT
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 318, 'type': EventSourceType.URL_REQUEST},
      'time': '953675923',
      'type': EventType.QUIC_SESSION
    }
  ];

  testCase.expectedText = 't=1338864773894 [st=  0] +QUIC_SESSION  [dt=475]\n' +
      '                          --> host = "www.example.com"\n' +
      't=1338864773906 [st= 12]    QUIC_SESSION_CRYPTO_HANDSHAKE_MESSAGE_SENT\n' +
      '                            --> REJ <\n' +
      '                                  STK : 4FDE\n' +
      '                                  SNO : A228\n' +
      '                                  PROF: 3045\n' +
      '                                  SCFG:\n' +
      '                                    SCFG<\n' +
      '                                      AEAD: AESG\n' +
      '                                      SCID: FED7\n' +
      '                                      PDMD: CHID\n' +
      '                                      PUBS: 2000\n' +
      '                                      VERS: 0000\n' +
      '                                      KEXS: C255,P256\n' +
      '                                      OBIT: 7883764781F2DFD0\n' +
      '                                      EXPY: FFEE725200000000\n' +
      '                                    >\n' +
      '                                  >\n' +
      't=1338864774369 [st=475] -QUIC_SESSION';

  return testCase;
}

/**
 * Tests the formatting of bytes sent/received as hex + ASCII. Note that the
 * test data was truncated which is why the byte_count doesn't quite match the
 * hex_encoded_bytes.
 */
function painterTestHexEncodedBytes() {
  var testCase = {};
  testCase.tickOffset = '1337911098473';

  testCase.logEntries = [
    {
      'params': {
        'source_dependency':
            {'id': 634, 'type': EventSourceType.TRANSPORT_CONNECT_JOB}
      },
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 637, 'type': EventSourceType.SOCKET},
      'time': '953918459',
      'type': EventType.SOCKET_ALIVE
    },
    {
      'params': {'address_list': ['184.30.253.15:80']},
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 637, 'type': EventSourceType.SOCKET},
      'time': '953918460',
      'type': EventType.TCP_CONNECT
    },
    {
      'params': {'address': '184.30.253.15:80'},
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 637, 'type': EventSourceType.SOCKET},
      'time': '953918461',
      'type': EventType.TCP_CONNECT_ATTEMPT
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 637, 'type': EventSourceType.SOCKET},
      'time': '953918464',
      'type': EventType.TCP_CONNECT_ATTEMPT
    },
    {
      'params': {'source_address': '127.0.0.1:54041'},
      'phase': EventPhase.PHASE_END,
      'source': {'id': 637, 'type': EventSourceType.SOCKET},
      'time': '953918465',
      'type': EventType.TCP_CONNECT
    },
    {
      'params': {
        'source_dependency':
            {'id': 628, 'type': EventSourceType.HTTP_STREAM_JOB}
      },
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 637, 'type': EventSourceType.SOCKET},
      'time': '953918472',
      'type': EventType.SOCKET_IN_USE
    },
    {
      'params': {
        'byte_count': 780,
        'hex_encoded_bytes': '474554202F66617669636F6E2E69636F20485454502' +
            'F312E310D0A486F73743A207777772E6170706C652E' +
            '636F6D0D0A436F6E6E656374696F6E3A20'
      },
      'phase': EventPhase.PHASE_NONE,
      'source': {'id': 637, 'type': EventSourceType.SOCKET},
      'time': '953918484',
      'type': EventType.SOCKET_BYTES_SENT
    },
    {
      'params': {
        'byte_count': 1024,
        'hex_encoded_bytes': '485454502F312E3120323030204F4B0D0A4C6173742' +
            'D4D6F6469666965643A204D6F6E2C20313920446563' +
            '20323031312032323A34363A353920474D'
      },
      'phase': EventPhase.PHASE_NONE,
      'source': {'id': 637, 'type': EventSourceType.SOCKET},
      'time': '953918596',
      'type': EventType.SOCKET_BYTES_RECEIVED
    }
  ];

  testCase.expectedText = 't=1338865016932 [st=  0] +SOCKET_ALIVE  [dt=?]\n' +
      '                          --> source_dependency = 634 (TRANSPORT_CONNECT_JOB' +
      ')\n' +
      't=1338865016933 [st=  1]   +TCP_CONNECT  [dt=5]\n' +
      '                            --> address_list = ["184.30.253.15:80"]\n' +
      't=1338865016934 [st=  2]      TCP_CONNECT_ATTEMPT  [dt=3]\n' +
      '                              --> address = "184.30.253.15:80"\n' +
      't=1338865016938 [st=  6]   -TCP_CONNECT\n' +
      '                            --> source_address = "127.0.0.1:54041"\n' +
      't=1338865016945 [st= 13]   +SOCKET_IN_USE  [dt=?]\n' +
      '                            --> source_dependency = 628 (HTTP_STREAM_JOB)\n' +
      't=1338865016957 [st= 25]      SOCKET_BYTES_SENT\n' +
      '                              --> byte_count = 780\n' +
      '                              --> hex_encoded_bytes =\n' +
      '                                47 45 54 20 2F 66 61 76  69 63 6F 6E 2E 69 ' +
      '63 6F   GET /favicon.ico\n' +
      '                                20 48 54 54 50 2F 31 2E  31 0D 0A 48 6F 73 ' +
      '74 3A    HTTP/1.1..Host:\n' +
      '                                20 77 77 77 2E 61 70 70  6C 65 2E 63 6F 6D ' +
      '0D 0A    www.apple.com..\n' +
      '                                43 6F 6E 6E 65 63 74 69  6F 6E 3A 20       ' +
      '        Connection: \n' +
      't=1338865017069 [st=137]      SOCKET_BYTES_RECEIVED\n' +
      '                              --> byte_count = 1024\n' +
      '                              --> hex_encoded_bytes =\n' +
      '                                48 54 54 50 2F 31 2E 31  20 32 30 30 20 4F ' +
      '4B 0D   HTTP/1.1 200 OK.\n' +
      '                                0A 4C 61 73 74 2D 4D 6F  64 69 66 69 65 64 ' +
      '3A 20   .Last-Modified: \n' +
      '                                4D 6F 6E 2C 20 31 39 20  44 65 63 20 32 30 ' +
      '31 31   Mon, 19 Dec 2011\n' +
      '                                20 32 32 3A 34 36 3A 35  39 20 47 4D       ' +
      '         22:46:59 GM';

  return testCase;
}

/**
 * Tests the formatting of certificates.
 */
function painterTestCertVerifierJob() {
  var testCase = {};
  testCase.tickOffset = '1337911098481';

  testCase.logEntries = [
    {
      'params': {
        'certificates': [
          '-----BEGIN CERTIFICATE-----\n1\n-----END CERTIFICATE-----\n',
          '-----BEGIN CERTIFICATE-----\n2\n-----END CERTIFICATE-----\n',
        ]
      },
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 752, 'type': EventSourceType.CERT_VERIFIER_JOB},
      'time': '954124663',
      'type': EventType.CERT_VERIFIER_JOB
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 752, 'type': EventSourceType.CERT_VERIFIER_JOB},
      'time': '954124697',
      'type': EventType.CERT_VERIFIER_JOB
    }
  ];

  testCase.expectedText =
      't=1338865223144 [st=0]  CERT_VERIFIER_JOB  [dt=34]\n' +
      '                        --> certificates =\n' +
      '                               -----BEGIN CERTIFICATE-----\n' +
      '                               1\n' +
      '                               -----END CERTIFICATE-----\n' +
      '                               \n' +
      '                               -----BEGIN CERTIFICATE-----\n' +
      '                               2\n' +
      '                               -----END CERTIFICATE-----';
  return testCase;
}

/**
 * Tests the formatting of CertVerifyResult fields.
 */
function painterTestCertVerifyResult() {
  var testCase = {};
  testCase.tickOffset = '1337911098481';

  testCase.logEntries = [
    {
      'params': {
        'certificates': [
          '-----BEGIN CERTIFICATE-----\n1\n-----END CERTIFICATE-----\n',
          '-----BEGIN CERTIFICATE-----\n2\n-----END CERTIFICATE-----\n',
        ]
      },
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 752, 'type': EventSourceType.CERT_VERIFIER_JOB},
      'time': '954124663',
      'type': EventType.CERT_VERIFIER_JOB
    },
    {
      'params': {
        'has_md5': true,
        'has_md2': false,
        'has_md4': true,
        'is_issued_by_known_root': true,
        'is_issued_by_additional_trust_anchor': false,
        'cert_status': 5,
        'verified_cert': {
          'certificates': [
            '-----BEGIN CERTIFICATE-----\n1\n-----END CERTIFICATE-----\n',
            '-----BEGIN CERTIFICATE-----\n2\n-----END CERTIFICATE-----\n',
          ]
        },
        'public_key_hashes': [
          'hash1',
          'hash2',
        ]
      },
      'phase': EventPhase.PHASE_END,
      'source': {'id': 752, 'type': EventSourceType.CERT_VERIFIER_JOB},
      'time': '954124697',
      'type': EventType.CERT_VERIFIER_JOB
    }
  ];

  testCase.expectedText =
      't=1338865223144 [st= 0] +CERT_VERIFIER_JOB  [dt=34]\n' +
      '                         --> certificates =\n' +
      '                                -----BEGIN CERTIFICATE-----\n' +
      '                                1\n' +
      '                                -----END CERTIFICATE-----\n' +
      '                                \n' +
      '                                -----BEGIN CERTIFICATE-----\n' +
      '                                2\n' +
      '                                -----END CERTIFICATE-----\n' +
      '                                \n' +
      't=1338865223178 [st=34] -CERT_VERIFIER_JOB\n' +
      '                         --> verified_cert =\n' +
      '                                -----BEGIN CERTIFICATE-----\n' +
      '                                1\n' +
      '                                -----END CERTIFICATE-----\n' +
      '                                \n' +
      '                                -----BEGIN CERTIFICATE-----\n' +
      '                                2\n' +
      '                                -----END CERTIFICATE-----\n' +
      '                                \n' +
      '                         --> cert_status = 5 (AUTHORITY_INVALID |' +
      ' COMMON_NAME_INVALID)\n' +
      '                         --> has_md5 = true\n' +
      '                         --> has_md2 = false\n' +
      '                         --> has_md4 = true\n' +
      '                         --> is_issued_by_known_root = true\n' +
      '                         --> is_issued_by_additional_trust_anchor =' +
      ' false\n' +
      '                         --> public_key_hashes = ["hash1","hash2"]';

  return testCase;
}

/**
 * Tests the formatting of checked certificates
 */
function painterTestCheckedCert() {
  var testCase = {};
  testCase.tickOffset = '1337911098481';

  testCase.logEntries = [{
    'params': {
      'certificate': {
        'certificates': [
          '-----BEGIN CERTIFICATE-----\n1\n-----END CERTIFICATE-----\n',
          '-----BEGIN CERTIFICATE-----\n2\n-----END CERTIFICATE-----\n'
        ]
      }
    },
    'phase': EventPhase.PHASE_NONE,
    'source': {'id': 752, 'type': EventSourceType.SOCKET},
    'time': '954124697',
    'type': EventType.CERT_CT_COMPLIANCE_CHECKED
  }];

  testCase.expectedText =
      't=1338865223178 [st=0]  CERT_CT_COMPLIANCE_CHECKED\n' +
      '                        --> certificate =\n' +
      '                               -----BEGIN CERTIFICATE-----\n' +
      '                               1\n' +
      '                               -----END CERTIFICATE-----\n' +
      '                               \n' +
      '                               -----BEGIN CERTIFICATE-----\n' +
      '                               2\n' +
      '                               -----END CERTIFICATE-----';

  return testCase;
}

/**
 * Tests the formatting of proxy configurations when using one proxy server for
 * all URL schemes.
 */
function painterTestProxyConfigOneProxyAllSchemes() {
  var testCase = {};
  testCase.tickOffset = '1337911098481';

  testCase.logEntries = [{
    'params': {
      'new_config': {
        'auto_detect': true,
        'bypass_list': ['*.local', 'foo', '<local>'],
        'pac_url': 'https://config/wpad.dat',
        'single_proxy': 'cache-proxy:3128',
        'source': 'SYSTEM'
      },
      'old_config': {'auto_detect': true}
    },
    'phase': EventPhase.PHASE_NONE,
    'source': {'id': 814, 'type': EventSourceType.NONE},
    'time': '954443578',
    'type': EventType.PROXY_CONFIG_CHANGED
  }];

  testCase.expectedText = 't=1338865542059 [st=0]  PROXY_CONFIG_CHANGED\n' +
      '                        --> old_config =\n' +
      '                               Auto-detect\n' +
      '                        --> new_config =\n' +
      '                               (1) Auto-detect\n' +
      '                               (2) PAC script: https://config/wpad.dat\n' +
      '                               (3) Proxy server: cache-proxy:3128\n' +
      '                                   Bypass list: \n' +
      '                                     *.local\n' +
      '                                     foo\n' +
      '                                     <local>\n' +
      '                               Source: SYSTEM';

  return testCase;
}

/**
 * Tests the formatting of proxy configurations when using two proxy servers for
 * all URL schemes.
 */
function painterTestProxyConfigTwoProxiesAllSchemes() {
  var testCase = {};
  testCase.tickOffset = '1337911098481';

  testCase.logEntries = [{
    'params': {
      'new_config': {
        'auto_detect': true,
        'bypass_list': ['*.local', 'foo', '<local>'],
        'pac_url': 'https://config/wpad.dat',
        'single_proxy': ['cache-proxy:3128', 'socks4://other:999'],
        'source': 'SYSTEM'
      },
      'old_config': {'auto_detect': true}
    },
    'phase': EventPhase.PHASE_NONE,
    'source': {'id': 814, 'type': EventSourceType.NONE},
    'time': '954443578',
    'type': EventType.PROXY_CONFIG_CHANGED
  }];

  testCase.expectedText = 't=1338865542059 [st=0]  PROXY_CONFIG_CHANGED\n' +
      '                        --> old_config =\n' +
      '                               Auto-detect\n' +
      '                        --> new_config =\n' +
      '                               (1) Auto-detect\n' +
      '                               (2) PAC script: https://config/wpad.dat\n' +
      '                               (3) Proxy server: [cache-proxy:3128, ' +
      'socks4://other:999]\n' +
      '                                   Bypass list: \n' +
      '                                     *.local\n' +
      '                                     foo\n' +
      '                                     <local>\n' +
      '                               Source: SYSTEM';

  return testCase;
}

/**
 * Tests that when there are more custom parameters than we expect for an
 * event type, they are printed out in addition to the custom formatting.
 */
function painterTestExtraCustomParameter() {
  var testCase = {};
  testCase.tickOffset = '1337911098446';

  testCase.logEntries = [
    {
      'params': {
        'headers': ['Host: www.google.com', 'Connection: keep-alive'],
        'line': 'GET / HTTP/1.1\r\n',
        // This is unexpected!
        'hello': 'yo dawg, i heard you like strings'
      },
      'phase': EventPhase.PHASE_NONE,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953534910',
      'type': EventType.HTTP_TRANSACTION_SEND_REQUEST_HEADERS
    },
  ];

  testCase.expectedText =
      't=1338864633356 [st=0]  HTTP_TRANSACTION_SEND_REQUEST_HEADERS\n' +
      '                        --> GET / HTTP/1.1\n' +
      '                            Host: www.google.com\n' +
      '                            Connection: keep-alive\n' +
      '                        --> hello = "yo dawg, i heard you like strings"';

  return testCase;
}

/**
 * Tests that when the custom parameters for an event type don't match
 * what we expect, we fall back to default formatting.
 */
function painterTestMissingCustomParameter() {
  var testCase = {};
  testCase.tickOffset = '1337911098446';

  testCase.logEntries = [
    {
      'params': {
        // The expectation is for this to be called "headers".
        'headersWRONG': ['Host: www.google.com', 'Connection: keep-alive'],
        'line': 'GET / HTTP/1.1\r\n'
      },
      'phase': EventPhase.PHASE_NONE,
      'source': {'id': 146, 'type': EventSourceType.URL_REQUEST},
      'time': '953534910',
      'type': EventType.HTTP_TRANSACTION_SEND_REQUEST_HEADERS
    },
  ];

  testCase.expectedText =
      't=1338864633356 [st=0]  HTTP_TRANSACTION_SEND_REQUEST_HEADERS\n' +
      '                        --> headersWRONG = ["Host: www.google.com",' +
      '"Connection: keep-alive"]\n' +
      '                        --> line = "GET / HTTP/1.1\\r\\n"';

  return testCase;
}

/**
 * Tests the formatting of a URL request that was just finishing up when
 * net-internals was opened.
 */
function painterTestInProgressURLRequest() {
  var testCase = {};
  testCase.tickOffset = '1337911098446';
  testCase.loadFlags =
      LoadFlag.MAIN_FRAME_DEPRECATED | LoadFlag.MAYBE_USER_GESTURE;

  testCase.logEntries = [
    {
      'params': {
        'load_flags': testCase.loadFlags,
        'load_state': LoadState.READING_RESPONSE,
        'method': 'GET',
        'url': 'http://www.MagicPonyShopper.com'
      },
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 318, 'type': EventSourceType.URL_REQUEST},
      'time': '953675548',
      'type': EventType.REQUEST_ALIVE
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 318, 'type': EventSourceType.URL_REQUEST},
      'time': '953675699',
      'type': EventType.HTTP_STREAM_REQUEST
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 318, 'type': EventSourceType.URL_REQUEST},
      'time': '953675705',
      'type': EventType.URL_REQUEST_START_JOB
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 318, 'type': EventSourceType.URL_REQUEST},
      'time': '953675923',
      'type': EventType.REQUEST_ALIVE
    }
  ];

  testCase.expectedText =
      't=1338864773994 [st=  0] +REQUEST_ALIVE  [dt=375]\n' +
      '                          --> load_flags = ' +
      testCase.loadFlags.toString() +
      ' (MAIN_FRAME_DEPRECATED | MAYBE_USER_GESTURE)\n' +
      '                          --> load_state = ' +
      LoadState.READING_RESPONSE + ' (READING_RESPONSE)\n' +
      '                          --> method = "GET"\n' +
      '                          --> url = "http://www.MagicPonyShopper.com"\n' +
      't=1338864774145 [st=151]   -HTTP_STREAM_REQUEST\n' +
      't=1338864774151 [st=157]   -URL_REQUEST_START_JOB\n' +
      't=1338864774369 [st=375] -REQUEST_ALIVE';

  return testCase;
}

/**
 * Tests the formatting using a non-zero base time.  Also has no final event,
 * to make sure logCreationTime is handled correctly.
 */
function painterTestBaseTime() {
  var testCase = {};
  testCase.tickOffset = '1337911098446';
  testCase.logCreationTime = 1338864774783;
  testCase.baseTimeTicks = '953675546';

  testCase.logEntries = [
    {
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 318, 'type': EventSourceType.URL_REQUEST},
      'time': '953675548',
      'type': EventType.REQUEST_ALIVE
    },
    {
      'phase': EventPhase.PHASE_BEGIN,
      'source': {'id': 318, 'type': EventSourceType.URL_REQUEST},
      'time': '953675698',
      'type': EventType.HTTP_STREAM_REQUEST
    },
    {
      'phase': EventPhase.PHASE_END,
      'source': {'id': 318, 'type': EventSourceType.URL_REQUEST},
      'time': '953675699',
      'type': EventType.HTTP_STREAM_REQUEST
    },
  ];

  testCase.expectedText = 't=  2 [st=  0] +REQUEST_ALIVE  [dt=789+]\n' +
      't=152 [st=150]    HTTP_STREAM_REQUEST  [dt=1]\n' +
      't=791 [st=789]';

  return testCase;
}

})();  // Anonymous namespace
