/**
 * @license
 * Copyright (c) 2015 The Polymer Project Authors. All rights reserved.
 * This code may only be used under the BSD style license found at http://polymer.github.io/LICENSE.txt
 * The complete set of authors may be found at http://polymer.github.io/AUTHORS.txt
 * The complete set of contributors may be found at http://polymer.github.io/CONTRIBUTORS.txt
 * Code distributed by Google as part of the polymer project is also
 * subject to an additional IP rights grant found at http://polymer.github.io/PATENTS.txt
 */

'use strict';
require("babel-polyfill");
/**
 * Static analysis for Polymer.
 * @namespace hydrolysis
 */
exports.Analyzer = require('./lib/analyzer').Analyzer;
exports.FSResolver = require('./lib/loader/fs-resolver').FSResolver;
exports.Loader = require('./lib/loader/file-loader').FileLoader;
exports.NoopResolver = require('./lib/loader/noop-resolver').NoopResolver;
exports.RedirectResolver =
    require('./lib/loader/redirect-resolver').RedirectResolver;
exports.XHRResolver = require('./lib/loader/xhr-resolver').XHRResolver;
exports.StringResolver = require('./lib/loader/string-resolver').StringResolver;
exports._jsParse = require('./lib/ast-utils/js-parse').jsParse;
exports._importParse = require('./lib/ast-utils/import-parse').importParse;

exports.docs = require('./lib/ast-utils/docs');
exports.jsdoc = require('./lib/ast-utils/jsdoc');
