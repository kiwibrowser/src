// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.module('mr.MojoUtils');
goog.module.declareLegacyNamespace();


/**
 * Converts a mojo.Origin object to a string.
 * @param {!mojo.Origin} origin
 * @return {string}
 */
exports.mojoOriginToString = function(origin) {
  if (origin.unique) {
    return '';
  } else {
    return `${origin.scheme}:\/\/${origin.host}
       ${origin.port ? `:${origin.port}` : ''}/`;
  }
};


/**
 * Converts an origin string to a mojo.Origin object.
 * @param {string} origin
 * @return {!mojo.Origin}
 */
exports.stringToMojoOrigin = function(origin) {
  const url = new URL(origin);
  const mojoOrigin = {};
  mojoOrigin.scheme = url.protocol.replace(':', '');
  mojoOrigin.host = url.hostname;
  var port = url.port ? Number.parseInt(url.port, 10) : 0;
  switch (mojoOrigin.scheme) {
    case 'http':
      mojoOrigin.port = port || 80;
      break;
    case 'https':
      mojoOrigin.port = port || 443;
      break;
    default:
      throw new Error('Scheme must be http or https');
  }
  mojoOrigin.suborigin = '';
  return new mojo.Origin(mojoOrigin);
};

/**
 * @param {?mojo.TimeDelta} timeDelta
 * @return {number}
 */
exports.timeDeltaToSeconds = function(timeDelta) {
  return timeDelta ? timeDelta.microseconds / 1000000 : 0;
};

/**
 * @param {number} seconds
 * @return {!mojo.TimeDelta}
 */
exports.secondsToTimeDelta = function(seconds) {
  return new mojo.TimeDelta({microseconds: Math.floor(seconds * 1000000)});
};
