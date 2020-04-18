"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
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
const polymer_analyzer_1 = require("polymer-analyzer");
function getAnalysisDocument(analysis, url) {
    const document = analysis.getDocument(url);
    if (document instanceof polymer_analyzer_1.Document) {
        return document;
    }
    if (document instanceof polymer_analyzer_1.Warning || !document ||
        (typeof document === 'object' && document['code'] &&
            document['message'])) {
        const reason = document && document.message || 'unknown';
        const message = `Unable to get document ${url}: ${reason}`;
        throw new Error(message);
    }
    throw new Error(`Bundler was given a different version of polymer-analyzer than ` +
        `expected.  Please ensure only one version of polymer-analyzer ` +
        `present in node_modules folder:\n\n` +
        `$ npm ls polymer- analyzer`);
}
exports.getAnalysisDocument = getAnalysisDocument;
//# sourceMappingURL=analyzer-utils.js.map