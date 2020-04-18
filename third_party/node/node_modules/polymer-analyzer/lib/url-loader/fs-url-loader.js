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
const fs = require("fs");
const pathlib = require("path");
const utils_1 = require("../core/utils");
/**
 * Resolves requests via the file system.
 */
class FSUrlLoader {
    constructor(root) {
        this.root = root || '';
    }
    canLoad(url) {
        const urlObject = utils_1.parseUrl(url);
        const pathname = pathlib.normalize(decodeURIComponent(urlObject.pathname || ''));
        return this._isValid(urlObject, pathname);
    }
    _isValid(urlObject, pathname) {
        return (urlObject.protocol === 'file' || !urlObject.hostname) &&
            !pathname.startsWith('../');
    }
    load(url) {
        return new Promise((resolve, reject) => {
            const filepath = this.getFilePath(url);
            fs.readFile(filepath, 'utf8', (error, contents) => {
                if (error) {
                    reject(error);
                }
                else {
                    resolve(contents);
                }
            });
        });
    }
    getFilePath(url) {
        const urlObject = utils_1.parseUrl(url);
        const pathname = pathlib.normalize(decodeURIComponent(urlObject.pathname || ''));
        if (!this._isValid(urlObject, pathname)) {
            throw new Error(`Invalid URL ${url}`);
        }
        return this.root ? pathlib.join(this.root, pathname) : pathname;
    }
    readDirectory(pathFromRoot, deep) {
        return __awaiter(this, void 0, void 0, function* () {
            const files = yield new Promise((resolve, reject) => {
                fs.readdir(pathlib.join(this.root, pathFromRoot), (err, files) => err ? reject(err) : resolve(files));
            });
            const results = [];
            const subDirResultPromises = [];
            for (const basename of files) {
                const file = pathlib.join(pathFromRoot, basename);
                const stat = yield new Promise((resolve, reject) => fs.stat(pathlib.join(this.root, file), (err, stat) => err ? reject(err) : resolve(stat)));
                if (stat.isDirectory()) {
                    if (deep) {
                        subDirResultPromises.push(this.readDirectory(file, deep));
                    }
                }
                else {
                    results.push(file);
                }
            }
            const arraysOfFiles = yield Promise.all(subDirResultPromises);
            for (const dirResults of arraysOfFiles) {
                for (const file of dirResults) {
                    results.push(file);
                }
            }
            return results;
        });
    }
}
exports.FSUrlLoader = FSUrlLoader;

//# sourceMappingURL=fs-url-loader.js.map
