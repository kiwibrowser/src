// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview The media source URN related utilities methods.
 */

goog.provide('mr.MediaSourceUtils');

goog.require('mr.Config');




/**
 * @param {string} sourceUrn
 * @return {boolean} True if it is a mirror URN.
 */
mr.MediaSourceUtils.isMirrorSource = function(sourceUrn) {
  return mr.MediaSourceUtils.isTabMirrorSource(sourceUrn) ||
      mr.MediaSourceUtils.isDesktopMirrorSource(sourceUrn);
};


/**
 * @param {string} sourceUrn
 * @return {boolean} True if it is a two UA mode presentation source.
 *     A presentation source has a sourceUrn that is a valid uri and
 *     is not a Cast custom receiver app.
 */
mr.MediaSourceUtils.isPresentationSource = function(sourceUrn) {

  if (!sourceUrn.startsWith('http:') && !sourceUrn.startsWith('https:')) {
    return false;
  }
  // Use the DOM to parse sourceUrn.
  const link = document.createElement('a');
  link.href = sourceUrn;
  // Protocol must be http or https.
  if (link.protocol != 'http:' && link.protocol != 'https:') {
    return false;
  }

  // Must not be a custom Cast receiver app.
  return link.hash.indexOf(mr.MediaSourceUtils.CAST_APP_ID_) == -1;
};



/** @const {string} */
mr.MediaSourceUtils.CAST_STREAMING_APP_ID = '0F5096E8';


/** @const {string} */
mr.MediaSourceUtils.TAB_MIRROR_URN_PREFIX =
    'urn:x-org.chromium.media:source:tab:';

/** @const {string} */
mr.MediaSourceUtils.TAB_REMOTING_URN_PREFIX =
    'urn:x-org.chromium.media:source:tab_content_remoting:';

/** @const {string} */
mr.MediaSourceUtils.DESKTOP_MIRROR_URN =
    'urn:x-org.chromium.media:source:desktop';


/** @private @const {string} */
mr.MediaSourceUtils.CAST_APP_ID_ = '__castAppId__';


/**
 * @private @const {!Array<string>}
 */
mr.MediaSourceUtils.MIRROR_APP_ID_ORIGIN_WHITELIST_ = [
  'https://docs.google.com',  // slides
];


/**
 * @param {string} sourceUrn
 * @return {?Array<string>} array of origins whitelisted for the sourceUrn or
 *     |null| if any origin is allowed for the sourceUrn.
 */
mr.MediaSourceUtils.getWhitelistedOrigins = function(sourceUrn) {
  if (mr.Config.isDebugChannel &&
      window.localStorage['debug.allowAllOrigins']) {
    return null;
  }
  return sourceUrn.indexOf(mr.MediaSourceUtils.CAST_STREAMING_APP_ID) != -1 ?
      mr.MediaSourceUtils.MIRROR_APP_ID_ORIGIN_WHITELIST_ :
      null;
};


/**
 * @param {string} sourceUrn
 * @return {boolean} True if it is a tab mirror URN.
 */
mr.MediaSourceUtils.isTabMirrorSource = function(sourceUrn) {
  return sourceUrn.startsWith(mr.MediaSourceUtils.TAB_MIRROR_URN_PREFIX) ||
      sourceUrn.indexOf(mr.MediaSourceUtils.CAST_STREAMING_APP_ID) != -1;
};


/**
 * @param {string} sourceUrn
 * @return {boolean} True if it is a desktop mirror URN.
 */
mr.MediaSourceUtils.isDesktopMirrorSource = function(sourceUrn) {
  return sourceUrn == mr.MediaSourceUtils.DESKTOP_MIRROR_URN;
};


/**
 * Get the tab ID from |sourceUrn|. Returns null if the sourceUrn is not a tab
 * mirror URN or if it doesn't contain a valid tab ID.
 * @param {string} sourceUrn
 * @return {?number}
 */
mr.MediaSourceUtils.getMirrorTabId = function(sourceUrn) {
  const pos = sourceUrn.search(mr.MediaSourceUtils.TAB_MIRROR_URN_PREFIX);
  if (pos == -1) return null;
  const tabIdStr =
      sourceUrn.substr(pos + mr.MediaSourceUtils.TAB_MIRROR_URN_PREFIX.length);
  return parseInt(tabIdStr, 10) || null;
};
