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
 * Resolves a URL using multiple resolvers.
 */
class MultiUrlResolver {
    constructor(_resolvers) {
        this._resolvers = _resolvers;
        if (!this._resolvers) {
            this._resolvers = [];
        }
    }
    canResolve(url) {
        return this._resolvers.some((resolver) => {
            return resolver.canResolve(url);
        });
    }
    resolve(url) {
        for (let i = 0; i < this._resolvers.length; i++) {
            const resolver = this._resolvers[i];
            if (resolver.canResolve(url)) {
                return resolver.resolve(url);
            }
        }
        throw new Error('No resolver can resolve: ' + url);
    }
}
exports.MultiUrlResolver = MultiUrlResolver;

//# sourceMappingURL=multi-url-resolver.js.map
