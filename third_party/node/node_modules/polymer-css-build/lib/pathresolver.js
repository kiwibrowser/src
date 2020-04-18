/**
@license
Copyright (c) 2016 The Polymer Project Authors. All rights reserved.
This code may only be used under the BSD style license found at http://polymer.github.io/LICENSE.txt
The complete set of authors may be found at http://polymer.github.io/AUTHORS.txt
The complete set of contributors may be found at http://polymer.github.io/CONTRIBUTORS.txt
Code distributed by Google as part of the polymer project is also
subject to an additional IP rights grant found at http://polymer.github.io/PATENTS.txt
*/
'use strict';

// Adapted from vulcanize's pathResolver, but only used for css text
const path = require('path').posix;
const url = require('url');

const URL = /url\([^)]*\)/g;
const ABS_URL = /(^\/)|(^#)|(^[\w-\d]*:)/;

module.exports = {
  rewriteRelPath: function rewriteRelPath(importUrl, mainDocUrl, relUrl) {
    if (ABS_URL.test(relUrl)) {
      return relUrl;
    }
    const absUrl = url.resolve(importUrl, relUrl);
    const parsedFrom = url.parse(mainDocUrl);
    const parsedTo = url.parse(absUrl);
    if (parsedFrom.protocol === parsedTo.protocol
      && parsedFrom.host === parsedTo.host) {
      const pathname = path.relative(
        path.dirname(parsedFrom.pathname), parsedTo.pathname);
      return url.format({
        pathname,
        search: parsedTo.search,
        hash: parsedTo.hash
      });
    }
    return absUrl;
  },

  rewriteURL: function rewriteURL(importUrl, mainDocUrl, cssText) {
    return cssText.replace(URL, match => {
      let path = match.replace(/["']/g, "").slice(4, -1);
      path = this.rewriteRelPath(importUrl, mainDocUrl, path);
      return 'url("' + path + '")';
    });
  }
}
