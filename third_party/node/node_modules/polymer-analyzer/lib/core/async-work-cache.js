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
 * A map from keys to promises of values. Used for caching asynchronous work.
 */
class AsyncWorkCache {
    constructor(from) {
        if (from) {
            this._keyToResultMap = new Map(from._keyToResultMap);
        }
        else {
            this._keyToResultMap = new Map();
        }
    }
    /**
     * If work has already begun to compute the given key, return a promise for
     * the result of that work.
     *
     * If not, compute it with the given function.
     *
     * This method ensures that we will only try to compute the value for `key`
     * once, no matter how often or with what timing getOrCompute is called, even
     * recursively.
     */
    getOrCompute(key, compute) {
        return __awaiter(this, void 0, void 0, function* () {
            const cachedResult = this._keyToResultMap.get(key);
            if (cachedResult) {
                return cachedResult;
            }
            const promise = (() => __awaiter(this, void 0, void 0, function* () {
                // Make sure we wait and return a Promise before doing any work, so that
                // the Promise is cached before control flow enters compute().
                yield Promise.resolve();
                return compute();
            }))();
            this._keyToResultMap.set(key, promise);
            return promise;
        });
    }
    delete(key) {
        this._keyToResultMap.delete(key);
    }
    clear() {
        this._keyToResultMap.clear();
    }
    set(key, value) {
        this._keyToResultMap.set(key, Promise.resolve(value));
    }
    has(key) {
        return this._keyToResultMap.has(key);
    }
}
exports.AsyncWorkCache = AsyncWorkCache;

//# sourceMappingURL=async-work-cache.js.map
