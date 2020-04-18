/**
 * @license
 * Copyright (c) 2015 The Polymer Project Authors. All rights reserved.
 * This code may only be used under the BSD style license found at http://polymer.github.io/LICENSE.txt
 * The complete set of authors may be found at http://polymer.github.io/AUTHORS.txt
 * The complete set of contributors may be found at http://polymer.github.io/CONTRIBUTORS.txt
 * Code distributed by Google as part of the polymer project is also
 * subject to an additional IP rights grant found at http://polymer.github.io/PATENTS.txt
 */

// We declare it as a namespace to be compatible with JSCompiler.
/** @const */ var TestHelpers = {};

(function(scope, global) {
  'use strict';

  // In case the var above was not global, or if it was renamed.
  global.TestHelpers = scope;

  /**
   * Forces distribution of light children, and lifecycle callbacks on the
   * Custom Elements polyfill. Used when testing elements that rely on their
   * distributed children.
   */
  global.flushAsynchronousOperations = function() {
    // force distribution
    Polymer.dom.flush();
    // force lifecycle callback to fire on polyfill
    window.CustomElements && window.CustomElements.takeRecords();
  };

  /**
   * Stamps and renders a `dom-if` template.
   *
   * @param {!Element} node The node containing the template,
   */
  global.forceXIfStamp = function(node) {
    var templates = Polymer.dom(node.root).querySelectorAll('template[is=dom-if]');
    for (var tmpl, i = 0; tmpl = templates[i]; i++) {
      tmpl.render();
    }

    global.flushAsynchronousOperations();
  };

  /**
   * Fires a custom event on a specific node. This event bubbles and is cancellable.
   *
   * @param {string} type The type of event.
   * @param {?Object} props Any custom properties the event contains.
   * @param {!Element} node The node to fire the event on.
   */
  global.fireEvent = function(type, props, node) {
    var event = new CustomEvent(type, {
      bubbles: true,
      cancelable: true
    });
    for (var p in props) {
      event[p] = props[p];
    }
    node.dispatchEvent(event);
  };

  /**
   * Skips a test unless a condition is met. Sample use:
   *    function isNotIE() {
   *      return !navigator.userAgent.match(/MSIE/i);
   *    }
   *    test('runs on non IE browsers', skipUnless(isNotIE, function() {
   *      ...
   *    });
   *
   * @param {Function} condition The name of a Boolean function determining if the test should be run.
   * @param {Function} test The test to be run.
   */
  global.skipUnless = function(condition, test) {
    var isAsyncTest = !!test.length;

    return function(done) {
      var testCalledDone = false;

      if (!condition()) {
        return done();
      }

      var result = test.call(this, done);

      if (!isAsyncTest) {
        done();
      }

      return result;
    };
  };

  scope.flushAsynchronousOperations = global.flushAsynchronousOperations;
  scope.forceXIfStamp = global.forceXIfStamp;
  scope.fireEvent = global.fireEvent;
  scope.skipUnless = global.skipUnless;
})(TestHelpers, this);
