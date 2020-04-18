"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
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
const url_1 = require("url");
const unspecifiedProtocol = '-:';
function parseUrl(url) {
    if (!url.startsWith('//')) {
        return url_1.parse(url);
    }
    const urlObject = url_1.parse(`${unspecifiedProtocol}${url}`);
    urlObject.protocol = undefined;
    urlObject.href = urlObject.href.replace(/^-:/, '');
    return urlObject;
}
exports.parseUrl = parseUrl;
function trimLeft(str, char) {
    let leftEdge = 0;
    while (str[leftEdge] === char) {
        leftEdge++;
    }
    return str.substring(leftEdge);
}
exports.trimLeft = trimLeft;
class Deferred {
    constructor() {
        this.resolved = false;
        this.rejected = false;
        this.promise = new Promise((resolve, reject) => {
            this.resolve = (result) => {
                if (this.resolved) {
                    throw new Error('Already resolved');
                }
                if (this.rejected) {
                    throw new Error('Already rejected');
                }
                this.resolved = true;
                resolve(result);
            };
            this.reject = (error) => {
                if (this.resolved) {
                    throw new Error('Already resolved');
                }
                if (this.rejected) {
                    throw new Error('Already rejected');
                }
                this.rejected = true;
                this.error = error;
                reject(error);
            };
        });
    }
    toNodeCallback() {
        return (error, value) => {
            if (error) {
                this.reject(error);
            }
            else {
                this.resolve(value);
            }
        };
    }
}
exports.Deferred = Deferred;

//# sourceMappingURL=utils.js.map
