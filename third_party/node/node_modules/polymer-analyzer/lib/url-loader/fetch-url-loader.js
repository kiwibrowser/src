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
const url_1 = require("url");
/**
 * Resolves requests via the the DOM fetch API.
 */
class FetchUrlLoader {
    constructor(baseUrl) {
        this.baseUrl = baseUrl;
    }
    _resolve(url) {
        return this.baseUrl ? url_1.resolve(this.baseUrl, url) : url;
    }
    canLoad(_) {
        return true;
    }
    load(url) {
        return window.fetch(this._resolve(url)).then((response) => {
            if (response.ok) {
                return response.text();
            }
            else {
                return response.text().then((content) => {
                    throw new Error(`Response not ok: ${content}`);
                });
            }
        });
    }
}
exports.FetchUrlLoader = FetchUrlLoader;

//# sourceMappingURL=fetch-url-loader.js.map
