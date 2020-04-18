"use strict";
/**
 * Copyright (c) 2015 The Polymer Project Authors. All rights reserved.
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
const parse5 = require("parse5");
const model_1 = require("../model/model");
/**
 * Given a position and an HTML document, try to describe what new text typed
 * at the given position would be.
 *
 * Where possible we try to return the ASTNode describing that position, but
 * sometimes there does not actually exist one. (for a simple case, the empty
 * string should be interpreted as a text node, but there is no text node in
 * an empty document, but there would be after the first character was typed).
 */
function getLocationInfoForPosition(document, position) {
    const location = _getLocationInfoForPosition(document.ast, position, document);
    if (!location) {
        /** Eh, we're probably in a text node. */
        return { kind: 'text' };
    }
    return location;
}
exports.getLocationInfoForPosition = getLocationInfoForPosition;
function _getLocationInfoForPosition(node, position, document) {
    const sourceRange = document.sourceRangeForNode(node);
    const location = node.__location;
    /**
     * An HTML5 parser must hallucinate certain nodes, even if they don't exist
     * in the original source text. e.g. <html> or <body>. So we might have
     * elements that have no sourceRange (because they don't exist in the text)
     * but they do have children that do. So we should check those children.
     */
    if (!(sourceRange && location)) {
        return _findLocationInChildren(node, position, document);
    }
    if (!model_1.isPositionInsideRange(position, sourceRange)) {
        // definitively not in this node or any of its children
        return;
    }
    const locationInChildren = _findLocationInChildren(node, position, document);
    if (locationInChildren) {
        return locationInChildren;
    }
    const attributeLocation = getAttributeLocation(node, position, document, location);
    if (attributeLocation) {
        return attributeLocation;
    }
    const startTagRange = document.sourceRangeForStartTag(node);
    const endTagRange = document.sourceRangeForEndTag(node);
    // If we're in the end tag... we're in the end tag.
    if (model_1.isPositionInsideRange(position, endTagRange, false)) {
        return { kind: 'endTag', element: node };
    }
    if (startTagRange && model_1.isPositionInsideRange(position, startTagRange, false)) {
        if (position.line === startTagRange.start.line) {
            // If the cursor is in the "<my-elem" part of the start tag.
            if (position.column <=
                startTagRange.start.column + (node.tagName || '').length + 1) {
                return { kind: 'tagName', element: node };
            }
        }
        // Otherwise we're in the start tag, but not in the tag name or any
        // particular attribute, but definitely in the attributes section.
        return { kind: 'attribute', attribute: null, element: node };
    }
    // The edges of a comment aren't part of the comment.
    if (parse5.treeAdapters.default.isCommentNode(node) &&
        model_1.isPositionInsideRange(position, sourceRange, false)) {
        return { kind: 'comment', commentNode: node };
    }
    if (parse5.treeAdapters.default.isTextNode(node)) {
        const parent = node.parentNode;
        if (parent && parent.tagName === 'script') {
            return { kind: 'scriptTagContents', textNode: node };
        }
        if (parent && parent.tagName === 'style') {
            return { kind: 'styleTagContents', textNode: node };
        }
        return { kind: 'text', textNode: node };
    }
    if (model_1.isPositionInsideRange(position, sourceRange, false)) {
        /**
         * This is tricky. Consider the position inside an empty element, i.e.
         * here:
         *    <script>|</script>.
         *
         * You can be between the start and end tags, but there won't be a text
         * node to attach to, but if you started typeing, there would be, so we
         * want to treat you as though you are.
         */
        if (startTagRange && endTagRange &&
            model_1.comparePositionAndRange(position, startTagRange, false) > 0 &&
            model_1.comparePositionAndRange(position, endTagRange, false) < 0) {
            if (node.tagName === 'script') {
                return { kind: 'scriptTagContents' };
            }
            if (node.tagName === 'style') {
                return { kind: 'styleTagContents' };
            }
            return { kind: 'text' };
        }
        /**
         * Ok, we're in this node, we're not in any of its children, but we're not
         * obviously in any attribute, tagname, start tag, or end tag. We might be
         * part of a unclosed tag in a mostly empty document. parse5 doesn't give
         * us much explicit signal in this case, but we can kinda infer it from the
         * tagName.
         */
        if (node.tagName) {
            if (position.column <=
                sourceRange.start.column + node.tagName.length + 1) {
                return { kind: 'tagName', element: node };
            }
            return { kind: 'attribute', element: node, attribute: null };
        }
    }
}
function _findLocationInChildren(node, position, document) {
    for (const child of node.childNodes || []) {
        const result = _getLocationInfoForPosition(child, position, document);
        if (result) {
            return result;
        }
    }
    if (node.tagName === 'template') {
        const content = parse5.treeAdapters.default.getTemplateContent(node);
        const result = _getLocationInfoForPosition(content, position, document);
        if (result) {
            return result;
        }
    }
}
function isElementLocationInfo(location) {
    const loc = location;
    return (loc.startTag && loc.endTag) !== undefined;
}
/**
 * If the position is inside of the node's attributes section, return the
 * correct LocationResult.
 */
function getAttributeLocation(node, position, document, location) {
    /**
     * TODO(rictic): upstream to @types the fact that regular locations (not just
     * element locations) can have attrs sometimes.
     */
    const attrs = (isElementLocationInfo(location) && location.startTag.attrs) ||
        location.attrs || {};
    for (const attrName in attrs) {
        const range = document.sourceRangeForAttribute(node, attrName);
        if (model_1.isPositionInsideRange(position, range)) {
            if (model_1.isPositionInsideRange(position, document.sourceRangeForAttributeValue(node, attrName))) {
                return { kind: 'attributeValue', attribute: attrName, element: node };
            }
            return { kind: 'attribute', attribute: attrName, element: node };
        }
    }
}

//# sourceMappingURL=ast-from-source-position.js.map
