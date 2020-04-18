// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Installs Translate management functions on the __gCrWeb object.
 *
 * TODO(crbug.com/659442): Enable checkTypes, checkVars errors for this file.
 * @suppress {checkTypes, checkVars}
 */

__gCrWeb.translate = {};

// Store translate namespace object in a global __gCrWeb object referenced by a
// string, so it does not get renamed by closure compiler during the
// minification.
__gCrWeb['translate'] = __gCrWeb.translate;

(function() {
/**
 * The delay a wait performed (in milliseconds) before checking whether the
 * translation has finished.
 * @type {number}
 */
__gCrWeb.translate.TRANSLATE_STATUS_CHECK_DELAY = 400;

/**
 * The delay in milliseconds that we'll wait to check if a page has finished
 * loading before attempting a translation.
 * @type {number}
 */
__gCrWeb.translate.TRANSLATE_LOAD_CHECK_DELAY = 150;

/**
 * The maximum number of times a check is performed to see if the translate
 * library injected in the page is ready.
 * @type {number}
 */
__gCrWeb.translate.MAX_TRANSLATE_INIT_CHECK_ATTEMPTS = 5;

// The number of times polling for the ready status of the translate script has
//  been performed.
var translationAttemptCount = 0;

/**
 * Polls every TRANSLATE_LOAD_CHECK_DELAY milliseconds to check if the translate
 * script is ready and informs the host when it is.
 */
__gCrWeb.translate['checkTranslateReady'] = function() {
  translationAttemptCount += 1;
  if (cr.googleTranslate.libReady) {
    translationAttemptCount = 0;
    __gCrWeb.message.invokeOnHost({
        'command': 'translate.ready',
        'timeout': false,
        'loadTime': cr.googleTranslate.loadTime,
        'readyTime': cr.googleTranslate.readyTime});
  } else if (translationAttemptCount >=
             __gCrWeb.translate.MAX_TRANSLATE_INIT_CHECK_ATTEMPTS) {
    __gCrWeb.message.invokeOnHost({
        'command': 'translate.ready',
        'timeout': true});
  } else {
    // The translation is still pending, check again later.
    window.setTimeout(__gCrWeb.translate.checkTranslateReady,
                      __gCrWeb.translate.TRANSLATE_LOAD_CHECK_DELAY);
  }
}

/**
 * Polls every TRANSLATE_STATUS_CHECK_DELAY milliseconds to check if translate
 * is ready and informs the host when it is.
 */
__gCrWeb.translate['checkTranslateStatus'] = function() {
  if (cr.googleTranslate.error) {
    __gCrWeb.message.invokeOnHost({'command': 'translate.status',
                                   'success': false});
  } else if (cr.googleTranslate.finished) {
    __gCrWeb.message.invokeOnHost({
        'command': 'translate.status',
        'success': true,
        'originalPageLanguage': cr.googleTranslate.sourceLang,
        'translationTime': cr.googleTranslate.translationTime});
  } else {
    // The translation is still pending, check again later.
    window.setTimeout(__gCrWeb.translate.checkTranslateStatus,
                      __gCrWeb.translate.TRANSLATE_STATUS_CHECK_DELAY);
  }
}

}());  // anonymous function
