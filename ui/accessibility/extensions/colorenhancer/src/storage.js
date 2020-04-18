// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TODO(wnwen): Move to chrome.storage.local, wrap calls, add JsDocs.

/**
 * Convert the string to boolean if possible.
 * @return {(boolean|string)} The Boolean value if possible, 'undefined'
 *     otherwise.
 */
function stringToBoolean(str) {
  return (str == 'true') ? true : (str == 'false') ? false : 'undefined';
}

function validBoolean(b) {
  return b == true || b == false;
}


// ======= Delta setting =======

/** @const {number} */ var DEFAULT_DELTA = 0.5;
/** @const {string} */ var LOCAL_STORAGE_TAG_DELTA = 'cvd_delta';
/** @const {string} */ var LOCAL_STORAGE_TAG_SITE_DELTA = 'cvd_site_delta';


function validDelta(delta) {
  return delta >= 0 && delta <= 1;
}


function getDefaultDelta() {
  var delta = localStorage[LOCAL_STORAGE_TAG_DELTA];
  if (validDelta(delta)) {
    return delta;
  }
  delta = DEFAULT_DELTA;
  localStorage[LOCAL_STORAGE_TAG_DELTA] = delta;
  return delta;
}


function setDefaultDelta(delta) {
  if (!validDelta(delta)) {
    delta = DEFAULT_DELTA;
  }
  localStorage[LOCAL_STORAGE_TAG_DELTA] = delta;
}


function getSiteDelta(site) {
  var delta = getDefaultDelta();
  try {
    var siteDeltas = JSON.parse(localStorage[LOCAL_STORAGE_TAG_SITE_DELTA]);
    delta = siteDeltas[site];
    if (!validDelta(delta)) {
      delta = getDefaultDelta();
    }
  } catch (e) {
    delta = getDefaultDelta();
  }
  return delta;
}


function setSiteDelta(site, delta) {
  if (!validDelta(delta)) {
    delta = getDefaultDelta();
  }
  var siteDeltas = {};
  try {
    siteDeltas = JSON.parse(localStorage[LOCAL_STORAGE_TAG_SITE_DELTA]);
  } catch (e) {
    siteDeltas = {};
  }
  siteDeltas[site] = delta;
  localStorage[LOCAL_STORAGE_TAG_SITE_DELTA] = JSON.stringify(siteDeltas);
}


function resetSiteDeltas() {
  var siteDeltas = {};
  localStorage[LOCAL_STORAGE_TAG_SITE_DELTA] = JSON.stringify(siteDeltas);
}


// ======= Severity setting =======

/** @const {number} */ var DEFAULT_SEVERITY = 1.0;
/** @const {string} */ var LOCAL_STORAGE_TAG_SEVERITY = 'cvd_severity';


function validSeverity(severity) {
  return severity >= 0 && severity <= 1;
}


function getDefaultSeverity() {
  var severity = localStorage[LOCAL_STORAGE_TAG_SEVERITY];
  if (validSeverity(severity)) {
    return severity;
  }
  severity = DEFAULT_SEVERITY;
  localStorage[LOCAL_STORAGE_TAG_SEVERITY] = severity;
  return severity;
}


function setDefaultSeverity(severity) {
  if (!validSeverity(severity)) {
    severity = DEFAULT_SEVERITY;
  }
  localStorage[LOCAL_STORAGE_TAG_SEVERITY] = severity;
}


// ======= Type setting =======

/** @const {string} */ var INVALID_TYPE_PLACEHOLDER = '';
/** @const {string} */ var LOCAL_STORAGE_TAG_TYPE = 'cvd_type';


function validType(type) {
  return type === 'PROTANOMALY' ||
      type === 'DEUTERANOMALY' ||
      type === 'TRITANOMALY';
}


function getDefaultType() {
  var type = localStorage[LOCAL_STORAGE_TAG_TYPE];
  if (validType(type)) {
    return type;
  }
}


function setDefaultType(type) {
  if (!validType(type)) {
    type = INVALID_TYPE_PLACEHOLDER;
  }
  localStorage[LOCAL_STORAGE_TAG_TYPE] = type;
}


// ======= Simulate setting =======

/** @const {boolean} */ var DEFAULT_SIMULATE = false;
/** @const {string} */ var LOCAL_STORAGE_TAG_SIMULATE = 'cvd_simulate';


function getDefaultSimulate() {
  var simulate = localStorage[LOCAL_STORAGE_TAG_SIMULATE];

  simulate = stringToBoolean(simulate);

  if (validBoolean(simulate)) {
    return simulate;
  }
  simulate = DEFAULT_SIMULATE;
  localStorage[LOCAL_STORAGE_TAG_SIMULATE] = simulate;
  return simulate;
}


function setDefaultSimulate(simulate) {
  if (!validBoolean(simulate)) {
    simulate = DEFAULT_SIMULATE;
  }
  localStorage[LOCAL_STORAGE_TAG_SIMULATE] = simulate;
}


// ======= Enable setting =======

/** @const {boolean} */ var DEFAULT_ENABLE = false;
/** @const {string} */ var LOCAL_STORAGE_TAG_ENABLE = 'cvd_enable';


function validEnable(enable) {
  return enable == true || enable == false;
}


function getDefaultEnable() {
  var enable = localStorage[LOCAL_STORAGE_TAG_ENABLE];

  enable = stringToBoolean(enable);

  if (validBoolean(enable)) {
    return enable;
  }
  enable = DEFAULT_ENABLE;
  localStorage[LOCAL_STORAGE_TAG_ENABLE] = enable;
  return enable;
}


function setDefaultEnable(enable) {
  if (!validBoolean(enable)) {
    enable = DEFAULT_ENABLE;
  }
  localStorage[LOCAL_STORAGE_TAG_ENABLE] = enable;
}
