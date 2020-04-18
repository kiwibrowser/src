"use strict";
/**
 * @license
 * Copyright (c) 2016 The Polymer Project Authors. All rights reserved.
 * This code may only be used under the BSD style license found at
 * http://polymer.github.io/LICENSE.txt
 * The complete set of authors may be found at
 * http://polymer.github.io/AUTHORS.txt
 * The complete set of contributors may be found at
 * http://polymer.github.io/CONTRIBUTORS.txt
 * Code distributed by Google as part of the polymer project is also
 * subject to an additional IP rights grant found at
 * http://polymer.github.io/PATENTS.txt
 */
Object.defineProperty(exports, "__esModule", { value: true });
const path_1 = require("path");
const utils_1 = require("../core/utils");
/**
 * Resolves a URL to a canonical URL within a package.
 */
class PackageUrlResolver {
    constructor(options) {
        options = options || {};
        this.componentDir = options.componentDir || 'bower_components/';
        this.hostname = options.hostname || null;
    }
    canResolve(url) {
        const urlObject = utils_1.parseUrl(url);
        let decodedUrl;
        try {
            decodedUrl = decodeURI(urlObject.pathname || '');
        }
        catch (e) {
            return false;
        }
        const pathname = path_1.posix.normalize(decodedUrl);
        return this._isValid(urlObject, pathname);
    }
    _isValid(urlObject, pathname) {
        return (urlObject.hostname === this.hostname || !urlObject.hostname) &&
            !pathname.startsWith('../../');
    }
    resolve(url) {
        const urlObject = utils_1.parseUrl(url);
        let pathname = path_1.posix.normalize(decodeURI(urlObject.pathname || ''));
        if (!this._isValid(urlObject, pathname)) {
            throw new Error(`Invalid URL ${url}`);
        }
        // If the path points to a sibling directory, resolve it to the
        // component directory
        if (pathname.startsWith('../')) {
            pathname = path_1.posix.join(this.componentDir, pathname.substring(3));
        }
        // make all paths relative to the root directory
        if (path_1.posix.isAbsolute(pathname)) {
            pathname = pathname.substring(1);
        }
        // Re-encode URI, since it is expected we are emitting a relative URL.
        return encodeURI(pathname);
    }
}
exports.PackageUrlResolver = PackageUrlResolver;

//# sourceMappingURL=package-url-resolver.js.map
