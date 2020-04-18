// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


/**
 * @fileoverview Utilities for rendering most visited thumbnails and titles.
 */

// Don't remove; see crbug.com/678778.
// <include src="instant_iframe_validation.js">


/**
 * The origin of this request.
 * @const {string}
 */
var DOMAIN_ORIGIN = '{{ORIGIN}}';

/**
 * Parses query parameters from Location.
 * @param {string} location The URL to generate the CSS url for.
 * @return {Object} Dictionary containing name value pairs for URL.
 */
function parseQueryParams(location) {
  var params = Object.create(null);
  var query = location.search.substring(1);
  var vars = query.split('&');
  for (var i = 0; i < vars.length; i++) {
    var pair = vars[i].split('=');
    var k = decodeURIComponent(pair[0]);
    if (k in params) {
      // Duplicate parameters are not allowed to prevent attackers who can
      // append things to |location| from getting their parameter values to
      // override legitimate ones.
      return Object.create(null);
    } else {
      params[k] = decodeURIComponent(pair[1]);
    }
  }
  return params;
}


/**
 * Creates a new most visited link element.
 * @param {Object} params URL parameters containing styles for the link.
 * @param {string} href The destination for the link.
 * @param {string} title The title for the link.
 * @param {string|undefined} text The text for the link or none.
 * @param {string|undefined} direction The text direction.
 * @return {HTMLAnchorElement} A new link element.
 */
function createMostVisitedLink(params, href, title, text, direction) {
  var styles = getMostVisitedStyles(params, !!text);
  var link = document.createElement('a');
  link.style.color = styles.color;
  link.style.fontSize = styles.fontSize + 'px';
  if (styles.fontFamily)
    link.style.fontFamily = styles.fontFamily;
  if (styles.textAlign)
    link.style.textAlign = styles.textAlign;
  if (styles.textFadePos) {
    var dir = /^rtl$/i.test(direction) ? 'to left' : 'to right';
    // The fading length in pixels is passed by the caller.
    var mask = 'linear-gradient(' + dir + ', rgba(0,0,0,1), rgba(0,0,0,1) ' +
        styles.textFadePos + 'px, rgba(0,0,0,0))';
    link.style.textOverflow = 'clip';
    link.style.webkitMask = mask;
  }
  if (styles.numTitleLines && styles.numTitleLines > 1) {
    link.classList.add('multiline');
  }

  link.href = href;
  link.title = title;
  link.target = '_top';
  // Include links in the tab order.  The tabIndex is necessary for
  // accessibility.
  link.tabIndex = '0';
  if (text) {
    // Wrap text with span so ellipsis will appear at the end of multiline.
    var spanWrap = document.createElement('span');
    spanWrap.textContent = text;
    link.appendChild(spanWrap);
  }
  link.addEventListener('focus', function() {
    window.parent.postMessage('linkFocused', DOMAIN_ORIGIN);
  });
  link.addEventListener('blur', function() {
    window.parent.postMessage('linkBlurred', DOMAIN_ORIGIN);
  });

  link.addEventListener('keydown', function(event) {
    if (event.keyCode == 46 /* DELETE */ ||
        event.keyCode == 8 /* BACKSPACE */) {
      event.preventDefault();
      window.parent.postMessage('tileBlacklisted,' + params.pos, DOMAIN_ORIGIN);
    } else if (
        event.keyCode == 13 /* ENTER */ || event.keyCode == 32 /* SPACE */) {
      // Event target is the <a> tag. Send a click event on it, which will
      // trigger the 'click' event registered above.
      event.preventDefault();
      event.target.click();
    }
  });

  return link;
}


/**
 * Returns the color to display string with, depending on whether title is
 * displayed, the current theme, and URL parameters.
 * @param {Object<string>} params URL parameters specifying style.
 * @param {boolean} isTitle if the style is for the Most Visited Title.
 * @return {string} The color to use, in "rgba(#,#,#,#)" format.
 */
function getTextColor(params, isTitle) {
  // 'RRGGBBAA' color format overrides everything.
  if ('c' in params && params.c.match(/^[0-9A-Fa-f]{8}$/)) {
    // Extract the 4 pairs of hex digits, map to number, then form rgba().
    var t = params.c.match(/(..)(..)(..)(..)/).slice(1).map(function(s) {
      return parseInt(s, 16);
    });
    return 'rgba(' + t[0] + ',' + t[1] + ',' + t[2] + ',' + t[3] / 255 + ')';
  }

  // For backward compatibility with server-side NTP, look at themes directly
  // and use param.c for non-title or as fallback.
  var apiHandle = chrome.embeddedSearch.newTabPage;
  var themeInfo = apiHandle.themeBackgroundInfo;
  var c = '#777';
  if (isTitle && themeInfo && !themeInfo.usingDefaultTheme) {
    // Read from theme directly
    c = convertArrayToRGBAColor(themeInfo.textColorRgba) || c;
  } else if ('c' in params) {
    c = convertToHexColor(parseInt(params.c, 16)) || c;
  }
  return c;
}


/**
 * Decodes most visited styles from URL parameters.
 * - c: A hexadecimal number interpreted as a hex color code.
 * - f: font-family.
 * - fs: font-size as a number in pixels.
 * - ta: text-align property, as a string.
 * - tf: text fade starting position, in pixels.
 * - ntl: number of lines in the title.
 * @param {Object<string>} params URL parameters specifying style.
 * @param {boolean} isTitle if the style is for the Most Visited Title.
 * @return {Object} Styles suitable for CSS interpolation.
 */
function getMostVisitedStyles(params, isTitle) {
  var styles = {
    color: getTextColor(params, isTitle),  // Handles 'c' in params.
    fontFamily: '',
    fontSize: 11
  };
  if ('f' in params && /^[-0-9a-zA-Z ,]+$/.test(params.f))
    styles.fontFamily = params.f;
  if ('fs' in params && isFinite(parseInt(params.fs, 10)))
    styles.fontSize = parseInt(params.fs, 10);
  if ('ta' in params && /^[-0-9a-zA-Z ,]+$/.test(params.ta))
    styles.textAlign = params.ta;
  if ('tf' in params) {
    var tf = parseInt(params.tf, 10);
    if (isFinite(tf))
      styles.textFadePos = tf;
  }
  if ('ntl' in params) {
    var ntl = parseInt(params.ntl, 10);
    if (isFinite(ntl))
      styles.numTitleLines = ntl;
  }
  return styles;
}


/**
 * Returns whether the given URL has a known, safe scheme.
 * @param {string} url URL to check.
 */
var isSchemeAllowed = function(url) {
  return url.startsWith('http://') || url.startsWith('https://') ||
      url.startsWith('ftp://') || url.startsWith('chrome-extension://');
};


/**
 * @param {string} location A location containing URL parameters.
 * @param {function(Object, Object)} fill A function called with styles and
 *     data to fill.
 */
function fillMostVisited(location, fill) {
  var params = parseQueryParams(location);
  params.rid = parseInt(params.rid, 10);
  if (!isFinite(params.rid))
    return;
  var data =
      chrome.embeddedSearch.newTabPage.getMostVisitedItemData(params.rid);
  if (!data)
    return;
  if (data.url && !isSchemeAllowed(data.url))
    return;

  if (isFinite(params.dummy) && parseInt(params.dummy, 10))
    data.dummy = true;

  if (data.direction)
    document.body.dir = data.direction;
  fill(params, data);
}
