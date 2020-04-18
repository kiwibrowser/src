/**
@license
Copyright (c) 2016 The Polymer Project Authors. All rights reserved.
This code may only be used under the BSD style license found at http://polymer.github.io/LICENSE.txt
The complete set of authors may be found at http://polymer.github.io/AUTHORS.txt
The complete set of contributors may be found at http://polymer.github.io/CONTRIBUTORS.txt
Code distributed by Google as part of the polymer project is also
subject to an additional IP rights grant found at http://polymer.github.io/PATENTS.txt
*/

'use strict';
const hyd = require('hydrolysis');
const dom5 = require('dom5');
const Polymer = require('./lib/polymer-styling.js');

const pathResolver = require('./lib/pathresolver');
const initialValues = require('./lib/initial-values').initialValues;

/*
 * Apply Shim uses a dynamicly create DOM element
 * to lookup initial property values at runtime.
 *
 * Override with a fixed table for building
 */
Polymer.ApplyShim._getInitialValueForProperty = (property) =>
  initialValues[property] || '';


const pred = dom5.predicates;

const domModuleCache = Object.create(null);

const domModuleMatch = pred.AND(
  pred.hasTagName('dom-module'),
  pred.OR(
    pred.hasAttr('id'),
    pred.hasAttr('name'),
    pred.hasAttr('is')
  )
);

// TODO: upstream to dom5
const styleMatch = pred.AND(
  pred.hasTagName('style'),
  pred.OR(
    pred.NOT(
      pred.hasAttr('type')
    ),
    pred.hasAttrValue('type', 'text/css')
  )
);

const notStyleMatch = pred.NOT(styleMatch);

const customStyleMatch = pred.AND(
  pred.hasTagName('style'),
  pred.hasAttrValue('is', 'custom-style')
);

const styleIncludeMatch = pred.AND(styleMatch, pred.hasAttr('include'));

// TODO: upstream to dom5
const inlineScriptMatch = pred.AND(
  pred.hasTagName('script'),
  pred.OR(
    pred.NOT(
      pred.hasAttr('type')
    ),
    pred.hasAttrValue('type', 'text/javascript'),
    pred.hasAttrValue('type', 'application/javascript')
  ),
  pred.NOT(
    pred.hasAttr('src')
  )
);

const scopeMap = new WeakMap();

// TODO: upstream to dom5
function ancestorWalk(node, match) {
  while(node) {
    if (match(node)) {
      return node;
    }
    node = node.parentNode;
  }
  return null;
}

function prepend(parent, node) {
  if (parent.childNodes.length > 0) {
    dom5.insertBefore(parent, parent.childNodes[0], node);
  } else {
    dom5.appendChild(parent, node);
  }
}

/*
 * Collect styles from dom-module
 * In addition, make sure those styles are inside a template
 */
function getAndFixDomModuleStyles(module) {
  // TODO: support `.styleModules = ['module-id', ...]` ?
  const styles = dom5.queryAll(module, styleMatch);
  if (!styles.length) {
    return [];
  }
  let template = dom5.query(module, pred.hasTagName('template'));
  if (!template) {
    template = dom5.constructors.element('template');
    const content = dom5.constructors.fragment();
    styles.forEach(s => dom5.append(content, s));
    dom5.append(template, content);
    dom5.append(module, template);
  } else {
    styles.forEach(s => {
      // create a template if one does not exist for this dom-module with styles
      let templateContent = template.childNodes[0];
      if (!templateContent) {
        templateContent = dom5.constructors.fragment();
        dom5.append(template, templateContent);
      }
      // ensure element styles are inside the template element
      const parent = ancestorWalk(s, n =>
        n === templateContent || n === module
      );
      if (parent !== templateContent) {
        prepend(templateContent, s);
      }
    })
  }
  return styles;
}

// TODO: consider upstreaming to dom5
function getAttributeArray(node, attribute) {
  const attr = dom5.getAttribute(node, attribute);
  let array;
  if (!attr) {
    array = [];
  } else {
    array = attr.split(' ');
  }
  return array;
}

function inlineStyleIncludes(style) {
  if (!styleIncludeMatch(style)) {
    return;
  }
  const styleText = [];
  const includes = getAttributeArray(style, 'include');
  const leftover = [];
  const baseDocument = style.__ownerDocument;
  includes.forEach(id => {
    const module = domModuleCache[id];
    if (!module) {
      // we missed this one, put it back on later
      leftover.push(id);
      return;
    }
    const includedStyles = getAndFixDomModuleStyles(module);
    // gather included styles
    includedStyles.forEach(ism => {
      // this style may also have includes
      inlineStyleIncludes(ism);
      const inlineDocument = module.__ownerDocument;
      let includeText = dom5.getTextContent(ism);
      // adjust paths
      includeText = pathResolver.rewriteURL(inlineDocument, baseDocument, includeText);
      styleText.push(includeText);
    });
  });
  // remove inlined includes
  if (leftover.length) {
    dom5.setAttribute(style, 'include', leftover.join(' '));
  } else {
    dom5.removeAttribute(style, 'include');
  }
  // prepend included styles
  if (styleText.length) {
    let text = dom5.getTextContent(style);
    text = styleText.join('') + text;
    dom5.setTextContent(style, text);
  }
}

function applyShim(ast) {
  /*
   * `transform` expects an array of decorated <style> elements
   *
   * Decorated <style> elements are ones with `__cssRules` property
   * with a value of the CSS ast
   */
  Polymer.StyleUtil.forEachRule(ast, rule => Polymer.ApplyShim.transformRule(rule));
}

function getModuleDefinition(moduleName, elementDescriptors) {
  for (let ed of elementDescriptors) {
    if (ed.is.toLowerCase() === moduleName) {
      return ed;
    }
  }
  return null;
}

function getTypeExtends(elementDescriptor) {
  let props = elementDescriptor.properties || [];
  let ret = '';
  // loop over element properties with javascript AST
  for (let i = 0; i < props.length; i++) {
    if (props[i].name === 'extends') {
      // node is espree AST node
      let node = props[i].javascriptNode;
      // property node has a value node with key and value nodes
      if (node && node.type === 'Property') {
        ret = node.value.value;
        break;
      }
    }
  }
  return ret.toLowerCase();
}

function shadyShim(ast, style, elements) {
  const scope = scopeMap.get(style);
  const moduleDefinition = getModuleDefinition(scope, elements);
  // only shim if module is a full polymer element, not just a style module
  if (!scope || !moduleDefinition) {
    return;
  }
  const ext = getTypeExtends(moduleDefinition);
  Polymer.StyleTransformer.css(ast, scope, ext);
}

function addClass(node, className) {
  const classList = getAttributeArray(node, 'class');
  if (classList.indexOf('style-scope') === -1) {
    classList.push('style-scope');
  }
  if (classList.indexOf(className) === -1) {
    classList.push(className);
  }
  dom5.setAttribute(node, 'class', classList.join(' '));
}

function markElement(domModule, scope) {
  const buildType = Polymer.Settings.useNativeShadow ? 'shadow' : 'shady';
  dom5.setAttribute(domModule, 'css-build', buildType);
  // mark elements' subtree under shady build
  if (buildType === 'shady' && scope) {
    const template = dom5.query(domModule, pred.hasTagName('template'));
    // apply scoping to template
    if (template) {
      const elements = dom5.queryAll(template, notStyleMatch);
      elements.forEach(el => addClass(el, scope));
    }
  }
}

function slotToContent(ast) {
  Polymer.StyleUtil.forEachRule(ast, (rule) => {
    rule.selector = Polymer.StyleTransformer._slottedToContent(rule.selector);
  });
}

function polymerCssBuild(paths, options) {
  if (options && options['build-for-shady']) {
    Polymer.Settings.useNativeShadow = false;
  }
  const nativeShadow = Polymer.Settings.useNativeShadow;
  // build hydrolysis loader
  const loader = new hyd.Loader();
  // ignore all files we can't find
  loader.addResolver(new hyd.NoopResolver({test: () => true}));
  // load given files as strings
  paths.forEach(p => {
    loader.addResolver(new hyd.StringResolver(p));
  });
  const analyzer = new hyd.Analyzer(true, loader);
  // run analyzer on all given files
  return Promise.all(
    paths.map(p => analyzer.metadataTree(p.url))
  ).then(() => {
    // un-inline scripts that hydrolysis accendentally inlined
    analyzer.nodeWalkAllDocuments(inlineScriptMatch).forEach(script => {
      if (script.__hydrolysisInlined) {
        dom5.setAttribute(script, 'src', script.__hydrolysisInlined);
        dom5.setTextContent(script, '');
      }
    });
  }).then(() => {
    // map dom modules to styles
    return analyzer.nodeWalkAllDocuments(domModuleMatch).map(el => {
      const id = dom5.getAttribute(el, 'id') || dom5.getAttribute(el, 'name') ||
          dom5.getAttribute(el, 'is');
      if (!id) {
        return [];
      }
      const scope = id.toLowerCase();
      // populate cache
      domModuleCache[scope] = el;
      // mark the module as built
      markElement(el, scope);
      const styles = getAndFixDomModuleStyles(el);
      styles.forEach(s => scopeMap.set(s, scope));
      return styles;
    });
  }).then(moduleStyles => {
    // inline and flatten styles into a single list
    const flatStyles = [];
    moduleStyles.forEach(styles => {
      if (!styles.length) {
        return;
      }
      // do style includes
      if (!options['no-inline-includes']) {
        styles.forEach(s => inlineStyleIncludes(s));
      }
      // reduce styles to one
      const finalStyle = styles[styles.length - 1];
      dom5.setAttribute(finalStyle, 'scope', scopeMap.get(finalStyle));
      if (styles.length > 1) {
        const consumed = styles.slice(0, -1);
        const text = styles.map(s => dom5.getTextContent(s));
        const includes = styles.map(s => getAttributeArray(s, 'include')).reduce((acc, inc) => acc.concat(inc));
        consumed.forEach(c => dom5.remove(c));
        dom5.setTextContent(finalStyle, text.join(''));
        const oldInclude = getAttributeArray(finalStyle, 'include');
        const newInclude = oldInclude.concat(includes).join(' ');
        if (newInclude) {
          dom5.setAttribute(finalStyle, 'include', newInclude);
        }
      }
      flatStyles.push(finalStyle);
    });
    return flatStyles;
  }).then(styles => {
    // find custom styles
    const customStyles = analyzer.nodeWalkAllDocuments(customStyleMatch);
    // inline custom styles with includes
    customStyles.forEach(s => inlineStyleIncludes(s));
    // add in custom styles
    return customStyles.concat(styles);
  }).then(styles => {
    // populate mixin map
    styles.forEach(s => {
      const text = dom5.getTextContent(s);
      const ast = Polymer.CssParse.parse(text);
      applyShim(ast);
    });
    // parse, transform, emit
    styles.forEach(s => {
      let text = dom5.getTextContent(s);
      const ast = Polymer.CssParse.parse(text);
      if (customStyleMatch(s)) {
        // custom-style `:root` selectors need to be processed to `html`
        Polymer.StyleUtil.forEachRule(ast, rule => {
          Polymer.StyleTransformer.documentRule(rule);
        });
        // mark the style as built
        markElement(s);
      }
      applyShim(ast);
      if (nativeShadow) {
        slotToContent(ast);
      } else {
        shadyShim(ast, s, analyzer.elements);
      }
      text = Polymer.CssParse.stringify(ast, true);
      dom5.setTextContent(s, text);
    });
  }).then(() => {
    return paths.map(p => {
      return {
        url: p.url,
        content: dom5.serialize(analyzer.parsedDocuments[p.url])
      };
    });
  });
}

exports.polymerCssBuild = polymerCssBuild;
