if (this.importScripts) {
  importScripts('testharness-helpers.js');
  importScripts('observer-actions.js');
}

window=this;
var isWorker = window.document === undefined;

// This runs two tests with the given body function, one where the database
// changes happen in the same frame, and another where the changes happen in
// a separate worker.
function indexeddb_observers_test(body_func, description) {
  var db1_prefix = 'db' + self.location.pathname + '-' + description + '1';
  var db2_prefix = 'db' + self.location.pathname + '-' + description + '2';

  var createDatabases = function(t, db1_name, db2_name, done_callback) {
    var done_barrier = create_barrier(done_callback);
    delete_then_open(t, db1_name, function(t, db) {
      var os1 = db.createObjectStore('store1');
      var os2 = db.createObjectStore('store2');
      os1.put('b', 'a');
      os2.put('y', 'x');
    }, done_barrier(t));

    delete_then_open(t, db2_name, function(t, db) {
      var os1 = db.createObjectStore('store3');
      var os2 = db.createObjectStore('store4');
      os1.put('c', 'c');
      os2.put('w', 'w');
    }, done_barrier(t));
  }

  async_test(function(t) {
    var db1_name = db1_prefix + "-frame";
    var db2_name = db2_prefix + "-frame";

    var observers_added_callback = t.step_func(function() {
      indexeddb_observers_actions(
          db1_name, db2_name,
          t.unreached_func('Error performing observer actions in frame'));
    });
    var start_tests = t.step_func(function() {
      body_func(t, db1_name, db2_name, observers_added_callback);
    });
    createDatabases(t, db1_name, db2_name, start_tests);
  }, description + " (changes in frame).", { timeout: 20000 });

  async_test(function(t) {
    var db1_name = db1_prefix + "-worker";
    var db2_name = db2_prefix + "-worker";

    var observers_added_callback = t.step_func(function() {
      var name_dict = {
        db1_name: db1_name,
        db2_name: db2_name
      };
      var hash_string = encodeURIComponent(JSON.stringify(name_dict));
      var url = 'resources/observer-actions.js#' + hash_string;

      // Since Chrome doesn't support starting workers in workers, we have to
      // implement this postMessage protocol to ask our host frame to start the
      // given worker.
      if (isWorker) {
        postMessage({'start_worker_url': url});
      } else {
        new Worker(url);
      }
    });

    var start_tests = t.step_func(function() {
      body_func(t, db1_name, db2_name, observers_added_callback);
    });
    createDatabases(t, db1_name, db2_name, start_tests);
  }, description + ' (changes in worker).', { timeout: 20000 });
}
