"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const model_1 = require("../model/model");
const polymer_element_1 = require("./polymer-element");
class ScannedPolymerElementMixin extends model_1.ScannedElementMixin {
    constructor({ name, jsdoc, description, summary, privacy, sourceRange, mixins, astNode, classAstNode }) {
        super({ name });
        this.properties = new Map();
        this.methods = new Map();
        this.staticMethods = new Map();
        this.observers = [];
        this.listeners = [];
        this.behaviorAssignments = [];
        // FIXME(rictic): domModule and scriptElement aren't known at a file local
        //     level. Remove them here, they should only exist on PolymerElement.
        this.domModule = undefined;
        this.scriptElement = undefined;
        this.pseudo = false;
        this.abstract = false;
        this.jsdoc = jsdoc;
        this.description = description;
        this.summary = summary;
        this.privacy = privacy;
        this.sourceRange = sourceRange;
        this.mixins = mixins;
        this.astNode = astNode;
        this.classAstNode = classAstNode;
    }
    addProperty(prop) {
        polymer_element_1.addProperty(this, prop);
    }
    addMethod(method) {
        polymer_element_1.addMethod(this, method);
    }
    resolve(document) {
        return new PolymerElementMixin(this, document);
    }
}
exports.ScannedPolymerElementMixin = ScannedPolymerElementMixin;
class PolymerElementMixin extends model_1.ElementMixin {
    constructor(scannedMixin, document) {
        super(scannedMixin, document);
        this.behaviorAssignments = [];
        this.localIds = [];
        this.kinds.add('polymer-element-mixin');
        this.domModule = scannedMixin.domModule;
        this.pseudo = scannedMixin.pseudo;
        this.scriptElement = scannedMixin.scriptElement;
        this.behaviorAssignments = Array.from(scannedMixin.behaviorAssignments);
        this.observers = Array.from(scannedMixin.observers);
    }
    emitPropertyMetadata(property) {
        const polymerMetadata = {};
        const polymerMetadataFields = ['notify', 'observer', 'readOnly'];
        for (const field of polymerMetadataFields) {
            if (field in property) {
                polymerMetadata[field] = property[field];
            }
        }
        return { polymer: polymerMetadata };
    }
    _getSuperclassAndMixins(document, init) {
        const prototypeChain = super._getSuperclassAndMixins(document, init);
        const { warnings, behaviors } = polymer_element_1.getBehaviors(init.behaviorAssignments, document);
        this.warnings.push(...warnings);
        prototypeChain.push(...behaviors);
        return prototypeChain;
    }
}
exports.PolymerElementMixin = PolymerElementMixin;

//# sourceMappingURL=polymer-element-mixin.js.map
