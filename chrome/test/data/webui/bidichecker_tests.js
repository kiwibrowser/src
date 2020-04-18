// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// The following test runs the BiDi Checker on the current page with LTR
// base direction. Any bad mixture or RTL and LTR will be reported back
// by this test.

function filtersForPage(pageName, isRTL) {
  // Suppression filters should be added to the object below. You may omit
  // either RTL or LTR filters, or may not include filters for your page at all.
  // For additional info about BidiChecker go to
  // http://code.google.com/p/bidichecker
  // For specific info about filters, check out the following links:
  // http://bidichecker.googlecode.com/svn/trunk/docs/users_guide.html
  // #TOC-Error-descriptions
  // http://bidichecker.googlecode.com/svn/trunk/docs/jsdoc/index.html
  // TODO(ofri): Link to more comprehensive documentation when available.
  var filters = {
    // Page filters
    'chrome://history': {
      // Filters for LTR UI
      'LTR': [
        // BUG: http://crbug.com/80791
        bidichecker.FilterFactory.atText('בדיקה')
      ],
      // Filters for RTL UI
      'RTL': [
        // BUG: http://crbug.com/80791
        bidichecker.FilterFactory.atText('Google'),
        bidichecker.FilterFactory.atText('www.google.com'),
        // The following two are probably false positives since we can't
        // completely change the environment to RTL on Linux.
        // TODO(ofri): Verify that it's indeed a false positive.
        bidichecker.FilterFactory.locationClass('day'),
        bidichecker.FilterFactory.locationClass('time')
      ]
    },
    'chrome://settings/autofill': {
      'LTR': [
        // BUG: http://crbug.com/82267
        bidichecker.FilterFactory.atText('משה ב כהן, דרך מנחם בגין')
      ],
      'RTL': [
        // BUG: http://crbug.com/90322
        bidichecker.FilterFactory.atText(
            'Milton C. Waddams, 4120 Freidrich Lane')
      ]
    },
    'chrome://plugins': {
      'RTL': [
        // False positive
        bidichecker.FilterFactory.atText('x'),
        // Apparently also a false positive
        bidichecker.FilterFactory.atText('undefined\n      undefined'),
        bidichecker.FilterFactory.locationClass('plugin-text')
      ]
    },
    'chrome://newtab': {
      'RTL': [
        // BUG: http://crbug.com/93339
        bidichecker.FilterFactory.atText('Chrome Web Store'),
        bidichecker.FilterFactory.atText('File Manager'),
        bidichecker.FilterFactory.atText('Chrome Apps Debugger')
      ]
    },
    'chrome://feedback#0?description=%D7%91%D7%93%D7%99%D7%A7%D7%94': {
      'LTR': [
        // BUG: http://crbug.com/90835
        bidichecker.FilterFactory.atText('בדיקה')
      ]
    },
    'chrome://feedback#0?description=test': {
      'RTL': [
        // BUG: http://crbug.com/90835
        bidichecker.FilterFactory.atText('test'),
        bidichecker.FilterFactory.atText('stub-user@example.com')
      ]
    },
    'chrome://settings/browser': {
      'LTR': [
        // BUG: http://crbug.com/93702
        bidichecker.FilterFactory.atText('חדשות תוכן ועדכונים - ידיעות אחרונות')
      ]
    },
    'chrome://history-frame': {
      'LTR': [
        // BUG: http://crbug.com/119595
        bidichecker.FilterFactory.atText('בדיקה')
      ],
      'RTL': [
        // BUG: http://crbug.com/119595
        bidichecker.FilterFactory.atText('Google'),
        bidichecker.FilterFactory.atText('www.google.com')
      ],
    },
  };
  var globalFilters = {
    'RTL': [
      // BUG: http://crbug/158750
      bidichecker.FilterFactory.locationId('timezone-select')
    ]
  };

  var dir = isRTL ? 'RTL' : 'LTR';
  if (!filters.hasOwnProperty(pageName))
    pageName += '/';
  if (!filters.hasOwnProperty(pageName)) {
    if (pageName.charAt(pageName.length - 2) == '/')
      pageName = pageName.substr(0, pageName.length - 2);
    else
      return globalFilters[dir];
  }
  if (filters.hasOwnProperty(pageName) &&
      filters[pageName].hasOwnProperty(dir)) {
    return filters[pageName][dir].concat(globalFilters[dir]);
  } else {
    return globalFilters[dir];
  }
}

function buildPrettyErrors(bidiErrors) {
  var idx;
  var prettyErrors;
  for (idx = 0; idx < bidiErrors.length; ++idx) {
    prettyErrors += '\n\n';
    prettyErrors += bidiErrors[idx].toString();
  }
  prettyErrors += '\n\n';
  return prettyErrors;
}

function runBidiChecker(pageName, isRTL) {
  var filters = filtersForPage(pageName, isRTL);
  var bidiErrors = bidichecker.checkPage(isRTL, top.document.body, filters);
  assertTrue(bidiErrors.length == 0, buildPrettyErrors(bidiErrors));
}
