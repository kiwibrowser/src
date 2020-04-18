// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Contains a simple factory for creating and opening Gnubby
 * instances.
 */
'use strict';

/**
 * @param {Gnubbies} gnubbies Gnubbies singleton instance
 * @constructor
 * @implements {GnubbyFactory}
 */
function UsbGnubbyFactory(gnubbies) {
  /** @private {Gnubbies} */
  this.gnubbies_ = gnubbies;
  Gnubby.setGnubbies(gnubbies);
}

/**
 * Creates a new gnubby object, and opens the gnubby with the given index.
 * @param {GnubbyDeviceId} which The device to open.
 * @param {boolean} forEnroll Whether this gnubby is being opened for enrolling.
 * @param {FactoryOpenCallback} cb Called with result of opening the gnubby.
 * @param {string=} opt_appIdHash The base64-encoded hash of the app id for
 *     which the gnubby being opened.
 * @param {string=} opt_logMsgUrl The url to post log messages to.
 * @param {string=} opt_caller Identifier for the caller.
 * @return {undefined} no open canceller needed for this type of gnubby
 * @override
 */
UsbGnubbyFactory.prototype.openGnubby = function(
    which, forEnroll, cb, opt_appIdHash, opt_logMsgUrl, opt_caller) {
  var gnubby = new Gnubby();
  gnubby.open(which, GnubbyEnumerationTypes.ANY, function(rc) {
    if (rc) {
      cb(rc, gnubby);
      return;
    }
    gnubby.sync(function(rc) {
      cb(rc, gnubby);
    });
  }, opt_caller);
};

/**
 * Enumerates gnubbies.
 * @param {function(number, Array<GnubbyDeviceId>)} cb Enumerate callback
 */
UsbGnubbyFactory.prototype.enumerate = function(cb) {
  this.gnubbies_.enumerate(cb);
};

/**
 * No-op prerequisite check.
 * @param {Gnubby} gnubby The not-enrolled gnubby.
 * @param {string} appIdHash The base64-encoded hash of the app id for which
 *     the gnubby being enrolled.
 * @param {FactoryOpenCallback} cb Called with the result of the prerequisite
 *     check. (A non-zero status indicates failure.)
 */
UsbGnubbyFactory.prototype.notEnrolledPrerequisiteCheck = function(
    gnubby, appIdHash, cb) {
  cb(DeviceStatusCodes.OK_STATUS, gnubby);
};

/**
 * No-op post enroll action.
 * @param {Gnubby} gnubby The just-enrolled gnubby.
 * @param {string} appIdHash The base64-encoded hash of the app id for which
 *     the gnubby was enrolled.
 * @param {FactoryOpenCallback} cb Called with the result of the action.
 *     (A non-zero status indicates failure.)
 */
UsbGnubbyFactory.prototype.postEnrollAction = function(gnubby, appIdHash, cb) {
  cb(DeviceStatusCodes.OK_STATUS, gnubby);
};
