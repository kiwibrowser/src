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
const document_1 = require("./document");
const warning_1 = require("./warning");
/**
 * Represents an import, such as an HTML import, an external script or style
 * tag, or an JavaScript import.
 *
 * @template N The AST node type
 */
class ScannedImport {
    constructor(type, url, sourceRange, urlSourceRange, ast, lazy) {
        this.warnings = [];
        this.type = type;
        this.url = url;
        this.sourceRange = sourceRange;
        this.urlSourceRange = urlSourceRange;
        this.astNode = ast;
        this.lazy = lazy;
    }
    resolve(document) {
        if (!document._analysisContext.canResolveUrl(this.url)) {
            return;
        }
        const importedDocumentOrWarning = document._analysisContext.getDocument(this.url);
        if (!(importedDocumentOrWarning instanceof document_1.Document)) {
            const error = this.error ? (this.error.message || this.error) : '';
            document.warnings.push(new warning_1.Warning({
                code: 'could-not-load',
                message: `Unable to load import: ${error}`,
                sourceRange: (this.urlSourceRange || this.sourceRange),
                severity: warning_1.Severity.ERROR,
                parsedDocument: document.parsedDocument,
            }));
            return undefined;
        }
        return new Import(this.url, this.type, importedDocumentOrWarning, this.sourceRange, this.urlSourceRange, this.astNode, this.warnings, this.lazy);
    }
}
exports.ScannedImport = ScannedImport;
class Import {
    constructor(url, type, document, sourceRange, urlSourceRange, ast, warnings, lazy) {
        this.identifiers = new Set();
        this.kinds = new Set(['import']);
        this.url = url;
        this.type = type;
        this.document = document;
        this.kinds.add(this.type);
        this.sourceRange = sourceRange;
        this.urlSourceRange = urlSourceRange;
        this.astNode = ast;
        this.warnings = warnings;
        this.lazy = lazy;
        if (lazy) {
            this.kinds.add('lazy-import');
        }
    }
    toString() {
        return `<Import type="${this.type}" url="${this.url}">`;
    }
}
exports.Import = Import;

//# sourceMappingURL=import.js.map
