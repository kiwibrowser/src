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
const jsdoc = require("../javascript/jsdoc");
const class_1 = require("./class");
const model_1 = require("./model");
const warning_1 = require("./warning");
/**
 * Base class for ScannedElement and ScannedElementMixin.
 */
class ScannedElementBase {
    constructor() {
        this.properties = new Map();
        this.attributes = new Map();
        this.description = '';
        this.summary = '';
        this.demos = [];
        this.events = new Map();
        this.warnings = [];
        this['slots'] = [];
        this.mixins = [];
        this.abstract = false;
        this.superClass = undefined;
    }
    applyHtmlComment(commentText, containingDocument) {
        if (commentText && containingDocument) {
            const commentJsdoc = jsdoc.parseJsdoc(commentText);
            // Add a Warning if there are already jsdoc tags or a description for this
            // element.
            if (this.sourceRange &&
                (this.description || this.jsdoc && this.jsdoc.tags.length > 0)) {
                this.warnings.push(new model_1.Warning({
                    severity: warning_1.Severity.WARNING,
                    code: 'multiple-doc-comments',
                    message: `${this.constructor.name} has both HTML doc and JSDoc comments.`,
                    sourceRange: this.sourceRange,
                    parsedDocument: containingDocument
                }));
            }
            this.jsdoc =
                this.jsdoc ? jsdoc.join(commentJsdoc, this.jsdoc) : commentJsdoc;
            this.description = [
                commentJsdoc.description || '',
                this.description || ''
            ].join('\n\n').trim();
        }
    }
    resolve(_document) {
        throw new Error('abstract');
    }
}
exports.ScannedElementBase = ScannedElementBase;
class Slot {
    constructor(name, range) {
        this.name = name;
        this.range = range;
    }
}
exports.Slot = Slot;
/**
 * Base class for Element and ElementMixin.
 */
class ElementBase extends class_1.Class {
    constructor(init, document) {
        super(init, document);
        this['slots'] = [];
        const { events, attributes, slots = [], } = init;
        this.slots = Array.from(slots);
        // Initialization of these attributes is kinda awkward, as they're part
        // of the inheritance system. See `inheritFrom` below which *may* be
        // called by our superclass, but may not be.
        this.attributes = this.attributes || new Map();
        this.events = this.events || new Map();
        if (attributes !== undefined) {
            this._overwriteInherited(this.attributes, attributes, undefined, true);
        }
        if (events !== undefined) {
            this._overwriteInherited(this.events, events, undefined, true);
        }
    }
    inheritFrom(superClass) {
        // This may run as part of the call to the super constructor, so we need
        // to validate initialization.
        this.attributes = this.attributes || new Map();
        this.events = this.events || new Map();
        super.inheritFrom(superClass);
        if (superClass instanceof ElementBase) {
            this._overwriteInherited(this.attributes, superClass.attributes, superClass.name);
            this._overwriteInherited(this.events, superClass.events, superClass.name);
        }
        // TODO(justinfagnani): slots, listeners, observers, dom-module?
        // What actually inherits?
    }
    emitAttributeMetadata(_attribute) {
        return {};
    }
    emitEventMetadata(_event) {
        return {};
    }
}
exports.ElementBase = ElementBase;

//# sourceMappingURL=element-base.js.map
