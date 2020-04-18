if (this.importScripts) {
  importScripts('../../../resources/testharness.js');
  importScripts('generic-idb-operations.js');
  importScripts('observer-helpers.js');
  importScripts('testharness-helpers.js');
}

setup({timeout: 20000});

var openDB = function(t, dbName, openFunc) {
  var openRequest = indexedDB.open(dbName);
  openRequest.onupgradeneeded = t.unreached_func('upgrade should not be needed');
  openRequest.onsuccess = t.step_func(function() {
    openFunc(openRequest.result);
  });
  openRequest.onerror = t.unreached_func('opening database should not fail');
};

indexeddb_observers_test(function(t, db1_name, db2_name, observers_added_callback) {
  var expectedChanges = {
    dbName: db1_name,
    records: {
      'store1': [{type: 'delete', key: {lower: 'a', upper: 'b'}},
                 {type: 'put', key: 'a'}],
      'store2': [{type: 'add', key: 'z'}]
    }
  };
  var connection = null;
  var observedTimes = 0;
  var observeFunction = function(changes) {
    assert_true(connection != null, "Observer called before db opened.");
    observedTimes++;
    assert_equals(1, observedTimes, "Observer was called after calling unobserve.");
    assertChangesEqual(changes, expectedChanges);
    obs.unobserve(connection);
    t.done();
  };

  var obs = new IDBObserver(t.step_func(observeFunction));

  openDB(t, db1_name, t.step_func(function(db) {
    connection = db;
    var txn = db.transaction(['store1', 'store2'], 'readwrite');

    // Verify initial state.
    var os1 = txn.objectStore('store1');
    var readReq1 = os1.get('a');
    readReq1.onsuccess = t.step_func(function() {
      assert_equals(readReq1.result, 'b', 'Initial state incorrect.');
    });
    var os2 = txn.objectStore('store2');
    var readReq2 = os2.get('x');
    readReq2.onsuccess = t.step_func(function() {
      assert_equals(readReq2.result, 'y', 'Initial state incorrect.');
    });

    // Start observing!
    obs.observe(db, txn, {operationTypes: ['clear', 'put', 'add', 'delete']});

    txn.oncomplete = observers_added_callback;
    txn.onerror = t.unreached_func('transaction should not fail');
  }));
}, 'IDB Observers: Verify initial state and record all operations');

indexeddb_observers_test(function(t, db1_name, db2_name, observers_added_callback) {
  var expectedChanges = {
    dbName: db1_name,
    records: {
      'store2': [{type: 'add', key: 'z'}]
    }
  };

  var connection = null;
  var observedTimes = 0;
  var observeFunction = function(changes) {
    assert_true(connection != null, "Observer called before db opened.");
    observedTimes++;
    assert_equals(1, observedTimes, "Observer was called after calling unobserve.");
    assertChangesEqual(changes, expectedChanges);
    obs.unobserve(connection);
    t.done();
  };

  var obs = new IDBObserver(t.step_func(observeFunction));

  openDB(t, db1_name, t.step_func(function(db) {
    connection = db;
    var txn = db.transaction(['store2'], 'readonly');
    obs.observe(db, txn, {operationTypes: ['add']});
    txn.oncomplete = observers_added_callback;
    txn.onerror = t.unreached_func('transaction should not fail')
  }));
}, 'IDB Observers: Operation filtering');

indexeddb_observers_test(function(t, db1_name, db2_name, observers_added_callback) {
  var expectedChanges = {
    dbName: db1_name,
    records: {
      'store2': [{type: 'add', key: 'z', value: 'z'}]
    }
  };

  var connection = null;
  var observedTimes = 0;
  var observeFunction = function(changes) {
    assert_true(connection != null, "Observer called before db opened.");
    observedTimes++;
    assert_equals(1, observedTimes, "Observer was called after calling unobserve.");
    assertChangesEqual(changes, expectedChanges);
    obs.unobserve(connection);
    t.done();
  };

  var obs = new IDBObserver(t.step_func(observeFunction));

  openDB(t, db1_name, t.step_func(function(db) {
    connection = db;
    var txn = db.transaction(['store2'], 'readonly');
    obs.observe(db, txn, {operationTypes: ['add'], values: true});
    txn.oncomplete = observers_added_callback;
    txn.onerror = t.unreached_func('transaction should not fail')
  }));
}, 'IDB Observers: Values');


indexeddb_observers_test(function(t, db1_name, db2_name, observers_added_callback) {
  var expectedChanges = {
    dbName: db1_name,
    records: {
      'store2': [{type: 'add', key: 'z'}],
    }
  };

  var connection = null;
  var observedTimes = 0;
  var observeFunction = function(changes) {
    assert_true(connection != null, "Observer called before db opened.");
    observedTimes++;
    assert_equals(1, observedTimes, "Observer was called after calling unobserve.");
    assertChangesEqual(changes, expectedChanges);
    obs.unobserve(connection);
    assert_true(changes.transaction != null);
    var store2 = changes.transaction.objectStore('store2');
    var request = store2.get('z');
    request.onsuccess = t.step_func(function() {
      assert_equals(request.result, 'z');
      t.done();
    });
  };

  var obs = new IDBObserver(t.step_func(observeFunction));

  openDB(t, db1_name, t.step_func(function(db) {
    connection = db;
    var txn = db.transaction(['store2'], 'readonly');
    obs.observe(db, txn, {operationTypes: ['add'], transaction: true});
    txn.oncomplete = observers_added_callback;
    txn.onerror = t.unreached_func('transaction should not fail')
  }));
}, 'IDB Observers: Transaction');

indexeddb_observers_test(function(t, db1_name, db2_name, observers_added_callback) {
  var expectedChanges = {
    dbName: db1_name,
    records: {
      'store2': [{type: 'add', key: 'z'}]
    }
  };

  var connection = null;
  var observedTimes = 0;
  var observeFunction = function(changes) {
    assert_true(connection != null, "Observer called before db opened.");
    observedTimes++;
    assert_equals(1, observedTimes, "Observer was called after calling unobserve.");
    assertChangesEqual(changes, expectedChanges);
    obs.unobserve(connection);
    t.done();
  };

  var obs = new IDBObserver(t.step_func(observeFunction));

  openDB(t, db1_name, t.step_func(function(db) {
    connection = db;
    var txn = db.transaction(['store2'], 'readonly');
    obs.observe(db, txn, {operationTypes: ['clear', 'put', 'add', 'delete']});
    txn.oncomplete = observers_added_callback;
    txn.onerror = t.unreached_func('transaction should not fail')
  }));
}, 'IDB Observers: Object store filtering');

indexeddb_observers_test(function(t, db1_name, db2_name, observers_added_callback) {
  var expectedChanges1 = {
    dbName: db1_name,
    records: {
      'store1': [{type: 'delete', key: {lower: 'a', upper: 'b'}},
                 {type: 'put', key: 'a'}],
      'store2': [{type: 'add', key: 'z'}]
    }
  };

  var expectedChanges2 = {
    dbName: db2_name,
    records: {
      'store3': [{type: 'put', key: 'c'}],
      'store4': [{type: 'add', key: 'z'}]
    }
  };

  var observers_added_barrier = create_barrier(observers_added_callback);

  var connection1 = null;
  var connection2 = null;
  var pendingObserves = 2;
  var observeFunction = function(changes) {
    pendingObserves = pendingObserves - 1;
    if (changes.database.name === db1_name) {
      assertChangesEqual(changes, expectedChanges1);
      assert_true(connection1 != null);
      obs.unobserve(connection1);
    } else if (changes.database.name === db2_name) {
      assertChangesEqual(changes, expectedChanges2);
      assert_true(connection2 != null);
      obs.unobserve(connection2);
    }
    if (pendingObserves === 0) {
      t.done();
    } else if (pendingObserves < 0) {
      assert_unreached("incorrect pendingObserves");
    }
  };

  var obs = new IDBObserver(t.step_func(observeFunction));

  var cb1 = observers_added_barrier(t);
  openDB(t, db1_name, t.step_func(function(db) {
    connection1 = db;
    var txn = db.transaction(['store1', 'store2'], 'readonly');
    obs.observe(db, txn, {operationTypes: ['clear', 'put', 'add', 'delete']});
    txn.oncomplete = cb1;
    txn.onerror = t.unreached_func('transaction should not fail')
  }));
  var cb2 = observers_added_barrier(t);
  openDB(t, db2_name, t.step_func(function(db) {
    connection2 = db;
    var txn = db.transaction(['store3', 'store4'], 'readonly');
    obs.observe(db, txn, {operationTypes: ['clear', 'put', 'add', 'delete']});
    txn.oncomplete = cb2;
    txn.onerror = t.unreached_func('transaction should not fail')
  }));
}, 'IDB Observers: Multiple connections');

indexeddb_observers_test(function(t, db1_name, db2_name, observers_added_callback) {
  var expectedChanges = {
    dbName: db1_name,
    records: {
      'store1': [{type: 'delete', key: {lower: 'a', upper: 'b'}},
                 {type: 'put', key: 'a'}],
      'store2': [{type: 'add', key: 'z'}]
    }
  };

  var connection = null;
  var pendingObserves = 2;
  var observeFunction = function(changes) {
    assertChangesEqual(changes, expectedChanges);
    pendingObserves = pendingObserves - 1;
    if (pendingObserves === 0) {
      assert_true(connection != null, "Observer called before db opened.");
      obs.unobserve(connection);
      t.done();
    } else if (pendingObserves < 0) {
      assert_unreached("incorrect pendingObserves");
    }
  };

  var obs = new IDBObserver(t.step_func(observeFunction));

  openDB(t, db1_name, t.step_func(function(db) {
    connection = db;
    var txn = db.transaction(['store1', 'store2'], 'readonly');
    obs.observe(db, txn, {operationTypes: ['clear', 'put', 'add', 'delete']});
    obs.observe(db, txn, {operationTypes: ['clear', 'put', 'add', 'delete']});
    txn.oncomplete = observers_added_callback;
    txn.onerror = t.unreached_func('transaction should not fail');
  }));
}, 'IDB Observers: Multiple observer calls');

indexeddb_observers_test(function(t, db1_name, db2_name, observers_added_callback) {
  var expectedChanges1 = {
    dbName: db1_name,
    records: {
      'store1': [{type: 'put', key: 'a'}]
    }
  };
  var expectedChanges2 = {
    dbName: db1_name,
    records: {
      'store2': [{type: 'add', key: 'z'}]
    }
  };

  var connection = null;
  var changeNumber = 0;
  var observeFunction = function(changes) {
    assert_true(connection != null, "Observer called before db opened.");
    if (changes.records.has('store1')) {
      assertChangesEqual(changes, expectedChanges1);
    } else if(changes.records.has('store2')) {
      assertChangesEqual(changes, expectedChanges2);
    }
    ++changeNumber;
    assert_less_than_equal(changeNumber, 2, "incorrect pendingObserves");
    if (changeNumber == 2) {
      obs.unobserve(connection);
      t.done();
    }
  };

  var obs = new IDBObserver(t.step_func(observeFunction));

  openDB(t, db1_name, t.step_func(function(db) {
    connection = db;
    var txn = db.transaction(['store1', 'store2'], 'readonly');
    obs.observe(db, txn, {operationTypes: ['put']});
    obs.observe(db, txn, {operationTypes: ['add']});
    txn.oncomplete = observers_added_callback;
    txn.onerror = t.unreached_func('transaction should not fail');
  }));
}, 'IDB Observers: Multiple observer calls with filtering');

indexeddb_observers_test(function(t, db1_name, db2_name, observers_added_callback) {
  var partOneChanges1 = {
    dbName: db1_name,
    records: {
      'store1': [{type: 'delete', key: {lower: 'a', upper: 'b'}},
                 {type: 'put', key: 'a'}]
    }
  };

  var partOneChanges2 = {
    dbName: db1_name,
    records: {
      'store2': [{type: 'add', key: 'z'}]
    }
  };

  var partOneChanges3 = {
    dbName: db2_name,
    records: {
      'store3': [{type: 'put', key: 'c'}],
      'store4': [{type: 'add', key: 'z'}]
    }
  };

  var partTwoChanges2 = {
    dbName: db1_name,
    records: {
      'store2': [{type: 'put', key: 'z'}]
    }
  };

  var connection1 = null;
  var connection2 = null;
  var connection3 = null;
  var observers_added_barrier = create_barrier(observers_added_callback);

  var pendingObserves = 4;
  var observeFunction = function(changes) {
    pendingObserves = pendingObserves - 1;
    var firstRound = pendingObserves != 0;

    if (changes.database === connection1) {
      assert_true(firstRound, "connection 1 should have been unobserved");
      assertChangesEqual(changes, partOneChanges1);
      obs.unobserve(connection1);
    } else if (changes.database === connection2) {
      if (firstRound)
        assertChangesEqual(changes, partOneChanges2);
      else
        assertChangesEqual(changes, partTwoChanges2);
    } else if (changes.database === connection3) {
      assert_true(firstRound, "connection 3 should have been unobserved ");
      assertChangesEqual(changes, partOneChanges3);
      obs.unobserve(connection3);
    } else {
      assert_unreached('Unknown connection supplied with changes.');
    }
    if (pendingObserves === 0) {
      t.done();
    } else if (pendingObserves < 0) {
      assert_unreached("incorrect pendingObserves");
    }
  };

  var obs = new IDBObserver(t.step_func(observeFunction));

  var cb1 = observers_added_barrier(t);
  openDB(t, db1_name, t.step_func(function(db) {
    connection1 = db;
    var txn = db.transaction(['store1'], 'readonly');
    obs.observe(db, txn, {operationTypes: ['clear', 'put', 'add', 'delete']});
    txn.oncomplete = cb1;
    txn.onerror = t.unreached_func('transaction should not fail');
  }));
  var cb2 = observers_added_barrier(t);
  openDB(t, db1_name, t.step_func(function(db) {
    connection2 = db;
    var txn = db.transaction(['store2'], 'readonly');
    obs.observe(db, txn, {operationTypes: ['clear', 'put', 'add', 'delete']});
    txn.oncomplete = cb2;
    txn.onerror = t.unreached_func('transaction should not fail');
  }));
  var cb3 = observers_added_barrier(t);
  openDB(t, db2_name, t.step_func(function(db) {
    connection3 = db;
    var txn = db.transaction(['store3', 'store4'], 'readonly');
    obs.observe(db, txn, {operationTypes: ['clear', 'put', 'add', 'delete']});
    txn.oncomplete = cb3;
    txn.onerror = t.unreached_func('transaction should not fail')
  }));
}, 'IDB Observers: Three connections, unobserve two');

indexeddb_observers_test(function(t, db1_name, db2_name, observers_added_callback) {
  var connection1 = null;
  var connection2 = null;

  var observedTimes = 0;
  var observeFunction = function(changes) {
    assert_true(connection2 != null, "Observer called before db opened.");
    observedTimes++;
    assert_equals(1, observedTimes, "Observer was called after calling unobserve.");
    assert_equals(changes.database, connection2);
    obs.unobserve(connection2);
    t.done();
  };

  var obs = new IDBObserver(t.step_func(observeFunction));

  openDB(t, db1_name, t.step_func(function(db) {
    connection1 = db;
    var txn = db.transaction(['store1'], 'readonly');
    obs.observe(db, txn, {operationTypes: ['clear', 'put', 'add', 'delete']});
    obs.unobserve(db);
    txn.oncomplete = t.step_func(function() {
      openDB(t, db1_name, t.step_func(function(db) {
        connection2 = db;
        var txn = db.transaction(['store2'], 'readonly');
        obs.observe(
            db, txn, {operationTypes: ['clear', 'put', 'add', 'delete']});
        txn.oncomplete = observers_added_callback;
        txn.onerror = t.unreached_func('transaction should not fail');
      }));
    });
    txn.onerror = t.unreached_func('transaction should not fail');
  }));
}, 'IDB Observers: Unobserve immediately');

indexeddb_observers_test(function(t, db1_name, db2_name, observers_added_callback) {
  var connection1 = null;
  var connection2 = null;

  var observeFunction = function(changes) {
    assert_equals(changes.database, connection2);
    obs.unobserve(connection2);
    t.done();
  };

  var obs = new IDBObserver(t.step_func(observeFunction));

  openDB(t, db1_name, t.step_func(function(db) {
    connection1 = db;
    var txn = db.transaction(['store1'], 'readonly');
    obs.observe(db, txn, {operationTypes: ['clear', 'put', 'add', 'delete']});
    obs.observe(db, txn, {operationTypes: ['clear', 'put', 'add', 'delete']});
    obs.observe(db, txn, {operationTypes: ['clear', 'put', 'add', 'delete']});
    obs.unobserve(db);
    txn.oncomplete = t.step_func(function() {
      openDB(t, db1_name, t.step_func(function(db) {
        connection2 = db;
        var txn = db.transaction(['store2'], 'readonly');
        obs.observe(
            db, txn, {operationTypes: ['clear', 'put', 'add', 'delete']});
        txn.oncomplete = observers_added_callback;
        txn.onerror = t.unreached_func('transaction should not fail');
      }));
    });
    txn.onerror = t.unreached_func('transaction should not fail');
  }));
}, 'IDB Observers: Unobserve immediately on multiple');

indexeddb_observers_test(function(t, db1_name, db2_name, observers_added_callback) {
  var expectedChanges1 = {
    dbName: db1_name,
    records: {
      'store1': [{type: 'delete', key: {lower: 'a', upper: 'b'}},
                 {type: 'put', key: 'a'}]
    }
  };
  var expectedChanges2 = {
    dbName: db1_name,
    records: {
      'store2': [{type: 'add', key: 'z'}]
    }
  };

  var connection1 = null;
  var connection2 = null;
  var observers_added_barrier = create_barrier(observers_added_callback);

  var connection1Observes = 0;
  var connection2Observes = 0;
  var observeFunction = function(changes) {
    if (changes.database === connection1) {
      connection1Observes = connection1Observes + 1;
      assert_true(connection1Observes <= 2);
      if (changes.records.has('store1')) {
        assertChangesEqual(changes, expectedChanges1);
      } else if (changes.records.has('store2')) {
        assertChangesEqual(changes, expectedChanges2);
      } else {
        assert_unreached("unknown changes");
      }
      if (connection1Observes === 2) {
        obs.unobserve(connection1);
      }
    } else if (changes.database === connection2) {
      connection2Observes = connection2Observes + 1;
      if (connection2Observes > 1) {
        t.done();
      }
    } else {
      assert_unreached("unknown changes");
    }
  };

  var obs = new IDBObserver(t.step_func(observeFunction));

  var cb1 = observers_added_barrier(t);
  var cb2 = observers_added_barrier(t);
  openDB(t, db1_name, t.step_func(function(db) {
    connection1 = db;
    var txn1 = db.transaction(['store1'], 'readonly');
    var txn2 = db.transaction(['store2'], 'readonly');
    // Start observing!
    obs.observe(db, txn1, {operationTypes: ['clear', 'put', 'add', 'delete']});
    obs.observe(db, txn2, {operationTypes: ['clear', 'put', 'add', 'delete']});

    txn1.oncomplete = cb1;
    txn2.oncomplete = cb2;
    txn1.onerror = t.unreached_func('transaction should not fail');
    txn2.onerror = t.unreached_func('transaction should not fail');
  }));
  var cb3 = observers_added_barrier(t);
  openDB(t, db2_name, t.step_func(function(db) {
    connection2 = db;
    var txn = db.transaction(['store3'], 'readonly');
    obs.observe(db, txn, {operationTypes: ['clear', 'put', 'add', 'delete']});
    txn.oncomplete = cb3;
    txn.onerror = t.unreached_func('transaction should not fail');
  }));
}, 'IDB Observers: Unobserve connection removes observers');

indexeddb_observers_test(function(t, db1_name, db2_name, observers_added_callback) {
  var expectedChanges1 = {
    dbName: db1_name,
    records: {
      'store1': [{type: 'delete', key: {lower: 'a', upper: 'b'}},
                 {type: 'put', key: 'a'}]
    }
  };
  var expectedChanges2 = {
    dbName: db1_name,
    records: {
      'store2': [{type: 'add', key: 'z'}]
    }
  };

  var connection1 = null;
  var connection2 = null;
  var observers_added_barrier = create_barrier(observers_added_callback);

  var connection1Observes = 0;
  var connection2Observes = 0;
  var observeFunction = function(changes) {
    if (changes.database === connection1) {
      connection1Observes = connection1Observes + 1;
      assert_true(connection1Observes <= 2);
      if (changes.records.has('store1')) {
        assertChangesEqual(changes, expectedChanges1);
      } else if (changes.records.has('store2')) {
        assertChangesEqual(changes, expectedChanges2);
      } else {
        assert_unreached("unknown changes");
      }
      if (connection1Observes === 2) {
        connection1.close();
      }
    } else if (changes.database === connection2) {
      connection2Observes = connection2Observes + 1;
      if (connection2Observes > 1) {
        t.done();
      }
    } else {
      assert_unreached("unknown changes");
    }
  };

  var obs = new IDBObserver(t.step_func(observeFunction));

  var cb1 = observers_added_barrier(t);
  var cb2 = observers_added_barrier(t);
  openDB(t, db1_name, t.step_func(function(db) {
    connection1 = db;
    var txn1 = db.transaction(['store1'], 'readonly');
    var txn2 = db.transaction(['store2'], 'readonly');
    // Start observing!
    obs.observe(db, txn1, {operationTypes: ['clear', 'put', 'add', 'delete']});
    obs.observe(db, txn2, {operationTypes: ['clear', 'put', 'add', 'delete']});

    txn1.oncomplete = cb1;
    txn2.oncomplete = cb2;
    txn1.onerror = t.unreached_func('transaction should not fail');
    txn2.onerror = t.unreached_func('transaction should not fail');
  }));
  var cb3 = observers_added_barrier(t);
  openDB(t, db2_name, t.step_func(function(db) {
    connection2 = db;
    var txn = db.transaction(['store3'], 'readonly');
    obs.observe(db, txn, {operationTypes: ['clear', 'put', 'add', 'delete']});
    txn.oncomplete = cb3;
    txn.onerror = t.unreached_func('transaction should not fail');
  }));
}, 'IDB Observers: Close connection removes observers');

done();
