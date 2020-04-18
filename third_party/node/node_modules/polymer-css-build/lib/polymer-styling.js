/**
@license
Copyright (c) 2016 The Polymer Project Authors. All rights reserved.
This code may only be used under the BSD style license found at http://polymer.github.io/LICENSE.txt
The complete set of authors may be found at http://polymer.github.io/AUTHORS.txt
The complete set of contributors may be found at http://polymer.github.io/CONTRIBUTORS.txt
Code distributed by Google as part of the polymer project is also
subject to an additional IP rights grant found at http://polymer.github.io/PATENTS.txt
*/
var Polymer = {
  Settings: {
    useNativeShadow: true,
    useNativeCSSProperties: true
  },
  ResolveUrl: {
    resolveCss: function(css) {
      return css;
    }
  },
  Base: {
    mixin: function(target, source) {
      for (var i in source) {
        target[i] = source[i];
      }
      return target;
    }
  }
};
module.exports = Polymer;
/*
  Extremely simple css parser. Intended to be not more than what we need
  and definitely not necessarily correct =).
*/
Polymer.CssParse = (function() {

  return {
    // given a string of css, return a simple rule tree
    parse: function(text) {
      text = this._clean(text);
      return this._parseCss(this._lex(text), text);
    },

    // remove stuff we don't care about that may hinder parsing
    _clean: function (cssText) {
      return cssText.replace(this._rx.comments, '').replace(this._rx.port, '');
    },

    // super simple {...} lexer that returns a node tree
    _lex: function(text) {
      var root = {start: 0, end: text.length};
      var n = root;
      for (var i=0, l=text.length; i < l; i++) {
        switch (text[i]) {
          case this.OPEN_BRACE:
            //console.group(i);
            if (!n.rules) {
              n.rules = [];
            }
            var p = n;
            var previous = p.rules[p.rules.length-1];
            n = {start: i+1, parent: p, previous: previous};
            p.rules.push(n);
            break;
          case this.CLOSE_BRACE:
            //console.groupEnd(n.start);
            n.end = i+1;
            n = n.parent || root;
            break;
        }
      }
      return root;
    },

    // add selectors/cssText to node tree
    _parseCss: function(node, text) {
      var t = text.substring(node.start, node.end-1);
      node.parsedCssText = node.cssText = t.trim();
      if (node.parent) {
        var ss = node.previous ? node.previous.end : node.parent.start;
        t = text.substring(ss, node.start-1);
        t = this._expandUnicodeEscapes(t);
        t = t.replace(this._rx.multipleSpaces, ' ');
        // TODO(sorvell): ad hoc; make selector include only after last ;
        // helps with mixin syntax
        t = t.substring(t.lastIndexOf(';')+1);
        var s = node.parsedSelector = node.selector = t.trim();
        node.atRule = (s.indexOf(this.AT_START) === 0);
        // note, support a subset of rule types...
        if (node.atRule) {
          if (s.indexOf(this.MEDIA_START) === 0) {
            node.type = this.types.MEDIA_RULE;
          } else if (s.match(this._rx.keyframesRule)) {
            node.type = this.types.KEYFRAMES_RULE;
            node.keyframesName =
                node.selector.split(this._rx.multipleSpaces).pop();
          }
        } else {
          if (s.indexOf(this.VAR_START) === 0) {
            node.type = this.types.MIXIN_RULE;
          } else {
            node.type = this.types.STYLE_RULE;
          }
        }
      }
      var r$ = node.rules;
      if (r$) {
        for (var i=0, l=r$.length, r; (i<l) && (r=r$[i]); i++) {
          this._parseCss(r, text);
        }
      }
      return node;
    },

    // conversion of sort unicode escapes with spaces like `\33 ` (and longer) into
    // expanded form that doesn't require trailing space `\000033`
    _expandUnicodeEscapes : function(s) {
      return s.replace(/\\([0-9a-f]{1,6})\s/gi, function() {
        var code = arguments[1], repeat = 6 - code.length;
        while (repeat--) {
          code = '0' + code;
        }
        return '\\' + code;
      });
    },

    // stringify parsed css.
    stringify: function(node, preserveProperties, text) {
      text = text || '';
      // calc rule cssText
      var cssText = '';
      if (node.cssText || node.rules) {
        var r$ = node.rules;
        if (r$ && !this._hasMixinRules(r$)) {
          for (var i=0, l=r$.length, r; (i<l) && (r=r$[i]); i++) {
            cssText = this.stringify(r, preserveProperties, cssText);
          }
        } else {
          cssText = preserveProperties ? node.cssText :
            this.removeCustomProps(node.cssText);
          cssText = cssText.trim();
          if (cssText) {
            cssText = '  ' + cssText + '\n';
          }
        }
      }
      // emit rule if there is cssText
      if (cssText) {
        if (node.selector) {
          text += node.selector + ' ' + this.OPEN_BRACE + '\n';
        }
        text += cssText;
        if (node.selector) {
          text += this.CLOSE_BRACE + '\n\n';
        }
      }
      return text;
    },

    _hasMixinRules: function(rules) {
      return rules[0].selector.indexOf(this.VAR_START) === 0;
    },

    removeCustomProps: function(cssText) {
      cssText = this.removeCustomPropAssignment(cssText);
      return this.removeCustomPropApply(cssText);
    },

    removeCustomPropAssignment: function(cssText) {
      return cssText
        .replace(this._rx.customProp, '')
        .replace(this._rx.mixinProp, '');
    },

    removeCustomPropApply: function(cssText) {
      return cssText
        .replace(this._rx.mixinApply, '')
        .replace(this._rx.varApply, '');
    },

    types: {
      STYLE_RULE: 1,
      KEYFRAMES_RULE: 7,
      MEDIA_RULE: 4,
      MIXIN_RULE: 1000
    },

    OPEN_BRACE: '{',
    CLOSE_BRACE: '}',

    // helper regexp's
    _rx: {
      comments: /\/\*[^*]*\*+([^/*][^*]*\*+)*\//gim,
      port: /@import[^;]*;/gim,
      customProp: /(?:^[^;\-\s}]+)?--[^;{}]*?:[^{};]*?(?:[;\n]|$)/gim,
      mixinProp:  /(?:^[^;\-\s}]+)?--[^;{}]*?:[^{};]*?{[^}]*?}(?:[;\n]|$)?/gim,
      mixinApply: /@apply\s*\(?[^);]*\)?\s*(?:[;\n]|$)?/gim,
      varApply: /[^;:]*?:[^;]*?var\([^;]*\)(?:[;\n]|$)?/gim,
      keyframesRule: /^@[^\s]*keyframes/,
      multipleSpaces: /\s+/g
    },

    VAR_START: '--',
    MEDIA_START: '@media',
    AT_START: '@'

  };

})();
Polymer.StyleUtil = (function() {
    var settings = Polymer.Settings;

    return {
      // chrome 49 has semi-working css vars, check if box-shadow works
      // safari 9.1 has a recalc bug: https://bugs.webkit.org/show_bug.cgi?id=155782
      NATIVE_VARIABLES: Polymer.Settings.useNativeCSSProperties,
      MODULE_STYLES_SELECTOR: 'style, link[rel=import][type~=css], template',
      INCLUDE_ATTR: 'include',

      toCssText: function(rules, callback) {
        if (typeof rules === 'string') {
          rules = this.parser.parse(rules);
        }
        if (callback) {
          this.forEachRule(rules, callback);
        }
        return this.parser.stringify(rules, this.NATIVE_VARIABLES);
      },

      forRulesInStyles: function(styles, styleRuleCallback, keyframesRuleCallback) {
        if (styles) {
          for (var i=0, l=styles.length, s; (i<l) && (s=styles[i]); i++) {
            this.forEachRuleInStyle(
                s,
                styleRuleCallback,
                keyframesRuleCallback);
          }
        }
      },

      forActiveRulesInStyles: function(styles, styleRuleCallback, keyframesRuleCallback) {
        if (styles) {
          for (var i=0, l=styles.length, s; (i<l) && (s=styles[i]); i++) {
            this.forEachRuleInStyle(
                s,
                styleRuleCallback,
                keyframesRuleCallback,
                true
              );
          }
        }
      },

      rulesForStyle: function(style) {
        if (!style.__cssRules && style.textContent) {
          style.__cssRules = this.parser.parse(style.textContent);
        }
        return style.__cssRules;
      },

      // Tests if a rule is a keyframes selector, which looks almost exactly
      // like a normal selector but is not (it has nothing to do with scoping
      // for example).
      isKeyframesSelector: function(rule) {
        return rule.parent &&
            rule.parent.type === this.ruleTypes.KEYFRAMES_RULE;
      },

      forEachRuleInStyle: function(style, styleRuleCallback, keyframesRuleCallback, onlyActiveRules) {
        var rules = this.rulesForStyle(style);
        var styleCallback, keyframeCallback;
        if (styleRuleCallback) {
          styleCallback = function(rule) {
            styleRuleCallback(rule, style);
          };
        }
        if (keyframesRuleCallback) {
          keyframeCallback = function(rule) {
            keyframesRuleCallback(rule, style);
          }
        }
        this.forEachRule(
          rules,
          styleCallback,
          keyframeCallback,
          onlyActiveRules
        );
      },

      forEachRule: function(node, styleRuleCallback, keyframesRuleCallback, onlyActiveRules) {
        if (!node) {
          return;
        }
        var skipRules = false;
        if (onlyActiveRules) {
          if (node.type === this.ruleTypes.MEDIA_RULE) {
            var matchMedia = node.selector.match(this.rx.MEDIA_MATCH);
            if (matchMedia) {
              // if rule is a non matching @media rule, skip subrules
              if (!window.matchMedia(matchMedia[1]).matches) {
                skipRules = true;
              }
            }
          }
        }
        if (node.type === this.ruleTypes.STYLE_RULE) {
          styleRuleCallback(node);
        } else if (keyframesRuleCallback &&
                   node.type === this.ruleTypes.KEYFRAMES_RULE) {
          keyframesRuleCallback(node);
        } else if (node.type === this.ruleTypes.MIXIN_RULE) {
          skipRules = true;
        }
        var r$ = node.rules;
        if (r$ && !skipRules) {
          for (var i=0, l=r$.length, r; (i<l) && (r=r$[i]); i++) {
            this.forEachRule(r, styleRuleCallback, keyframesRuleCallback, onlyActiveRules);
          }
        }
      },

      // add a string of cssText to the document.
      applyCss: function(cssText, moniker, target, contextNode) {
        var style = this.createScopeStyle(cssText, moniker);
        return this.applyStyle(style, target, contextNode);
      },

      applyStyle: function(style, target, contextNode) {
        target = target || document.head;
        var after = (contextNode && contextNode.nextSibling) ||
          target.firstChild;
        this.__lastHeadApplyNode = style;
        return target.insertBefore(style, after);
      },

      createScopeStyle: function(cssText, moniker) {
        var style = document.createElement('style');
        if (moniker) {
          style.setAttribute('scope', moniker);
        }
        style.textContent = cssText;
        return style;
      },

      __lastHeadApplyNode: null,

      // insert a comment node as a styling position placeholder.
      applyStylePlaceHolder: function(moniker) {
        var placeHolder = document.createComment(' Shady DOM styles for ' +
          moniker + ' ');
        var after = this.__lastHeadApplyNode ?
          this.__lastHeadApplyNode.nextSibling : null;
        var scope = document.head;
        scope.insertBefore(placeHolder, after || scope.firstChild);
        this.__lastHeadApplyNode = placeHolder;
        return placeHolder;
      },

      cssFromModules: function(moduleIds, warnIfNotFound) {
        var modules = moduleIds.trim().split(' ');
        var cssText = '';
        for (var i=0; i < modules.length; i++) {
          cssText += this.cssFromModule(modules[i], warnIfNotFound);
        }
        return cssText;
      },

      // returns cssText of styles in a given module; also un-applies any
      // styles that apply to the document.
      cssFromModule: function(moduleId, warnIfNotFound) {
        var m = Polymer.DomModule.import(moduleId);
        if (m && !m._cssText) {
          m._cssText = this.cssFromElement(m);
        }
        if (!m && warnIfNotFound) {
          console.warn('Could not find style data in module named', moduleId);
        }
        return m && m._cssText || '';
      },

      // support lots of ways to discover css...
      cssFromElement: function(element) {
        var cssText = '';
        // if element is a template, get content from its .content
        var content = element.content || element;
        var e$ = Polymer.TreeApi.arrayCopy(
          content.querySelectorAll(this.MODULE_STYLES_SELECTOR));
        for (var i=0, e; i < e$.length; i++) {
          e = e$[i];
          // look inside templates for elements
          if (e.localName === 'template') {
            // retain css content when specified,
            if (!e.hasAttribute('preserve-content')) {
              cssText += this.cssFromElement(e);
            }
          } else {
            // style elements inside dom-modules will apply to the main document
            // we don't want this, so we remove them here.
            if (e.localName === 'style') {
              var include = e.getAttribute(this.INCLUDE_ATTR);
              // now support module refs on 'styling' elements
              if (include) {
                cssText += this.cssFromModules(include, true);
              }
              // get style element applied to main doc via HTMLImports polyfill
              e = e.__appliedElement || e;
              e.parentNode.removeChild(e);
              cssText += this.resolveCss(e.textContent, element.ownerDocument);
            // it's an import, assume this is a text file of css content.
            // TODO(sorvell): plan is to deprecate this way to get styles;
            // remember to add deprecation warning when this is done.
            } else if (e.import && e.import.body) {
              cssText += this.resolveCss(e.import.body.textContent, e.import);
            }
          }
        }
        return cssText;
      },

      styleIncludesToTemplate: function(targetTemplate) {
        var styles = targetTemplate.content.querySelectorAll('style[include]');
        for (var i=0, s; i < styles.length; i++) {
          s = styles[i];
          s.parentNode.insertBefore(
            this._includesToFragment(s.getAttribute('include')), s);
        }
      },

      _includesToFragment: function(styleIncludes) {
        var includeArray = styleIncludes.trim().split(' ');
        var frag = document.createDocumentFragment();
        for (var i=0; i < includeArray.length; i++) {
          var t = Polymer.DomModule.import(includeArray[i], 'template');
          if (t) {
            this._addStylesToFragment(frag, t.content);
          }
        }
        return frag;
      },

      _addStylesToFragment: function(frag, source) {
        var s$ = source.querySelectorAll('style');
        for (var i=0, s; i < s$.length; i++) {
          s = s$[i];
          var include = s.getAttribute('include');
          if (include) {
            frag.appendChild(this._includesToFragment(include));
          }
          if (s.textContent) {
            frag.appendChild(s.cloneNode(true));
          }

        }
      },

      isTargetedBuild: function(buildType) {
        return settings.useNativeShadow ? buildType === 'shadow' : buildType === 'shady';
      },

      cssBuildTypeForModule: function (module) {
        var dm = Polymer.DomModule.import(module);
        if (dm) {
          return this.getCssBuildType(dm);
        }
      },

      getCssBuildType: function(element) {
        return element.getAttribute('css-build');
      },

      // Walk from text[start] matching parens
      // returns position of the outer end paren
      _findMatchingParen: function(text, start) {
        var level = 0;
        for (var i=start, l=text.length; i < l; i++) {
          switch (text[i]) {
            case '(':
              level++;
              break;
            case ')':
              if (--level === 0) {
                return i;
              }
              break;
          }
        }
        return -1;
      },

      processVariableAndFallback: function(str, callback) {
        // find 'var('
        var start = str.indexOf('var(');
        if (start === -1) {
          // no var?, everything is prefix
          return callback(str, '', '', '');
        }
        //${prefix}var(${inner})${suffix}
        var end = this._findMatchingParen(str, start + 3);
        var inner = str.substring(start + 4, end);
        var prefix = str.substring(0, start);
        // suffix may have other variables
        var suffix = this.processVariableAndFallback(str.substring(end + 1), callback);
        var comma = inner.indexOf(',');
        // value and fallback args should be trimmed to match in property lookup
        if (comma === -1) {
          // variable, no fallback
          return callback(prefix, inner.trim(), '', suffix);
        }
        // var(${value},${fallback})
        var value = inner.substring(0, comma).trim();
        var fallback = inner.substring(comma + 1).trim();
        return callback(prefix, value, fallback, suffix);
      },

      rx: {
        VAR_ASSIGN: /(?:^|[;\s{]\s*)(--[\w-]*?)\s*:\s*(?:([^;{]*)|{([^}]*)})(?:(?=[;\s}])|$)/gi,
        MIXIN_MATCH: /(?:^|\W+)@apply\s*\(?([^);\n]*)\)?/gi,
        VAR_CONSUMED: /(--[\w-]+)\s*([:,;)]|$)/gi,
        ANIMATION_MATCH: /(animation\s*:)|(animation-name\s*:)/,
        MEDIA_MATCH: /@media[^(]*(\([^)]*\))/,
        IS_VAR: /^--/,
        BRACKETED: /\{[^}]*\}/g,
        HOST_PREFIX: '(?:^|[^.#[:])',
        HOST_SUFFIX: '($|[.:[\\s>+~])'
      },

      resolveCss: Polymer.ResolveUrl.resolveCss,
      parser: Polymer.CssParse,
      ruleTypes: Polymer.CssParse.types

    };

  })();
Polymer.StyleTransformer = (function() {

    var styleUtil = Polymer.StyleUtil;
    var settings = Polymer.Settings;

    /* Transforms ShadowDOM styling into ShadyDOM styling

     * scoping:

        * elements in scope get scoping selector class="x-foo-scope"
        * selectors re-written as follows:

          div button -> div.x-foo-scope button.x-foo-scope

     * :host -> scopeName

     * :host(...) -> scopeName...

     * ::content -> ' '

     * ::shadow, /deep/: processed similar to ::content

     * :host-context(...): scopeName..., ... scopeName

    */
    var api = {

      // Given a node and scope name, add a scoping class to each node
      // in the tree. This facilitates transforming css into scoped rules.
      dom: function(node, scope, useAttr, shouldRemoveScope) {
        this._transformDom(node, scope || '', useAttr, shouldRemoveScope);
      },

      _transformDom: function(node, selector, useAttr, shouldRemoveScope) {
        if (node.setAttribute) {
          this.element(node, selector, useAttr, shouldRemoveScope);
        }
        var c$ = Polymer.dom(node).childNodes;
        for (var i=0; i<c$.length; i++) {
          this._transformDom(c$[i], selector, useAttr, shouldRemoveScope);
        }
      },

      element: function(element, scope, useAttr, shouldRemoveScope) {
        if (useAttr) {
          if (shouldRemoveScope) {
            element.removeAttribute(SCOPE_NAME);
          } else {
            element.setAttribute(SCOPE_NAME, scope);
          }
        } else {
          // note: if using classes, we add both the general 'style-scope' class
          // as well as the specific scope. This enables easy filtering of all
          // `style-scope` elements
          if (scope) {
            // note: svg on IE does not have classList so fallback to class
            if (element.classList) {
              if (shouldRemoveScope) {
                element.classList.remove(SCOPE_NAME);
                element.classList.remove(scope);
              } else {
                element.classList.add(SCOPE_NAME);
                element.classList.add(scope);
              }
            } else if (element.getAttribute) {
              var c = element.getAttribute(CLASS);
              if (shouldRemoveScope) {
                if (c) {
                  element.setAttribute(CLASS, c.replace(SCOPE_NAME, '')
                    .replace(scope, ''));
                }
              } else {
                element.setAttribute(CLASS, (c ? c + ' ' : '') +
                  SCOPE_NAME + ' ' + scope);
              }
            }
          }
        }
      },

      elementStyles: function(element, callback) {
        var styles = element._styles;
        var cssText = '';
        var cssBuildType = element.__cssBuild;
        var passthrough = settings.useNativeShadow || cssBuildType === 'shady';
        var cb;
        // use the style node visitor callback to update the selector
        if (passthrough) {
          var self = this;
          cb = function(rule) {
            rule.selector = self._slottedToContent(rule.selector);
            rule.selector = rule.selector.replace(ROOT, ':host > *');
            if (callback) {
              callback(rule);
            }
          }
        }
        for (var i=0, l=styles.length, s; (i<l) && (s=styles[i]); i++) {
          var rules = styleUtil.rulesForStyle(s);
          // no need to shim selectors if settings.useNativeShadow, also
          // a shady css build will already have transformed selectors
          // NOTE: This method may be called as part of static or property shimming.
          // When there is a targeted build it will not be called for static shimming,
          // but when the property shim is used it is called and should opt out of
          // static shimming work when a proper build exists.
          cssText += (passthrough) ?
            styleUtil.toCssText(rules, cb) :
            this.css(rules, element.is, element.extends, callback,
            element._scopeCssViaAttr) + '\n\n';
        }
        return cssText.trim();
      },

      // Given a string of cssText and a scoping string (scope), returns
      // a string of scoped css where each selector is transformed to include
      // a class created from the scope. ShadowDOM selectors are also transformed
      // (e.g. :host) to use the scoping selector.
      css: function(rules, scope, ext, callback, useAttr) {
        var hostScope = this._calcHostScope(scope, ext);
        scope = this._calcElementScope(scope, useAttr);
        var self = this;
        return styleUtil.toCssText(rules, function(rule) {
          if (!rule.isScoped) {
            self.rule(rule, scope, hostScope);
            rule.isScoped = true;
          }
          if (callback) {
            callback(rule, scope, hostScope);
          }
        });
      },

      _calcElementScope: function (scope, useAttr) {
        if (scope) {
          return useAttr ?
            CSS_ATTR_PREFIX + scope + CSS_ATTR_SUFFIX :
            CSS_CLASS_PREFIX + scope;
        } else {
          return '';
        }
      },

      _calcHostScope: function(scope, ext) {
        return ext ? '[is=' +  scope + ']' : scope;
      },

      rule: function (rule, scope, hostScope) {
        this._transformRule(rule, this._transformComplexSelector,
          scope, hostScope);
      },

      // transforms a css rule to a scoped rule.
      _transformRule: function(rule, transformer, scope, hostScope) {
        // NOTE: save transformedSelector for subsequent matching of elements
        // against selectors (e.g. when calculating style properties)
        rule.selector = rule.transformedSelector =
          this._transformRuleCss(rule, transformer, scope, hostScope);
      },

      _transformRuleCss: function(rule, transformer, scope, hostScope) {
        var p$ = rule.selector.split(COMPLEX_SELECTOR_SEP);
        // we want to skip transformation of rules that appear in keyframes,
        // because they are keyframe selectors, not element selectors.
        if (!styleUtil.isKeyframesSelector(rule)) {
          for (var i=0, l=p$.length, p; (i<l) && (p=p$[i]); i++) {
            p$[i] = transformer.call(this, p, scope, hostScope);
          }
        }
        return p$.join(COMPLEX_SELECTOR_SEP);
      },

      _transformComplexSelector: function(selector, scope, hostScope) {
        var stop = false;
        var hostContext = false;
        var self = this;
        selector = selector.trim();
        selector = this._slottedToContent(selector);
        selector = selector.replace(ROOT, ':host > *');
        selector = selector.replace(CONTENT_START, HOST + ' $1');
        selector = selector.replace(SIMPLE_SELECTOR_SEP, function(m, c, s) {
          if (!stop) {
            var info = self._transformCompoundSelector(s, c, scope, hostScope);
            stop = stop || info.stop;
            hostContext = hostContext || info.hostContext;
            c = info.combinator;
            s = info.value;
          } else {
            s = s.replace(SCOPE_JUMP, ' ');
          }
          return c + s;
        });
        if (hostContext) {
          selector = selector.replace(HOST_CONTEXT_PAREN,
            function(m, pre, paren, post) {
              return pre + paren + ' ' + hostScope + post +
                COMPLEX_SELECTOR_SEP + ' ' + pre + hostScope + paren + post;
             });
        }
        return selector;
      },

      _transformCompoundSelector: function(selector, combinator, scope, hostScope) {
        // replace :host with host scoping class
        var jumpIndex = selector.search(SCOPE_JUMP);
        var hostContext = false;
        if (selector.indexOf(HOST_CONTEXT) >=0) {
          hostContext = true;
        } else if (selector.indexOf(HOST) >=0) {
          selector = this._transformHostSelector(selector, hostScope);
        // replace other selectors with scoping class
        } else if (jumpIndex !== 0) {
          selector = scope ? this._transformSimpleSelector(selector, scope) :
            selector;
        }
        // remove left-side combinator when dealing with ::content.
        if (selector.indexOf(CONTENT) >= 0) {
          combinator = '';
        }
        // process scope jumping selectors up to the scope jump and then stop
        // e.g. .zonk ::content > .foo ==> .zonk.scope > .foo
        var stop;
        if (jumpIndex >= 0) {
          selector = selector.replace(SCOPE_JUMP, ' ');
          stop = true;
        }
        return {value: selector, combinator: combinator, stop: stop,
          hostContext: hostContext};
      },

      _transformSimpleSelector: function(selector, scope) {
        var p$ = selector.split(PSEUDO_PREFIX);
        p$[0] += scope;
        return p$.join(PSEUDO_PREFIX);
      },

      // :host(...) -> scopeName...
      _transformHostSelector: function(selector, hostScope) {
        var m = selector.match(HOST_PAREN);
        var paren = m && m[2].trim() || '';
        if (paren) {
          if (!paren[0].match(SIMPLE_SELECTOR_PREFIX)) {
            // paren starts with a type selector
            var typeSelector = paren.split(SIMPLE_SELECTOR_PREFIX)[0];
            // if the type selector is our hostScope then avoid pre-pending it
            if (typeSelector === hostScope) {
              return paren;
            // otherwise, this selector should not match in this scope so
            // output a bogus selector.
            } else {
              return SELECTOR_NO_MATCH;
            }
          } else {
            // make sure to do a replace here to catch selectors like:
            // `:host(.foo)::before`
            return selector.replace(HOST_PAREN, function(m, host, paren) {
              return hostScope + paren;
            });
          }
        // if no paren, do a straight :host replacement.
        // TODO(sorvell): this should not strictly be necessary but
        // it's needed to maintain support for `:host[foo]` type selectors
        // which have been improperly used under Shady DOM. This should be
        // deprecated.
        } else {
          return selector.replace(HOST, hostScope);
        }
      },

      documentRule: function(rule) {
        // reset selector in case this is redone.
        rule.selector = rule.parsedSelector;
        this.normalizeRootSelector(rule);
        if (!settings.useNativeShadow) {
          this._transformRule(rule, this._transformDocumentSelector);
        }
      },

      normalizeRootSelector: function(rule) {
        rule.selector = rule.selector.replace(ROOT, 'html');
      },

      _transformDocumentSelector: function(selector) {
        return selector.match(SCOPE_JUMP) ?
          this._transformComplexSelector(selector, SCOPE_DOC_SELECTOR) :
          this._transformSimpleSelector(selector.trim(), SCOPE_DOC_SELECTOR);
      },

      // For forward compatibility with ShadowDOM v1 and Polymer 2.x,
      // replace ::slotted(${inner}) with ::content > ${inner}
      _slottedToContent: function(cssText) {
        return cssText.replace(SLOTTED_PAREN, CONTENT + '> $1');
      },

      SCOPE_NAME: 'style-scope'
    };

    var SCOPE_NAME = api.SCOPE_NAME;
    var SCOPE_DOC_SELECTOR = ':not([' + SCOPE_NAME + '])' +
      ':not(.' + SCOPE_NAME + ')';
    var COMPLEX_SELECTOR_SEP = ',';
    var SIMPLE_SELECTOR_SEP = /(^|[\s>+~]+)((?:\[.+?\]|[^\s>+~=\[])+)/g;
    var SIMPLE_SELECTOR_PREFIX = /[[.:#*]/;
    var HOST = ':host';
    var ROOT = ':root';
    // NOTE: this supports 1 nested () pair for things like
    // :host(:not([selected]), more general support requires
    // parsing which seems like overkill
    var HOST_PAREN = /(:host)(?:\(((?:\([^)(]*\)|[^)(]*)+?)\))/;
    var HOST_CONTEXT = ':host-context';
    var HOST_CONTEXT_PAREN = /(.*)(?::host-context)(?:\(((?:\([^)(]*\)|[^)(]*)+?)\))(.*)/;
    var CONTENT = '::content';
    var SCOPE_JUMP = /::content|::shadow|\/deep\//;
    var CSS_CLASS_PREFIX = '.';
    var CSS_ATTR_PREFIX = '[' + SCOPE_NAME + '~=';
    var CSS_ATTR_SUFFIX = ']';
    var PSEUDO_PREFIX = ':';
    var CLASS = 'class';
    var CONTENT_START = new RegExp('^(' + CONTENT + ')');
    var SELECTOR_NO_MATCH = 'should_not_match';
    var SLOTTED_PAREN = /(?:::slotted)(?:\(((?:\([^)(]*\)|[^)(]*)+?)\))/g;

    // exports
    return api;

  })();
/**
 * The apply shim simulates the behavior of `@apply` proposed at
 * https://tabatkins.github.io/specs/css-apply-rule/.
 * The approach is to convert a property like this:
 *
 *    --foo: {color: red; background: blue;}
 *
 * to this:
 *
 *    --foo_-_color: red;
 *    --foo_-_background: blue;
 *
 * Then where `@apply --foo` is used, that is converted to:
 *
 *    color: var(--foo_-_color);
 *    background: var(--foo_-_background);
 *
 * This approach generally works but there are some issues and limitations.
 * Consider, for example, that somewhere *between* where `--foo` is set and used,
 * another element sets it to:
 *
 *    --foo: { border: 2px solid red; }
 *
 * We must now ensure that the color and background from the previous setting
 * do not apply. This is accomplished by changing the property set to this:
 *
 *    --foo_-_border: 2px solid red;
 *    --foo_-_color: initial;
 *    --foo_-_background: initial;
 *
 * This works but introduces one new issue.
 * Consider this setup at the point where the `@apply` is used:
 *
 *    background: orange;
 *    @apply --foo;
 *
 * In this case the background will be unset (initial) rather than the desired
 * `orange`. We address this by altering the property set to use a fallback
 * value like this:
 *
 *    color: var(--foo_-_color);
 *    background: var(--foo_-_background, orange);
 *    border: var(--foo_-_border);
 *
 * Note that the default is retained in the property set and the `background` is
 * the desired `orange`. This leads us to a limitation.
 *
 * Limitation 1:

 * Only properties in the rule where the `@apply`
 * is used are considered as default values.
 * If another rule matches the element and sets `background` with
 * less specificity than the rule in which `@apply` appears,
 * the `background` will not be set.
 *
 * Limitation 2:
 *
 * When using Polymer's `updateStyles` api, new properties may not be set for
 * `@apply` properties.

*/
Polymer.ApplyShim = (function(){
  'use strict';

  var styleUtil = Polymer.StyleUtil;

  var MIXIN_MATCH = styleUtil.rx.MIXIN_MATCH;
  var VAR_ASSIGN = styleUtil.rx.VAR_ASSIGN;
  // match var(--a, --b) to make var(--a, var(--b));
  var BAD_VAR = /var\(\s*(--[^,]*),\s*(--[^)]*)\)/g;
  var APPLY_NAME_CLEAN = /;\s*/m;
  var INITIAL_INHERIT = /^\s*(initial)|(inherit)\s*$/;

  // separator used between mixin-name and mixin-property-name when producing properties
  // NOTE: plain '-' may cause collisions in user styles
  var MIXIN_VAR_SEP = '_-_';

  // map of mixin to property names
  // --foo: {border: 2px} -> {properties: {(--foo, ['border'])}, dependants: {'element-name': proto}}
  var mixinMap = {};

  function mapSet(name, props) {
    name = name.trim();
    mixinMap[name] = {
      properties: props,
      dependants: {}
    };
  }

  function mapGet(name) {
    name = name.trim();
    return mixinMap[name];
  }

  function replaceInitialOrInherit(property, value) {
    var match = INITIAL_INHERIT.exec(value);
    if (match) {
      if (match[1]) {
        // initial
        // replace `initial` with the concrete initial value for this property
        value = ApplyShim._getInitialValueForProperty(property);
      } else {
        // inherit
        // with this purposfully illegal value, the variable will be invalid at
        // compute time (https://www.w3.org/TR/css-variables/#invalid-at-computed-value-time)
        // and for inheriting values, will behave similarly
        // we cannot support the same behavior for non inheriting values like 'border'
        value = 'apply-shim-inherit';
      }
    }
    return value;
  }

  // "parse" a mixin definition into a map of properties and values
  // cssTextToMap('border: 2px solid black') -> ('border', '2px solid black')
  function cssTextToMap(text) {
    var props = text.split(';');
    var property, value;
    var out = {};
    for (var i = 0, p, sp; i < props.length; i++) {
      p = props[i];
      if (p) {
        sp = p.split(':');
        // ignore lines that aren't definitions like @media
        if (sp.length > 1) {
          property = sp[0].trim();
          // some properties may have ':' in the value, like data urls
          value = replaceInitialOrInherit(property, sp.slice(1).join(':'));
          out[property] = value;
        }
      }
    }
    return out;
  }

  function invalidateMixinEntry(mixinEntry) {
    var currentProto = ApplyShim.__currentElementProto;
    var currentElementName = currentProto && currentProto.is;
    for (var elementName in mixinEntry.dependants) {
      if (elementName !== currentElementName) {
        mixinEntry.dependants[elementName].__applyShimInvalid = true;
      }
    }
  }

  function produceCssProperties(matchText, propertyName, valueProperty, valueMixin) {
    // handle case where property value is a mixin
    if (valueProperty) {
      // form: --mixin2: var(--mixin1), where --mixin1 is in the map
      styleUtil.processVariableAndFallback(valueProperty, function(prefix, value) {
        if (value && mapGet(value)) {
          valueMixin = '@apply ' + value + ';';
        }
      });
    }
    if (!valueMixin) {
      return matchText;
    }
    var mixinAsProperties = consumeCssProperties(valueMixin);
    var prefix = matchText.slice(0, matchText.indexOf('--'));
    var mixinValues = cssTextToMap(mixinAsProperties);
    var combinedProps = mixinValues;
    var mixinEntry = mapGet(propertyName);
    var oldProps = mixinEntry && mixinEntry.properties;
    if (oldProps) {
      // NOTE: since we use mixin, the map of properties is updated here
      // and this is what we want.
      combinedProps = Object.create(oldProps);
      combinedProps = Polymer.Base.mixin(combinedProps, mixinValues);
    } else {
      mapSet(propertyName, combinedProps);
    }
    var out = [];
    var p, v;
    // set variables defined by current mixin
    var needToInvalidate = false;
    for (p in combinedProps) {
      v = mixinValues[p];
      // if property not defined by current mixin, set initial
      if (v === undefined) {
        v = 'initial';
      }
      if (oldProps && !(p in oldProps)) {
        needToInvalidate = true;
      }
      out.push(propertyName + MIXIN_VAR_SEP + p + ': ' + v);
    }
    if (needToInvalidate) {
      invalidateMixinEntry(mixinEntry);
    }
    if (mixinEntry) {
      mixinEntry.properties = combinedProps;
    }
    // because the mixinMap is global, the mixin might conflict with
    // a different scope's simple variable definition:
    // Example:
    // some style somewhere:
    // --mixin1:{ ... }
    // --mixin2: var(--mixin1);
    // some other element:
    // --mixin1: 10px solid red;
    // --foo: var(--mixin1);
    // In this case, we leave the original variable definition in place.
    if (valueProperty) {
      prefix = matchText + ';' + prefix;
    }
    return prefix + out.join('; ') + ';';
  }

  // fix shim'd var syntax
  // var(--a, --b) -> var(--a,var(--b))
  function fixVars(matchText, varA, varB) {
    // if fallback doesn't exist, or isn't a broken variable, abort
    return 'var(' + varA + ',' + 'var(' + varB + '))';
  }

  // produce variable consumption at the site of mixin consumption
  // @apply --foo; -> for all props (${propname}: var(--foo_-_${propname}, ${fallback[propname]}}))
  // Example:
  // border: var(--foo_-_border); padding: var(--foo_-_padding, 2px)
  function atApplyToCssProperties(mixinName, fallbacks) {
    mixinName = mixinName.replace(APPLY_NAME_CLEAN, '');
    var vars = [];
    var mixinEntry = mapGet(mixinName);
    // if we depend on a mixin before it is created
    // make a sentinel entry in the map to add this element as a dependency for when it is defined.
    if (!mixinEntry) {
      mapSet(mixinName, {});
      mixinEntry = mapGet(mixinName);
    }
    if (mixinEntry) {
      var currentProto = ApplyShim.__currentElementProto;
      if (currentProto) {
        mixinEntry.dependants[currentProto.is] = currentProto;
      }
      var p, parts, f;
      for (p in mixinEntry.properties) {
        f = fallbacks && fallbacks[p];
        parts = [p, ': var(', mixinName, MIXIN_VAR_SEP, p];
        if (f) {
          parts.push(',', f);
        }
        parts.push(')');
        vars.push(parts.join(''));
      }
    }
    return vars.join('; ');
  }

  // replace mixin consumption with variable consumption
  function consumeCssProperties(text) {
    var m;
    // loop over text until all mixins with defintions have been applied
    while((m = MIXIN_MATCH.exec(text))) {
      var matchText = m[0];
      var mixinName = m[1];
      var idx = m.index;
      // collect properties before apply to be "defaults" if mixin might override them
      // match includes a "prefix", so find the start and end positions of @apply
      var applyPos = idx + matchText.indexOf('@apply');
      var afterApplyPos = idx + matchText.length;
      // find props defined before this @apply
      var textBeforeApply = text.slice(0, applyPos);
      var textAfterApply = text.slice(afterApplyPos);
      var defaults = cssTextToMap(textBeforeApply);
      var replacement = atApplyToCssProperties(mixinName, defaults);
      // use regex match position to replace mixin, keep linear processing time
      text = [textBeforeApply, replacement, textAfterApply].join('');
      // move regex search to _after_ replacement
      MIXIN_MATCH.lastIndex = idx + replacement.length;
    }
    return text;
  }

  var ApplyShim = {
    _measureElement: null,
    _map: mixinMap,
    _separator: MIXIN_VAR_SEP,
    transform: function(styles, elementProto) {
      this.__currentElementProto = elementProto;
      styleUtil.forRulesInStyles(styles, this._boundFindDefinitions);
      styleUtil.forRulesInStyles(styles, this._boundFindApplications)
      if (elementProto) {
        elementProto.__applyShimInvalid = false;
      }
      this.__currentElementProto = null;
    },
    _findDefinitions: function(rule) {
      var cssText = rule.parsedCssText;
      // fix shim variables
      cssText = cssText.replace(BAD_VAR, fixVars);
      // produce variables
      cssText = cssText.replace(VAR_ASSIGN, produceCssProperties);
      rule.cssText = cssText;
      // :root was only used for variable assignment in property shim,
      // but generates invalid selectors with real properties.
      // replace with `:host > *`, which serves the same effect
      if (rule.selector === ':root') {
        rule.selector = ':host > *';
      }
    },
    _findApplications: function(rule) {
      // consume mixins
      rule.cssText = consumeCssProperties(rule.cssText);
    },
    transformRule: function(rule) {
      this._findDefinitions(rule);
      this._findApplications(rule);
    },
    _getInitialValueForProperty: function(property) {
      if (!this._measureElement) {
        this._measureElement = document.createElement('meta');
        this._measureElement.style.all = 'initial';
        document.head.appendChild(this._measureElement);
      }
      return window.getComputedStyle(this._measureElement).getPropertyValue(property);
    }
  };

  ApplyShim._boundTransformRule = ApplyShim.transformRule.bind(ApplyShim);
  ApplyShim._boundFindDefinitions = ApplyShim._findDefinitions.bind(ApplyShim);
  ApplyShim._boundFindApplications = ApplyShim._findApplications.bind(ApplyShim);
  return ApplyShim;
})();