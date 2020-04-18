/**
 * @license
 * Copyright (c) 2015 The Polymer Project Authors. All rights reserved.
 * This code may only be used under the BSD style license found at http://polymer.github.io/LICENSE.txt
 * The complete set of authors may be found at http://polymer.github.io/AUTHORS.txt
 * The complete set of contributors may be found at http://polymer.github.io/CONTRIBUTORS.txt
 * Code distributed by Google as part of the polymer project is also
 * subject to an additional IP rights grant found at http://polymer.github.io/PATENTS.txt
 */
'use strict';

var dom5 = require('dom5');
var p = dom5.predicates;
var isHtmlImportNode = p.AND(p.hasTagName('link'), p.hasAttrValue('rel', 'import'), p.NOT(p.hasAttrValue('type', 'css')));
var isStyleNode = p.OR(
// inline style
p.hasTagName('style'),
// external stylesheet
p.AND(p.hasTagName('link'), p.hasAttrValue('rel', 'stylesheet')),
// polymer specific external stylesheet
p.AND(p.hasTagName('link'), p.hasAttrValue('rel', 'import'), p.hasAttrValue('type', 'css')));
var isJSScriptNode = p.AND(p.hasTagName('script'), p.OR(p.NOT(p.hasAttr('type')), p.hasAttrValue('type', 'text/javascript'), p.hasAttrValue('type', 'application/javascript')));
function addNode(node, registry) {
    if (isHtmlImportNode(node)) {
        registry.import.push(node);
    } else if (isStyleNode(node)) {
        registry.style.push(node);
    } else if (isJSScriptNode(node)) {
        registry.script.push(node);
    } else if (node.tagName === 'base') {
        registry.base.push(node);
    } else if (node.tagName === 'template') {
        registry.template.push(node);
    } else if (node.tagName === 'dom-module') {
        registry['dom-module'].push(node);
    } else if (dom5.isCommentNode(node)) {
        registry.comment.push(node);
    }
}
function getLineAndColumn(string, charNumber) {
    if (charNumber > string.length) {
        return undefined;
    }
    // TODO(ajo): Caching the line lengths of each document could be much faster.
    var sliced = string.slice(0, charNumber + 1);
    var split = sliced.split('\n');
    var line = split.length;
    var column = split[split.length - 1].length;
    return { line: line, column: column };
}
/**
* Parse html into ASTs.
*
* htmlString is a utf8, html5 document containing polymer elements
* or module definitons.
*
* href is the path of the document.
*/
function importParse(htmlString, href) {
    var doc;
    try {
        doc = dom5.parse(htmlString, { locationInfo: true });
    } catch (err) {
        console.log(err);
        return null;
    }
    // Add line/column information
    dom5.treeMap(doc, function (node) {
        if (node.__location && node.__location.start >= 0) {
            node.__locationDetail = getLineAndColumn(htmlString, node.__location.start);
            if (href) {
                node.__ownerDocument = href;
            }
        }
    });
    var registry = {
        base: [],
        template: [],
        script: [],
        style: [],
        import: [],
        'dom-module': [],
        comment: [],
        ast: doc };
    var queue = [].concat(doc.childNodes);
    var nextNode;
    while (queue.length > 0) {
        nextNode = queue.shift();
        if (nextNode) {
            queue = queue.concat(nextNode.childNodes);
            addNode(nextNode, registry);
        }
    }
    ;
    return registry;
}
exports.importParse = importParse;
;