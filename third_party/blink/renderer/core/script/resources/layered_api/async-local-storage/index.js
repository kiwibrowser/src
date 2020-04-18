// TODOs/spec-noncompliances:
// - Susceptible to tampering of built-in prototypes and globals. We want to work on tooling to ameliorate that.
// - Uses symbols for information hiding but those are interceptable and forgeable. Need private fields/methods.

const databaseName = Symbol("[[DatabaseName]]");
const databasePromise = Symbol("[[DatabasePromise]]");

export class StorageArea {
  constructor(name) {
    this[databasePromise] = null;
    this[databaseName] = "async-local-storage:" + `${name}`;
  }

  async set(key, value) {
    throwForDisallowedKey(key);

    return performDatabaseOperation(this, "readwrite", (transaction, store) => {
      store.put(value, key);

      return new Promise((resolve, reject) => {
        transaction.oncomplete = () => resolve();
        transaction.onabort = () => reject(transaction.error);
        transaction.onerror = () => reject(transaction.error);
      });
    });
  }

  async get(key) {
    throwForDisallowedKey(key);

    return performDatabaseOperation(this, "readonly", (transaction, store) => {
      const request = store.get(key);

      return new Promise((resolve, reject) => {
        request.onsuccess = () => resolve(request.result);
        request.onerror = () => reject(request.error);
      });
    });
  }

  async has(key) {
    throwForDisallowedKey(key);

    return performDatabaseOperation(this, "readonly", (transaction, store) => {
      const request = store.count(key);

      return new Promise((resolve, reject) => {
        request.onsuccess = () => resolve(request.result === 0 ? false : true);
        request.onerror = () => reject(request.error);
      });
    });
  }

  async delete(key) {
    throwForDisallowedKey(key);

    return performDatabaseOperation(this, "readwrite", (transaction, store) => {
      store.delete(key);

      return new Promise((resolve, reject) => {
        transaction.oncomplete = () => resolve();
        transaction.onabort = () => reject(transaction.error);
        transaction.onerror = () => reject(transaction.error);
      });
    });
  }

  async clear() {
    if (!(databasePromise in this)) {
      return Promise.reject(new TypeError("Invalid this value"));
    }

    if (this[databasePromise] !== null) {
      return this[databasePromise].then(
        () => {
          this[databasePromise] = null;
          return deleteDatabase(this[databaseName]);
        },
        () => {
          this[databasePromise] = null;
          return deleteDatabase(this[databaseName]);
        }
      );
    }

    return deleteDatabase(this[databaseName]);
  }

  async keys() {
    return performDatabaseOperation(this, "readonly", (transaction, store) => {
      const request = store.getAllKeys(undefined);

      return new Promise((resolve, reject) => {
        request.onsuccess = () => resolve(request.result);
        request.onerror = () => reject(request.error);
      });
    });
  }

  async values() {
    return performDatabaseOperation(this, "readonly", (transaction, store) => {
      const request = store.getAll(undefined);

      return new Promise((resolve, reject) => {
        request.onsuccess = () => resolve(request.result);
        request.onerror = () => reject(request.error);
      });
    });
  }

  async entries() {
    return performDatabaseOperation(this, "readonly", (transaction, store) => {
      const keysRequest = store.getAllKeys(undefined);
      const valuesRequest = store.getAll(undefined);

      return new Promise((resolve, reject) => {
        keysRequest.onerror = () => reject(keysRequest.error);
        valuesRequest.onerror = () => reject(valuesRequest.error);

        valuesRequest.onsuccess = () => {
          resolve(zip(keysRequest.result, valuesRequest.result));
        };
      });
    });
  }

  get backingStore() {
    if (!(databasePromise in this)) {
      throw new TypeError("Invalid this value");
    }

    return {
      database: this[databaseName],
      store: "store",
      version: 1
    };
  }
}

export const storage = new StorageArea("default");

function performDatabaseOperation(area, mode, steps) {
  if (!(databasePromise in area)) {
    return Promise.reject(new TypeError("Invalid this value"));
  }

  if (area[databasePromise] === null) {
    initializeDatabasePromise(area);
  }

  return area[databasePromise].then(database => {
    const transaction = database.transaction("store", mode);
    const store = transaction.objectStore("store");

    return steps(transaction, store);
  });
}

function initializeDatabasePromise(area) {
  area[databasePromise] = new Promise((resolve, reject) => {
    const request = self.indexedDB.open(area[databaseName], 1);

    request.onsuccess = () => {
      const database = request.result;
      database.onclose = () => area[databasePromise] = null;
      database.onversionchange = () => {
        database.close();
        area[databasePromise] = null;
      }
      resolve(database);
    };

    request.onerror = () => reject(request.error);

    request.onupgradeneeded = () => {
      try {
        request.result.createObjectStore("store");
      } catch (e) {
        reject(e);
      }
    };
  });
}

function isAllowedAsAKey(value) {
  if (typeof value === "number" || typeof value === "string") {
    return true;
  }

  if (Array.isArray(value)) {
    return true;
  }

  if (isDate(value)) {
    return true;
  }

  if (ArrayBuffer.isView(value)) {
    return true;
  }

  if (isArrayBuffer(value)) {
    return true;
  }

  return false;
}

function isDate(value) {
  try {
    Date.prototype.getTime.call(value);
    return true;
  } catch (e) { // TODO: remove useless binding
    return false;
  }
}

const byteLengthGetter = Object.getOwnPropertyDescriptor(ArrayBuffer.prototype, "byteLength").get;
function isArrayBuffer(value) {
  try {
    byteLengthGetter.call(value);
    return true;
  } catch (e) { // TODO: remove useless binding
    return false;
  }
}

function throwForDisallowedKey(key) {
  if (!isAllowedAsAKey(key)) {
    throw new DOMException("The given value is not allowed as a key", "DataError");
  }
}

function zip(a, b) {
  const result = [];
  for (let i = 0; i < a.length; ++i) {
    result.push([a[i], b[i]]);
  }

  return result;
}

function deleteDatabase(name) {
  return new Promise((resolve, reject) => {
    const request = self.indexedDB.deleteDatabase(name);
    request.onsuccess = () => resolve();
    request.onerror = () => reject(request.error);
  });
}
