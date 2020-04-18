"use strict";
/**
 * @license
 * Copyright (c) 2016 The Polymer Project Authors. All rights reserved.
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
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : new P(function (resolve) { resolve(result.value); }).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
Object.defineProperty(exports, "__esModule", { value: true });
const dom5 = require("dom5");
const parse5_1 = require("parse5");
const model_1 = require("../model/model");
const expression_scanner_1 = require("./expression-scanner");
const polymer_element_1 = require("./polymer-element");
const p = dom5.predicates;
const isDomModule = p.hasTagName('dom-module');
class ScannedDomModule {
    constructor(id, node, sourceRange, ast, warnings, template, slots, localIds, databindings) {
        this.warnings = [];
        this.id = id;
        this.node = node;
        this.comment = model_1.getAttachedCommentText(node);
        this.sourceRange = sourceRange;
        this.astNode = ast;
        this.slots = slots;
        this.localIds = localIds;
        this.warnings = warnings;
        this.template = template;
        this.databindings = databindings;
    }
    resolve() {
        return new DomModule(this.node, this.id, this.comment, this.sourceRange, this.astNode, this.warnings, this.slots, this.localIds, this.template, this.databindings);
    }
}
exports.ScannedDomModule = ScannedDomModule;
class DomModule {
    constructor(node, id, comment, sourceRange, ast, warnings, slots, localIds, template, databindings) {
        this.kinds = new Set(['dom-module']);
        this.identifiers = new Set();
        this.node = node;
        this.id = id;
        this.comment = comment;
        if (id) {
            this.identifiers.add(id);
        }
        this.sourceRange = sourceRange;
        this.astNode = ast;
        this.warnings = warnings;
        this.slots = slots;
        this.localIds = localIds;
        this.template = template;
        this.databindings = databindings;
    }
}
exports.DomModule = DomModule;
class DomModuleScanner {
    scan(document, visit) {
        return __awaiter(this, void 0, void 0, function* () {
            const domModules = [];
            yield visit((node) => {
                if (isDomModule(node)) {
                    const children = dom5.defaultChildNodes(node) || [];
                    const template = children.find(dom5.predicates.hasTagName('template'));
                    let slots = [];
                    let localIds = [];
                    let databindings = [];
                    let warnings = [];
                    if (template) {
                        const templateContent = parse5_1.treeAdapters.default.getTemplateContent(template);
                        slots =
                            dom5.queryAll(templateContent, dom5.predicates.hasTagName('slot'))
                                .map((s) => new model_1.Slot(dom5.getAttribute(s, 'name') || '', document.sourceRangeForNode(s)));
                        localIds =
                            dom5.queryAll(templateContent, dom5.predicates.hasAttr('id'))
                                .map((e) => new polymer_element_1.LocalId(dom5.getAttribute(e, 'id'), document.sourceRangeForNode(e)));
                        const results = expression_scanner_1.scanDatabindingTemplateForExpressions(document, template);
                        warnings = results.warnings;
                        databindings = results.expressions;
                    }
                    domModules.push(new ScannedDomModule(dom5.getAttribute(node, 'id'), node, document.sourceRangeForNode(node), node, warnings, template, slots, localIds, databindings));
                }
            });
            return { features: domModules };
        });
    }
}
exports.DomModuleScanner = DomModuleScanner;

//# sourceMappingURL=dom-module-scanner.js.map
