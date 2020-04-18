if (self.importScripts) {
  importScripts('/fetch/resources/fetch-test-helpers.js');
  importScripts('/fetch/resources/thorough-util.js');
}

var TEST_TARGETS = [];

// Only [Exposed=(Window,DedicatedWorker,SharedWorker)].
if ('createObjectURL' in URL) {
  var url = URL.createObjectURL(new Blob(["report({jsonpResult: 'success'});"], {type: 'application/json'}));

  TEST_TARGETS = [
    // Same-origin blob: requests.
    [BASE_URL + 'url=' + encodeURIComponent(url) + '&mode=same-origin&method=GET',
     [fetchResolved, hasContentLength, noServerHeader, hasBody, typeBasic],
     [checkJsonpSuccess]],
    [BASE_URL + 'url=' + encodeURIComponent(url) + '&mode=cors&method=GET',
     [fetchResolved, hasContentLength, noServerHeader, hasBody, typeBasic],
     [checkJsonpSuccess]],
    [BASE_URL + 'url=' + encodeURIComponent(url) + '&mode=no-cors&method=GET',
     [fetchResolved, hasContentLength, noServerHeader, hasBody, typeBasic],
     [checkJsonpSuccess]],

    // blob: requests with non-GET methods.
    [BASE_URL + 'url=' + encodeURIComponent(url) +
     '&mode=same-origin&method=POST',
     [fetchRejected]],
    [BASE_URL + 'url=' + encodeURIComponent(url) +
     '&mode=same-origin&method=HEAD',
     [fetchRejected]],
  ];
}

if (self.importScripts) {
  executeTests(TEST_TARGETS);
  done();
}
