importScripts('worker-testharness.js');
importScripts('/resources/get-host-info.js');
importScripts('test-helpers.js');

async_test(function(t) {
    var url = get_host_info()['HTTP_REMOTE_ORIGIN'] + '/dummy.html';
    fetch(new Request(url, {mode: 'same-origin'}))
      .then(
        t.unreached_func('Fetching must fail.'),
        function(e) {
          assert_equals(e.message, 'Failed to fetch');
          t.done();
        })
      .catch(unreached_rejection(t));
  }, 'Fetch API error message - not same origin request');

async_test(function(t) {
    var url = 'ftp://example.com/dummy.html';
    fetch(new Request(url, {mode: 'cors'}))
      .then(
        t.unreached_func('Fetching must fail.'),
        function(e) {
          assert_equals(e.message, 'Failed to fetch');
          t.done();
        })
      .catch(unreached_rejection(t));
  }, 'Fetch API error message - non http cors request');

async_test(function(t) {
    var url = 'about://blank';
    fetch(new Request(url))
      .then(
        t.unreached_func('Fetching must fail.'),
        function(e) {
          assert_equals(e.message, 'Failed to fetch');
          t.done();
        })
      .catch(unreached_rejection(t));
  }, 'Fetch API error message - unsupported scheme.');

async_test(function(t) {
    var url =
        new URL(get_host_info()['HTTP_ORIGIN'] + base_path() +
                'invalid-chunked-encoding.php').toString();
    fetch(new Request(url))
      .then(
        function(response) {
          return response.text().then(
              t.unreached_func('Getting text must fail.'),
              function(e) {
                assert_equals(e.message, 'Failed to fetch');
                t.done();
              });
        },
        t.unreached_func('Fetching must succeed.'))
      .catch(unreached_rejection(t));
  }, 'Fetch API error message - invalid chunked encoding.');

async_test(function(t) {
    var url =
        new URL(get_host_info()['HTTP_REMOTE_ORIGIN'] + base_path() +
                'fetch-access-control.php').toString();
    fetch(new Request(url))
      .then(
        t.unreached_func('Fetching must fail.'),
        function(e) {
          assert_equals(e.message, 'Failed to fetch');
          t.done();
        })
      .catch(unreached_rejection(t));
  }, 'Fetch API error message - cors error.');
