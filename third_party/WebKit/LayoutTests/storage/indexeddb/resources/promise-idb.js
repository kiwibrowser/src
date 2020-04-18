// Quick and dirty promise wrapper of IDB.

var pdb = {
  _transformRequestToPromise: function(thisobj, func, argArray) {
    return new Promise(function(resolve, reject) {
      var request = func.apply(thisobj, argArray);
      request.onsuccess = function() {
        resolve(request.result);
      };
      request.onerror = reject;
    })
  },

  transact: function(db, objectStores, style) {
    return Promise.resolve(db.transaction(objectStores, style));
  },

  openCursor: function(txn, indexOrObjectStore, keyrange, callback) {
    return new Promise(function(resolve, reject) {
      var request = indexOrObjectStore.openCursor(keyrange);
      request.onerror = reject;
      request.onsuccess = function() {
        var cursor = request.result;
        var cont = false;
        var control = {
          continue: function() {cont = true;}
        };
        if (cursor) {
          callback(control, cursor.value);
          if (cont) {
            cursor.continue();
          } else {
            resolve(txn);
          }
        } else {
          resolve(txn);
        }
      };
    });
  },

  get: function(indexOrObjectStore, key) {
    return this._transformRequestToPromise(indexOrObjectStore, indexOrObjectStore.get, [key]);
  },

  count: function(indexOrObjectStore, key) {
    return this._transformRequestToPromise(indexOrObjectStore, indexOrObjectStore.count, [key]);
  },

  put: function(objectStore, key, value) {
    return this._transformRequestToPromise(objectStore, objectStore.put, [key, value]);
  },

  add: function(objectStore, key, value) {
    return this._transformRequestToPromise(objectStore, objectStore.add, [key, value]);
  },

  delete: function(objectStore, key) {
    return this._transformRequestToPromise(objectStore, objectStore.delete, [key]);
  },

  clear: function(objectStore) {
    return this._transformRequestToPromise(objectStore, objectStore.clear, []);
  },

  getKey: function(index, key) {
    return this._transformRequestToPromise(index, index.getKey, [key]);
  },

  waitForTransaction: function(txn) {
    return new Promise(function(resolve, reject) {
      txn.oncomplete = resolve;
      txn.onerror = reject;
      txn.onabort = reject;
    });
  }
}