"use strict";
/**
 * @license
 * Copyright (c) 2015 The Polymer Project Authors. All rights reserved.
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
 * Resolves requests via one of a sequence of loaders.
 */
class MultiUrlLoader {
    constructor(_loaders) {
        this._loaders = _loaders;
    }
    canLoad(url) {
        return this._loaders.some((loader) => loader.canLoad(url));
    }
    load(url) {
        return __awaiter(this, void 0, void 0, function* () {
            for (const loader of this._loaders) {
                if (loader.canLoad(url)) {
                    return loader.load(url);
                }
            }
            return Promise.reject(new Error(`Unable to load ${url}`));
        });
    }
}
exports.MultiUrlLoader = MultiUrlLoader;

//# sourceMappingURL=multi-url-loader.js.map
