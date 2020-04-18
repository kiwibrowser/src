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

var jsdoc = require('./jsdoc');
var dom5 = require('dom5');
/** Properties on element prototypes that are purely configuration. */
var ELEMENT_CONFIGURATION = ['attached', 'attributeChanged', 'beforeRegister', 'configure', 'constructor', 'created', 'detached', 'enableCustomStyleProperties', 'extends', 'hostAttributes', 'is', 'listeners', 'mixins', 'properties', 'ready', 'registered'];
/** Tags understood by the annotation process, to be removed during `clean`. */
var HANDLED_TAGS = ['param', 'return', 'type'];
/**
 * Annotates Hydrolysis descriptors, processing any `desc` properties as JSDoc.
 *
 * You probably want to use a more specialized version of this, such as
 * `annotateElement`.
 *
 * Processed JSDoc values will be made available via the `jsdoc` property on a
 * descriptor node.
 *
 * @param {Object} descriptor The descriptor node to process.
 * @return {Object} The descriptor that was given.
 */
function annotate(descriptor) {
    if (!descriptor || descriptor.jsdoc) return descriptor;
    if (typeof descriptor.desc === 'string') {
        descriptor.jsdoc = jsdoc.parseJsdoc(descriptor.desc);
        // We want to present the normalized form of a descriptor.
        descriptor.jsdoc.orig = descriptor.desc;
        descriptor.desc = descriptor.jsdoc.description;
    }
    return descriptor;
}
exports.annotate = annotate;
/**
 * Annotates @event, @hero, & @demo tags
 */
function annotateElementHeader(descriptor) {
    if (descriptor.events) {
        descriptor.events.forEach(function (event) {
            _annotateEvent(event);
        });
        descriptor.events.sort(function (a, b) {
            return a.name.localeCompare(b.name);
        });
    }
    descriptor.demos = [];
    if (descriptor.jsdoc && descriptor.jsdoc.tags) {
        descriptor.jsdoc.tags.forEach(function (tag) {
            switch (tag.tag) {
                case 'hero':
                    descriptor.hero = tag.name || 'hero.png';
                    break;
                case 'demo':
                    descriptor.demos.push({
                        desc: tag.description || 'demo',
                        path: tag.name || 'demo/index.html'
                    });
                    break;
            }
        });
    }
}
exports.annotateElementHeader = annotateElementHeader;
function copyProperties(from, to, behaviorsByName) {
    if (from.properties) {
        from.properties.forEach(function (fromProp) {
            for (var toProp, i = 0; i < to.properties.length; i++) {
                toProp = to.properties[i];
                if (fromProp.name === toProp.name) {
                    return;
                }
            }
            var newProp = { __fromBehavior: from.is };
            if (fromProp.__fromBehavior) {
                return;
            }
            Object.keys(fromProp).forEach(function (propertyField) {
                newProp[propertyField] = fromProp[propertyField];
            });
            to.properties.push(newProp);
        });
        from.events.forEach(function (fromEvent) {
            for (var toEvent, i = 0; i < to.events.length; i++) {
                toEvent = to.events[i];
                if (fromEvent.name === toEvent.name) {
                    return;
                }
            }
            if (fromEvent.__fromBehavior) {
                return;
            }
            var newEvent = { __fromBehavior: from.is };
            Object.keys(fromEvent).forEach(function (eventField) {
                newEvent[eventField] = fromEvent[eventField];
            });
            to.events.push(newEvent);
        });
    }
    if (!from.behaviors) {
        return;
    }
    for (var i = from.behaviors.length - 1; i >= 0; i--) {
        // TODO: what's up with behaviors sometimes being a literal, and sometimes
        // being a descriptor object?
        var localBehavior = from.behaviors[i];
        var definedBehavior = behaviorsByName[localBehavior] || behaviorsByName[localBehavior.symbol];
        if (!definedBehavior) {
            console.warn("Behavior " + localBehavior + " not found when mixing " + "properties into " + to.is + "!");
            return;
        }
        copyProperties(definedBehavior, to, behaviorsByName);
    }
}
function mixinBehaviors(descriptor, behaviorsByName) {
    if (descriptor.behaviors) {
        for (var i = descriptor.behaviors.length - 1; i >= 0; i--) {
            var behavior = descriptor.behaviors[i];
            if (!behaviorsByName[behavior]) {
                console.warn("Behavior " + behavior + " not found when mixing " + "properties into " + descriptor.is + "!");
                break;
            }
            var definedBehavior = behaviorsByName[behavior];
            copyProperties(definedBehavior, descriptor, behaviorsByName);
        }
    }
}
/**
 * Annotates documentation found within a Hydrolysis element descriptor. Also
 * supports behaviors.
 *
 * If the element was processed via `hydrolize`, the element's documentation
 * will also be extracted via its <dom-module>.
 *
 * @param {Object} descriptor The element descriptor.
 * @return {Object} The descriptor that was given.
 */
function annotateElement(descriptor, behaviorsByName) {
    if (!descriptor.desc && descriptor.type === 'element') {
        descriptor.desc = _findElementDocs(descriptor.is, descriptor.domModule, descriptor.scriptElement);
    }
    annotate(descriptor);
    // The `<dom-module>` is too low level for most needs, and it is _not_
    // serializable. So we drop it now that we've extracted all the useful bits
    // from it.
    // TODO: Don't worry about serializability here, provide an API to get JSON.
    delete descriptor.domModule;
    mixinBehaviors(descriptor, behaviorsByName);
    // Descriptors that should have their `desc` properties parsed as JSDoc.
    descriptor.properties.forEach(function (property) {
        // Feature properties are special, configuration is really just a matter of
        // inheritance...
        annotateProperty(property, descriptor.abstract);
    });
    // It may seem like overkill to always sort, but we have an assumption that
    // these properties are typically being consumed by user-visible tooling.
    // As such, it's good to have consistent output/ordering to aid the user.
    descriptor.properties.sort(function (a, b) {
        // Private properties are always last.
        if (a.private && !b.private) {
            return 1;
        } else if (!a.private && b.private) {
            return -1;
        } else {
            return a.name.localeCompare(b.name);
        }
    });
    annotateElementHeader(descriptor);
    return descriptor;
}
exports.annotateElement = annotateElement;
/**
 * Annotates behavior descriptor.
 * @param {Object} descriptor behavior descriptor
 * @return {Object} descriptor passed in as param
 */
function annotateBehavior(descriptor) {
    annotate(descriptor);
    annotateElementHeader(descriptor);
    return descriptor;
}
exports.annotateBehavior = annotateBehavior;
/**
 * Annotates event documentation
 */
function _annotateEvent(descriptor) {
    annotate(descriptor);
    // process @event
    var eventTag = jsdoc.getTag(descriptor.jsdoc, 'event');
    descriptor.name = eventTag ? eventTag.description : "N/A";
    // process @params
    descriptor.params = (descriptor.jsdoc.tags || []).filter(function (tag) {
        return tag.tag === 'param';
    }).map(function (tag) {
        return {
            type: tag.type || "N/A",
            desc: tag.description,
            name: tag.name || "N/A"
        };
    });
    // process @params
    return descriptor;
}
/**
 * Annotates documentation found about a Hydrolysis property descriptor.
 *
 * @param {Object} descriptor The property descriptor.
 * @param {boolean} ignoreConfiguration If true, `configuration` is not set.
 * @return {Object} The descriptior that was given.
 */
function annotateProperty(descriptor, ignoreConfiguration) {
    annotate(descriptor);
    if (descriptor.name[0] === '_' || jsdoc.hasTag(descriptor.jsdoc, 'private')) {
        descriptor.private = true;
    }
    if (!ignoreConfiguration && ELEMENT_CONFIGURATION.indexOf(descriptor.name) !== -1) {
        descriptor.private = true;
        descriptor.configuration = true;
    }
    // @type JSDoc wins
    descriptor.type = jsdoc.getTag(descriptor.jsdoc, 'type', 'type') || descriptor.type;
    if (descriptor.type.match(/^function/i)) {
        _annotateFunctionProperty(descriptor);
    }
    // @default JSDoc wins
    var defaultTag = jsdoc.getTag(descriptor.jsdoc, 'default');
    if (defaultTag !== null) {
        var newDefault = (defaultTag.name || '') + (defaultTag.description || '');
        if (newDefault !== '') {
            descriptor.default = newDefault;
        }
    }
    return descriptor;
}
function _annotateFunctionProperty(descriptor) {
    descriptor.function = true;
    var returnTag = jsdoc.getTag(descriptor.jsdoc, 'return');
    if (returnTag) {
        descriptor.return = {
            type: returnTag.type,
            desc: returnTag.description
        };
    }
    var paramsByName = {};
    (descriptor.params || []).forEach(function (param) {
        paramsByName[param.name] = param;
    });
    (descriptor.jsdoc && descriptor.jsdoc.tags || []).forEach(function (tag) {
        if (tag.tag !== 'param') return;
        var param = paramsByName[tag.name];
        if (!param) {
            return;
        }
        param.type = tag.type || param.type;
        param.desc = tag.description;
    });
}
/**
 * Converts raw features into an abstract `Polymer.Base` element.
 *
 * Note that docs on this element _are not processed_. You must call
 * `annotateElement` on it yourself if you wish that.
 *
 * @param {Array<FeatureDescriptor>} features
 * @return {ElementDescriptor}
 */
function featureElement(features) {
    var properties = features.reduce(function (result, feature) {
        return result.concat(feature.properties);
    }, []);
    return {
        type: 'element',
        is: 'Polymer.Base',
        abstract: true,
        properties: properties,
        desc: '`Polymer.Base` acts as a base prototype for all Polymer ' + 'elements. It is composed via various calls to ' + '`Polymer.Base._addFeature()`.\n' + '\n' + 'The properties reflected here are the combined view of all ' + 'features found in this library. There may be more properties ' + 'added via other libraries, as well.'
    };
}
exports.featureElement = featureElement;
/**
 * Cleans redundant properties from a descriptor, assuming that you have already
 * called `annotate`.
 *
 * @param {Object} descriptor
 */
function clean(descriptor) {
    if (!descriptor.jsdoc) return;
    // The doctext was written to `descriptor.desc`
    delete descriptor.jsdoc.description;
    delete descriptor.jsdoc.orig;
    var cleanTags = [];
    (descriptor.jsdoc.tags || []).forEach(function (tag) {
        // Drop any tags we've consumed.
        if (HANDLED_TAGS.indexOf(tag.tag) !== -1) return;
        cleanTags.push(tag);
    });
    if (cleanTags.length === 0) {
        // No tags? no docs left!
        delete descriptor.jsdoc;
    } else {
        descriptor.jsdoc.tags = cleanTags;
    }
}
exports.clean = clean;
/**
 * Cleans redundant properties from an element, assuming that you have already
 * called `annotateElement`.
 *
 * @param {ElementDescriptor|BehaviorDescriptor} element
 */
function cleanElement(element) {
    clean(element);
    element.properties.forEach(cleanProperty);
}
exports.cleanElement = cleanElement;
/**
 * Cleans redundant properties from a property, assuming that you have already
 * called `annotateProperty`.
 *
 * @param {PropertyDescriptor} property
 */
function cleanProperty(property) {
    clean(property);
}
/**
 * Parse elements defined only in comments.
 * @param  {comments} Array<string> A list of comments to parse.
 * @return {ElementDescriptor}      A list of pseudo-elements.
 */
function parsePseudoElements(comments) {
    var elements = [];
    comments.forEach(function (comment) {
        var parsedJsdoc = jsdoc.parseJsdoc(comment);
        var pseudoTag = jsdoc.getTag(parsedJsdoc, 'pseudoElement', 'name');
        if (pseudoTag) {
            var element = {
                is: pseudoTag,
                type: 'element',
                jsdoc: { description: parsedJsdoc.description, tags: parsedJsdoc.tags },
                properties: [],
                desc: parsedJsdoc.description
            };
            annotateElementHeader(element);
            elements.push(element);
        }
    });
    return elements;
}
exports.parsePseudoElements = parsePseudoElements;
/**
 * @param {string} elementId
 * @param {DocumentAST} domModule
 * @param {DocumentAST} scriptElement The script that the element was
 *     defined in.
 */
function _findElementDocs(elementId, domModule, scriptElement) {
    // Note that we concatenate docs from all sources if we find them.
    // element can be defined in:
    // html comment right before dom-module
    // html commnet right before script defining the module,
    // if dom-module is empty
    var found = [];
    // Do we have a HTML comment on the `<dom-module>` or `<script>`?
    //
    // Confusingly, with our current style, the comment will be attached to
    // `<head>`, rather than being a sibling to the `<dom-module>`
    var searchRoot = domModule || scriptElement;
    var parents = dom5.nodeWalkAllPrior(searchRoot, dom5.isCommentNode);
    var comment = parents.length > 0 ? parents[0] : null;
    if (comment && comment.data) {
        found.push(comment.data);
    }
    if (found.length === 0) return null;
    return found.filter(function (comment) {
        // skip @license comments
        if (comment && comment.indexOf('@license') === -1) {
            return true;
        } else {
            return false;
        }
    }).map(jsdoc.unindent).join('\n');
}
function _findLastChildNamed(name, parent) {
    var children = parent.childNodes;
    for (var i = children.length - 1; i >= 0; i--) {
        var child = children[i];
        if (child.nodeName === name) return child;
    }
    return null;
}
// TODO(nevir): parse5-utils!
function _getNodeAttribute(node, name) {
    for (var i = 0; i < node.attrs.length; i++) {
        var attr = node.attrs[i];
        if (attr.name === name) {
            return attr.value;
        }
    }
}