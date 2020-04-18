/*
 * Copyright 2008 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @fileoverview
 * Utility functions and classes for Soy gencode
 *
 * <p>
 * This file contains utilities that should only be called by Soy-generated
 * JS code. Please do not use these functions directly from
 * your hand-written code. Their names all start with '$$', or exist within the
 * soydata.VERY_UNSAFE namespace.
 *
 * <p>TODO(lukes): ensure that the above pattern is actually followed
 * consistently.
 *
 * @author Garrett Boyer
 * @author Mike Samuel
 * @author Kai Huang
 * @author Aharon Lanin
 */
goog.provide('soy');
goog.provide('soy.asserts');
goog.provide('soy.esc');
goog.provide('soydata');
goog.provide('soydata.SanitizedHtml');
goog.provide('soydata.VERY_UNSAFE');

goog.require('goog.array');
goog.require('goog.asserts');
goog.require('goog.debug');
goog.require('goog.format');
goog.require('goog.html.SafeHtml');
goog.require('goog.html.SafeScript');
goog.require('goog.html.SafeStyle');
goog.require('goog.html.SafeStyleSheet');
goog.require('goog.html.SafeUrl');
goog.require('goog.html.TrustedResourceUrl');
goog.require('goog.html.uncheckedconversions');
goog.require('goog.i18n.BidiFormatter');
goog.require('goog.i18n.bidi');
goog.require('goog.object');
goog.require('goog.soy.data.SanitizedContent');
goog.require('goog.soy.data.SanitizedContentKind');
goog.require('goog.soy.data.SanitizedCss');
goog.require('goog.soy.data.SanitizedHtml');
goog.require('goog.soy.data.SanitizedHtmlAttribute');
goog.require('goog.soy.data.SanitizedJs');
goog.require('goog.soy.data.SanitizedStyle');
goog.require('goog.soy.data.SanitizedTrustedResourceUri');
goog.require('goog.soy.data.SanitizedUri');
goog.require('goog.soy.data.UnsanitizedText');
goog.require('goog.string');
goog.require('goog.string.Const');

// -----------------------------------------------------------------------------
// soydata: Defines typed strings, e.g. an HTML string {@code "a<b>c"} is
// semantically distinct from the plain text string {@code "a<b>c"} and smart
// templates can take that distinction into account.

/**
 * Checks whether a given value is of a given content kind.
 *
 * @param {*} value The value to be examined.
 * @param {goog.soy.data.SanitizedContentKind} contentKind The desired content
 *     kind.
 * @return {boolean} Whether the given value is of the given kind.
 * @private
 */
soydata.isContentKind_ = function(value, contentKind) {
  // TODO(aharon): This function should really include the assert on
  // value.constructor that is currently sprinkled at most of the call sites.
  // Unfortunately, that would require a (debug-mode-only) switch statement.
  // TODO(aharon): Perhaps we should get rid of the contentKind property
  // altogether and only at the constructor.
  return value != null && value.contentKind === contentKind;
};


/**
 * Returns a given value's contentDir property, constrained to a
 * goog.i18n.bidi.Dir value or null. Returns null if the value is null,
 * undefined, a primitive or does not have a contentDir property, or the
 * property's value is not 1 (for LTR), -1 (for RTL), or 0 (for neutral).
 *
 * @param {*} value The value whose contentDir property, if any, is to
 *     be returned.
 * @return {?goog.i18n.bidi.Dir} The contentDir property.
 */
soydata.getContentDir = function(value) {
  if (value != null) {
    switch (value.contentDir) {
      case goog.i18n.bidi.Dir.LTR:
        return goog.i18n.bidi.Dir.LTR;
      case goog.i18n.bidi.Dir.RTL:
        return goog.i18n.bidi.Dir.RTL;
      case goog.i18n.bidi.Dir.NEUTRAL:
        return goog.i18n.bidi.Dir.NEUTRAL;
    }
  }
  return null;
};


/**
 * This class is only a holder for {@code soydata.SanitizedHtml.from}. Do not
 * instantiate or extend it. Use {@code goog.soy.data.SanitizedHtml} instead.
 *
 * @constructor
 * @extends {goog.soy.data.SanitizedHtml}
 * @abstract
 */
soydata.SanitizedHtml = function() {
  soydata.SanitizedHtml.base(this, 'constructor');  // Throws an exception.
};
goog.inherits(soydata.SanitizedHtml, goog.soy.data.SanitizedHtml);

/**
 * Returns a SanitizedHtml object for a particular value. The content direction
 * is preserved.
 *
 * This HTML-escapes the value unless it is already SanitizedHtml or SafeHtml.
 *
 * @param {*} value The value to convert. If it is already a SanitizedHtml
 *     object, it is left alone.
 * @return {!goog.soy.data.SanitizedHtml} A SanitizedHtml object derived from
 *     the stringified value. It is escaped unless the input is SanitizedHtml or
 *     SafeHtml.
 */
soydata.SanitizedHtml.from = function(value) {
  // The check is soydata.isContentKind_() inlined for performance.
  if (value != null &&
      value.contentKind === goog.soy.data.SanitizedContentKind.HTML) {
    goog.asserts.assert(value.constructor === goog.soy.data.SanitizedHtml);
    return /** @type {!goog.soy.data.SanitizedHtml} */ (value);
  }
  if (value instanceof goog.html.SafeHtml) {
    return soydata.VERY_UNSAFE.ordainSanitizedHtml(
        goog.html.SafeHtml.unwrap(value), value.getDirection());
  }
  return soydata.VERY_UNSAFE.ordainSanitizedHtml(
      soy.esc.$$escapeHtmlHelper(String(value)), soydata.getContentDir(value));
};


/**
 * Empty string, used as a type in Soy templates.
 * @enum {string}
 * @private
 */
soydata.$$EMPTY_STRING_ = {
  VALUE: ''
};


/**
 * Creates a factory for SanitizedContent types.
 *
 * This is a hack so that the soydata.VERY_UNSAFE.ordainSanitized* can
 * instantiate Sanitized* classes, without making the Sanitized* constructors
 * publicly usable. Requiring all construction to use the VERY_UNSAFE names
 * helps callers and their reviewers easily tell that creating SanitizedContent
 * is not always safe and calls for careful review.
 *
 * @param {function(new: T)} ctor A constructor.
 * @return {!function(*, ?goog.i18n.bidi.Dir=): T} A factory that takes
 *     content and an optional content direction and returns a new instance. If
 *     the content direction is undefined, ctor.prototype.contentDir is used.
 * @template T
 * @private
 */
soydata.$$makeSanitizedContentFactory_ = function(ctor) {
  /**
   * @param {string} content
   * @constructor
   * @extends {goog.soy.data.SanitizedContent}
   */
  function InstantiableCtor(content) {
    /** @override */
    this.content = content;
  }
  InstantiableCtor.prototype = ctor.prototype;
  /**
   * Creates a ctor-type SanitizedContent instance.
   *
   * @param {*} content The content to put in the instance.
   * @param {?goog.i18n.bidi.Dir=} opt_contentDir The content direction. If
   *     undefined, ctor.prototype.contentDir is used.
   * @return {!goog.soy.data.SanitizedContent} The new instance. It is actually
   *     of type T above (ctor's type, a descendant of SanitizedContent), but
   *     there is no way to express that here.
   */
  function sanitizedContentFactory(content, opt_contentDir) {
    var result = new InstantiableCtor(String(content));
    if (opt_contentDir !== undefined) {
      result.contentDir = opt_contentDir;
    }
    return result;
  }
  return sanitizedContentFactory;
};


/**
 * Creates a factory for SanitizedContent types that should always have their
 * default directionality.
 *
 * This is a hack so that the soydata.VERY_UNSAFE.ordainSanitized* can
 * instantiate Sanitized* classes, without making the Sanitized* constructors
 * publicly usable. Requiring all construction to use the VERY_UNSAFE names
 * helps callers and their reviewers easily tell that creating SanitizedContent
 * is not always safe and calls for careful review.
 *
 * @param {function(new: T, string)} ctor A constructor.
 * @return {!function(*): T} A factory that takes content and returns a new
 *     instance (with default directionality, i.e. ctor.prototype.contentDir).
 * @template T
 * @private
 */
soydata.$$makeSanitizedContentFactoryWithDefaultDirOnly_ = function(ctor) {
  /**
   * @param {string} content
   * @constructor
   * @extends {goog.soy.data.SanitizedContent}
   */
  function InstantiableCtor(content) {
    /** @override */
    this.content = content;
  }
  InstantiableCtor.prototype = ctor.prototype;
  /**
   * Creates a ctor-type SanitizedContent instance.
   *
   * @param {*} content The content to put in the instance.
   * @return {!goog.soy.data.SanitizedContent} The new instance. It is actually
   *     of type T above (ctor's type, a descendant of SanitizedContent), but
   *     there is no way to express that here.
   */
  function sanitizedContentFactory(content) {
    var result = new InstantiableCtor(String(content));
    return result;
  }
  return sanitizedContentFactory;
};


// -----------------------------------------------------------------------------
// Sanitized content ordainers. Please use these with extreme caution (with the
// exception of markUnsanitizedText). A good recommendation is to limit usage
// of these to just a handful of files in your source tree where usages can be
// carefully audited.


/**
 * Protects a string from being used in an noAutoescaped context.
 *
 * This is useful for content where there is significant risk of accidental
 * unescaped usage in a Soy template. A great case is for user-controlled
 * data that has historically been a source of vulernabilities.
 *
 * @param {*} content Text to protect.
 * @param {?goog.i18n.bidi.Dir=} opt_contentDir The content direction; null if
 *     unknown and thus to be estimated when necessary. Default: null.
 * @return {!goog.soy.data.UnsanitizedText} A wrapper that is rejected by the
 *     Soy noAutoescape print directive.
 */
soydata.markUnsanitizedText = function(content, opt_contentDir) {
  return new goog.soy.data.UnsanitizedText(content, opt_contentDir);
};


/**
 * Takes a leap of faith that the provided content is "safe" HTML.
 *
 * @param {*} content A string of HTML that can safely be embedded in
 *     a PCDATA context in your app. If you would be surprised to find that an
 *     HTML sanitizer produced {@code s} (e.g. it runs code or fetches bad URLs)
 *     and you wouldn't write a template that produces {@code s} on security or
 *     privacy grounds, then don't pass {@code s} here.
 * @param {?goog.i18n.bidi.Dir=} opt_contentDir The content direction; null if
 *     unknown and thus to be estimated when necessary. Default: null.
 * @return {!goog.soy.data.SanitizedHtml} Sanitized content wrapper that
 *     indicates to Soy not to escape when printed as HTML.
 */
soydata.VERY_UNSAFE.ordainSanitizedHtml =
    soydata.$$makeSanitizedContentFactory_(goog.soy.data.SanitizedHtml);


/**
 * Takes a leap of faith that the provided content is "safe" (non-attacker-
 * controlled, XSS-free) Javascript.
 *
 * @param {*} content Javascript source that when evaluated does not
 *     execute any attacker-controlled scripts.
 * @return {!goog.soy.data.SanitizedJs} Sanitized content wrapper that indicates
 *     to Soy not to escape when printed as Javascript source.
 */
soydata.VERY_UNSAFE.ordainSanitizedJs =
    soydata.$$makeSanitizedContentFactoryWithDefaultDirOnly_(
        goog.soy.data.SanitizedJs);


/**
 * Takes a leap of faith that the provided content is "safe" to use as a URI
 * in a Soy template.
 *
 * This creates a Soy SanitizedContent object which indicates to Soy there is
 * no need to escape it when printed as a URI (e.g. in an href or src
 * attribute), such as if it's already been encoded or  if it's a Javascript:
 * URI.
 *
 * @param {*} content A chunk of URI that the caller knows is safe to
 *     emit in a template.
 * @return {!goog.soy.data.SanitizedUri} Sanitized content wrapper that
 *     indicates to Soy not to escape or filter when printed in URI context.
 */
soydata.VERY_UNSAFE.ordainSanitizedUri =
    soydata.$$makeSanitizedContentFactoryWithDefaultDirOnly_(
        goog.soy.data.SanitizedUri);


/**
 * Takes a leap of faith that the provided content is "safe" to use as a
 * TrustedResourceUri in a Soy template.
 *
 * This creates a Soy SanitizedContent object which indicates to Soy there is
 * no need to filter it when printed as a TrustedResourceUri.
 *
 * @param {*} content A chunk of TrustedResourceUri such as that the caller
 *     knows is safe to emit in a template.
 * @return {!goog.soy.data.SanitizedTrustedResourceUri} Sanitized content
 *     wrapper that indicates to Soy not to escape or filter when printed in
 *     TrustedResourceUri context.
 */
soydata.VERY_UNSAFE.ordainSanitizedTrustedResourceUri =
    soydata.$$makeSanitizedContentFactoryWithDefaultDirOnly_(
        goog.soy.data.SanitizedTrustedResourceUri);


/**
 * Takes a leap of faith that the provided content is "safe" to use as an
 * HTML attribute.
 *
 * @param {*} content An attribute name and value, such as
 *     {@code dir="ltr"}.
 * @return {!goog.soy.data.SanitizedHtmlAttribute} Sanitized content wrapper
 *     that indicates to Soy not to escape when printed as an HTML attribute.
 */
soydata.VERY_UNSAFE.ordainSanitizedHtmlAttribute =
    soydata.$$makeSanitizedContentFactoryWithDefaultDirOnly_(
        goog.soy.data.SanitizedHtmlAttribute);


/**
 * Takes a leap of faith that the provided content is "safe" to use as STYLE
 * in a style attribute.
 *
 * @param {*} content CSS, such as {@code color:#c3d9ff}.
 * @return {!goog.soy.data.SanitizedStyle} Sanitized style wrapper that
 *     indicates to Soy there is no need to escape or filter when printed in CSS
 *     context.
 */
soydata.VERY_UNSAFE.ordainSanitizedStyle =
    soydata.$$makeSanitizedContentFactoryWithDefaultDirOnly_(
        goog.soy.data.SanitizedStyle);


/**
 * Takes a leap of faith that the provided content is "safe" to use as CSS
 * in a style block.
 *
 * @param {*} content CSS, such as {@code color:#c3d9ff}.
 * @return {!goog.soy.data.SanitizedCss} Sanitized CSS wrapper that indicates to
 *     Soy there is no need to escape or filter when printed in CSS context.
 */
soydata.VERY_UNSAFE.ordainSanitizedCss =
    soydata.$$makeSanitizedContentFactoryWithDefaultDirOnly_(
        goog.soy.data.SanitizedCss);


// -----------------------------------------------------------------------------
// Soy-generated utilities in the soy namespace.  Contains implementations for
// common soyfunctions (e.g. keys()) and escaping/print directives.


/**
 * Whether the locale is right-to-left.
 *
 * @type {boolean}
 */
soy.$$IS_LOCALE_RTL = goog.i18n.bidi.IS_RTL;


/**
 * Builds an augmented map. The returned map will contain mappings from both
 * the base map and the additional map. If the same key appears in both, then
 * the value from the additional map will be visible, while the value from the
 * base map will be hidden. The base map will be used, but not modified.
 *
 * @param {!Object} baseMap The original map to augment.
 * @param {!Object} additionalMap A map containing the additional mappings.
 * @return {!Object} An augmented map containing both the original and
 *     additional mappings.
 */
soy.$$augmentMap = function(baseMap, additionalMap) {
  return soy.$$assignDefaults(soy.$$assignDefaults({}, additionalMap), baseMap);
};


/**
 * Copies extra properties into an object if they do not already exist. The
 * destination object is mutated in the process.
 *
 * @param {!Object} obj The destination object to update.
 * @param {!Object} defaults An object with default properties to apply.
 * @return {!Object} The destination object for convenience.
 */
soy.$$assignDefaults = function(obj, defaults) {
  for (var key in defaults) {
    if (!(key in obj)) {
      obj[key] = defaults[key];
    }
  }

  return obj;
};


/**
 * Checks that the given map key is a string.
 * @param {*} key Key to check.
 * @return {string} The given key.
 */
soy.$$checkMapKey = function(key) {
  // TODO: Support map literal with nonstring key.
  if ((typeof key) != 'string') {
    throw Error(
        'Map literal\'s key expression must evaluate to string' +
        ' (encountered type "' + (typeof key) + '").');
  }
  return key;
};


/**
 * Gets the keys in a map as an array. There are no guarantees on the order.
 * @param {Object} map The map to get the keys of.
 * @return {!Array<string>} The array of keys in the given map.
 */
soy.$$getMapKeys = function(map) {
  var mapKeys = [];
  for (var key in map) {
    mapKeys.push(key);
  }
  return mapKeys;
};


/**
 * Returns the argument if it is not null.
 *
 * @param {T} val The value to check
 * @return {T} val if is isn't null
 * @template T
 */
soy.$$checkNotNull = function(val) {
  if (val == null) {
    throw Error('unexpected null value');
  }
  return val;
};


/**
 * Parses the given string into a base 10 integer. Returns null if parse is
 * unsuccessful.
 * @param {string} str The string to parse
 * @return {?number} The string parsed as a base 10 integer, or null if
 * unsuccessful
 */
soy.$$parseInt = function(str) {
  var parsed = parseInt(str, 10);
  return isNaN(parsed) ? null : parsed;
};


/**
 * Parses the given string into a float. Returns null if parse is unsuccessful.
 * @param {string} str The string to parse
 * @return {?number} The string parsed as a float, or null if unsuccessful.
 */
soy.$$parseFloat = function(str) {
  var parsed = parseFloat(str);
  return isNaN(parsed) ? null : parsed;
};


/**
 * Gets a consistent unique id for the given delegate template name. Two calls
 * to this function will return the same id if and only if the input names are
 * the same.
 *
 * <p> Important: This function must always be called with a string constant.
 *
 * <p> If Closure Compiler is not being used, then this is just this identity
 * function. If Closure Compiler is being used, then each call to this function
 * will be replaced with a short string constant, which will be consistent per
 * input name.
 *
 * @param {string} delTemplateName The delegate template name for which to get a
 *     consistent unique id.
 * @return {string} A unique id that is consistent per input name.
 *
 * @idGenerator {consistent}
 */
soy.$$getDelTemplateId = function(delTemplateName) {
  return delTemplateName;
};


/**
 * Map from registered delegate template key to the priority of the
 * implementation.
 * @type {Object}
 * @private
 */
soy.$$DELEGATE_REGISTRY_PRIORITIES_ = {};

/**
 * Map from registered delegate template key to the implementation function.
 * @type {Object}
 * @private
 */
soy.$$DELEGATE_REGISTRY_FUNCTIONS_ = {};


/**
 * Registers a delegate implementation. If the same delegate template key (id
 * and variant) has been registered previously, then priority values are
 * compared and only the higher priority implementation is stored (if
 * priorities are equal, an error is thrown).
 *
 * @param {string} delTemplateId The delegate template id.
 * @param {string} delTemplateVariant The delegate template variant (can be
 *     empty string).
 * @param {number} delPriority The implementation's priority value.
 * @param {Function} delFn The implementation function.
 */
soy.$$registerDelegateFn = function(
    delTemplateId, delTemplateVariant, delPriority, delFn) {

  var mapKey = 'key_' + delTemplateId + ':' + delTemplateVariant;
  var currPriority = soy.$$DELEGATE_REGISTRY_PRIORITIES_[mapKey];
  if (currPriority === undefined || delPriority > currPriority) {
    // Registering new or higher-priority function: replace registry entry.
    soy.$$DELEGATE_REGISTRY_PRIORITIES_[mapKey] = delPriority;
    soy.$$DELEGATE_REGISTRY_FUNCTIONS_[mapKey] = delFn;
  } else if (delPriority == currPriority) {
    // Registering same-priority function: error.
    throw Error(
        'Encountered two active delegates with the same priority ("' +
            delTemplateId + ':' + delTemplateVariant + '").');
  } else {
    // Registering lower-priority function: do nothing.
  }
};


/**
 * Retrieves the (highest-priority) implementation that has been registered for
 * a given delegate template key (id and variant). If no implementation has
 * been registered for the key, then the fallback is the same id with empty
 * variant. If the fallback is also not registered, and allowsEmptyDefault is
 * true, then returns an implementation that is equivalent to an empty template
 * (i.e. rendered output would be empty string).
 *
 * @param {string} delTemplateId The delegate template id.
 * @param {string} delTemplateVariant The delegate template variant (can be
 *     empty string).
 * @param {boolean} allowsEmptyDefault Whether to default to the empty template
 *     function if there's no active implementation.
 * @return {Function} The retrieved implementation function.
 */
soy.$$getDelegateFn = function(
    delTemplateId, delTemplateVariant, allowsEmptyDefault) {

  var delFn = soy.$$DELEGATE_REGISTRY_FUNCTIONS_[
      'key_' + delTemplateId + ':' + delTemplateVariant];
  if (! delFn && delTemplateVariant != '') {
    // Fallback to empty variant.
    delFn = soy.$$DELEGATE_REGISTRY_FUNCTIONS_['key_' + delTemplateId + ':'];
  }

  if (delFn) {
    return delFn;
  } else if (allowsEmptyDefault) {
    return soy.$$EMPTY_TEMPLATE_FN_;
  } else {
    throw Error(
        'Found no active impl for delegate call to "' + delTemplateId + ':' +
            delTemplateVariant + '" (and not allowemptydefault="true").');
  }
};


/**
 * Private helper soy.$$getDelegateFn(). This is the empty template function
 * that is returned whenever there's no delegate implementation found.
 *
 * @param {Object<string, *>=} opt_data
 * @param {Object<string, *>=} opt_ijData
 * @param {Object<string, *>=} opt_ijData_deprecated TODO(b/36644846): remove
 * @return {string}
 * @private
 */
soy.$$EMPTY_TEMPLATE_FN_ = function(
    opt_data, opt_ijData, opt_ijData_deprecated) {
  return '';
};


// -----------------------------------------------------------------------------
// Internal sanitized content wrappers.


/**
 * Creates a SanitizedContent factory for SanitizedContent types for internal
 * Soy let and param blocks.
 *
 * This is a hack within Soy so that SanitizedContent objects created via let
 * and param blocks will truth-test as false if they are empty string.
 * Tricking the Javascript runtime to treat empty SanitizedContent as falsey is
 * not possible, and changing the Soy compiler to wrap every boolean statement
 * for just this purpose is impractical.  Instead, we just avoid wrapping empty
 * string as SanitizedContent, since it's a no-op for empty strings anyways.
 *
 * @param {function(new: T)} ctor A constructor.
 * @return {!function(*, ?goog.i18n.bidi.Dir=): (T|soydata.$$EMPTY_STRING_)}
 *     A factory that takes content and an optional content direction and
 *     returns a new instance, or an empty string. If the content direction is
 *     undefined, ctor.prototype.contentDir is used.
 * @template T
 * @private
 */
soydata.$$makeSanitizedContentFactoryForInternalBlocks_ = function(ctor) {
  /**
   * @param {string} content
   * @constructor
   * @extends {goog.soy.data.SanitizedContent}
   */
  function InstantiableCtor(content) {
    /** @override */
    this.content = content;
  }
  InstantiableCtor.prototype = ctor.prototype;
  /**
   * Creates a ctor-type SanitizedContent instance.
   *
   * @param {*} content The content to put in the instance.
   * @param {?goog.i18n.bidi.Dir=} opt_contentDir The content direction. If
   *     undefined, ctor.prototype.contentDir is used.
   * @return {!goog.soy.data.SanitizedContent|soydata.$$EMPTY_STRING_} The new
   *     instance, or an empty string. A new instance is actually of type T
   *     above (ctor's type, a descendant of SanitizedContent), but there's no
   *     way to express that here.
   */
  function sanitizedContentFactory(content, opt_contentDir) {
    var contentString = String(content);
    if (!contentString) {
      return soydata.$$EMPTY_STRING_.VALUE;
    }
    var result = new InstantiableCtor(contentString);
    if (opt_contentDir !== undefined) {
      result.contentDir = opt_contentDir;
    }
    return result;
  }
  return sanitizedContentFactory;
};


/**
 * Creates a SanitizedContent factory for SanitizedContent types that should
 * always have their default directionality for internal Soy let and param
 * blocks.
 *
 * This is a hack within Soy so that SanitizedContent objects created via let
 * and param blocks will truth-test as false if they are empty string.
 * Tricking the Javascript runtime to treat empty SanitizedContent as falsey is
 * not possible, and changing the Soy compiler to wrap every boolean statement
 * for just this purpose is impractical.  Instead, we just avoid wrapping empty
 * string as SanitizedContent, since it's a no-op for empty strings anyways.
 *
 * @param {function(new: T)} ctor A constructor.
 * @return {!function(*): (T|soydata.$$EMPTY_STRING_)} A
 *     factory that takes content and returns a
 *     new instance (with default directionality, i.e.
 *     ctor.prototype.contentDir), or an empty string.
 * @template T
 * @private
 */
soydata.$$makeSanitizedContentFactoryWithDefaultDirOnlyForInternalBlocks_ =
    function(ctor) {
  /**
   * @param {string} content
   * @constructor
   * @extends {goog.soy.data.SanitizedContent}
   */
  function InstantiableCtor(content) {
    /** @override */
    this.content = content;
  }
  InstantiableCtor.prototype = ctor.prototype;
  /**
   * Creates a ctor-type SanitizedContent instance.
   *
   * @param {*} content The content to put in the instance.
   * @return {!goog.soy.data.SanitizedContent|soydata.$$EMPTY_STRING_} The new
   *     instance, or an empty string. A new instance is actually of type T
   *     above (ctor's type, a descendant of SanitizedContent), but there's no
   *     way to express that here.
   */
  function sanitizedContentFactory(content) {
    var contentString = String(content);
    if (!contentString) {
      return soydata.$$EMPTY_STRING_.VALUE;
    }
    var result = new InstantiableCtor(contentString);
    return result;
  }
  return sanitizedContentFactory;
};


/**
 * Creates kind="text" block contents (internal use only).
 *
 * @param {*} content Text.
 * @param {?goog.i18n.bidi.Dir=} opt_contentDir The content direction; null if
 *     unknown and thus to be estimated when necessary. Default: null.
 * @return {!goog.soy.data.UnsanitizedText|soydata.$$EMPTY_STRING_} Wrapped result.
 */
soydata.$$markUnsanitizedTextForInternalBlocks = function(
    content, opt_contentDir) {
  var contentString = String(content);
  if (!contentString) {
    return soydata.$$EMPTY_STRING_.VALUE;
  }
  return new goog.soy.data.UnsanitizedText(contentString, opt_contentDir);
};


/**
 * Creates kind="html" block contents (internal use only).
 *
 * @param {*} content Text.
 * @param {?goog.i18n.bidi.Dir=} opt_contentDir The content direction; null if
 *     unknown and thus to be estimated when necessary. Default: null.
 * @return {!goog.soy.data.SanitizedHtml|soydata.$$EMPTY_STRING_} Wrapped
 *     result.
 */
soydata.VERY_UNSAFE.$$ordainSanitizedHtmlForInternalBlocks =
    soydata.$$makeSanitizedContentFactoryForInternalBlocks_(
        goog.soy.data.SanitizedHtml);


/**
 * Creates kind="js" block contents (internal use only).
 *
 * @param {*} content Text.
 * @return {!goog.soy.data.SanitizedJs|soydata.$$EMPTY_STRING_} Wrapped result.
 */
soydata.VERY_UNSAFE.$$ordainSanitizedJsForInternalBlocks =
    soydata.$$makeSanitizedContentFactoryWithDefaultDirOnlyForInternalBlocks_(
        goog.soy.data.SanitizedJs);


/**
 * Creates kind="trustedResourceUri" block contents (internal use only).
 *
 * @param {*} content Text.
 * @return {goog.soy.data.SanitizedTrustedResourceUri|soydata.$$EMPTY_STRING_}
 *     Wrapped result.
 */
soydata.VERY_UNSAFE.$$ordainSanitizedTrustedResourceUriForInternalBlocks =
    soydata.$$makeSanitizedContentFactoryWithDefaultDirOnlyForInternalBlocks_(
        goog.soy.data.SanitizedTrustedResourceUri);


/**
 * Creates kind="uri" block contents (internal use only).
 *
 * @param {*} content Text.
 * @return {goog.soy.data.SanitizedUri|soydata.$$EMPTY_STRING_} Wrapped result.
 */
soydata.VERY_UNSAFE.$$ordainSanitizedUriForInternalBlocks =
    soydata.$$makeSanitizedContentFactoryWithDefaultDirOnlyForInternalBlocks_(
        goog.soy.data.SanitizedUri);


/**
 * Creates kind="attributes" block contents (internal use only).
 *
 * @param {*} content Text.
 * @return {goog.soy.data.SanitizedHtmlAttribute|soydata.$$EMPTY_STRING_}
 *     Wrapped result.
 */
soydata.VERY_UNSAFE.$$ordainSanitizedAttributesForInternalBlocks =
    soydata.$$makeSanitizedContentFactoryWithDefaultDirOnlyForInternalBlocks_(
        goog.soy.data.SanitizedHtmlAttribute);


/**
 * Creates kind="style" block contents (internal use only).
 *
 * @param {*} content Text.
 * @return {goog.soy.data.SanitizedStyle|soydata.$$EMPTY_STRING_} Wrapped
 *     result.
 */
soydata.VERY_UNSAFE.$$ordainSanitizedStyleForInternalBlocks =
    soydata.$$makeSanitizedContentFactoryWithDefaultDirOnlyForInternalBlocks_(
        goog.soy.data.SanitizedStyle);


/**
 * Creates kind="css" block contents (internal use only).
 *
 * @param {*} content Text.
 * @return {goog.soy.data.SanitizedCss|soydata.$$EMPTY_STRING_} Wrapped result.
 */
soydata.VERY_UNSAFE.$$ordainSanitizedCssForInternalBlocks =
    soydata.$$makeSanitizedContentFactoryWithDefaultDirOnlyForInternalBlocks_(
        goog.soy.data.SanitizedCss);


// -----------------------------------------------------------------------------
// Escape/filter/normalize.


/**
 * Returns a SanitizedHtml object for a particular value. The content direction
 * is preserved.
 *
 * This HTML-escapes the value unless it is already SanitizedHtml. Escapes
 * double quote '"' in addition to '&', '<', and '>' so that a string can be
 * included in an HTML tag attribute value within double quotes.
 *
 * @param {*} value The value to convert. If it is already a SanitizedHtml
 *     object, it is left alone.
 * @return {!goog.soy.data.SanitizedHtml} An escaped version of value.
 */
soy.$$escapeHtml = function(value) {
  return soydata.SanitizedHtml.from(value);
};


/**
 * Strips unsafe tags to convert a string of untrusted HTML into HTML that
 * is safe to embed. The content direction is preserved.
 *
 * @param {?} value The string-like value to be escaped. May not be a string,
 *     but the value will be coerced to a string.
 * @param {Array<string>=} opt_safeTags Additional tag names to whitelist.
 * @return {!goog.soy.data.SanitizedHtml} A sanitized and normalized version of
 *     value.
 */
soy.$$cleanHtml = function(value, opt_safeTags) {
  if (soydata.isContentKind_(value, goog.soy.data.SanitizedContentKind.HTML)) {
    goog.asserts.assert(value.constructor === goog.soy.data.SanitizedHtml);
    return /** @type {!goog.soy.data.SanitizedHtml} */ (value);
  }
  var tagWhitelist;
  if (opt_safeTags) {
    tagWhitelist = goog.object.createSet(opt_safeTags);
    goog.object.extend(tagWhitelist, soy.esc.$$SAFE_TAG_WHITELIST_);
  } else {
    tagWhitelist = soy.esc.$$SAFE_TAG_WHITELIST_;
  }
  return soydata.VERY_UNSAFE.ordainSanitizedHtml(
      soy.$$stripHtmlTags(value, tagWhitelist), soydata.getContentDir(value));
};


/**
 * Escapes HTML, except preserves entities.
 *
 * Used mainly internally for escaping message strings in attribute and rcdata
 * context, where we explicitly want to preserve any existing entities.
 *
 * @param {*} value Value to normalize.
 * @return {string} A value safe to insert in HTML without any quotes or angle
 *     brackets.
 */
soy.$$normalizeHtml = function(value) {
  return soy.esc.$$normalizeHtmlHelper(value);
};


/**
 * Escapes HTML special characters in a string so that it can be embedded in
 * RCDATA.
 * <p>
 * Escapes HTML special characters so that the value will not prematurely end
 * the body of a tag like {@code <textarea>} or {@code <title>}. RCDATA tags
 * cannot contain other HTML entities, so it is not strictly necessary to escape
 * HTML special characters except when part of that text looks like an HTML
 * entity or like a close tag : {@code </textarea>}.
 * <p>
 * Will normalize known safe HTML to make sure that sanitized HTML (which could
 * contain an innocuous {@code </textarea>} don't prematurely end an RCDATA
 * element.
 *
 * @param {?} value The string-like value to be escaped. May not be a string,
 *     but the value will be coerced to a string.
 * @return {string} An escaped version of value.
 */
soy.$$escapeHtmlRcdata = function(value) {
  if (soydata.isContentKind_(value, goog.soy.data.SanitizedContentKind.HTML)) {
    goog.asserts.assert(value.constructor === goog.soy.data.SanitizedHtml);
    return soy.esc.$$normalizeHtmlHelper(value.getContent());
  }
  return soy.esc.$$escapeHtmlHelper(value);
};


/**
 * Matches any/only HTML5 void elements' start tags.
 * See http://www.w3.org/TR/html-markup/syntax.html#syntax-elements
 * @type {RegExp}
 * @private
 */
soy.$$HTML5_VOID_ELEMENTS_ = new RegExp(
    '^<(?:area|base|br|col|command|embed|hr|img|input' +
    '|keygen|link|meta|param|source|track|wbr)\\b');


/**
 * Removes HTML tags from a string of known safe HTML.
 * If opt_tagWhitelist is not specified or is empty, then
 * the result can be used as an attribute value.
 *
 * @param {*} value The HTML to be escaped. May not be a string, but the
 *     value will be coerced to a string.
 * @param {Object<string, boolean>=} opt_tagWhitelist Has an own property whose
 *     name is a lower-case tag name and whose value is {@code 1} for
 *     each element that is allowed in the output.
 * @return {string} A representation of value without disallowed tags,
 *     HTML comments, or other non-text content.
 */
soy.$$stripHtmlTags = function(value, opt_tagWhitelist) {
  if (!opt_tagWhitelist) {
    // If we have no white-list, then use a fast track which elides all tags.
    return String(value).replace(soy.esc.$$HTML_TAG_REGEX_, '')
        // This is just paranoia since callers should normalize the result
        // anyway, but if they didn't, it would be necessary to ensure that
        // after the first replace non-tag uses of < do not recombine into
        // tags as in "<<foo>script>alert(1337)</<foo>script>".
        .replace(soy.esc.$$LT_REGEX_, '&lt;');
  }

  // Escapes '[' so that we can use [123] below to mark places where tags
  // have been removed.
  var html = String(value).replace(/\[/g, '&#91;');

  // Consider all uses of '<' and replace whitelisted tags with markers like
  // [1] which are indices into a list of approved tag names.
  // Replace all other uses of < and > with entities.
  var tags = [];
  var attrs = [];
  html = html.replace(
    soy.esc.$$HTML_TAG_REGEX_,
    function(tok, tagName) {
      if (tagName) {
        tagName = tagName.toLowerCase();
        if (opt_tagWhitelist.hasOwnProperty(tagName) &&
            opt_tagWhitelist[tagName]) {
          var isClose = tok.charAt(1) == '/';
          var index = tags.length;
          var start = '</';
          var attributes = '';
          if (!isClose) {
            start = '<';
            var match;
            while ((match = soy.esc.$$HTML_ATTRIBUTE_REGEX_.exec(tok))) {
              if (match[1] && match[1].toLowerCase() == 'dir') {
                var dir = match[2];
                if (dir) {
                  if (dir.charAt(0) == '\'' || dir.charAt(0) == '"') {
                    dir = dir.substr(1, dir.length - 2);
                  }
                  dir = dir.toLowerCase();
                  if (dir == 'ltr' || dir == 'rtl' || dir == 'auto') {
                    attributes = ' dir="' + dir + '"';
                  }
                }
                break;
              }
            }
            soy.esc.$$HTML_ATTRIBUTE_REGEX_.lastIndex = 0;
          }
          tags[index] = start + tagName + '>';
          attrs[index] = attributes;
          return '[' + index + ']';
        }
      }
      return '';
    });

  // Escape HTML special characters. Now there are no '<' in html that could
  // start a tag.
  html = soy.esc.$$normalizeHtmlHelper(html);

  var finalCloseTags = soy.$$balanceTags_(tags);

  // Now html contains no tags or less-than characters that could become
  // part of a tag via a replacement operation and tags only contains
  // approved tags.
  // Reinsert the white-listed tags.
  html = html.replace(/\[(\d+)\]/g, function(_, index) {
    if (attrs[index] && tags[index]) {
      return tags[index].substr(0, tags[index].length - 1) + attrs[index] + '>';
    }
    return tags[index];
  });

  // Close any still open tags.
  // This prevents unclosed formatting elements like <ol> and <table> from
  // breaking the layout of containing HTML.
  return html + finalCloseTags;
};


/**
 * Make sure that tag boundaries are not broken by Safe CSS when embedded in a
 * {@code <style>} element.
 * @param {string} css
 * @return {string}
 * @private
 */
soy.$$embedCssIntoHtml_ = function(css) {
  // Port of a method of the same name in
  // com.google.template.soy.shared.restricted.Sanitizers
  return css.replace(/<\//g, '<\\/').replace(/\]\]>/g, ']]\\>');
};


/**
 * Throw out any close tags that don't correspond to start tags.
 * If {@code <table>} is used for formatting, embedded HTML shouldn't be able
 * to use a mismatched {@code </table>} to break page layout.
 *
 * @param {Array<string>} tags Array of open/close tags (e.g. '<p>', '</p>')
 *    that will be modified in place to be either an open tag, one or more close
 *    tags concatenated, or the empty string.
 * @return {string} zero or more closed tags that close all elements that are
 *    opened in tags but not closed.
 * @private
 */
soy.$$balanceTags_ = function(tags) {
  var open = [];
  for (var i = 0, n = tags.length; i < n; ++i) {
    var tag = tags[i];
    if (tag.charAt(1) == '/') {
      var openTagIndex = goog.array.lastIndexOf(open, tag);
      if (openTagIndex < 0) {
        tags[i] = '';  // Drop close tag with no corresponding open tag.
      } else {
        tags[i] = open.slice(openTagIndex).reverse().join('');
        open.length = openTagIndex;
      }
    } else if (tag == '<li>' &&
        goog.array.lastIndexOf(open, '</ol>') < 0 &&
        goog.array.lastIndexOf(open, '</ul>') < 0) {
      // Drop <li> if it isn't nested in a parent <ol> or <ul>.
      tags[i] = '';
    } else if (!soy.$$HTML5_VOID_ELEMENTS_.test(tag)) {
      open.push('</' + tag.substring(1));
    }
  }
  return open.reverse().join('');
};


/**
 * Escapes HTML special characters in an HTML attribute value.
 *
 * @param {?} value The HTML to be escaped. May not be a string, but the
 *     value will be coerced to a string.
 * @return {string} An escaped version of value.
 */
soy.$$escapeHtmlAttribute = function(value) {
  // NOTE: We don't accept ATTRIBUTES here because ATTRIBUTES is actually not
  // the attribute value context, but instead k/v pairs.
  if (soydata.isContentKind_(value, goog.soy.data.SanitizedContentKind.HTML)) {
    // NOTE: After removing tags, we also escape quotes ("normalize") so that
    // the HTML can be embedded in attribute context.
    goog.asserts.assert(value.constructor === goog.soy.data.SanitizedHtml);
    return soy.esc.$$normalizeHtmlHelper(
        soy.$$stripHtmlTags(value.getContent()));
  }
  return soy.esc.$$escapeHtmlHelper(value);
};


/**
 * Escapes HTML special characters in a string including space and other
 * characters that can end an unquoted HTML attribute value.
 *
 * @param {?} value The HTML to be escaped. May not be a string, but the
 *     value will be coerced to a string.
 * @return {string} An escaped version of value.
 */
soy.$$escapeHtmlAttributeNospace = function(value) {
  if (soydata.isContentKind_(value, goog.soy.data.SanitizedContentKind.HTML)) {
    goog.asserts.assert(value.constructor === goog.soy.data.SanitizedHtml);
    return soy.esc.$$normalizeHtmlNospaceHelper(
        soy.$$stripHtmlTags(value.getContent()));
  }
  return soy.esc.$$escapeHtmlNospaceHelper(value);
};


/**
 * Filters out strings that cannot be a substring of a valid HTML attribute.
 *
 * Note the input is expected to be key=value pairs.
 *
 * @param {?} value The value to escape. May not be a string, but the value
 *     will be coerced to a string.
 * @return {string} A valid HTML attribute name part or name/value pair.
 *     {@code "zSoyz"} if the input is invalid.
 */
soy.$$filterHtmlAttributes = function(value) {
  // NOTE: Explicitly no support for SanitizedContentKind.HTML, since that is
  // meaningless in this context, which is generally *between* html attributes.
  if (soydata.isContentKind_(
    value, goog.soy.data.SanitizedContentKind.ATTRIBUTES)) {
    goog.asserts.assert(
        value.constructor === goog.soy.data.SanitizedHtmlAttribute);
    // Add a space at the end to ensure this won't get merged into following
    // attributes, unless the interpretation is unambiguous (ending with quotes
    // or a space).
    return value.getContent().replace(/([^"'\s])$/, '$1 ');
  }
  // TODO: Dynamically inserting attributes that aren't marked as trusted is
  // probably unnecessary.  Any filtering done here will either be inadequate
  // for security or not flexible enough.  Having clients use kind="attributes"
  // in parameters seems like a wiser idea.
  return soy.esc.$$filterHtmlAttributesHelper(value);
};


/**
 * Filters out strings that cannot be a substring of a valid HTML element name.
 *
 * @param {*} value The value to escape. May not be a string, but the value
 *     will be coerced to a string.
 * @return {string} A valid HTML element name part.
 *     {@code "zSoyz"} if the input is invalid.
 */
soy.$$filterHtmlElementName = function(value) {
  // NOTE: We don't accept any SanitizedContent here. HTML indicates valid
  // PCDATA, not tag names. A sloppy developer shouldn't be able to cause an
  // exploit:
  // ... {let userInput}script src=http://evil.com/evil.js{/let} ...
  // ... {param tagName kind="html"}{$userInput}{/param} ...
  // ... <{$tagName}>Hello World</{$tagName}>
  return soy.esc.$$filterHtmlElementNameHelper(value);
};


/**
 * Escapes characters in the value to make it valid content for a JS string
 * literal.
 *
 * @param {*} value The value to escape. May not be a string, but the value
 *     will be coerced to a string.
 * @return {string} An escaped version of value.
 */
soy.$$escapeJsString = function(value) {
  return soy.esc.$$escapeJsStringHelper(value);
};


/**
 * Encodes a value as a JavaScript literal.
 *
 * @param {*} value The value to escape. May not be a string, but the value
 *     will be coerced to a string.
 * @return {string} A JavaScript code representation of the input.
 */
soy.$$escapeJsValue = function(value) {
  // We surround values with spaces so that they can't be interpolated into
  // identifiers by accident.
  // We could use parentheses but those might be interpreted as a function call.
  if (value == null) {  // Intentionally matches undefined.
    // Java returns null from maps where there is no corresponding key while
    // JS returns undefined.
    // We always output null for compatibility with Java which does not have a
    // distinct undefined value.
    return ' null ';
  }
  if (soydata.isContentKind_(value, goog.soy.data.SanitizedContentKind.JS)) {
    goog.asserts.assert(value.constructor === goog.soy.data.SanitizedJs);
    return value.getContent();
  }
  if (value instanceof goog.html.SafeScript) {
    return goog.html.SafeScript.unwrap(value);
  }
  switch (typeof value) {
    case 'boolean': case 'number':
      return ' ' + value + ' ';
    default:
      return "'" + soy.esc.$$escapeJsStringHelper(String(value)) + "'";
  }
};


/**
 * Escapes characters in the string to make it valid content for a JS regular
 * expression literal.
 *
 * @param {*} value The value to escape. May not be a string, but the value
 *     will be coerced to a string.
 * @return {string} An escaped version of value.
 */
soy.$$escapeJsRegex = function(value) {
  return soy.esc.$$escapeJsRegexHelper(value);
};


/**
 * Matches all URI mark characters that conflict with HTML attribute delimiters
 * or that cannot appear in a CSS uri.
 * From <a href="http://www.w3.org/TR/CSS2/grammar.html">G.2: CSS grammar</a>
 * <pre>
 *     url        ([!#$%&*-~]|{nonascii}|{escape})*
 * </pre>
 *
 * @type {RegExp}
 * @private
 */
soy.$$problematicUriMarks_ = /['()]/g;

/**
 * @param {string} ch A single character in {@link soy.$$problematicUriMarks_}.
 * @return {string}
 * @private
 */
soy.$$pctEncode_ = function(ch) {
  return '%' + ch.charCodeAt(0).toString(16);
};

/**
 * Escapes a string so that it can be safely included in a URI.
 *
 * @param {*} value The value to escape. May not be a string, but the value
 *     will be coerced to a string.
 * @return {string} An escaped version of value.
 */
soy.$$escapeUri = function(value) {
  // NOTE: We don't check for SanitizedUri or SafeUri, because just because
  // something is already a valid complete URL doesn't mean we don't want to
  // encode it as a component.  For example, it would be bad if
  // ?redirect={$url} didn't escape ampersands, because in that template, the
  // continue URL should be treated as a single unit.

  // Apostophes and parentheses are not matched by encodeURIComponent.
  // They are technically special in URIs, but only appear in the obsolete mark
  // production in Appendix D.2 of RFC 3986, so can be encoded without changing
  // semantics.
  var encoded = soy.esc.$$escapeUriHelper(value);
  soy.$$problematicUriMarks_.lastIndex = 0;
  if (soy.$$problematicUriMarks_.test(encoded)) {
    return encoded.replace(soy.$$problematicUriMarks_, soy.$$pctEncode_);
  }
  return encoded;
};


/**
 * Removes rough edges from a URI by escaping any raw HTML/JS string delimiters.
 *
 * @param {*} value The value to escape. May not be a string, but the value
 *     will be coerced to a string.
 * @return {string} An escaped version of value.
 */
soy.$$normalizeUri = function(value) {
  return soy.esc.$$normalizeUriHelper(value);
};


/**
 * Vets a URI's protocol and removes rough edges from a URI by escaping
 * any raw HTML/JS string delimiters.
 *
 * @param {?} value The value to escape. May not be a string, but the value
 *     will be coerced to a string.
 * @return {string} An escaped version of value.
 */
soy.$$filterNormalizeUri = function(value) {
  if (soydata.isContentKind_(value, goog.soy.data.SanitizedContentKind.URI)) {
    goog.asserts.assert(value.constructor === goog.soy.data.SanitizedUri);
    return soy.$$normalizeUri(value);
  }
  if (soydata.isContentKind_(value,
      goog.soy.data.SanitizedContentKind.TRUSTED_RESOURCE_URI)) {
    goog.asserts.assert(
        value.constructor === goog.soy.data.SanitizedTrustedResourceUri);
    return soy.$$normalizeUri(value);
  }
  if (value instanceof goog.html.SafeUrl) {
    return soy.$$normalizeUri(goog.html.SafeUrl.unwrap(value));
  }
  if (value instanceof goog.html.TrustedResourceUrl) {
    return soy.$$normalizeUri(goog.html.TrustedResourceUrl.unwrap(value));
  }
  return soy.esc.$$filterNormalizeUriHelper(value);
};


/**
 * Vets a URI for usage as an image source.
 *
 * @param {?} value The value to filter. Might not be a string, but the value
 *     will be coerced to a string.
 * @return {string} An escaped version of value.
 */
soy.$$filterNormalizeMediaUri = function(value) {
  // Image URIs are filtered strictly more loosely than other types of URIs.
  // TODO(shwetakarwa): Add tests for this in soyutils_test_helper while adding
  // tests for filterTrustedResourceUri.
  if (soydata.isContentKind_(value, goog.soy.data.SanitizedContentKind.URI)) {
    goog.asserts.assert(value.constructor === goog.soy.data.SanitizedUri);
    return soy.$$normalizeUri(value);
  }
  if (soydata.isContentKind_(value,
      goog.soy.data.SanitizedContentKind.TRUSTED_RESOURCE_URI)) {
    goog.asserts.assert(
        value.constructor === goog.soy.data.SanitizedTrustedResourceUri);
    return soy.$$normalizeUri(value);
  }
  if (value instanceof goog.html.SafeUrl) {
    return soy.$$normalizeUri(goog.html.SafeUrl.unwrap(value));
  }
  if (value instanceof goog.html.TrustedResourceUrl) {
    return soy.$$normalizeUri(goog.html.TrustedResourceUrl.unwrap(value));
  }
  return soy.esc.$$filterNormalizeMediaUriHelper(value);
};


/**
 * Vets a URI for usage as a resource. Makes sure the input value is a compile
 * time constant or a TrustedResouce not in attacker's control.
 *
 * @param {?} value The value to filter.
 * @return {string} The value content.
 */
soy.$$filterTrustedResourceUri = function(value) {
  if (soydata.isContentKind_(value,
      goog.soy.data.SanitizedContentKind.TRUSTED_RESOURCE_URI)) {
    goog.asserts.assert(
        value.constructor === goog.soy.data.SanitizedTrustedResourceUri);
    return value.getContent();
  }
  if (value instanceof goog.html.TrustedResourceUrl) {
    return goog.html.TrustedResourceUrl.unwrap(value);
  }
  goog.asserts.fail('Bad value `%s` for |filterTrustedResourceUri',
      [String(value)]);
  return 'about:invalid#zSoyz';
};


/**
 * For any resource string/variable which has
 * |blessStringAsTrustedResuorceUrlForLegacy directive return the value as is.
 *
 * @param {*} value The value to be blessed. Might not be a string
 * @return {*} value Return current value.
 */
soy.$$blessStringAsTrustedResourceUrlForLegacy = function(value) {
  return value;
};


/**
 * Allows only data-protocol image URI's.
 *
 * @param {*} value The value to process. May not be a string, but the value
 *     will be coerced to a string.
 * @return {!goog.soy.data.SanitizedUri} An escaped version of value.
 */
soy.$$filterImageDataUri = function(value) {
  // NOTE: Even if it's a SanitizedUri, we will still filter it.
  return soydata.VERY_UNSAFE.ordainSanitizedUri(
      soy.esc.$$filterImageDataUriHelper(value));
};


/**
 * Allows only tel URIs.
 *
 * @param {*} value The value to process. May not be a string, but the value
 *     will be coerced to a string.
 * @return {!goog.soy.data.SanitizedUri} An escaped version of value.
 */
soy.$$filterTelUri = function(value) {
  // NOTE: Even if it's a SanitizedUri, we will still filter it.
  return soydata.VERY_UNSAFE.ordainSanitizedUri(
      soy.esc.$$filterTelUriHelper(value));
};


/**
 * Escapes a string so it can safely be included inside a quoted CSS string.
 *
 * @param {*} value The value to escape. May not be a string, but the value
 *     will be coerced to a string.
 * @return {string} An escaped version of value.
 */
soy.$$escapeCssString = function(value) {
  return soy.esc.$$escapeCssStringHelper(value);
};


/**
 * Encodes a value as a CSS identifier part, keyword, or quantity.
 *
 * @param {?} value The value to escape. May not be a string, but the value
 *     will be coerced to a string.
 * @return {string} A safe CSS identifier part, keyword, or quanitity.
 */
soy.$$filterCssValue = function(value) {
  if (soydata.isContentKind_(value, goog.soy.data.SanitizedContentKind.CSS)) {
    goog.asserts.assertInstanceof(value, goog.soy.data.SanitizedCss);
    return soy.$$embedCssIntoHtml_(value.getContent());
  }
  // Uses == to intentionally match null and undefined for Java compatibility.
  if (value == null) {
    return '';
  }
  if (value instanceof goog.html.SafeStyle) {
    return soy.$$embedCssIntoHtml_(goog.html.SafeStyle.unwrap(value));
  }
  // Note: SoyToJsSrcCompiler uses soy.$$filterCssValue both for the contents of
  // <style> (list of rules) and for the contents of style="" (one set of
  // declarations). We support SafeStyleSheet here to be used inside <style> but
  // it also wrongly allows it inside style="". We should instead change
  // SoyToJsSrcCompiler to use a different function inside <style>.
  if (value instanceof goog.html.SafeStyleSheet) {
    return soy.$$embedCssIntoHtml_(goog.html.SafeStyleSheet.unwrap(value));
  }
  return soy.esc.$$filterCssValueHelper(value);
};


/**
 * Sanity-checks noAutoescape input for explicitly tainted content.
 *
 * SanitizedContentKind.TEXT is used to explicitly mark input that was never
 * meant to be used unescaped.
 *
 * @param {?} value The value to filter.
 * @return {*} The value, that we dearly hope will not cause an attack.
 */
soy.$$filterNoAutoescape = function(value) {
  if (soydata.isContentKind_(value, goog.soy.data.SanitizedContentKind.TEXT)) {
    // Fail in development mode.
    goog.asserts.fail(
        'Tainted SanitizedContentKind.TEXT for |noAutoescape: `%s`',
        [value.getContent()]);
    // Return innocuous data in production.
    return 'zSoyz';
  }

  return value;
};


// -----------------------------------------------------------------------------
// Basic directives/functions.


/**
 * Converts \r\n, \r, and \n to <br>s
 * @param {*} value The string in which to convert newlines.
 * @return {string|!goog.soy.data.SanitizedHtml} A copy of {@code value} with
 *     converted newlines. If {@code value} is SanitizedHtml, the return value
 *     is also SanitizedHtml, of the same known directionality.
 */
soy.$$changeNewlineToBr = function(value) {
  var result = goog.string.newLineToBr(String(value), false);
  if (soydata.isContentKind_(value, goog.soy.data.SanitizedContentKind.HTML)) {
    return soydata.VERY_UNSAFE.ordainSanitizedHtml(
        result, soydata.getContentDir(value));
  }
  return result;
};


/**
 * Inserts word breaks ('wbr' tags) into a HTML string at a given interval. The
 * counter is reset if a space is encountered. Word breaks aren't inserted into
 * HTML tags or entities. Entites count towards the character count; HTML tags
 * do not.
 *
 * @param {*} value The HTML string to insert word breaks into. Can be other
 *     types, but the value will be coerced to a string.
 * @param {number} maxCharsBetweenWordBreaks Maximum number of non-space
 *     characters to allow before adding a word break.
 * @return {string|!goog.soy.data.SanitizedHtml} The string including word
 *     breaks. If {@code value} is SanitizedHtml, the return value
 *     is also SanitizedHtml, of the same known directionality.
 * @deprecated The |insertWordBreaks directive is deprecated.
 *     Prefer wrapping with CSS white-space: break-word.
 */
soy.$$insertWordBreaks = function(value, maxCharsBetweenWordBreaks) {
  var result = goog.format.insertWordBreaks(
      String(value), maxCharsBetweenWordBreaks);
  if (soydata.isContentKind_(value, goog.soy.data.SanitizedContentKind.HTML)) {
    return soydata.VERY_UNSAFE.ordainSanitizedHtml(
        result, soydata.getContentDir(value));
  }
  return result;
};


/**
 * Truncates a string to a given max length (if it's currently longer),
 * optionally adding ellipsis at the end.
 *
 * @param {*} str The string to truncate. Can be other types, but the value will
 *     be coerced to a string.
 * @param {number} maxLen The maximum length of the string after truncation
 *     (including ellipsis, if applicable).
 * @param {boolean} doAddEllipsis Whether to add ellipsis if the string needs
 *     truncation.
 * @return {string} The string after truncation.
 */
soy.$$truncate = function(str, maxLen, doAddEllipsis) {

  str = String(str);
  if (str.length <= maxLen) {
    return str;  // no need to truncate
  }

  // If doAddEllipsis, either reduce maxLen to compensate, or else if maxLen is
  // too small, just turn off doAddEllipsis.
  if (doAddEllipsis) {
    if (maxLen > 3) {
      maxLen -= 3;
    } else {
      doAddEllipsis = false;
    }
  }

  // Make sure truncating at maxLen doesn't cut up a unicode surrogate pair.
  if (soy.$$isHighSurrogate_(str.charCodeAt(maxLen - 1)) &&
      soy.$$isLowSurrogate_(str.charCodeAt(maxLen))) {
    maxLen -= 1;
  }

  // Truncate.
  str = str.substring(0, maxLen);

  // Add ellipsis.
  if (doAddEllipsis) {
    str += '...';
  }

  return str;
};

/**
 * Private helper for $$truncate() to check whether a char is a high surrogate.
 * @param {number} cc The codepoint to check.
 * @return {boolean} Whether the given codepoint is a unicode high surrogate.
 * @private
 */
soy.$$isHighSurrogate_ = function(cc) {
  return 0xD800 <= cc && cc <= 0xDBFF;
};

/**
 * Private helper for $$truncate() to check whether a char is a low surrogate.
 * @param {number} cc The codepoint to check.
 * @return {boolean} Whether the given codepoint is a unicode low surrogate.
 * @private
 */
soy.$$isLowSurrogate_ = function(cc) {
  return 0xDC00 <= cc && cc <= 0xDFFF;
};


// -----------------------------------------------------------------------------
// Bidi directives/functions.


/**
 * Cache of bidi formatter by context directionality, so we don't keep on
 * creating new objects.
 * @type {!Object<!goog.i18n.BidiFormatter>}
 * @private
 */
soy.$$bidiFormatterCache_ = {};


/**
 * Returns cached bidi formatter for bidiGlobalDir, or creates a new one.
 * @param {number} bidiGlobalDir The global directionality context: 1 if ltr, -1
 *     if rtl, 0 if unknown.
 * @return {!goog.i18n.BidiFormatter} A formatter for bidiGlobalDir.
 * @private
 */
soy.$$getBidiFormatterInstance_ = function(bidiGlobalDir) {
  return soy.$$bidiFormatterCache_[bidiGlobalDir] ||
         (soy.$$bidiFormatterCache_[bidiGlobalDir] =
             new goog.i18n.BidiFormatter(bidiGlobalDir));
};


/**
 * Estimate the overall directionality of text. If opt_isHtml, makes sure to
 * ignore the LTR nature of the mark-up and escapes in text, making the logic
 * suitable for HTML and HTML-escaped text.
 * If text has a goog.i18n.bidi.Dir-valued contentDir, this is used instead of
 * estimating the directionality.
 *
 * @param {*} text The content whose directionality is to be estimated.
 * @param {boolean=} opt_isHtml Whether text is HTML/HTML-escaped.
 *     Default: false.
 * @return {number} 1 if text is LTR, -1 if it is RTL, and 0 if it is neutral.
 */
soy.$$bidiTextDir = function(text, opt_isHtml) {
  var contentDir = soydata.getContentDir(text);
  if (contentDir != null) {
    return contentDir;
  }
  var isHtml = opt_isHtml ||
      soydata.isContentKind_(text, goog.soy.data.SanitizedContentKind.HTML);
  return goog.i18n.bidi.estimateDirection(text + '', isHtml);
};


/**
 * Returns 'dir="ltr"' or 'dir="rtl"', depending on text's estimated
 * directionality, if it is not the same as bidiGlobalDir.
 * Otherwise, returns the empty string.
 * If opt_isHtml, makes sure to ignore the LTR nature of the mark-up and escapes
 * in text, making the logic suitable for HTML and HTML-escaped text.
 * If text has a goog.i18n.bidi.Dir-valued contentDir, this is used instead of
 * estimating the directionality.
 *
 * @param {number} bidiGlobalDir The global directionality context: 1 if ltr, -1
 *     if rtl, 0 if unknown.
 * @param {*} text The content whose directionality is to be estimated.
 * @param {boolean=} opt_isHtml Whether text is HTML/HTML-escaped.
 *     Default: false.
 * @return {!goog.soy.data.SanitizedHtmlAttribute} 'dir="rtl"' for RTL text in
 *     non-RTL context; 'dir="ltr"' for LTR text in non-LTR context;
 *     else, the empty string.
 */
soy.$$bidiDirAttr = function(bidiGlobalDir, text, opt_isHtml) {
  var formatter = soy.$$getBidiFormatterInstance_(bidiGlobalDir);
  var contentDir = soydata.getContentDir(text);
  if (contentDir == null) {
    var isHtml = opt_isHtml ||
        soydata.isContentKind_(text, goog.soy.data.SanitizedContentKind.HTML);
    contentDir = goog.i18n.bidi.estimateDirection(text + '', isHtml);
  }
  return soydata.VERY_UNSAFE.ordainSanitizedHtmlAttribute(
      formatter.knownDirAttr(contentDir));
};


/**
 * Returns a Unicode BiDi mark matching bidiGlobalDir (LRM or RLM) if the
 * directionality or the exit directionality of text are opposite to
 * bidiGlobalDir. Otherwise returns the empty string.
 * If opt_isHtml, makes sure to ignore the LTR nature of the mark-up and escapes
 * in text, making the logic suitable for HTML and HTML-escaped text.
 * If text has a goog.i18n.bidi.Dir-valued contentDir, this is used instead of
 * estimating the directionality.
 *
 * @param {number} bidiGlobalDir The global directionality context: 1 if ltr, -1
 *     if rtl, 0 if unknown.
 * @param {*} text The content whose directionality is to be estimated.
 * @param {boolean=} opt_isHtml Whether text is HTML/HTML-escaped.
 *     Default: false.
 * @return {string} A Unicode bidi mark matching bidiGlobalDir, or the empty
 *     string when text's overall and exit directionalities both match
 *     bidiGlobalDir, or bidiGlobalDir is 0 (unknown).
 */
soy.$$bidiMarkAfter = function(bidiGlobalDir, text, opt_isHtml) {
  var formatter = soy.$$getBidiFormatterInstance_(bidiGlobalDir);
  var isHtml = opt_isHtml ||
      soydata.isContentKind_(text, goog.soy.data.SanitizedContentKind.HTML);
  return formatter.markAfterKnownDir(soydata.getContentDir(text), text + '',
      isHtml);
};


/**
 * Returns text wrapped in a <span dir="ltr|rtl"> according to its
 * directionality - but only if that is neither neutral nor the same as the
 * global context. Otherwise, returns text unchanged.
 * Always treats text as HTML/HTML-escaped, i.e. ignores mark-up and escapes
 * when estimating text's directionality.
 * If text has a goog.i18n.bidi.Dir-valued contentDir, this is used instead of
 * estimating the directionality.
 *
 * @param {number} bidiGlobalDir The global directionality context: 1 if ltr, -1
 *     if rtl, 0 if unknown.
 * @param {*} text The string to be wrapped. Can be other types, but the value
 *     will be coerced to a string.
 * @return {!goog.soy.data.SanitizedContent|string} The wrapped text.
 */
soy.$$bidiSpanWrap = function(bidiGlobalDir, text) {
  var formatter = soy.$$getBidiFormatterInstance_(bidiGlobalDir);

  // We always treat the value as HTML, because span-wrapping is only useful
  // when its output will be treated as HTML (without escaping), and because
  // |bidiSpanWrap is not itself specified to do HTML escaping in Soy. (Both
  // explicit and automatic HTML escaping, if any, is done before calling
  // |bidiSpanWrap because the BidiSpanWrapDirective Java class implements
  // SanitizedContentOperator, but this does not mean that the input has to be
  // HTML SanitizedContent. In legacy usage, a string that is not
  // SanitizedContent is often printed in an autoescape="false" template or by
  // a print with a |noAutoescape, in which case our input is just SoyData.) If
  // the output will be treated as HTML, the input had better be safe
  // HTML/HTML-escaped (even if it isn't HTML SanitizedData), or we have an XSS
  // opportunity and a much bigger problem than bidi garbling.
  var html = goog.html.uncheckedconversions.
      safeHtmlFromStringKnownToSatisfyTypeContract(
          goog.string.Const.from(
              'Soy |bidiSpanWrap is applied on an autoescaped text.'),
          String(text));
  var wrappedHtml = formatter.spanWrapSafeHtmlWithKnownDir(
      soydata.getContentDir(text), html);

  // Like other directives whose Java class implements SanitizedContentOperator,
  // |bidiSpanWrap is called after the escaping (if any) has already been done,
  // and thus there is no need for it to produce actual SanitizedContent.
  return goog.html.SafeHtml.unwrap(wrappedHtml);
};


/**
 * Returns text wrapped in Unicode BiDi formatting characters according to its
 * directionality, i.e. either LRE or RLE at the beginning and PDF at the end -
 * but only if text's directionality is neither neutral nor the same as the
 * global context. Otherwise, returns text unchanged.
 * Only treats SanitizedHtml as HTML/HTML-escaped, i.e. ignores mark-up
 * and escapes when estimating text's directionality.
 * If text has a goog.i18n.bidi.Dir-valued contentDir, this is used instead of
 * estimating the directionality.
 *
 * @param {number} bidiGlobalDir The global directionality context: 1 if ltr, -1
 *     if rtl, 0 if unknown.
 * @param {*} text The string to be wrapped. Can be other types, but the value
 *     will be coerced to a string.
 * @return {!goog.soy.data.SanitizedContent|string} The wrapped string.
 */
soy.$$bidiUnicodeWrap = function(bidiGlobalDir, text) {
  var formatter = soy.$$getBidiFormatterInstance_(bidiGlobalDir);

  // We treat the value as HTML if and only if it says it's HTML, even though in
  // legacy usage, we sometimes have an HTML string (not SanitizedContent) that
  // is passed to an autoescape="false" template or a {print $foo|noAutoescape},
  // with the output going into an HTML context without escaping. We simply have
  // no way of knowing if this is what is happening when we get
  // non-SanitizedContent input, and most of the time it isn't.
  var isHtml =
      soydata.isContentKind_(text, goog.soy.data.SanitizedContentKind.HTML);
  var wrappedText = formatter.unicodeWrapWithKnownDir(
      soydata.getContentDir(text), text + '', isHtml);

  // Bidi-wrapping a value converts it to the context directionality. Since it
  // does not cost us anything, we will indicate this known direction in the
  // output SanitizedContent, even though the intended consumer of that
  // information - a bidi wrapping directive - has already been run.
  var wrappedTextDir = formatter.getContextDir();

  // Unicode-wrapping UnsanitizedText gives UnsanitizedText.
  // Unicode-wrapping safe HTML or JS string data gives valid, safe HTML or JS
  // string data.
  // ATTENTION: Do these need to be ...ForInternalBlocks()?
  if (soydata.isContentKind_(text, goog.soy.data.SanitizedContentKind.TEXT)) {
    return new goog.soy.data.UnsanitizedText(wrappedText, wrappedTextDir);
  }
  if (isHtml) {
    return soydata.VERY_UNSAFE.ordainSanitizedHtml(wrappedText, wrappedTextDir);
  }

  // Unicode-wrapping does not conform to the syntax of the other types of
  // content. For lack of anything better to do, we we do not declare a content
  // kind at all by falling through to the non-SanitizedContent case below.
  // TODO(aharon): Consider throwing a runtime error on receipt of
  // SanitizedContent other than TEXT, HTML, or JS_STR_CHARS.

  // The input was not SanitizedContent, so our output isn't SanitizedContent
  // either.
  return wrappedText;
};

// -----------------------------------------------------------------------------
// Assertion methods used by runtime.

/**
 * Checks if the type assertion is true if goog.asserts.ENABLE_ASSERTS is
 * true. Report errors on runtime types if goog.DEBUG is true.
 * @param {boolean} condition The type check condition.
 * @param {string} paramName The Soy name of the parameter.
 * @param {?} param The JS object for the parameter.
 * @param {!string} jsDocTypeStr SoyDoc type str.
 * @return {?} the param value
 * @throws {goog.asserts.AssertionError} When the condition evaluates to false.
 */
soy.asserts.assertType = function(condition, paramName, param, jsDocTypeStr) {
  if (goog.asserts.ENABLE_ASSERTS && !condition) {
    var msg = 'expected param ' + paramName + ' of type ' + jsDocTypeStr +
        (goog.DEBUG ? (', but got ' + goog.debug.runtimeType(param)) : '') +
        '.';
    goog.asserts.fail(msg);
  }
  return param;
};

// -----------------------------------------------------------------------------
// Used for inspecting Soy template information from rendered pages.

/**
 * Whether we should generate additional HTML comments.
 * @type {boolean}
 */
soy.$$debugSoyTemplateInfo = false;

if (goog.DEBUG) {
  /**
   * Configures whether we should generate additional HTML comments for
   * inspecting Soy template information from rendered pages.
   * @param {boolean} debugSoyTemplateInfo
   */
  soy.setDebugSoyTemplateInfo = function(debugSoyTemplateInfo) {
    soy.$$debugSoyTemplateInfo = debugSoyTemplateInfo;
  };
}

// -----------------------------------------------------------------------------
// Generated code.


// START GENERATED CODE FOR ESCAPERS.

/**
 * @type {function (*) : string}
 */
soy.esc.$$escapeHtmlHelper = function(v) {
  return goog.string.htmlEscape(String(v));
};

/**
 * @type {function (*) : string}
 */
soy.esc.$$escapeUriHelper = function(v) {
  return goog.string.urlEncode(String(v));
};

/**
 * Maps characters to the escaped versions for the named escape directives.
 * @private {!Object<string, string>}
 */
soy.esc.$$ESCAPE_MAP_FOR_NORMALIZE_HTML__AND__ESCAPE_HTML_NOSPACE__AND__NORMALIZE_HTML_NOSPACE_ = {
  '\x00': '\x26#0;',
  '\x09': '\x26#9;',
  '\x0a': '\x26#10;',
  '\x0b': '\x26#11;',
  '\x0c': '\x26#12;',
  '\x0d': '\x26#13;',
  ' ': '\x26#32;',
  '\x22': '\x26quot;',
  '\x26': '\x26amp;',
  '\x27': '\x26#39;',
  '-': '\x26#45;',
  '\/': '\x26#47;',
  '\x3c': '\x26lt;',
  '\x3d': '\x26#61;',
  '\x3e': '\x26gt;',
  '`': '\x26#96;',
  '\x85': '\x26#133;',
  '\xa0': '\x26#160;',
  '\u2028': '\x26#8232;',
  '\u2029': '\x26#8233;'
};

/**
 * A function that can be used with String.replace.
 * @param {string} ch A single character matched by a compatible matcher.
 * @return {string} A token in the output language.
 * @private
 */
soy.esc.$$REPLACER_FOR_NORMALIZE_HTML__AND__ESCAPE_HTML_NOSPACE__AND__NORMALIZE_HTML_NOSPACE_ = function(ch) {
  return soy.esc.$$ESCAPE_MAP_FOR_NORMALIZE_HTML__AND__ESCAPE_HTML_NOSPACE__AND__NORMALIZE_HTML_NOSPACE_[ch];
};

/**
 * Maps characters to the escaped versions for the named escape directives.
 * @private {!Object<string, string>}
 */
soy.esc.$$ESCAPE_MAP_FOR_ESCAPE_JS_STRING__AND__ESCAPE_JS_REGEX_ = {
  '\x00': '\\x00',
  '\x08': '\\x08',
  '\x09': '\\t',
  '\x0a': '\\n',
  '\x0b': '\\x0b',
  '\x0c': '\\f',
  '\x0d': '\\r',
  '\x22': '\\x22',
  '$': '\\x24',
  '\x26': '\\x26',
  '\x27': '\\x27',
  '(': '\\x28',
  ')': '\\x29',
  '*': '\\x2a',
  '+': '\\x2b',
  ',': '\\x2c',
  '-': '\\x2d',
  '.': '\\x2e',
  '\/': '\\\/',
  ':': '\\x3a',
  '\x3c': '\\x3c',
  '\x3d': '\\x3d',
  '\x3e': '\\x3e',
  '?': '\\x3f',
  '\x5b': '\\x5b',
  '\\': '\\\\',
  '\x5d': '\\x5d',
  '^': '\\x5e',
  '\x7b': '\\x7b',
  '|': '\\x7c',
  '\x7d': '\\x7d',
  '\x85': '\\x85',
  '\u2028': '\\u2028',
  '\u2029': '\\u2029'
};

/**
 * A function that can be used with String.replace.
 * @param {string} ch A single character matched by a compatible matcher.
 * @return {string} A token in the output language.
 * @private
 */
soy.esc.$$REPLACER_FOR_ESCAPE_JS_STRING__AND__ESCAPE_JS_REGEX_ = function(ch) {
  return soy.esc.$$ESCAPE_MAP_FOR_ESCAPE_JS_STRING__AND__ESCAPE_JS_REGEX_[ch];
};

/**
 * Maps characters to the escaped versions for the named escape directives.
 * @private {!Object<string, string>}
 */
soy.esc.$$ESCAPE_MAP_FOR_ESCAPE_CSS_STRING_ = {
  '\x00': '\\0 ',
  '\x08': '\\8 ',
  '\x09': '\\9 ',
  '\x0a': '\\a ',
  '\x0b': '\\b ',
  '\x0c': '\\c ',
  '\x0d': '\\d ',
  '\x22': '\\22 ',
  '\x26': '\\26 ',
  '\x27': '\\27 ',
  '(': '\\28 ',
  ')': '\\29 ',
  '*': '\\2a ',
  '\/': '\\2f ',
  ':': '\\3a ',
  ';': '\\3b ',
  '\x3c': '\\3c ',
  '\x3d': '\\3d ',
  '\x3e': '\\3e ',
  '@': '\\40 ',
  '\\': '\\5c ',
  '\x7b': '\\7b ',
  '\x7d': '\\7d ',
  '\x85': '\\85 ',
  '\xa0': '\\a0 ',
  '\u2028': '\\2028 ',
  '\u2029': '\\2029 '
};

/**
 * A function that can be used with String.replace.
 * @param {string} ch A single character matched by a compatible matcher.
 * @return {string} A token in the output language.
 * @private
 */
soy.esc.$$REPLACER_FOR_ESCAPE_CSS_STRING_ = function(ch) {
  return soy.esc.$$ESCAPE_MAP_FOR_ESCAPE_CSS_STRING_[ch];
};

/**
 * Maps characters to the escaped versions for the named escape directives.
 * @private {!Object<string, string>}
 */
soy.esc.$$ESCAPE_MAP_FOR_NORMALIZE_URI__AND__FILTER_NORMALIZE_URI__AND__FILTER_NORMALIZE_MEDIA_URI_ = {
  '\x00': '%00',
  '\x01': '%01',
  '\x02': '%02',
  '\x03': '%03',
  '\x04': '%04',
  '\x05': '%05',
  '\x06': '%06',
  '\x07': '%07',
  '\x08': '%08',
  '\x09': '%09',
  '\x0a': '%0A',
  '\x0b': '%0B',
  '\x0c': '%0C',
  '\x0d': '%0D',
  '\x0e': '%0E',
  '\x0f': '%0F',
  '\x10': '%10',
  '\x11': '%11',
  '\x12': '%12',
  '\x13': '%13',
  '\x14': '%14',
  '\x15': '%15',
  '\x16': '%16',
  '\x17': '%17',
  '\x18': '%18',
  '\x19': '%19',
  '\x1a': '%1A',
  '\x1b': '%1B',
  '\x1c': '%1C',
  '\x1d': '%1D',
  '\x1e': '%1E',
  '\x1f': '%1F',
  ' ': '%20',
  '\x22': '%22',
  '\x27': '%27',
  '(': '%28',
  ')': '%29',
  '\x3c': '%3C',
  '\x3e': '%3E',
  '\\': '%5C',
  '\x7b': '%7B',
  '\x7d': '%7D',
  '\x7f': '%7F',
  '\x85': '%C2%85',
  '\xa0': '%C2%A0',
  '\u2028': '%E2%80%A8',
  '\u2029': '%E2%80%A9',
  '\uff01': '%EF%BC%81',
  '\uff03': '%EF%BC%83',
  '\uff04': '%EF%BC%84',
  '\uff06': '%EF%BC%86',
  '\uff07': '%EF%BC%87',
  '\uff08': '%EF%BC%88',
  '\uff09': '%EF%BC%89',
  '\uff0a': '%EF%BC%8A',
  '\uff0b': '%EF%BC%8B',
  '\uff0c': '%EF%BC%8C',
  '\uff0f': '%EF%BC%8F',
  '\uff1a': '%EF%BC%9A',
  '\uff1b': '%EF%BC%9B',
  '\uff1d': '%EF%BC%9D',
  '\uff1f': '%EF%BC%9F',
  '\uff20': '%EF%BC%A0',
  '\uff3b': '%EF%BC%BB',
  '\uff3d': '%EF%BC%BD'
};

/**
 * A function that can be used with String.replace.
 * @param {string} ch A single character matched by a compatible matcher.
 * @return {string} A token in the output language.
 * @private
 */
soy.esc.$$REPLACER_FOR_NORMALIZE_URI__AND__FILTER_NORMALIZE_URI__AND__FILTER_NORMALIZE_MEDIA_URI_ = function(ch) {
  return soy.esc.$$ESCAPE_MAP_FOR_NORMALIZE_URI__AND__FILTER_NORMALIZE_URI__AND__FILTER_NORMALIZE_MEDIA_URI_[ch];
};

/**
 * Matches characters that need to be escaped for the named directives.
 * @private {!RegExp}
 */
soy.esc.$$MATCHER_FOR_NORMALIZE_HTML_ = /[\x00\x22\x27\x3c\x3e]/g;

/**
 * Matches characters that need to be escaped for the named directives.
 * @private {!RegExp}
 */
soy.esc.$$MATCHER_FOR_ESCAPE_HTML_NOSPACE_ = /[\x00\x09-\x0d \x22\x26\x27\x2d\/\x3c-\x3e`\x85\xa0\u2028\u2029]/g;

/**
 * Matches characters that need to be escaped for the named directives.
 * @private {!RegExp}
 */
soy.esc.$$MATCHER_FOR_NORMALIZE_HTML_NOSPACE_ = /[\x00\x09-\x0d \x22\x27\x2d\/\x3c-\x3e`\x85\xa0\u2028\u2029]/g;

/**
 * Matches characters that need to be escaped for the named directives.
 * @private {!RegExp}
 */
soy.esc.$$MATCHER_FOR_ESCAPE_JS_STRING_ = /[\x00\x08-\x0d\x22\x26\x27\/\x3c-\x3e\x5b-\x5d\x7b\x7d\x85\u2028\u2029]/g;

/**
 * Matches characters that need to be escaped for the named directives.
 * @private {!RegExp}
 */
soy.esc.$$MATCHER_FOR_ESCAPE_JS_REGEX_ = /[\x00\x08-\x0d\x22\x24\x26-\/\x3a\x3c-\x3f\x5b-\x5e\x7b-\x7d\x85\u2028\u2029]/g;

/**
 * Matches characters that need to be escaped for the named directives.
 * @private {!RegExp}
 */
soy.esc.$$MATCHER_FOR_ESCAPE_CSS_STRING_ = /[\x00\x08-\x0d\x22\x26-\x2a\/\x3a-\x3e@\\\x7b\x7d\x85\xa0\u2028\u2029]/g;

/**
 * Matches characters that need to be escaped for the named directives.
 * @private {!RegExp}
 */
soy.esc.$$MATCHER_FOR_NORMALIZE_URI__AND__FILTER_NORMALIZE_URI__AND__FILTER_NORMALIZE_MEDIA_URI_ = /[\x00- \x22\x27-\x29\x3c\x3e\\\x7b\x7d\x7f\x85\xa0\u2028\u2029\uff01\uff03\uff04\uff06-\uff0c\uff0f\uff1a\uff1b\uff1d\uff1f\uff20\uff3b\uff3d]/g;

/**
 * A pattern that vets values produced by the named directives.
 * @private {!RegExp}
 */
soy.esc.$$FILTER_FOR_FILTER_CSS_VALUE_ = /^(?!-*(?:expression|(?:moz-)?binding))(?!\s+)(?:[.#]?-?(?:[_a-z0-9-]+)(?:-[_a-z0-9-]+)*-?|(?:rgb|hsl)a?\([0-9.%,\u0020]+\)|-?(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)(?:[a-z]{1,2}|%)?|!important|\s+)*$/i;

/**
 * A pattern that vets values produced by the named directives.
 * @private {!RegExp}
 */
soy.esc.$$FILTER_FOR_FILTER_NORMALIZE_URI_ = /^(?![^#?]*\/(?:\.|%2E){2}(?:[\/?#]|$))(?:(?:https?|mailto):|[^&:\/?#]*(?:[\/?#]|$))/i;

/**
 * A pattern that vets values produced by the named directives.
 * @private {!RegExp}
 */
soy.esc.$$FILTER_FOR_FILTER_NORMALIZE_MEDIA_URI_ = /^[^&:\/?#]*(?:[\/?#]|$)|^https?:|^data:image\/[a-z0-9+]+;base64,[a-z0-9+\/]+=*$|^blob:/i;

/**
 * A pattern that vets values produced by the named directives.
 * @private {!RegExp}
 */
soy.esc.$$FILTER_FOR_FILTER_IMAGE_DATA_URI_ = /^data:image\/(?:bmp|gif|jpe?g|png|tiff|webp);base64,[a-z0-9+\/]+=*$/i;

/**
 * A pattern that vets values produced by the named directives.
 * @private {!RegExp}
 */
soy.esc.$$FILTER_FOR_FILTER_TEL_URI_ = /^tel:[0-9a-z;=\-+._!~*'\u0020\/():&$#?@,]+$/i;

/**
 * A pattern that vets values produced by the named directives.
 * @private {!RegExp}
 */
soy.esc.$$FILTER_FOR_FILTER_HTML_ATTRIBUTES_ = /^(?!on|src|(?:style|action|archive|background|cite|classid|codebase|data|dsync|href|longdesc|usemap)\s*$)(?:[a-z0-9_$:-]*)$/i;

/**
 * A pattern that vets values produced by the named directives.
 * @private {!RegExp}
 */
soy.esc.$$FILTER_FOR_FILTER_HTML_ELEMENT_NAME_ = /^(?!script|style|title|textarea|xmp|no)[a-z0-9_$:-]*$/i;

/**
 * A helper for the Soy directive |normalizeHtml
 * @param {*} value Can be of any type but will be coerced to a string.
 * @return {string} The escaped text.
 */
soy.esc.$$normalizeHtmlHelper = function(value) {
  var str = String(value);
  return str.replace(
      soy.esc.$$MATCHER_FOR_NORMALIZE_HTML_,
      soy.esc.$$REPLACER_FOR_NORMALIZE_HTML__AND__ESCAPE_HTML_NOSPACE__AND__NORMALIZE_HTML_NOSPACE_);
};

/**
 * A helper for the Soy directive |escapeHtmlNospace
 * @param {*} value Can be of any type but will be coerced to a string.
 * @return {string} The escaped text.
 */
soy.esc.$$escapeHtmlNospaceHelper = function(value) {
  var str = String(value);
  return str.replace(
      soy.esc.$$MATCHER_FOR_ESCAPE_HTML_NOSPACE_,
      soy.esc.$$REPLACER_FOR_NORMALIZE_HTML__AND__ESCAPE_HTML_NOSPACE__AND__NORMALIZE_HTML_NOSPACE_);
};

/**
 * A helper for the Soy directive |normalizeHtmlNospace
 * @param {*} value Can be of any type but will be coerced to a string.
 * @return {string} The escaped text.
 */
soy.esc.$$normalizeHtmlNospaceHelper = function(value) {
  var str = String(value);
  return str.replace(
      soy.esc.$$MATCHER_FOR_NORMALIZE_HTML_NOSPACE_,
      soy.esc.$$REPLACER_FOR_NORMALIZE_HTML__AND__ESCAPE_HTML_NOSPACE__AND__NORMALIZE_HTML_NOSPACE_);
};

/**
 * A helper for the Soy directive |escapeJsString
 * @param {*} value Can be of any type but will be coerced to a string.
 * @return {string} The escaped text.
 */
soy.esc.$$escapeJsStringHelper = function(value) {
  var str = String(value);
  return str.replace(
      soy.esc.$$MATCHER_FOR_ESCAPE_JS_STRING_,
      soy.esc.$$REPLACER_FOR_ESCAPE_JS_STRING__AND__ESCAPE_JS_REGEX_);
};

/**
 * A helper for the Soy directive |escapeJsRegex
 * @param {*} value Can be of any type but will be coerced to a string.
 * @return {string} The escaped text.
 */
soy.esc.$$escapeJsRegexHelper = function(value) {
  var str = String(value);
  return str.replace(
      soy.esc.$$MATCHER_FOR_ESCAPE_JS_REGEX_,
      soy.esc.$$REPLACER_FOR_ESCAPE_JS_STRING__AND__ESCAPE_JS_REGEX_);
};

/**
 * A helper for the Soy directive |escapeCssString
 * @param {*} value Can be of any type but will be coerced to a string.
 * @return {string} The escaped text.
 */
soy.esc.$$escapeCssStringHelper = function(value) {
  var str = String(value);
  return str.replace(
      soy.esc.$$MATCHER_FOR_ESCAPE_CSS_STRING_,
      soy.esc.$$REPLACER_FOR_ESCAPE_CSS_STRING_);
};

/**
 * A helper for the Soy directive |filterCssValue
 * @param {*} value Can be of any type but will be coerced to a string.
 * @return {string} The escaped text.
 */
soy.esc.$$filterCssValueHelper = function(value) {
  var str = String(value);
  if (!soy.esc.$$FILTER_FOR_FILTER_CSS_VALUE_.test(str)) {
    goog.asserts.fail('Bad value `%s` for |filterCssValue', [str]);
    return 'zSoyz';
  }
  return str;
};

/**
 * A helper for the Soy directive |normalizeUri
 * @param {*} value Can be of any type but will be coerced to a string.
 * @return {string} The escaped text.
 */
soy.esc.$$normalizeUriHelper = function(value) {
  var str = String(value);
  return str.replace(
      soy.esc.$$MATCHER_FOR_NORMALIZE_URI__AND__FILTER_NORMALIZE_URI__AND__FILTER_NORMALIZE_MEDIA_URI_,
      soy.esc.$$REPLACER_FOR_NORMALIZE_URI__AND__FILTER_NORMALIZE_URI__AND__FILTER_NORMALIZE_MEDIA_URI_);
};

/**
 * A helper for the Soy directive |filterNormalizeUri
 * @param {*} value Can be of any type but will be coerced to a string.
 * @return {string} The escaped text.
 */
soy.esc.$$filterNormalizeUriHelper = function(value) {
  var str = String(value);
  if (!soy.esc.$$FILTER_FOR_FILTER_NORMALIZE_URI_.test(str)) {
    goog.asserts.fail('Bad value `%s` for |filterNormalizeUri', [str]);
    return 'about:invalid#zSoyz';
  }
  return str.replace(
      soy.esc.$$MATCHER_FOR_NORMALIZE_URI__AND__FILTER_NORMALIZE_URI__AND__FILTER_NORMALIZE_MEDIA_URI_,
      soy.esc.$$REPLACER_FOR_NORMALIZE_URI__AND__FILTER_NORMALIZE_URI__AND__FILTER_NORMALIZE_MEDIA_URI_);
};

/**
 * A helper for the Soy directive |filterNormalizeMediaUri
 * @param {*} value Can be of any type but will be coerced to a string.
 * @return {string} The escaped text.
 */
soy.esc.$$filterNormalizeMediaUriHelper = function(value) {
  var str = String(value);
  if (!soy.esc.$$FILTER_FOR_FILTER_NORMALIZE_MEDIA_URI_.test(str)) {
    goog.asserts.fail('Bad value `%s` for |filterNormalizeMediaUri', [str]);
    return 'about:invalid#zSoyz';
  }
  return str.replace(
      soy.esc.$$MATCHER_FOR_NORMALIZE_URI__AND__FILTER_NORMALIZE_URI__AND__FILTER_NORMALIZE_MEDIA_URI_,
      soy.esc.$$REPLACER_FOR_NORMALIZE_URI__AND__FILTER_NORMALIZE_URI__AND__FILTER_NORMALIZE_MEDIA_URI_);
};

/**
 * A helper for the Soy directive |filterImageDataUri
 * @param {*} value Can be of any type but will be coerced to a string.
 * @return {string} The escaped text.
 */
soy.esc.$$filterImageDataUriHelper = function(value) {
  var str = String(value);
  if (!soy.esc.$$FILTER_FOR_FILTER_IMAGE_DATA_URI_.test(str)) {
    goog.asserts.fail('Bad value `%s` for |filterImageDataUri', [str]);
    return 'data:image/gif;base64,zSoyz';
  }
  return str;
};

/**
 * A helper for the Soy directive |filterTelUri
 * @param {*} value Can be of any type but will be coerced to a string.
 * @return {string} The escaped text.
 */
soy.esc.$$filterTelUriHelper = function(value) {
  var str = String(value);
  if (!soy.esc.$$FILTER_FOR_FILTER_TEL_URI_.test(str)) {
    goog.asserts.fail('Bad value `%s` for |filterTelUri', [str]);
    return 'about:invalid#zSoyz';
  }
  return str;
};

/**
 * A helper for the Soy directive |filterHtmlAttributes
 * @param {*} value Can be of any type but will be coerced to a string.
 * @return {string} The escaped text.
 */
soy.esc.$$filterHtmlAttributesHelper = function(value) {
  var str = String(value);
  if (!soy.esc.$$FILTER_FOR_FILTER_HTML_ATTRIBUTES_.test(str)) {
    goog.asserts.fail('Bad value `%s` for |filterHtmlAttributes', [str]);
    return 'zSoyz';
  }
  return str;
};

/**
 * A helper for the Soy directive |filterHtmlElementName
 * @param {*} value Can be of any type but will be coerced to a string.
 * @return {string} The escaped text.
 */
soy.esc.$$filterHtmlElementNameHelper = function(value) {
  var str = String(value);
  if (!soy.esc.$$FILTER_FOR_FILTER_HTML_ELEMENT_NAME_.test(str)) {
    goog.asserts.fail('Bad value `%s` for |filterHtmlElementName', [str]);
    return 'zSoyz';
  }
  return str;
};

/**
 * Matches all tags, HTML comments, and DOCTYPEs in tag soup HTML.
 * By removing these, and replacing any '<' or '>' characters with
 * entities we guarantee that the result can be embedded into a
 * an attribute without introducing a tag boundary.
 *
 * @private {!RegExp}
 */
soy.esc.$$HTML_TAG_REGEX_ = /<(?:!|\/?([a-zA-Z][a-zA-Z0-9:\-]*))(?:[^>'"]|"[^"]*"|'[^']*')*>/g;

/**
 * Matches all occurrences of '<'.
 *
 * @private {!RegExp}
 */
soy.esc.$$LT_REGEX_ = /</g;

/**
 * Maps lower-case names of innocuous tags to true.
 *
 * @private {!Object<string, boolean>}
 */
soy.esc.$$SAFE_TAG_WHITELIST_ = {'b': true, 'br': true, 'em': true, 'i': true, 's': true, 'sub': true, 'sup': true, 'u': true};

/**
 * Pattern for matching attribute name and value, where value is single-quoted
 * or double-quoted.
 * See http://www.w3.org/TR/2011/WD-html5-20110525/syntax.html#attributes-0
 *
 * @private {!RegExp}
 */
soy.esc.$$HTML_ATTRIBUTE_REGEX_ = /([a-zA-Z][a-zA-Z0-9:\-]*)[\t\n\r\u0020]*=[\t\n\r\u0020]*("[^"]*"|'[^']*')/g;

// END GENERATED CODE
