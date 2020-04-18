// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Utility functions for processing words within strings and nodes.

/**
 * Regular expression to find the start of the next word after a word boundary.
 * We cannot use \b\W to find the next word because it does not match many
 * unicode characters.
 * @type {RegExp}
 */
const WORD_START_REGEXP = /\b\S/;

/**
 * Regular expression to find the end of the next word, which is followed by
 * whitespace. We cannot use \w\b to find the end of the previous word because
 * \w does not know about many unicode characters.
 * @type {RegExp}
 */
const WORD_END_REGEXP = /\S\s/;

/**
 * Searches through text starting at an index to find the next word's
 * start boundary.
 * @param {string|undefined} text The string to search through
 * @param {number} indexAfter The index into text at which to start
 *      searching.
 * @param {NodeGroupItem} nodeGroupItem The node whose name we are
 *      searching through.
 * @return {number} The index of the next word's start
 */
function getNextWordStart(text, indexAfter, nodeGroupItem) {
  if (nodeGroupItem.hasInlineText && nodeGroupItem.node.children.length > 0) {
    let node = findInlineTextNodeByCharacterIndex(
        nodeGroupItem.node, indexAfter - nodeGroupItem.startChar);
    let startCharInParent = getStartCharIndexInParent(node);
    for (var i = 0; i < node.wordStarts.length; i++) {
      if (node.wordStarts[i] + nodeGroupItem.startChar + startCharInParent <
          indexAfter) {
        continue;
      }
      return node.wordStarts[i] + nodeGroupItem.startChar + startCharInParent;
    }
    // Default: We are just off the edge of this node.
    return node.name.length + nodeGroupItem.startChar + startCharInParent;
  } else {
    // Try to parse using a regex, which is imperfect.
    // Fall back to the given index if we can't find a match.
    return nextWordHelper(text, indexAfter, WORD_START_REGEXP, indexAfter);
  }
}

/**
 * Searches through text starting at an index to find the next word's
 * end boundary.
 * @param {string|undefined} text The string to search through
 * @param {number} indexAfter The index into text at which to start
 *      searching.
 * @param {NodeGroupItem} nodeGroupItem The node whose name we are
 *      searching through.
 * @return {number} The index of the next word's end
 */
function getNextWordEnd(text, indexAfter, nodeGroupItem) {
  if (nodeGroupItem.hasInlineText && nodeGroupItem.node.children.length > 0) {
    let node = findInlineTextNodeByCharacterIndex(
        nodeGroupItem.node, indexAfter - nodeGroupItem.startChar + 1);
    let startCharInParent = getStartCharIndexInParent(node);
    for (var i = 0; i < node.wordEnds.length; i++) {
      if (node.wordEnds[i] + nodeGroupItem.startChar + startCharInParent - 1 <
          indexAfter) {
        continue;
      }
      let result =
          node.wordEnds[i] + nodeGroupItem.startChar + startCharInParent;
      return text.length > result ? result : text.length;
    }
    // Default.
    return text.length;
  } else {
    // Try to parse using a regex, which is imperfect.
    // Fall back to the full length of the text if we can't find a match.
    return nextWordHelper(text, indexAfter, WORD_END_REGEXP, text.length - 1) +
        1;
  }
}

/**
 * Searches through text to find the first index of a regular expression
 * after a given starting index. Returns a default value if no match is
 * found.
 * @param {string|undefined} text The string to search through
 * @param {number} indexAfter The index at which to start searching
 * @param {RegExp} re A regular expression to search for
 * @param {number} defaultValue The default value to return if no
                     match is found.
 * @return {number} The index found by the regular expression, or -1
 *                    if none found.
 */
function nextWordHelper(text, indexAfter, re, defaultValue) {
  if (text === undefined) {
    return defaultValue;
  }
  let result = re.exec(text.substr(indexAfter));
  if (result != null && result.length > 0) {
    return indexAfter + result.index;
  }
  return defaultValue;
}
