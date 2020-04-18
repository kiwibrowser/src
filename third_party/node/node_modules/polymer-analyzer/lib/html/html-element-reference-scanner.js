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
const parse5_1 = require("parse5");
const element_reference_1 = require("../model/element-reference");
const isCustomElement = dom5.predicates.hasMatchingTagName(/(.+-)+.+/);
/**
 * Scans for HTML element references/uses in a given document.
 * All elements will be detected, including anything in <head>.
 * This scanner will not be loaded by default, but the custom
 * element extension of it will be.
 */
class HtmlElementReferenceScanner {
    matches(node) {
        return !!node;
    }
    scan(document, visit) {
        return __awaiter(this, void 0, void 0, function* () {
            const elements = [];
            const visitor = (node) => {
                if (node.tagName && this.matches(node)) {
                    const element = new element_reference_1.ScannedElementReference(node.tagName, document.sourceRangeForNode(node), node);
                    if (node.attrs) {
                        for (const attr of node.attrs) {
                            element.attributes.set(attr.name, {
                                name: attr.name,
                                value: attr.value,
                                sourceRange: document.sourceRangeForAttribute(node, attr.name),
                                nameSourceRange: document.sourceRangeForAttributeName(node, attr.name),
                                valueSourceRange: document.sourceRangeForAttributeValue(node, attr.name)
                            });
                        }
                    }
                    elements.push(element);
                }
                // Descend into templates.
                if (node.tagName === 'template') {
                    const content = parse5_1.treeAdapters.default.getTemplateContent(node);
                    if (content) {
                        dom5.nodeWalk(content, (n) => {
                            visitor(n);
                            return false;
                        });
                    }
                }
            };
            yield visit(visitor);
            return { features: elements };
        });
    }
}
exports.HtmlElementReferenceScanner = HtmlElementReferenceScanner;
/**
 * Scans for custom element references/uses.
 * All custom elements will be detected except <dom-module>.
 * This is a default scanner.
 */
class HtmlCustomElementReferenceScanner extends HtmlElementReferenceScanner {
    matches(node) {
        return isCustomElement(node) && node.nodeName !== 'dom-module';
    }
}
exports.HtmlCustomElementReferenceScanner = HtmlCustomElementReferenceScanner;

//# sourceMappingURL=html-element-reference-scanner.js.map
