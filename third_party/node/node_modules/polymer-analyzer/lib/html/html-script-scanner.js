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
const url_1 = require("url");
const model_1 = require("../model/model");
const html_script_tag_1 = require("./html-script-tag");
const p = dom5.predicates;
const isJsScriptNode = p.AND(p.hasTagName('script'), p.OR(p.NOT(p.hasAttr('type')), p.hasAttrValue('type', 'text/javascript'), p.hasAttrValue('type', 'application/javascript'), p.hasAttrValue('type', 'module')));
class HtmlScriptScanner {
    scan(document, visit) {
        return __awaiter(this, void 0, void 0, function* () {
            const features = [];
            const myVisitor = (node) => {
                if (isJsScriptNode(node)) {
                    const src = dom5.getAttribute(node, 'src');
                    if (src) {
                        const importUrl = url_1.resolve(document.baseUrl, src);
                        features.push(new html_script_tag_1.ScannedScriptTagImport('html-script', importUrl, document.sourceRangeForNode(node), document.sourceRangeForAttributeValue(node, 'src'), node, false));
                    }
                    else {
                        const locationOffset = model_1.getLocationOffsetOfStartOfTextContent(node);
                        const attachedCommentText = model_1.getAttachedCommentText(node) || '';
                        const contents = dom5.getTextContent(node);
                        features.push(new model_1.ScannedInlineDocument('js', contents, locationOffset, attachedCommentText, document.sourceRangeForNode(node), node));
                    }
                }
            };
            yield visit(myVisitor);
            return { features };
        });
    }
}
exports.HtmlScriptScanner = HtmlScriptScanner;

//# sourceMappingURL=html-script-scanner.js.map
