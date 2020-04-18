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
const dom5 = require("dom5");
const esutil_1 = require("../javascript/esutil");
const jsdoc = require("../javascript/jsdoc");
const docs_1 = require("./docs");
const polymer_element_1 = require("./polymer-element");
/**
 * A Polymer pseudo-element is an element that is declared in an unusual way,
 * such
 * that the analyzer couldn't normally analyze it, so instead it is declared in
 * comments.
 */
class PseudoElementScanner {
    scan(document, visit) {
        return __awaiter(this, void 0, void 0, function* () {
            const elements = [];
            yield visit((node) => {
                if (dom5.isCommentNode(node) && node.data &&
                    node.data.includes('@pseudoElement')) {
                    const parsedJsdoc = jsdoc.parseJsdoc(node.data);
                    const pseudoTag = jsdoc.getTag(parsedJsdoc, 'pseudoElement');
                    const tagName = pseudoTag && pseudoTag.name;
                    if (tagName) {
                        const element = new polymer_element_1.ScannedPolymerElement({
                            astNode: node,
                            tagName: tagName,
                            jsdoc: parsedJsdoc,
                            description: parsedJsdoc.description,
                            sourceRange: document.sourceRangeForNode(node),
                            privacy: esutil_1.getOrInferPrivacy(tagName, parsedJsdoc),
                            abstract: jsdoc.hasTag(parsedJsdoc, 'abstract'),
                            properties: [],
                            attributes: new Map(),
                            events: new Map(),
                            listeners: [],
                            behaviors: [],
                            className: undefined,
                            extends: undefined,
                            staticMethods: new Map(),
                            methods: new Map(),
                            mixins: [],
                            observers: [],
                            superClass: undefined
                        });
                        element.pseudo = true;
                        docs_1.annotateElementHeader(element);
                        elements.push(element);
                    }
                }
            });
            return { features: elements };
        });
    }
}
exports.PseudoElementScanner = PseudoElementScanner;

//# sourceMappingURL=pseudo-element-scanner.js.map
