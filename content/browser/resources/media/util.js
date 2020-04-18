// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Some utility functions that don't belong anywhere else in the
 * code.
 */

var util = (function() {
  var util = {};
  util.object = {};
  /**
   * Calls a function for each element in an object/map/hash.
   *
   * @param obj The object to iterate over.
   * @param f The function to call on every value in the object.  F should have
   * the following arguments: f(value, key, object) where value is the value
   * of the property, key is the corresponding key, and obj is the object that
   * was passed in originally.
   * @param optObj The object use as 'this' within f.
   */
  util.object.forEach = function(obj, f, optObj) {
    'use strict';
    var key;
    for (key in obj) {
      if (obj.hasOwnProperty(key)) {
        f.call(optObj, obj[key], key, obj);
      }
    }
  };
  util.millisecondsToString = function(timeMillis) {
    function pad(num) {
      num = num.toString();
      if (num.length < 2) {
        return '0' + num;
      }
      return num;
    }

    var date = new Date(timeMillis);
    return pad(date.getUTCHours()) + ':' + pad(date.getUTCMinutes()) + ':' +
        pad(date.getUTCSeconds()) + ' ' + pad((date.getMilliseconds()) % 1000);
  };

  return util;
}());
