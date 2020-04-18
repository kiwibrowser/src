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

var doctrine = require('doctrine');
/**
 * doctrine configuration,
 * CURRENTLY UNUSED BECAUSE PRIVATE
 */
// function configureDoctrine() {
//   // @hero [path/to/image]
//   doctrine.Rules['hero'] = ['parseNamePathOptional', 'ensureEnd'];
//   // // @demo [path/to/demo] [Demo title]
//   doctrine.Rules['demo'] = ['parseNamePathOptional', 'parseDescription', 'ensureEnd'];
//   // // @polymerBehavior [Polymer.BehaviorName]
//   doctrine.Rules['polymerBehavior'] = ['parseNamePathOptional', 'ensureEnd'];
// }
// configureDoctrine();
// @demo [path] [title]
function parseDemo(tag) {
    var match = (tag.description || "").match(/^\s*(\S*)\s*(.*)$/);
    return {
        tag: 'demo',
        type: null,
        name: match ? match[1] : null,
        description: match ? match[2] : null
    };
}
// @hero [path]
function parseHero(tag) {
    return {
        tag: tag.title,
        type: null,
        name: tag.description,
        description: null
    };
}
// @polymerBehavior [name]
function parsePolymerBehavior(tag) {
    return {
        tag: tag.title,
        type: null,
        name: tag.description,
        description: null
    };
}
// @pseudoElement name
function parsePseudoElement(tag) {
    return {
        tag: tag.title,
        type: null,
        name: tag.description,
        description: null
    };
}
var CUSTOM_TAGS = {
    demo: parseDemo,
    hero: parseHero,
    polymerBehavior: parsePolymerBehavior,
    pseudoElement: parsePseudoElement
};
/**
 * Convert doctrine tags to our tag format
 */
function _tagsToHydroTags(tags) {
    if (!tags) return null;
    return tags.map(function (tag) {
        if (tag.title in CUSTOM_TAGS) {
            return CUSTOM_TAGS[tag.title](tag);
        } else {
            return {
                tag: tag.title,
                type: tag.type ? doctrine.type.stringify(tag.type) : null,
                name: tag.name,
                description: tag.description
            };
        }
    });
}
/**
 * removes leading *, and any space before it
 */
function _removeLeadingAsterisks(description) {
    if (typeof description !== 'string') return description;
    return description.split('\n').map(function (line) {
        // remove leading '\s*' from each line
        var match = line.match(/^[\s]*\*\s?(.*)$/);
        return match ? match[1] : line;
    }).join('\n');
}
/**
 * Given a JSDoc string (minus opening/closing comment delimiters), extract its
 * description and tags.
 *
 * @param {string} docs
 * @return {?Annotation}
 */
function parseJsdoc(docs) {
    docs = _removeLeadingAsterisks(docs);
    var d = doctrine.parse(docs, {
        unwrap: false,
        lineNumber: true,
        preserveWhitespace: true
    });
    return {
        description: d.description,
        tags: _tagsToHydroTags(d.tags)
    };
}
exports.parseJsdoc = parseJsdoc;
// Utility
function hasTag(jsdoc, tagName) {
    if (!jsdoc || !jsdoc.tags) return false;
    return jsdoc.tags.some(function (tag) {
        return tag.tag === tagName;
    });
}
exports.hasTag = hasTag;
function getTag(jsdoc, tagName, key) {
    if (!jsdoc || !jsdoc.tags) return null;
    for (var i = 0; i < jsdoc.tags.length; i++) {
        var tag = jsdoc.tags[i];
        if (tag.tag === tagName) {
            return key ? tag[key] : tag;
        }
    }
    return null;
}
exports.getTag = getTag;
function unindent(text) {
    if (!text) return text;
    var lines = text.replace(/\t/g, '  ').split('\n');
    var indent = lines.reduce(function (prev, line) {
        if (/^\s*$/.test(line)) return prev; // Completely ignore blank lines.
        var lineIndent = line.match(/^(\s*)/)[0].length;
        if (prev === null) return lineIndent;
        return lineIndent < prev ? lineIndent : prev;
    }, null);
    return lines.map(function (l) {
        return l.substr(indent);
    }).join('\n');
}
exports.unindent = unindent;