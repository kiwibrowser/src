// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/*
 * Copyright (C) 2011 Google Inc.  All rights reserved.
 * Copyright (C) 2007, 2008 Apple Inc.  All rights reserved.
 * Copyright (C) 2008 Matt Lilek <webkit@mattlilek.com>
 * Copyright (C) 2009 Joseph Pecoraro
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
'use strict';

if (!xPathTools) {
  var xPathTools = {};

  /**
   * @param {!WebInspector.DOMNode} node
   * @param {boolean=} optimized
   * @return {string}
   */
  xPathTools.xPath = function(node, optimized) {
    if (node.nodeType === Node.DOCUMENT_NODE) {
      return '/';
    }

    var steps = [];
    var contextNode = node;
    while (contextNode) {
      var step = xPathTools._xPathValue(contextNode, optimized);
      if (!step) {
        break;  // Error - bail out early.
      }
      steps.push(step);
      if (step.unique) {
        break;
      }
      contextNode = contextNode.parentNode;
    }

    steps.reverse();
    return (steps.length && steps[0].unique ? '' : '/') + steps.join('/');
  };

  /**
   * @param {!WebInspector.DOMNode} node
   * @param {boolean=} optimized
   * @return {?xPathTools.DOMNodePathStep}
   */
  xPathTools._xPathValue = function(node, optimized) {
    var ownValue;
    var ownIndex = xPathTools._xPathIndex(node);
    if (ownIndex === -1) {
      return null;  // Error.
    }

    switch (node.nodeType) {
      case Node.ELEMENT_NODE:
        if (optimized) {
          const optimizedStep = xPathTools._optimizedElementNodeStep(node);

          if (optimizedStep !== null) {
            return optimizedStep;
          }
        }
        ownValue = node.localName;
        break;
      case Node.ATTRIBUTE_NODE:
        ownValue = '@' + node.nodeName;
        break;
      case Node.TEXT_NODE:
      case Node.CDATA_SECTION_NODE:
        ownValue = 'text()';
        break;
      case Node.PROCESSING_INSTRUCTION_NODE:
        ownValue = 'processing-instruction()';
        break;
      case Node.COMMENT_NODE:
        ownValue = 'comment()';
        break;
      case Node.DOCUMENT_NODE:
        ownValue = '';
        break;
      default:
        ownValue = '';
        break;
    }

    if (ownIndex > 0) {
      ownValue += '[' + ownIndex + ']';
    }

    return new xPathTools.DOMNodePathStep(
        ownValue, node.nodeType === Node.DOCUMENT_NODE);
  };

  /**
   * @constructor
   * @param {string} value
   * @param {boolean} unique
   */
  xPathTools.xPathBuilder = function(localName) {
    this._localName = localName;
    this._classifiers = [];
  };

  xPathTools.xPathBuilder.prototype = {
    /**
     * @return {number} The number of matching children of document
     */
    countMatches: function() {
      let queryResult = document.evaluate(
          `count(//${this})`, document, null, XPathResult.NUMBER_TYPE, null);
      return queryResult.numberValue;
    },

    /**
     * @param {string} classifier
     */
    addClassifier: function(classifier) {
      if (classifier) {
        this._classifiers.push(classifier);
      }
    },

    /**
     * @override
     * Note that the resultant string should be either prepended with one or two
     * forward slashes (for absolute and relative xPaths respectively).
     * @return {string}
     */
    toString: function() {
      if (this._classifiers.length > 0) {
        let xPathSelector = this._localName + '[';

        let first = true;

        for (var i = 0; i < this._classifiers.length; i++) {
          if (!first) {
            xPathSelector += ' and ';
          } else {
            first = false;
          }

          xPathSelector += this._classifiers[i];
        }

        xPathSelector += ']';

        return xPathSelector;
      } else {
        return this._localName;
      }
    }
  };

  /**
   * Generate an optimized node path step for a given element node.
   * @param  {!WebInspector.Element} element
   * @return {?xPathTools.DOMNodePathStep}
   */
  xPathTools._optimizedElementNodeStep = function(element) {
    let isUnique = false;

    // Use id if available
    if (element.id) {
      isUnique = true;
      return new xPathTools.DOMNodePathStep(
          `//*[@id=\"${element.id}\"]`, isUnique);
    }

    const builder = new xPathTools.xPathBuilder(element.localName);
    console.log(`All attributes for ${element.localName}`, element.attributes);

    switch (element.localName) {
      case 'input':
      case 'textarea':
      case 'select':
        const elementType = element.getAttribute('type');
        console.log('Element Type', elementType);

        // Use type if available & unique
        if (elementType) {
          builder.addClassifier(`@type='${elementType}'`);

          switch (elementType) {
            case 'radio':
            case 'submit':
              const elementValue = element.getAttribute('value');

              if (elementValue) {
                builder.addClassifier(`@value='${elementValue}'`);
              }
              break;
          }

          let matchCount = builder.countMatches();

          if (matchCount === 1) {
            isUnique = true;
            break;
          }
        }

        const elementName = element.getAttribute('name');
        console.log('Element Name', elementName);

        // Use name if available & unique
        if (elementName) {
          builder.addClassifier(`@name='${elementName}'`);

          let matchCount = builder.countMatches();

          if (matchCount === 1) {
            isUnique = true;
            break;
          }
        }
        break;
      case 'a':
      case 'span':
      case 'button':
        if (element.textContent) {
          let trimmedContent = element.textContent.trim();
          // .replace(
          //   /^[\t\n\ ]+|[\t\n\ ]+$/g, ''
          // );
          builder.addClassifier(`contains(., '${trimmedContent}')`);

          let matchCount = builder.countMatches();

          if (matchCount === 1) {
            isUnique = true;
            break;
          }
        }
        break;
    }

    if (builder._classifiers.length > 0) {
      let xPathSelector = builder.toString();

      if (isUnique) {
        xPathSelector = `//${xPathSelector}`;
      }

      console.log(
          `Generated xPathSelector for ${element.localName}`, xPathSelector);

      return new xPathTools.DOMNodePathStep(xPathSelector, isUnique);
    } else {
      return null;
    }
  };

  /**
   * @param {!WebInspector.DOMNode} node
   * @return {number}
   */
  xPathTools._xPathIndex = function(node) {
    // Returns -1 on error, 0 if no siblings matching the same expression
    // <XPath index among the same expression-matching sibling nodes> otherwise.
    function areNodesSimilar(left, right) {
      if (left === right) {
        return true;
      }

      if (left.nodeType === Node.ELEMENT_NODE &&
          right.nodeType === Node.ELEMENT_NODE) {
        return left.localName === right.localName;
      }

      if (left.nodeType === right.nodeType) {
        return true;
      }

      // XPath treats CDATA as text nodes.
      var leftType = left.nodeType === Node.CDATA_SECTION_NODE ?
          Node.TEXT_NODE :
          left.nodeType;
      var rightType = right.nodeType === Node.CDATA_SECTION_NODE ?
          Node.TEXT_NODE :
          right.nodeType;
      return leftType === rightType;
    }

    var siblings = node.parentNode ? node.parentNode.childNodes : null;
    if (!siblings) {
      return 0;  // Root node - no siblings.
    }
    var hasSameNamedElements;
    for (var i = 0; i < siblings.length; ++i) {
      if (areNodesSimilar(node, siblings[i]) && siblings[i] !== node) {
        hasSameNamedElements = true;
        break;
      }
    }
    if (!hasSameNamedElements) {
      return 0;
    }
    var ownIndex = 1;  // XPath indices start with 1.
    for (var i = 0; i < siblings.length; ++i) {
      if (areNodesSimilar(node, siblings[i])) {
        if (siblings[i] === node) {
          return ownIndex;
        }
        ++ownIndex;
      }
    }
    return -1;  // An error occurred: |node| not found in parent's children.
  };

  /**
   * @constructor
   * @param {string} value
   * @param {boolean} unique
   */
  xPathTools.DOMNodePathStep = function(value, unique) {
    this.value = value;
    this.unique = unique || false;
  };

  xPathTools.DOMNodePathStep.prototype = {
    /**
     * @override
     * @return {string}
     */
    toString: function() { return this.value; }
  };
}
