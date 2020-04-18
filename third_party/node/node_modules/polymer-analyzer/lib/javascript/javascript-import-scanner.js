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
const url_1 = require("url");
const model_1 = require("../model/model");
class JavaScriptImportScanner {
    scan(document, visit) {
        return __awaiter(this, void 0, void 0, function* () {
            const imports = [];
            yield visit({
                enterImportDeclaration(node, _) {
                    const source = node.source.value;
                    if (!isPathImport(source)) {
                        // TODO(justinfagnani): push a warning
                        return;
                    }
                    const importUrl = url_1.resolve(document.url, source);
                    imports.push(new model_1.ScannedImport('js-import', importUrl, document.sourceRangeForNode(node), document.sourceRangeForNode(node.source), node, false));
                }
            });
            return { features: imports, warnings: [] };
        });
    }
}
exports.JavaScriptImportScanner = JavaScriptImportScanner;
function isPathImport(source) {
    return /^(\/|\.\/|\.\.\/)/.test(source);
}

//# sourceMappingURL=javascript-import-scanner.js.map
