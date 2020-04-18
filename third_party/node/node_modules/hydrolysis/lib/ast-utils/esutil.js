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

var _typeof = typeof Symbol === "function" && typeof Symbol.iterator === "symbol" ? function (obj) { return typeof obj; } : function (obj) { return obj && typeof Symbol === "function" && obj.constructor === Symbol && obj !== Symbol.prototype ? "symbol" : typeof obj; };

var estraverse = require("estraverse");
var escodegen = require('escodegen');
/**
 * Returns whether an Espree node matches a particular object path.
 *
 * e.g. you have a MemberExpression node, and want to see whether it represents
 * `Foo.Bar.Baz`:
 *
 *     matchesCallExpression(node, ['Foo', 'Bar', 'Baz'])
 *
 * @param {ESTree.Node} expression The Espree node to match against.
 * @param {Array<string>} path The path to look for.
 */
function matchesCallExpression(expression, path) {
    if (!expression.property || !expression.object) return;
    console.assert(path.length >= 2);
    if (expression.property.type !== 'Identifier') {
        return;
    }
    var property = expression.property;
    // Unravel backwards, make sure properties match each step of the way.
    if (property.name !== path[path.length - 1]) return false;
    // We've got ourselves a final member expression.
    if (path.length == 2 && expression.object.type === 'Identifier') {
        return expression.object.name === path[0];
    }
    // Nested expressions.
    if (path.length > 2 && expression.object.type == 'MemberExpression') {
        return matchesCallExpression(expression.object, path.slice(0, path.length - 1));
    }
    return false;
}
exports.matchesCallExpression = matchesCallExpression;
/**
 * @param {Node} key The node representing an object key or expression.
 * @return {string} The name of that key.
 */
function objectKeyToString(key) {
    if (key.type == 'Identifier') {
        return key.name;
    }
    if (key.type == 'Literal') {
        return key.value.toString();
    }
    if (key.type == 'MemberExpression') {
        var mEx = key;
        return objectKeyToString(mEx.object) + '.' + objectKeyToString(mEx.property);
    }
}
exports.objectKeyToString = objectKeyToString;
var CLOSURE_CONSTRUCTOR_MAP = {
    'Boolean': 'boolean',
    'Number': 'number',
    'String': 'string'
};
/**
 * AST expression -> Closure type.
 *
 * Accepts literal values, and native constructors.
 *
 * @param {Node} node An Espree expression node.
 * @return {string} The type of that expression, in Closure terms.
 */
function closureType(node) {
    if (node.type.match(/Expression$/)) {
        return node.type.substr(0, node.type.length - 10);
    } else if (node.type === 'Literal') {
        return _typeof(node.value);
    } else if (node.type === 'Identifier') {
        var ident = node;
        return CLOSURE_CONSTRUCTOR_MAP[ident.name] || ident.name;
    } else {
        throw {
            message: 'Unknown Closure type for node: ' + node.type,
            location: node.loc.start
        };
    }
}
exports.closureType = closureType;
function getAttachedComment(node) {
    var comments = getLeadingComments(node) || getLeadingComments(node['key']);
    if (!comments) {
        return;
    }
    return comments[comments.length - 1];
}
exports.getAttachedComment = getAttachedComment;
/**
 * Returns all comments from a tree defined with @event.
 */
function getEventComments(node) {
    var eventComments = [];
    estraverse.traverse(node, {
        enter: function enter(node) {
            var comments = (node.leadingComments || []).concat(node.trailingComments || []).map(function (commentAST) {
                return commentAST.value;
            }).filter(function (comment) {
                return comment.indexOf("@event") != -1;
            });
            eventComments = eventComments.concat(comments);
        },
        keys: {
            Super: []
        }
    });
    // dedup
    return eventComments.filter(function (el, index, array) {
        return array.indexOf(el) === index;
    });
}
exports.getEventComments = getEventComments;
function getLeadingComments(node) {
    if (!node) {
        return;
    }
    var comments = node.leadingComments;
    if (!comments || comments.length === 0) return;
    return comments.map(function (comment) {
        return comment.value;
    });
}
/**
 * Converts a estree Property AST node into its Hydrolysis representation.
 */
function toPropertyDescriptor(node) {
    var type = closureType(node.value);
    if (type == "Function") {
        if (node.kind === "get" || node.kind === "set") {
            type = '';
            node[node.kind + "ter"] = true;
        }
    }
    var result = {
        name: objectKeyToString(node.key),
        type: type,
        desc: getAttachedComment(node),
        javascriptNode: node
    };
    if (type === 'Function') {
        var value = node.value;
        result.params = (value.params || []).map(function (param) {
            // With ES6 we can have a variety of param patterns. Best to leave the
            // formatting to escodegen.
            return { name: escodegen.generate(param) };
        });
    }
    return result;
}
exports.toPropertyDescriptor = toPropertyDescriptor;