"use strict";
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
Object.defineProperty(exports, "__esModule", { value: true });
const dom5 = require("dom5");
const parse5_1 = require("parse5");
const matchers = require("./matchers");
/**
 * Move the `node` to be the immediate sibling after the `target` node.
 * TODO(usergenic): Migrate this code to polymer/dom5 and when you do, use
 * insertNode which will handle the remove and the splicing in once you have
 * the index.
 */
function insertAfter(target, node) {
    dom5.remove(node);
    const index = target.parentNode.childNodes.indexOf(target);
    target.parentNode.childNodes.splice(index + 1, 0, node);
    node.parentNode = target.parentNode;
}
exports.insertAfter = insertAfter;
/**
 * Move the entire collection of nodes to be the immediate sibling before the
 * `after` node.
 */
function insertAllBefore(target, after, nodes) {
    let lastNode = after;
    for (let n = nodes.length - 1; n >= 0; n--) {
        const node = nodes[n];
        dom5.insertBefore(target, lastNode, node);
        lastNode = node;
    }
}
exports.insertAllBefore = insertAllBefore;
/**
 * Return true if node is a text node that is empty or consists only of white
 * space.
 */
function isBlankTextNode(node) {
    return node && dom5.isTextNode(node) &&
        dom5.getTextContent(node).trim() === '';
}
exports.isBlankTextNode = isBlankTextNode;
/**
 * Return true if comment starts with a `!` character indicating it is an
 * "important" comment, needing preservation.
 */
function isImportantComment(node) {
    return !!node.data && !!node.data.match(/^!/);
}
exports.isImportantComment = isImportantComment;
/**
 * Return true if node is a comment node consisting of a license (annotated by
 * the `@license` string.)
 */
function isLicenseComment(node) {
    if (dom5.isCommentNode(node)) {
        return dom5.getTextContent(node).indexOf('@license') > -1;
    }
    return false;
}
exports.isLicenseComment = isLicenseComment;
/**
 * Return true if node is a comment node that is a server-side-include.  E.g.
 * <!--#directive ...-->
 */
function isServerSideIncludeComment(node) {
    return !!node.data && !!node.data.match(/^#/);
}
exports.isServerSideIncludeComment = isServerSideIncludeComment;
/**
 * Inserts the node as the first child of the parent.
 * TODO(usergenic): Migrate this code to polymer/dom5
 */
function prepend(parent, node) {
    if (parent.childNodes && parent.childNodes.length) {
        dom5.insertBefore(parent, parent.childNodes[0], node);
    }
    else {
        dom5.append(parent, node);
    }
}
exports.prepend = prepend;
/**
 * Removes an AST Node and the whitespace-only text node following it, if
 * present.
 */
function removeElementAndNewline(node, replacement) {
    const siblings = Array.from(node.parentNode.childNodes);
    let nextIdx = siblings.indexOf(node) + 1;
    let next = siblings[nextIdx];
    while (next && isBlankTextNode(next)) {
        dom5.remove(next);
        next = siblings[++nextIdx];
    }
    if (replacement) {
        dom5.replace(node, replacement);
    }
    else {
        dom5.remove(node);
    }
}
exports.removeElementAndNewline = removeElementAndNewline;
/**
 * A common pattern is to parse html and then remove the fake nodes.
 * This function dries up that pattern.
 */
function parse(html, options) {
    const ast = parse5_1.parse(html, Object.assign({ locationInfo: true }, options));
    dom5.removeFakeRootElements(ast);
    return ast;
}
exports.parse = parse;
/**
 * Returns true if the nodes are given in order as they appear in the source
 * code.
 * TODO(usergenic): Port this to `dom5` and do it with typings for location info
 * instead of all of these string-based lookups.
 */
function inSourceOrder(left, right) {
    const l = left.__location, r = right.__location;
    return l && r && l['line'] && r['line'] &&
        (l['line'] < r['line'] ||
            (l['line'] === r['line'] && l['col'] < r['col']));
}
exports.inSourceOrder = inSourceOrder;
/**
 * Returns true if both nodes have the same line and column according to their
 * location info.
 * TODO(usergenic): Port this to `dom5` and do it with typings for location info
 * instead of all of these string-based lookups.
 */
function sameNode(node1, node2) {
    const l1 = node1.__location, l2 = node2.__location;
    return !!(l1 && l2 && l1['line'] && l1['col'] && l1['line'] === l2['line'] &&
        l1['col'] === l2['col']);
}
exports.sameNode = sameNode;
/**
 * Return all sibling nodes following node.
 */
function siblingsAfter(node) {
    const siblings = Array.from(node.parentNode.childNodes);
    return siblings.slice(siblings.indexOf(node) + 1);
}
exports.siblingsAfter = siblingsAfter;
/**
 * Find all comment nodes in the document, removing them from the document
 * if they are note license comments, and if they are license comments,
 * deduplicate them and prepend them in document's head.
 */
function stripComments(document) {
    const uniqueLicenseTexts = new Set();
    const licenseComments = [];
    for (const comment of dom5.nodeWalkAll(document, dom5.isCommentNode, undefined, dom5.childNodesIncludeTemplate)) {
        if (isImportantComment(comment) || isServerSideIncludeComment(comment)) {
            continue;
        }
        // Make whitespace uniform so we can deduplicate based on actual content.
        const commentText = (comment.data || '').replace(/\s+/g, ' ').trim();
        if (isLicenseComment(comment) && !uniqueLicenseTexts.has(commentText)) {
            uniqueLicenseTexts.add(commentText);
            licenseComments.push(comment);
        }
        removeElementAndNewline(comment);
    }
    const prependTarget = dom5.query(document, matchers.head) || document;
    for (const comment of licenseComments.reverse()) {
        prepend(prependTarget, comment);
    }
}
exports.stripComments = stripComments;
//# sourceMappingURL=ast-utils.js.map