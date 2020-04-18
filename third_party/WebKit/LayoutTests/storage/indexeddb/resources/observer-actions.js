if (this.importScripts) {
  importScripts('promise-idb.js');
}

window=this;
var isWorker = window.document === undefined;

function indexeddb_observers_actions(db1_name, db2_name, error_callback) {
  var perform_db1_actions_part1 = function() {
    var open_request = indexedDB.open(db1_name);
    open_request.onsuccess = function() {
      var db = open_request.result;
      pdb.transact(db, ['store1', 'store2'], 'readwrite').then(function(txn) {
        var os1 = txn.objectStore('store1');
        var os2 = txn.objectStore('store2');
        return Promise.all(
          [pdb.delete(os1, IDBKeyRange.bound('a', 'b')),
           pdb.put(os1, 'c', 'a'),
           pdb.add(os2, 'z', 'z'),
           pdb.waitForTransaction(txn)]);
      }).catch(error_callback).then(perform_db1_actions_part2);
    }
  }

  var perform_db1_actions_part2 = function() {
    var open_request = indexedDB.open(db1_name);
    open_request.onsuccess = function() {
      var db = open_request.result;
      pdb.transact(db, ['store1', 'store2'], 'readwrite').then(function(txn) {
        var os1 = txn.objectStore('store1');
        var os2 = txn.objectStore('store2');
        return Promise.all(
          [pdb.delete(os1, IDBKeyRange.bound('a', 'b')),
           pdb.put(os1, 'd', 'a'),
           pdb.put(os2, 'a', 'z'),
           pdb.waitForTransaction(txn)]);
      }).catch(error_callback);
    }
  }

  var perform_db2_actions_part1 = function() {
    var open_request = indexedDB.open(db2_name);
    open_request.onsuccess = function() {
      var db = open_request.result;
      pdb.transact(db, ['store3', 'store4'], 'readwrite').then(function(txn) {
        var os1 = txn.objectStore('store3');
        var os2 = txn.objectStore('store4');
        return Promise.all(
          [pdb.put(os1, 'd', 'c'),
           pdb.add(os2, 'z', 'z'),
           pdb.waitForTransaction(txn)]);
      }).catch(error_callback).then(perform_db2_actions_part2);
    }
  }

  var perform_db2_actions_part2 = function() {
    var open_request = indexedDB.open(db2_name);
    open_request.onsuccess = function() {
      var db = open_request.result;
      pdb.transact(db, ['store3', 'store4'], 'readwrite').then(function(txn) {
        var os1 = txn.objectStore('store3');
        var os2 = txn.objectStore('store4');
        return Promise.all(
          [pdb.put(os1, 'e', 'c'),
           pdb.put(os2, 'f', 'z'),
           pdb.waitForTransaction(txn)]);
      }).catch(error_callback);
    }
  }
  perform_db1_actions_part1();
  perform_db2_actions_part1();
}

function increment_key_actions(db_name, num_iters, key) {
  var open_request = indexedDB.open(db_name);
  open_request.onsuccess = function() {
    var db = open_request.result;

    var increment_number = function(old_value, num_left) {
      if (num_left == 0) return;
      var txn = db.transaction(['store'], 'readwrite');
      var os = txn.objectStore('store');
      var new_value = old_value + 1;
      os.put(new_value, key);
      txn.oncomplete = function() {
        increment_number(new_value, num_left - 1);
      };
    };
    increment_number(0, num_iters);
  };
};

if (isWorker && location.hash != "") {
  var hash = location.hash.split("#")[1];
  var names = JSON.parse(decodeURIComponent(hash));

  var errorCallback = function(event) {
    console.log('Error in actions: ' + event.target.error.message);
  }
  if (names.incrementing_actions) {
    increment_key_actions(names.db_name, names.num_iters, names.key);
  } else {
    indexeddb_observers_actions(names.db1_name, names.db2_name, errorCallback);
  }
}
