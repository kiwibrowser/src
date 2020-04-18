"use strict";
/**
 * @license
 * Copyright (c) 2017 The Polymer Project Authors. All rights reserved.
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
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : new P(function (resolve) { resolve(result.value); }).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
Object.defineProperty(exports, "__esModule", { value: true });
/**
 * Resolves requests via a given delegate loader for URLs matching a given
 * prefix. URLs are provided to their delegate without the prefix.
 */
class PrefixedUrlLoader {
    constructor(prefix, delegate) {
        this.prefix = prefix;
        this.delegate = delegate;
    }
    canLoad(url) {
        return url.startsWith(this.prefix) &&
            this.delegate.canLoad(this._unprefix(url));
    }
    load(url) {
        return __awaiter(this, void 0, void 0, function* () {
            if (!url.startsWith(this.prefix)) {
                throw new Error(`Can not load "${url}", does not match prefix "${this.prefix}".`);
            }
            return this.delegate.load(this._unprefix(url));
        });
    }
    _unprefix(url) {
        return url.slice(this.prefix.length);
    }
}
exports.PrefixedUrlLoader = PrefixedUrlLoader;

//# sourceMappingURL=prefixed-url-loader.js.map
