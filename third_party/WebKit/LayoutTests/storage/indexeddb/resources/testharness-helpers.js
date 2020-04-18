function delete_then_open(t, db_name, upgrade_func, body_func) {
  var delete_request = indexedDB.deleteDatabase(db_name);
  delete_request.onerror = t.unreached_func('deleteDatabase should not fail');
  delete_request.onsuccess = t.step_func(function(e) {
    var open_request = indexedDB.open(db_name);
    open_request.onupgradeneeded = t.step_func(function() {
      upgrade_func(t, open_request.result, open_request);
    });
    open_request.onsuccess = t.step_func(function() {
      body_func(t, open_request.result);
    });
  });
}

function indexeddb_test(upgrade_func, body_func, description) {
  async_test(function(t) {
    var db_name = 'db' + self.location.pathname + '-' + description;
    delete_then_open(t, db_name, upgrade_func, body_func);
  }, description);
}

function assert_key_equals(a, b, message) {
  assert_equals(indexedDB.cmp(a, b), 0, message);
}

// Call with a Test and an array of expected results in order. Returns
// a function; call the function when a result arrives and when the
// expected number appear the order will be asserted and test
// completed.
function expect(t, expected) {
  var results = [];
  return result => {
    results.push(result);
    if (results.length === expected.length) {
      assert_array_equals(results, expected);
      t.done();
    }
  };
}

// Creates a barrier that one calls
function create_barrier(callback) {
  var count = 0;
  var called = false;
  return function(t) {
    assert_false(called, "Barrier already used.");
    ++count;
    return t.step_func(function() {
      --count;
      if (count === 0) {
        assert_false(called, "Barrier already used.");
        called = true;
        callback();
      }
    });
  }
}
