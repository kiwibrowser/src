// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @typedef {Document|DocumentFragment|Element} */
var ProcessingRoot;

/**
 * @fileoverview This is a simple template engine inspired by JsTemplates
 * optimized for i18n.
 *
 * It currently supports three handlers:
 *
 *   * i18n-content which sets the textContent of the element.
 *
 *     <span i18n-content="myContent"></span>
 *
 *   * i18n-options which generates <option> elements for a <select>.
 *
 *     <select i18n-options="myOptionList"></select>
 *
 *   * i18n-values is a list of attribute-value or property-value pairs.
 *     Properties are prefixed with a '.' and can contain nested properties.
 *
 *     <span i18n-values="title:myTitle;.style.fontSize:fontSize"></span>
 *
 * This file is a copy of i18n_template.js, with minor tweaks to support using
 * load_time_data.js. It should replace i18n_template.js eventually.
 */

var i18nTemplate = (function() {
  /**
   * This provides the handlers for the templating engine. The key is used as
   * the attribute name and the value is the function that gets called for every
   * single node that has this attribute.
   * @type {!Object}
   */
  var handlers = {
    /**
     * This handler sets the textContent of the element.
     * @param {!HTMLElement} element The node to modify.
     * @param {string} key The name of the value in |data|.
     * @param {!LoadTimeData} data The data source to draw from.
     * @param {!Set<ProcessingRoot>} visited
     */
    'i18n-content': function(element, key, data, visited) {
      element.textContent = data.getString(key);
    },

    /**
     * This handler adds options to a <select> element.
     * @param {!HTMLElement} select The node to modify.
     * @param {string} key The name of the value in |data|. It should
     *     identify an array of values to initialize an <option>. Each value,
     *     if a pair, represents [content, value]. Otherwise, it should be a
     *     content string with no value.
     * @param {!LoadTimeData} data The data source to draw from.
     * @param {!Set<ProcessingRoot>} visited
     */
    'i18n-options': function(select, key, data, visited) {
      var options = data.getValue(key);
      options.forEach(function(optionData) {
        var option = typeof optionData == 'string' ?
            new Option(optionData) :
            new Option(optionData[1], optionData[0]);
        select.appendChild(option);
      });
    },

    /**
     * This is used to set HTML attributes and DOM properties. The syntax is:
     *   attributename:key;
     *   .domProperty:key;
     *   .nested.dom.property:key
     * @param {!HTMLElement} element The node to modify.
     * @param {string} attributeAndKeys The path of the attribute to modify
     *     followed by a colon, and the name of the value in |data|.
     *     Multiple attribute/key pairs may be separated by semicolons.
     * @param {!LoadTimeData} data The data source to draw from.
     * @param {!Set<ProcessingRoot>} visited
     */
    'i18n-values': function(element, attributeAndKeys, data, visited) {
      var parts = attributeAndKeys.replace(/\s/g, '').split(/;/);
      parts.forEach(function(part) {
        if (!part)
          return;

        var attributeAndKeyPair = part.match(/^([^:]+):(.+)$/);
        if (!attributeAndKeyPair)
          throw new Error('malformed i18n-values: ' + attributeAndKeys);

        var propName = attributeAndKeyPair[1];
        var propExpr = attributeAndKeyPair[2];

        var value = data.getValue(propExpr);

        // Allow a property of the form '.foo.bar' to assign a value into
        // element.foo.bar.
        if (propName[0] == '.') {
          var path = propName.slice(1).split('.');
          var targetObject = element;
          while (targetObject && path.length > 1) {
            targetObject = targetObject[path.shift()];
          }
          if (targetObject) {
            targetObject[path] = value;
            // In case we set innerHTML (ignoring others) we need to recursively
            // check the content.
            if (path == 'innerHTML') {
              for (var i = 0; i < element.children.length; ++i) {
                processWithoutCycles(element.children[i], data, visited, false);
              }
            }
          }
        } else {
          element.setAttribute(propName, /** @type {string} */ (value));
        }
      });
    }
  };

  var prefixes = [''];

  // Only look through shadow DOM when it's supported. As of April 2015, iOS
  // Chrome doesn't support shadow DOM.
  if (Element.prototype.createShadowRoot)
    prefixes.push('* /deep/ ');

  var attributeNames = Object.keys(handlers);
  var selector = prefixes
                     .map(function(prefix) {
                       return prefix + '[' +
                           attributeNames.join('], ' + prefix + '[') + ']';
                     })
                     .join(', ');

  /**
   * Processes a DOM tree using a |data| source to populate template values.
   * @param {!ProcessingRoot} root The root of the DOM tree to process.
   * @param {!LoadTimeData} data The data to draw from.
   */
  function process(root, data) {
    processWithoutCycles(root, data, new Set(), true);
  }

  /**
   * Internal process() method that stops cycles while processing.
   * @param {!ProcessingRoot} root
   * @param {!LoadTimeData} data
   * @param {!Set<ProcessingRoot>} visited Already visited roots.
   * @param {boolean} mark Whether nodes should be marked processed.
   */
  function processWithoutCycles(root, data, visited, mark) {
    if (visited.has(root)) {
      // Found a cycle. Stop it.
      return;
    }

    // Mark the node as visited before recursing.
    visited.add(root);

    var importLinks = root.querySelectorAll('link[rel=import]');
    for (var i = 0; i < importLinks.length; ++i) {
      var importLink = /** @type {!HTMLLinkElement} */ (importLinks[i]);
      if (!importLink.import) {
        // Happens when a <link rel=import> is inside a <template>.
        // TODO(dbeam): should we log an error if we detect that here?
        continue;
      }
      processWithoutCycles(importLink.import, data, visited, mark);
    }

    var templates = root.querySelectorAll('template');
    for (var i = 0; i < templates.length; ++i) {
      var template = /** @type {HTMLTemplateElement} */ (templates[i]);
      if (!template.content)
        continue;
      processWithoutCycles(template.content, data, visited, mark);
    }

    var isElement = root instanceof Element;
    if (isElement && root.webkitMatchesSelector(selector))
      processElement(/** @type {!Element} */ (root), data, visited);

    var elements = root.querySelectorAll(selector);
    for (var i = 0; i < elements.length; ++i) {
      processElement(elements[i], data, visited);
    }

    if (mark) {
      var processed = isElement ? [root] : root.children;
      if (processed) {
        for (var i = 0; i < processed.length; ++i) {
          processed[i].setAttribute('i18n-processed', '');
        }
      }
    }
  }

  /**
   * Run through various [i18n-*] attributes and populate.
   * @param {!Element} element
   * @param {!LoadTimeData} data
   * @param {!Set<ProcessingRoot>} visited
   */
  function processElement(element, data, visited) {
    for (var i = 0; i < attributeNames.length; i++) {
      var name = attributeNames[i];
      var attribute = element.getAttribute(name);
      if (attribute != null)
        handlers[name](element, attribute, data, visited);
    }
  }

  return {process: process};
}());
