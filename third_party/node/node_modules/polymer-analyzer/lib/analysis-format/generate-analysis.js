"use strict";
/**
 * @license
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
const fs = require("fs");
const jsonschema = require("jsonschema");
const pathLib = require("path");
function generateAnalysis(input, packagePath, filter) {
    const _filter = filter || ((_) => true);
    const members = {
        elements: new Set(iFilter(input.getFeatures({ kind: 'element' }), _filter)),
        mixins: new Set(iFilter(input.getFeatures({ kind: 'element-mixin' }), _filter)),
        namespaces: new Set(iFilter(input.getFeatures({ kind: 'namespace' }), _filter)),
        functions: new Set(iFilter(input.getFeatures({ kind: 'function' }), _filter)),
        polymerBehaviors: new Set(iFilter(input.getFeatures({ kind: 'behavior' }), _filter)),
        classes: new Set()
    };
    const allClasses = iFilter(input.getFeatures({ kind: 'class' }), _filter);
    for (const class_ of allClasses) {
        if (members.elements.has(class_) ||
            members.mixins.has(class_) ||
            members.polymerBehaviors.has(class_)) {
            continue;
        }
        members.classes.add(class_);
    }
    return buildAnalysis(members, packagePath);
}
exports.generateAnalysis = generateAnalysis;
function buildAnalysis(members, packagePath) {
    // Build mapping of namespaces
    const namespaces = new Map();
    for (const namespace of members.namespaces) {
        namespaces.set(namespace.name, serializeNamespace(namespace, packagePath));
    }
    const analysis = {
        schema_version: '1.0.0',
    };
    for (const namespace of namespaces.values()) {
        const namespaceName = getNamespaceName(namespace.name);
        const parent = namespaces.get(namespaceName) || analysis;
        parent.namespaces = parent.namespaces || [];
        parent.namespaces.push(namespace);
    }
    for (const element of members.elements) {
        const namespaceName = getNamespaceName(element.className);
        const namespace = namespaces.get(namespaceName) || analysis;
        namespace.elements = namespace.elements || [];
        namespace.elements.push(serializeElement(element, packagePath));
    }
    for (const mixin of members.mixins) {
        const namespaceName = getNamespaceName(mixin.name);
        const namespace = namespaces.get(namespaceName) || analysis;
        namespace.mixins = namespace.mixins || [];
        namespace.mixins.push(serializeElementMixin(mixin, packagePath));
    }
    for (const function_ of members.functions) {
        const namespaceName = getNamespaceName(function_.name);
        const namespace = namespaces.get(namespaceName) || analysis;
        namespace.functions = namespace.functions || [];
        namespace.functions.push(serializeFunction(function_, packagePath));
    }
    // TODO(usergenic): Consider moving framework-specific code to separate file.
    for (const behavior of members.polymerBehaviors) {
        const namespaceName = getNamespaceName(behavior.className);
        const namespace = namespaces.get(namespaceName) || analysis;
        namespace.metadata = namespace.metadata || {};
        namespace.metadata.polymer = namespace.metadata.polymer || {};
        namespace.metadata.polymer.behaviors =
            namespace.metadata.polymer.behaviors || [];
        namespace.metadata.polymer.behaviors.push(serializePolymerBehaviorAsElementMixin(behavior, packagePath));
    }
    for (const class_ of members.classes) {
        const namespaceName = getNamespaceName(class_.name);
        const namespace = namespaces.get(namespaceName) || analysis;
        namespace.classes = namespace.classes || [];
        namespace.classes.push(serializeClass(class_, packagePath));
    }
    return analysis;
}
function getNamespaceName(name) {
    if (name == null) {
        return undefined;
    }
    const lastDotIndex = name.lastIndexOf('.');
    if (lastDotIndex === -1) {
        return undefined;
    }
    return name.substring(0, lastDotIndex);
}
const validator = new jsonschema.Validator();
const schema = JSON.parse(fs.readFileSync(pathLib.join(__dirname, '../', 'analysis.schema.json'), 'utf-8'));
class ValidationError extends Error {
    constructor(result) {
        const message = `Unable to validate serialized Polymer analysis. ` +
            `Got ${result.errors.length} errors: ` +
            `${result.errors.map((err) => '    ' + (err.message || err)).join('\n')}`;
        super(message);
        this.errors = result.errors;
    }
}
exports.ValidationError = ValidationError;
/**
 * Throws if the given object isn't a valid AnalyzedPackage according to
 * the JSON schema.
 */
function validateAnalysis(analyzedPackage) {
    const result = validator.validate(analyzedPackage, schema);
    if (result.throwError) {
        throw result.throwError;
    }
    if (result.errors.length > 0) {
        throw new ValidationError(result);
    }
    if (!/^1\.\d+\.\d+$/.test(analyzedPackage.schema_version)) {
        throw new Error(`Invalid schema_version in AnalyzedPackage. ` +
            `Expected 1.x.x, got ${analyzedPackage.schema_version}`);
    }
}
exports.validateAnalysis = validateAnalysis;
function serializeNamespace(namespace, packagePath) {
    const packageRelativePath = pathLib.relative(packagePath, namespace.sourceRange.file);
    const metadata = {
        name: namespace.name,
        description: namespace.description,
        summary: namespace.summary,
        sourceRange: {
            file: packageRelativePath,
            start: namespace.sourceRange.start,
            end: namespace.sourceRange.end
        }
    };
    return metadata;
}
function serializeFunction(fn, packagePath) {
    const packageRelativePath = pathLib.relative(packagePath, fn.sourceRange.file);
    const metadata = {
        name: fn.name,
        description: fn.description,
        summary: fn.summary,
        sourceRange: {
            file: packageRelativePath,
            start: fn.sourceRange.start,
            end: fn.sourceRange.end
        },
        privacy: fn.privacy,
    };
    if (fn.params) {
        metadata.params = fn.params;
    }
    if (fn.return) {
        metadata.return = fn.return;
    }
    return metadata;
}
function serializeClass(class_, packagePath) {
    const path = class_.sourceRange.file;
    const packageRelativePath = pathLib.relative(packagePath, class_.sourceRange.file);
    const properties = [...class_.properties.values()].map((p) => serializeProperty(class_, path, p));
    const methods = [...class_.methods.values()].map((m) => serializeMethod(class_, path, m));
    const staticMethods = [...class_.staticMethods.values()].map((m) => serializeMethod(class_, path, m));
    const serialized = {
        description: class_.description || '',
        summary: class_.summary || '',
        path: packageRelativePath,
        properties: properties,
        methods: methods,
        staticMethods: staticMethods,
        demos: (class_.demos ||
            []).map(({ path, desc }) => ({ url: path, description: desc || '' })),
        metadata: class_.emitMetadata(),
        sourceRange: resolveSourceRangePath(path, class_.sourceRange),
        privacy: class_.privacy,
        superclass: class_.superClass ? class_.superClass.identifier : undefined,
    };
    if (class_.name) {
        serialized.name = class_.name;
    }
    return serialized;
}
function serializeElementLike(elementOrMixin, packagePath) {
    const class_ = serializeClass(elementOrMixin, packagePath);
    const path = elementOrMixin.sourceRange.file;
    class_.attributes =
        Array.from(elementOrMixin.attributes.values())
            .map((a) => serializeAttribute(elementOrMixin, path, a));
    class_.events = Array.from(elementOrMixin.events.values())
        .map((e) => serializeEvent(elementOrMixin, path, e));
    Object.assign(class_, {
        styling: {
            cssVariables: [],
            selectors: [],
        },
        slots: elementOrMixin.slots.map((s) => {
            return { description: '', name: s.name, range: s.range };
        }),
    });
    return class_;
}
function serializeElement(element, packagePath) {
    const metadata = serializeElementLike(element, packagePath);
    metadata.tagname = element.tagName;
    // TODO(justinfagnani): Mixins should be able to have mixins too
    if (element.mixins.length > 0) {
        metadata.mixins = element.mixins.map((m) => m.identifier);
    }
    metadata.superclass = 'HTMLElement';
    return metadata;
}
function serializeElementMixin(mixin, packagePath) {
    const metadata = serializeElementLike(mixin, packagePath);
    metadata.name = mixin.name;
    metadata.privacy = mixin.privacy;
    if (mixin.mixins.length > 0) {
        metadata.mixins = mixin.mixins.map((m) => m.identifier);
    }
    return metadata;
}
function serializePolymerBehaviorAsElementMixin(behavior, packagePath) {
    const metadata = serializeElementLike(behavior, packagePath);
    metadata.name = behavior.className;
    metadata.privacy = behavior.privacy;
    if (behavior.mixins.length > 0) {
        metadata.mixins = behavior.mixins.map((m) => m.identifier);
    }
    return metadata;
}
function serializeProperty(class_, elementPath, resolvedProperty) {
    const property = {
        name: resolvedProperty.name,
        type: resolvedProperty.type || '?',
        description: resolvedProperty.description || '',
        privacy: resolvedProperty.privacy,
        sourceRange: resolveSourceRangePath(elementPath, resolvedProperty.sourceRange),
        metadata: class_.emitPropertyMetadata(resolvedProperty),
    };
    if (resolvedProperty.default) {
        property.defaultValue = resolvedProperty.default;
    }
    if (resolvedProperty.inheritedFrom) {
        property.inheritedFrom = resolvedProperty.inheritedFrom;
    }
    return property;
}
function serializeAttribute(resolvedElement, elementPath, resolvedAttribute) {
    const attribute = {
        name: resolvedAttribute.name,
        description: resolvedAttribute.description || '',
        sourceRange: resolveSourceRangePath(elementPath, resolvedAttribute.sourceRange),
        metadata: resolvedElement.emitAttributeMetadata(resolvedAttribute),
    };
    if (resolvedAttribute.type) {
        attribute.type = resolvedAttribute.type;
    }
    if (resolvedAttribute.inheritedFrom != null) {
        attribute.inheritedFrom = resolvedAttribute.inheritedFrom;
    }
    return attribute;
}
function serializeMethod(class_, elementPath, resolvedMethod) {
    const method = {
        name: resolvedMethod.name,
        description: resolvedMethod.description || '',
        privacy: resolvedMethod.privacy,
        sourceRange: resolveSourceRangePath(elementPath, resolvedMethod.sourceRange),
        metadata: class_.emitMethodMetadata(resolvedMethod),
    };
    if (resolvedMethod.params) {
        method.params = Array.from(resolvedMethod.params);
    }
    if (resolvedMethod.return) {
        method.return = resolvedMethod.return;
    }
    if (resolvedMethod.inheritedFrom != null) {
        method.inheritedFrom = resolvedMethod.inheritedFrom;
    }
    return method;
}
function serializeEvent(resolvedElement, _elementPath, resolvedEvent) {
    const event = {
        type: 'CustomEvent',
        name: resolvedEvent.name,
        description: resolvedEvent.description || '',
        metadata: resolvedElement.emitEventMetadata(resolvedEvent),
    };
    if (resolvedEvent.inheritedFrom != null) {
        event.inheritedFrom = resolvedEvent.inheritedFrom;
    }
    return event;
}
function resolveSourceRangePath(elementPath, sourceRange) {
    if (!sourceRange) {
        return;
    }
    if (!sourceRange.file) {
        return sourceRange;
    }
    if (elementPath === sourceRange.file) {
        return { start: sourceRange.start, end: sourceRange.end };
    }
    // The source location's path is relative to file resolver's base, so first
    // we need to make it relative to the element.
    const filePath = pathLib.relative(pathLib.dirname(elementPath), sourceRange.file);
    return { file: filePath, start: sourceRange.start, end: sourceRange.end };
}
// TODO(rictic): figure out why type inference goes wrong with more general
//     types here.
function* iFilter(iter, f) {
    for (const val of iter) {
        if (f(val)) {
            yield val;
        }
    }
}

//# sourceMappingURL=generate-analysis.js.map
