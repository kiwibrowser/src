// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var INTERVAL_FOR_WAIT_UNTIL = 100; // ms

/**
 * Invokes a callback function depending on the result of promise.
 *
 * @param {Promise} promise Promise.
 * @param {function(boolean)} callback Callback function. True is passed if the
 *     test failed.
 */
function reportPromise(promise, callback) {
  promise.then(function() {
    callback(/* error */ false);
  }, function(error) {
    console.error(error.stack || error);
    callback(/* error */ true);
  });
}

/**
 * Waits until testFunction becomes true.
 * @param {function(): boolean} testFunction A function which is tested.
 * @return {!Promise} A promise which is fullfilled when the testFunction
 *     becomes true.
 */
function waitUntil(testFunction) {
  return new Promise(function(resolve) {
    var tryTestFunction = function() {
      if (testFunction())
        resolve();
      else
        setTimeout(tryTestFunction, INTERVAL_FOR_WAIT_UNTIL);
    };

    tryTestFunction();
  });
}

/**
 * Asserts that two lists contain the same set of Entries.  Entries are deemed
 * to be the same if they point to the same full path.
 *
 * @param {!Array<!FileEntry>} expected
 * @param {!Array<!FileEntry>} actual
 */
function assertFileEntryListEquals(expected, actual) {

  var entryToPath = function(entry) {
    assertTrue(entry.isFile);
    return entry.fullPath;
  };

  assertFileEntryPathsEqual(expected.map(entryToPath), actual);
}

/**
 * Asserts that a list of FileEntry instances point to the expected paths.
 *
 * @param {!Array<string>} expectedPaths
 * @param {!Array<!FileEntry>} fileEntries
 */
function assertFileEntryPathsEqual(expectedPaths, fileEntries) {
  assertEquals(expectedPaths.length, fileEntries.length);

  var entryToPath = function(entry) {
    assertTrue(entry.isFile);
    return entry.fullPath;
  };

  var actualPaths = fileEntries.map(entryToPath);
  actualPaths.sort();
  expectedPaths = expectedPaths.slice();
  expectedPaths.sort();

  assertEquals(
      JSON.stringify(expectedPaths),
      JSON.stringify(actualPaths));
}

/**
 * A class that captures calls to a funtion so that values can be validated.
 * For use in tests only.
 *
 * <p>Example:
 * <pre>
 *   var recorder = new TestCallRecorder();
 *   someClass.addListener(recorder.callback);
 *   // do stuff ...
 *   recorder.assertCallCount(1);
 *   assertEquals(recorder.getListCall()[0], 'hammy');
 * </pre>
 * @constructor
 */
function TestCallRecorder() {
  /** @private {!Array<!Argument>} */
  this.calls_ = [];

  /**
   * The recording funciton. Bound in our constructor to ensure we always
   * return the same object. This is necessary as some clients may make use
   * of object equality.
   *
   * @type {function()}
   */
  this.callback = this.recordArguments_.bind(this);
}

/**
 * Records the magic {@code arguments} value for later inspection.
 * @private
 */
TestCallRecorder.prototype.recordArguments_ = function() {
  this.calls_.push(arguments);
};

/**
 * Asserts that the recorder was called {@code expected} times.
 * @param {number} expected The expected number of calls.
 */
TestCallRecorder.prototype.assertCallCount = function(expected) {
  var actual = this.calls_.length;
  assertEquals(
      expected, actual,
      'Expected ' + expected + ' call(s), but was ' + actual + '.');
};

/**
 * @return {?Arguments} Returns the {@code Arguments} for the last call,
 *    or null if the recorder hasn't been called.
 */
TestCallRecorder.prototype.getLastArguments = function() {
  return (this.calls_.length === 0) ?
      null :
      this.calls_[this.calls_.length - 1];
};

/**
 * @param {number} index Index of which args to return.
 * @return {?Arguments} Returns the {@code Arguments} for the call specified
 *    by indexd.
 */
TestCallRecorder.prototype.getArguments = function(index) {
  return (index < this.calls_.length) ?
      this.calls_[index] :
      null;
};

/**
 * @constructor
 * @struct
 */
function MockAPIEvent() {
  /**
   * @type {!Array<!Function>}
   * @const
   */
  this.listeners_ = [];
}

/**
 * @param {!Function} callback
 */
MockAPIEvent.prototype.addListener = function(callback) {
  this.listeners_.push(callback);
};

/**
 * @param {!Function} callback
 */
MockAPIEvent.prototype.removeListener = function(callback) {
  var index = this.listeners_.indexOf(callback);
  if (index < 0)
    throw new Error('Tried to remove an unregistered listener.');
  this.listeners_.splice(index, 1);
};

/**
 * @param {...*} var_args
 */
MockAPIEvent.prototype.dispatch = function(var_args) {
  for (var i = 0; i < this.listeners_.length; i++) {
    this.listeners_[i].apply(null, arguments);
  }
};

/**
 * Stubs the chrome.storage API.
 * @construct
 * @struct
 */
function MockChromeStorageAPI() {
  /** @type {Object<?>} */
  this.state = {};

  window.chrome = window.chrome || {};
  window.chrome.runtime = window.chrome.runtime || {};  // For lastError.
  window.chrome.storage = {
    local: {
      get: this.get_.bind(this),
      set: this.set_.bind(this)
    }
  };
}

/**
 * @param {Array<string>|string} keys
 * @param {function(Object<?>)} callback
 * @private
 */
MockChromeStorageAPI.prototype.get_ = function(keys, callback) {
  var keys = keys instanceof Array ? keys : [keys];
  var result = {};
  keys.forEach(
      function(key) {
        if (key in this.state)
          result[key] = this.state[key];
      }.bind(this));
  callback(result);
};

/**
 * @param {Object<?>} values
 * @param {function()=} opt_callback
 * @private
 */
MockChromeStorageAPI.prototype.set_ = function(values, opt_callback) {
  for (var key in values) {
    this.state[key] = values[key];
  }
  if (opt_callback)
    opt_callback();
};

/**
 * Mocks chrome.commandLinePrivate.
 * @constructor
 */
function MockCommandLinePrivate() {
  this.flags_ = {};
  if (!chrome) {
    chrome = {};
  }
  if (!chrome.commandLinePrivate) {
    chrome.commandLinePrivate = {};
  }
  chrome.commandLinePrivate.hasSwitch = function(name, callback) {
    window.setTimeout(() => {
      callback(name in this.flags_);
    }, 0);
  }.bind(this);
}

/**
 * Add a switch.
 * @param {string} name of the switch to add.
 */
MockCommandLinePrivate.prototype.addSwitch = function(name) {
  this.flags_[name] = true;
};
