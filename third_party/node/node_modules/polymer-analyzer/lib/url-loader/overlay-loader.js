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
class FailUrlLoader {
    canLoad(_url) {
        return true;
    }
    load(url) {
        throw new Error(`${url} not known in InMemoryOverlayLoader`);
    }
}
/**
 * Resolves requests first from an in-memory map of file contents, and if a
 * file isn't found there, defers to another url loader.
 *
 * Useful for the editor use case. An editor will have a number of files in open
 * buffers at any time. For these files, the editor's in-memory buffer is
 * canonical, so that their contents are read even when they have unsaved
 * changes. For all other files, we can load the files using another loader,
 * e.g. from disk.
 *
 * TODO(rictic): make this a mixin that mixes another loader.
 */
class InMemoryOverlayUrlLoader {
    constructor(fallbackLoader) {
        this.urlContentsMap = new Map();
        this._fallbackLoader = fallbackLoader || new FailUrlLoader();
        if (this._fallbackLoader.readDirectory) {
            this.readDirectory =
                this._fallbackLoader.readDirectory.bind(this._fallbackLoader);
        }
    }
    canLoad(url) {
        return this.urlContentsMap.has(url) || this._fallbackLoader.canLoad(url);
    }
    load(url) {
        return __awaiter(this, void 0, void 0, function* () {
            const contents = this.urlContentsMap.get(url);
            if (typeof contents === 'string') {
                return contents;
            }
            return this._fallbackLoader.load(url);
        });
    }
}
exports.InMemoryOverlayUrlLoader = InMemoryOverlayUrlLoader;

//# sourceMappingURL=overlay-loader.js.map
