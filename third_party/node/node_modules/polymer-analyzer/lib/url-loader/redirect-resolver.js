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
/**
 * Resolves a URL having one prefix to another URL with a different prefix.
 */
class RedirectResolver {
    constructor(_redirectFrom, _redirectTo) {
        this._redirectFrom = _redirectFrom;
        this._redirectTo = _redirectTo;
    }
    canResolve(url) {
        return url.startsWith(this._redirectFrom);
    }
    resolve(url) {
        if (!this.canResolve(url)) {
            throw new Error(`RedirectResolver cannot resolve: "${url}" from:` +
                `"${this._redirectFrom}" to: "${this._redirectTo}"`);
        }
        return this._redirectTo + url.slice(this._redirectFrom.length);
    }
}
exports.RedirectResolver = RedirectResolver;

//# sourceMappingURL=redirect-resolver.js.map
