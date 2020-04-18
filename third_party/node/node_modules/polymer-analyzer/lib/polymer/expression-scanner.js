"use strict";
/**
 * @license
 * Copyright (c) 2017 The Polymer Project Authors. All rights reserved.
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
const dom5 = require("dom5");
const javascript_parser_1 = require("../javascript/javascript-parser");
const model_1 = require("../model/model");
const p = dom5.predicates;
const isTemplate = p.hasTagName('template');
const isDataBindingTemplate = p.AND(isTemplate, p.OR(p.hasAttrValue('is', 'dom-bind'), p.hasAttrValue('is', 'dom-if'), p.hasAttrValue('is', 'dom-repeat'), p.parentMatches(p.OR(p.hasTagName('dom-bind'), p.hasTagName('dom-if'), p.hasTagName('dom-repeat'), p.hasTagName('dom-module')))));
/**
 * Given a node, return all databinding templates inside it.
 *
 * A template is "databinding" if polymer databinding expressions are expected
 * to be evaluated inside. e.g. <template is='dom-if'> or <dom-module><template>
 *
 * Results include both direct and nested templates (e.g. dom-if inside
 * dom-module).
 */
function getAllDataBindingTemplates(node) {
    return dom5.queryAll(node, isDataBindingTemplate, [], dom5.childNodesIncludeTemplate);
}
exports.getAllDataBindingTemplates = getAllDataBindingTemplates;
class DatabindingExpression {
    constructor(sourceRange, expressionText, ast, limitation, document) {
        this.warnings = [];
        /**
         * Toplevel properties on the model that are referenced in this expression.
         *
         * e.g. in {{foo(bar, baz.zod)}} the properties are foo, bar, and baz
         * (but not zod).
         */
        this.properties = [];
        this.sourceRange = sourceRange;
        this.expressionText = expressionText;
        this._expressionAst = ast;
        this.locationOffset = {
            line: sourceRange.start.line,
            col: sourceRange.start.column
        };
        this._document = document;
        this._extractPropertiesAndValidate(limitation);
    }
    /**
     * Given an estree node in this databinding expression, give its source range.
     */
    sourceRangeForNode(node) {
        if (!node || !node.loc) {
            return;
        }
        const databindingRelativeSourceRange = {
            file: this.sourceRange.file,
            // Note: estree uses 1-indexed lines, but SourceRange uses 0 indexed.
            start: { line: (node.loc.start.line - 1), column: node.loc.start.column },
            end: { line: (node.loc.end.line - 1), column: node.loc.end.column }
        };
        return model_1.correctSourceRange(databindingRelativeSourceRange, this.locationOffset);
    }
    _extractPropertiesAndValidate(limitation) {
        if (this._expressionAst.body.length !== 1) {
            this.warnings.push(this._validationWarning(`Expected one expression, got ${this._expressionAst.body.length}`, this._expressionAst));
            return;
        }
        const expressionStatement = this._expressionAst.body[0];
        if (expressionStatement.type !== 'ExpressionStatement') {
            this.warnings.push(this._validationWarning(`Expect an expression, not a ${expressionStatement.type}`, expressionStatement));
            return;
        }
        let expression = expressionStatement.expression;
        this._validateLimitation(expression, limitation);
        if (expression.type === 'UnaryExpression' && expression.operator === '!') {
            expression = expression.argument;
        }
        this._extractAndValidateSubExpression(expression, true);
    }
    _validateLimitation(expression, limitation) {
        switch (limitation) {
            case 'identifierOnly':
                if (expression.type !== 'Identifier') {
                    this.warnings.push(this._validationWarning(`Expected just a name here, not an expression`, expression));
                }
                break;
            case 'callExpression':
                if (expression.type !== 'CallExpression') {
                    this.warnings.push(this._validationWarning(`Expected a function call here.`, expression));
                }
                break;
            case 'full':
                break; // no checks needed
            default:
                const never = limitation;
                throw new Error(`Got unknown limitation: ${never}`);
        }
    }
    _extractAndValidateSubExpression(expression, callAllowed) {
        if (expression.type === 'UnaryExpression' && expression.operator === '-') {
            if (expression.argument.type !== 'Literal' ||
                typeof expression.argument.value !== 'number') {
                this.warnings.push(this._validationWarning('The - operator is only supported for writing negative numbers.', expression));
                return;
            }
            this._extractAndValidateSubExpression(expression.argument, false);
            return;
        }
        if (expression.type === 'Literal') {
            return;
        }
        if (expression.type === 'Identifier') {
            this.properties.push({
                name: expression.name,
                sourceRange: this.sourceRangeForNode(expression)
            });
            return;
        }
        if (expression.type === 'MemberExpression') {
            this._extractAndValidateSubExpression(expression.object, false);
            return;
        }
        if (callAllowed && expression.type === 'CallExpression') {
            this._extractAndValidateSubExpression(expression.callee, false);
            for (const arg of expression.arguments) {
                this._extractAndValidateSubExpression(arg, false);
            }
            return;
        }
        this.warnings.push(this._validationWarning(`Only simple syntax is supported in Polymer databinding expressions. ` +
            `${expression.type} not expected here.`, expression));
    }
    _validationWarning(message, node) {
        return new model_1.Warning({
            code: 'invalid-polymer-expression',
            message,
            sourceRange: this.sourceRangeForNode(node),
            severity: model_1.Severity.WARNING,
            parsedDocument: this._document,
        });
    }
}
exports.DatabindingExpression = DatabindingExpression;
class AttributeDatabindingExpression extends DatabindingExpression {
    constructor(astNode, isCompleteBinding, direction, eventName, attribute, sourceRange, expressionText, ast, document) {
        super(sourceRange, expressionText, ast, 'full', document);
        this.databindingInto = 'attribute';
        this.astNode = astNode;
        this.isCompleteBinding = isCompleteBinding;
        this.direction = direction;
        this.eventName = eventName;
        this.attribute = attribute;
    }
}
exports.AttributeDatabindingExpression = AttributeDatabindingExpression;
class TextNodeDatabindingExpression extends DatabindingExpression {
    constructor(direction, astNode, sourceRange, expressionText, ast, document) {
        super(sourceRange, expressionText, ast, 'full', document);
        this.databindingInto = 'text-node';
        this.direction = direction;
        this.astNode = astNode;
    }
}
exports.TextNodeDatabindingExpression = TextNodeDatabindingExpression;
class JavascriptDatabindingExpression extends DatabindingExpression {
    constructor(astNode, sourceRange, expressionText, ast, kind, document) {
        super(sourceRange, expressionText, ast, kind, document);
        this.databindingInto = 'javascript';
        this.astNode = astNode;
    }
}
exports.JavascriptDatabindingExpression = JavascriptDatabindingExpression;
/**
 * Find and parse Polymer databinding expressions in HTML.
 */
function scanDocumentForExpressions(document) {
    return extractDataBindingsFromTemplates(document, getAllDataBindingTemplates(document.ast));
}
exports.scanDocumentForExpressions = scanDocumentForExpressions;
function scanDatabindingTemplateForExpressions(document, template) {
    return extractDataBindingsFromTemplates(document, [template].concat(getAllDataBindingTemplates(template.content)));
}
exports.scanDatabindingTemplateForExpressions = scanDatabindingTemplateForExpressions;
function extractDataBindingsFromTemplates(document, templates) {
    const results = [];
    const warnings = [];
    for (const template of templates) {
        dom5.nodeWalkAll(template.content, (node) => {
            if (dom5.isTextNode(node) && node.value) {
                extractDataBindingsFromTextNode(document, node, results, warnings);
            }
            if (node.attrs) {
                for (const attr of node.attrs) {
                    extractDataBindingsFromAttr(document, node, attr, results, warnings);
                }
            }
            return false;
        });
    }
    return { expressions: results, warnings };
}
function extractDataBindingsFromTextNode(document, node, results, warnings) {
    const text = node.value || '';
    const dataBindings = findDatabindingInString(text);
    if (dataBindings.length === 0) {
        return;
    }
    const nodeSourceRange = document.sourceRangeForNode(node);
    if (!nodeSourceRange) {
        return;
    }
    const startOfTextNodeOffset = document.sourcePositionToOffset(nodeSourceRange.start);
    for (const dataBinding of dataBindings) {
        const sourceRange = document.offsetsToSourceRange(dataBinding.startIndex + startOfTextNodeOffset, dataBinding.endIndex + startOfTextNodeOffset);
        const parseResult = parseExpression(dataBinding.expressionText, sourceRange);
        if (!parseResult) {
            continue;
        }
        if (parseResult.type === 'failure') {
            warnings.push(new model_1.Warning(Object.assign({ parsedDocument: document }, parseResult.warning)));
        }
        else {
            const expression = new TextNodeDatabindingExpression(dataBinding.direction, node, sourceRange, dataBinding.expressionText, parseResult.program, document);
            for (const warning of expression.warnings) {
                warnings.push(warning);
            }
            results.push(expression);
        }
    }
}
function extractDataBindingsFromAttr(document, node, attr, results, warnings) {
    if (!attr.value) {
        return;
    }
    const dataBindings = findDatabindingInString(attr.value);
    const attributeValueRange = document.sourceRangeForAttributeValue(node, attr.name, true);
    if (!attributeValueRange) {
        return;
    }
    const attributeOffset = document.sourcePositionToOffset(attributeValueRange.start);
    for (const dataBinding of dataBindings) {
        const isFullAttributeBinding = dataBinding.startIndex === 2 &&
            dataBinding.endIndex + 2 === attr.value.length;
        let expressionText = dataBinding.expressionText;
        let eventName = undefined;
        if (dataBinding.direction === '{') {
            const match = expressionText.match(/(.*)::(.*)/);
            if (match) {
                expressionText = match[1];
                eventName = match[2];
            }
        }
        const sourceRange = document.offsetsToSourceRange(dataBinding.startIndex + attributeOffset, dataBinding.endIndex + attributeOffset);
        const parseResult = parseExpression(expressionText, sourceRange);
        if (!parseResult) {
            continue;
        }
        if (parseResult.type === 'failure') {
            warnings.push(new model_1.Warning(Object.assign({ parsedDocument: document }, parseResult.warning)));
        }
        else {
            const expression = new AttributeDatabindingExpression(node, isFullAttributeBinding, dataBinding.direction, eventName, attr, sourceRange, expressionText, parseResult.program, document);
            for (const warning of expression.warnings) {
                warnings.push(warning);
            }
            results.push(expression);
        }
    }
}
function findDatabindingInString(str) {
    const expressions = [];
    const openers = /{{|\[\[/g;
    let match;
    while (match = openers.exec(str)) {
        const matchedOpeners = match[0];
        const startIndex = match.index + 2;
        const direction = matchedOpeners === '{{' ? '{' : '[';
        const closers = matchedOpeners === '{{' ? '}}' : ']]';
        const endIndex = str.indexOf(closers, startIndex);
        if (endIndex === -1) {
            // No closers, this wasn't an expression after all.
            break;
        }
        const expressionText = str.slice(startIndex, endIndex);
        expressions.push({ startIndex, endIndex, expressionText, direction });
        // Start looking for the next expression after the end of this one.
        openers.lastIndex = endIndex + 2;
    }
    return expressions;
}
function parseExpression(content, expressionSourceRange) {
    const expressionOffset = {
        line: expressionSourceRange.start.line,
        col: expressionSourceRange.start.column
    };
    const parseResult = javascript_parser_1.parseJs(content, expressionSourceRange.file, expressionOffset, 'polymer-expression-parse-error');
    if (parseResult.type === 'success') {
        return parseResult;
    }
    // The polymer databinding expression language allows for foo.0 and foo.*
    // formats when accessing sub properties. These aren't valid JS, but we don't
    // want to warn for them either. So just return undefined for now.
    if (/\.(\*|\d+)/.test(content)) {
        return undefined;
    }
    return parseResult;
}
function parseExpressionInJsStringLiteral(document, stringLiteral, kind) {
    const warnings = [];
    const result = {
        databinding: undefined,
        warnings
    };
    const sourceRangeForLiteral = document.sourceRangeForNode(stringLiteral);
    if (stringLiteral.type !== 'Literal') {
        // Should we warn here? It's potentially valid, just unanalyzable. Maybe
        // just an info that someone could escalate to a warning/error?
        warnings.push(new model_1.Warning({
            code: 'unanalyzable-polymer-expression',
            message: `Can only analyze databinding expressions in string literals.`,
            severity: model_1.Severity.INFO,
            sourceRange: sourceRangeForLiteral,
            parsedDocument: document
        }));
        return result;
    }
    const expressionText = stringLiteral.value;
    if (typeof expressionText !== 'string') {
        warnings.push(new model_1.Warning({
            code: 'invalid-polymer-expression',
            message: `Expected a string, got a ${typeof expressionText}.`,
            sourceRange: sourceRangeForLiteral,
            severity: model_1.Severity.WARNING,
            parsedDocument: document
        }));
        return result;
    }
    const sourceRange = {
        file: sourceRangeForLiteral.file,
        start: {
            column: sourceRangeForLiteral.start.column + 1,
            line: sourceRangeForLiteral.start.line
        },
        end: {
            column: sourceRangeForLiteral.end.column - 1,
            line: sourceRangeForLiteral.end.line
        }
    };
    const parsed = parseExpression(expressionText, sourceRange);
    if (parsed && parsed.type === 'failure') {
        warnings.push(new model_1.Warning(Object.assign({ parsedDocument: document }, parsed.warning)));
    }
    else if (parsed && parsed.type === 'success') {
        result.databinding = new JavascriptDatabindingExpression(stringLiteral, sourceRange, expressionText, parsed.program, kind, document);
        for (const warning of result.databinding.warnings) {
            warnings.push(warning);
        }
    }
    return result;
}
exports.parseExpressionInJsStringLiteral = parseExpressionInJsStringLiteral;

//# sourceMappingURL=expression-scanner.js.map
