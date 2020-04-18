// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var internalAPI = require('platformKeys.internalAPI');

var normalizeAlgorithm =
    requireNative('platform_keys_natives').NormalizeAlgorithm;

// Returns the normalized parameters of |importParams|.
// Any unknown parameters will be ignored.
function normalizeImportParams(importParams) {
  if (!importParams.name ||
      Object.prototype.toString.call(importParams.name) != '[object String]') {
    throw new Error('Algorithm: name: Missing or not a String');
  }

  var filteredParams = { name: importParams.name };

  var hashIsNone = false;
  if (importParams.hash) {
    if (importParams.hash.name.toLowerCase() === 'none') {
      hashIsNone = true;
      // Temporarily replace |hash| by a valid WebCrypto Hash for normalization.
      // This will be reverted to 'none' after normalization.
      filteredParams.hash = { name: 'SHA-1' };
    } else {
      filteredParams.hash = { name: importParams.hash.name }
    }
  }

  // Apply WebCrypto's algorithm normalization.
  var resultParams = normalizeAlgorithm(filteredParams, 'ImportKey');
  if (!resultParams ) {
    throw new Error('A required parameter was missing or out-of-range');
  }
  if (hashIsNone) {
    resultParams.hash = { name: 'none' };
  }
  return resultParams;
}

function combineAlgorithms(algorithm, importParams) {
  // internalAPI.getPublicKey returns publicExponent as ArrayBuffer, but it
  // should be a Uint8Array.
  if (algorithm.publicExponent) {
    algorithm.publicExponent = new Uint8Array(algorithm.publicExponent);
  }

  algorithm.hash = importParams.hash;
  return algorithm;
}

function getPublicKey(cert, importParams, callback) {
  importParams = normalizeImportParams(importParams);
  internalAPI.getPublicKey(
      cert, importParams.name, function(publicKey, algorithm) {
        if (chrome.runtime.lastError) {
          callback();
          return;
        }
        var combinedAlgorithm = combineAlgorithms(algorithm, importParams);
        callback(publicKey, combinedAlgorithm);
      });
}

exports.$set('getPublicKey', getPublicKey);
