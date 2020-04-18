/**
 * @license
 * Copyright (c) 2014 The Polymer Project Authors. All rights reserved.
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
// jshint node: true
'use strict';
Object.defineProperty(exports, "__esModule", { value: true });
const constants_1 = require("./constants");
const dom5_1 = require("dom5");
exports.jsMatcher = dom5_1.predicates.AND(dom5_1.predicates.hasTagName('script'), dom5_1.predicates.OR(dom5_1.predicates.NOT(dom5_1.predicates.hasAttr('type')), dom5_1.predicates.hasAttrValue('type', 'text/javascript'), dom5_1.predicates.hasAttrValue('type', 'application/javascript')));
exports.externalStyle = dom5_1.predicates.AND(dom5_1.predicates.hasTagName('link'), dom5_1.predicates.hasAttrValue('rel', 'stylesheet'));
// polymer specific external stylesheet
exports.polymerExternalStyle = dom5_1.predicates.AND(dom5_1.predicates.hasTagName('link'), dom5_1.predicates.hasAttrValue('rel', 'import'), dom5_1.predicates.hasAttrValue('type', 'css'));
exports.styleMatcher = dom5_1.predicates.AND(dom5_1.predicates.hasTagName('style'), dom5_1.predicates.OR(dom5_1.predicates.NOT(dom5_1.predicates.hasAttr('type')), dom5_1.predicates.hasAttrValue('type', 'text/css')));
exports.targetMatcher = dom5_1.predicates.AND(dom5_1.predicates.OR(dom5_1.predicates.hasTagName('a'), dom5_1.predicates.hasTagName('form')), dom5_1.predicates.NOT(dom5_1.predicates.hasAttr('target')));
exports.head = dom5_1.predicates.hasTagName('head');
exports.body = dom5_1.predicates.hasTagName('body');
exports.base = dom5_1.predicates.hasTagName('base');
exports.template = dom5_1.predicates.hasTagName('template');
exports.domModuleWithoutAssetpath = dom5_1.predicates.AND(dom5_1.predicates.hasTagName('dom-module'), dom5_1.predicates.hasAttr('id'), dom5_1.predicates.NOT(dom5_1.predicates.hasAttr('assetpath')));
exports.polymerElement = dom5_1.predicates.hasTagName('polymer-element');
exports.externalJavascript = dom5_1.predicates.AND(dom5_1.predicates.hasAttr('src'), exports.jsMatcher);
exports.inlineJavascript = dom5_1.predicates.AND(dom5_1.predicates.NOT(dom5_1.predicates.hasAttr('src')), exports.jsMatcher);
exports.eagerHtmlImport = dom5_1.predicates.AND(dom5_1.predicates.hasTagName('link'), dom5_1.predicates.hasAttrValue('rel', 'import'), dom5_1.predicates.hasAttr('href'), dom5_1.predicates.OR(dom5_1.predicates.hasAttrValue('type', 'text/html'), dom5_1.predicates.hasAttrValue('type', 'html'), dom5_1.predicates.NOT(dom5_1.predicates.hasAttr('type'))));
exports.lazyHtmlImport = dom5_1.predicates.AND(dom5_1.predicates.hasTagName('link'), dom5_1.predicates.hasAttrValue('rel', 'lazy-import'), dom5_1.predicates.hasAttr('href'), dom5_1.predicates.OR(dom5_1.predicates.hasAttrValue('type', 'text/html'), dom5_1.predicates.hasAttrValue('type', 'html'), dom5_1.predicates.NOT(dom5_1.predicates.hasAttr('type'))));
exports.htmlImport = dom5_1.predicates.OR(exports.eagerHtmlImport, exports.lazyHtmlImport);
exports.stylesheetImport = dom5_1.predicates.AND(dom5_1.predicates.hasTagName('link'), dom5_1.predicates.hasAttrValue('rel', 'import'), dom5_1.predicates.hasAttr('href'), dom5_1.predicates.hasAttrValue('type', 'css'));
exports.hiddenDiv = dom5_1.predicates.AND(dom5_1.predicates.hasTagName('div'), dom5_1.predicates.hasAttr('hidden'), dom5_1.predicates.hasAttr('by-polymer-bundler'));
exports.inHiddenDiv = dom5_1.predicates.parentMatches(exports.hiddenDiv);
exports.elementsWithUrlAttrsToRewrite = dom5_1.predicates.AND(dom5_1.predicates.OR(...constants_1.default.URL_ATTR.map((attr) => dom5_1.predicates.hasAttr(attr))), dom5_1.predicates.NOT(dom5_1.predicates.AND(dom5_1.predicates.parentMatches(dom5_1.predicates.hasTagName('dom-module')), exports.lazyHtmlImport)));
/**
 * TODO(usergenic): From garlicnation's PR comment - "This matcher needs to deal
 * with a number of edge cases. Whitespace-only text nodes should be ignored,
 * text nodes with meaningful space should be preserved. Comments should be
 * ignored if --strip-comments is set. License comments should be deduplicated
 * and moved to the start of the document."
 */
const nextToHiddenDiv = (offset) => {
    return (node) => {
        const siblings = node.parentNode.childNodes;
        const hiddenDivIndex = siblings.indexOf(node) + offset;
        if (hiddenDivIndex < 0 || hiddenDivIndex >= siblings.length) {
            return false;
        }
        return exports.hiddenDiv(siblings[hiddenDivIndex]);
    };
};
exports.beforeHiddenDiv = nextToHiddenDiv(1);
exports.afterHiddenDiv = nextToHiddenDiv(-1);
exports.orderedImperative = dom5_1.predicates.OR(exports.eagerHtmlImport, exports.jsMatcher, exports.styleMatcher, exports.externalStyle, exports.polymerExternalStyle);
//# sourceMappingURL=matchers.js.map