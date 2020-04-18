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
const url_1 = require("url");
const model_1 = require("../model/model");
const typescript_document_1 = require("./typescript-document");
class TypeScriptImportScanner {
    scan(document, visit) {
        return __awaiter(this, void 0, void 0, function* () {
            const imports = [];
            class ImportVisitor extends typescript_document_1.Visitor {
                visitImportDeclaration(node) {
                    // If getText() throws it's because it requires parent references
                    const moduleSpecifier = node.moduleSpecifier.getText();
                    // The specifier includes the quote characters, remove them
                    const specifierUrl = moduleSpecifier.substring(1, moduleSpecifier.length - 1);
                    const importUrl = url_1.resolve(document.url, specifierUrl);
                    imports.push(new model_1.ScannedImport('js-import', importUrl, 
                    // TODO(justinfagnani): make SourceRanges work
                    null, null, node, false));
                }
            }
            const visitor = new ImportVisitor();
            yield visit(visitor);
            return { features: imports };
        });
    }
}
exports.TypeScriptImportScanner = TypeScriptImportScanner;

//# sourceMappingURL=typescript-import-scanner.js.map
