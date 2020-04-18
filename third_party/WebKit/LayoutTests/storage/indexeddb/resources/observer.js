if (this.importScripts) {
    importScripts('../../../resources/testharness.js');
    importScripts('testharness-helpers.js');
    importScripts('generic-idb-operations.js');
}

async_test(function(t) {
  var dbname = location.pathname + ' - ' + 'empty transaction';
  var obs = new IDBObserver(t.step_func(() => { t.done(); }));
  delete_then_open(t, dbname, function(t, db, request) {
    db.createObjectStore('store');
  }, function(t, db) {
    var tx1 = db.transaction('store', 'readwrite');
    obs.observe(db, tx1, {operationTypes: ['put']});
    tx1.onerror = t.unreached_func('transaction should not fail');
    var tx2 = db.transaction('store', 'readwrite');
    tx2.objectStore('store').put(1, 1);
    tx2.onerror = t.unreached_func('transaction should not fail');
  });
}, 'Registering observe call with empty transaction');

async_test(function(t) {
  var dbname = location.pathname + ' - ' + 'observe in version change';
  var obs = new IDBObserver(t.step_func(function(changes) { }));

  delete_then_open(t, dbname, function(t, db, request) {
    request.result.createObjectStore('store');
    assert_throws("TransactionInactiveError", function() {
      obs.observe(db, request.transaction, { operationTypes: ['put'] });
    });
    t.done();
  }, function() {});
}, 'Cannot observe during version change');

async_test(function(t) {
  var dbname = location.pathname + ' - ' + 'abort associated transaction';
  var obs = new IDBObserver(t.unreached_func('Observe in aborted transaction should be ignored'));
  delete_then_open(t, dbname, function(t, db, request) {
    request.result.createObjectStore('store');
  }, function(t, db) {
    var tx1 = db.transaction('store', 'readwrite');
    tx1.objectStore('store').get(1);
    obs.observe(db, tx1, { operationTypes: ['put'] });
    tx1.oncomplete = t.unreached_func('transaction should not complete');
    tx1.abort();

    var tx2 = db.transaction('store', 'readwrite');
    tx2.objectStore('store').put(1, 1);
    tx2.onerror = t.unreached_func('transaction error should not fail');
    tx2.oncomplete = t.step_func(function() {
      t.done();
    });
  });
}, 'Abort transaction associated with observer');

async_test(function(t) {
  var dbname = location.pathname + ' - ' + 'abort transaction not recorded';
  var callback_count = 0;
  var obs = new IDBObserver(t.step_func(function(changes) {
      assert_equals(changes.records.get('store')[0].key.lower, 1);
      t.done();
    }));

  delete_then_open(t, dbname, function(t, db, request) {
    request.result.createObjectStore('store');
  }, function(t, db) {
    var tx1 = db.transaction('store', 'readwrite');
    obs.observe(db, tx1, { operationTypes: ['put'] });
    tx1.onerror = t.unreached_func('transaction should not fail');

    var tx2 = db.transaction('store', 'readwrite');
    tx2.objectStore('store').put(2, 2);
    tx2.oncomplete = t.unreached_func('transaction should not complete');
    tx2.abort();

    var tx3 = db.transaction('store', 'readwrite');
    tx3.objectStore('store').put(1, 1);
    tx3.onerror = t.unreached_func('transaction should not fail');
  });
}, 'Aborted transaction not recorded by observer');

done();
