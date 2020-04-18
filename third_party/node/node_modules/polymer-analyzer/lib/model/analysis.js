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
// A regexp that matches paths to external code.
// TODO(rictic): Make this extensible (polymer.json?).
// Note that we match directories named exactly `build`, but will match any
// directory name prefixed by `bower_components` or `node_modules`, in order to
// ignore `polymer install`'s variants, which look like bower_components-foo
const MATCHES_EXTERNAL = /(^|\/)(bower_components|node_modules|build($|\/))/;
/**
 * Represents a queryable interface over all documents in a package/project.
 *
 * Results of queries will include results from all documents in the package, as
 * well as from external dependencies that are transitively imported by
 * documents in the package.
 */
class Analysis {
    static isExternal(path) {
        return MATCHES_EXTERNAL.test(path);
    }
    constructor(results) {
        workAroundDuplicateJsScriptsBecauseOfHtmlScriptTags(results);
        this._results = results;
        const documents = Array.from(results.values())
            .filter((r) => r instanceof document_1.Document);
        const potentialRoots = new Set(documents);
        // We trim down the set of documents as a performance optimization. We only
        // need a set of documents such that all other documents we're interested in
        // can be reached from them. That way we'll do less duplicate work when we
        // query over all documents.
        for (const doc of potentialRoots) {
            for (const imprt of doc.getFeatures({ kind: 'import', imported: true })) {
                // When there's cycles we can keep any element of the cycle, so why not
                // this one.
                if (imprt.document !== doc) {
                    potentialRoots.delete(imprt.document);
                }
            }
        }
        this._searchRoots = potentialRoots;
    }
    getDocument(url) {
        const result = this._results.get(url);
        if (result != null) {
            return result;
        }
        const documents = Array
            .from(this.getFeatures({ kind: 'document', id: url, externalPackages: true }))
            .filter((d) => !d.isInline);
        if (documents.length !== 1) {
            return undefined;
        }
        return documents[0];
    }
    getFeatures(query = {}) {
        const result = new Set();
        const docQuery = this._getDocumentQuery(query);
        for (const doc of this._searchRoots) {
            addAll(result, doc.getFeatures(docQuery));
        }
        return result;
    }
    /**
     * Get all warnings in the project.
     */
    getWarnings(options) {
        const warnings = Array.from(this._results.values())
            .filter((r) => !(r instanceof document_1.Document));
        const result = new Set(warnings);
        const docQuery = this._getDocumentQuery(options);
        for (const doc of this._searchRoots) {
            addAll(result, new Set(doc.getWarnings(docQuery)));
        }
        return Array.from(result);
    }
    _getDocumentQuery(query = {}) {
        return {
            kind: query.kind,
            id: query.id,
            externalPackages: query.externalPackages,
            imported: true,
            noLazyImports: query.noLazyImports
        };
    }
}
exports.Analysis = Analysis;
// TODO(justinfagnani): move to utils
function addAll(set1, set2) {
    for (const val of set2) {
        set1.add(val);
    }
    return set1;
}
/**
 * So, we have this really terrible hack, whereby we generate a new Document for
 * a js file when it is referenced in an external script tag in an HTML
 * document. We do this so that we can inject an artificial import of the HTML
 * document into the js document, so that the HTML document's dependencies are
 * also dependencies of the js document.
 *
 * This works, but we want to eliminate these duplicate JS Documents from the
 * Analysis before the user sees them.
 *
 * https://github.com/Polymer/polymer-analyzer/issues/615 tracks a better
 * solution for this issue
 */
function workAroundDuplicateJsScriptsBecauseOfHtmlScriptTags(results) {
    const documents = Array.from(results.values())
        .filter((r) => r instanceof document_1.Document);
    // TODO(rictic): handle JS imported via script src from HTML better than
    //     this.
    const potentialDuplicates = new Set(documents.filter((r) => r.kinds.has('js-document')));
    const canonicalOverrides = new Set();
    for (const doc of documents) {
        if (potentialDuplicates.has(doc)) {
            continue;
        }
        for (const potentialDupe of potentialDuplicates) {
            const potentialCanonicalDocs = doc.getFeatures({ kind: 'js-document', id: potentialDupe.url, imported: true });
            for (const potentialCanonicalDoc of potentialCanonicalDocs) {
                if (!potentialCanonicalDoc.isInline) {
                    canonicalOverrides.add(potentialCanonicalDoc);
                }
            }
        }
    }
    for (const canonicalDoc of canonicalOverrides) {
        results.set(canonicalDoc.url, canonicalDoc);
    }
}

//# sourceMappingURL=analysis.js.map
