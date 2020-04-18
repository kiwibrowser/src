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
const ts = require("typescript");
class Visitor {
    visitNode(node) {
        switch (node.kind) {
            case ts.SyntaxKind.AnyKeyword:
                this.visitAnyKeyword(node);
                break;
            case ts.SyntaxKind.ArrayBindingPattern:
                this.visitBindingPattern(node);
                break;
            case ts.SyntaxKind.ArrayLiteralExpression:
                this.visitArrayLiteralExpression(node);
                break;
            case ts.SyntaxKind.ArrayType:
                this.visitArrayType(node);
                break;
            case ts.SyntaxKind.ArrowFunction:
                this.visitArrowFunction(node);
                break;
            case ts.SyntaxKind.BinaryExpression:
                this.visitBinaryExpression(node);
                break;
            case ts.SyntaxKind.BindingElement:
                this.visitBindingElement(node);
                break;
            case ts.SyntaxKind.Block:
                this.visitBlock(node);
                break;
            case ts.SyntaxKind.BreakStatement:
                this.visitBreakStatement(node);
                break;
            case ts.SyntaxKind.CallExpression:
                this.visitCallExpression(node);
                break;
            case ts.SyntaxKind.CallSignature:
                this.visitCallSignature(node);
                break;
            case ts.SyntaxKind.CaseClause:
                this.visitCaseClause(node);
                break;
            case ts.SyntaxKind.ClassDeclaration:
                this.visitClassDeclaration(node);
                break;
            case ts.SyntaxKind.ClassExpression:
                this.visitClassExpression(node);
                break;
            case ts.SyntaxKind.CatchClause:
                this.visitCatchClause(node);
                break;
            case ts.SyntaxKind.ConditionalExpression:
                this.visitConditionalExpression(node);
                break;
            case ts.SyntaxKind.ConstructSignature:
                this.visitConstructSignature(node);
                break;
            case ts.SyntaxKind.Constructor:
                this.visitConstructorDeclaration(node);
                break;
            case ts.SyntaxKind.ConstructorType:
                this.visitConstructorType(node);
                break;
            case ts.SyntaxKind.ContinueStatement:
                this.visitContinueStatement(node);
                break;
            case ts.SyntaxKind.DebuggerStatement:
                this.visitDebuggerStatement(node);
                break;
            case ts.SyntaxKind.DefaultClause:
                this.visitDefaultClause(node);
                break;
            case ts.SyntaxKind.DoStatement:
                this.visitDoStatement(node);
                break;
            case ts.SyntaxKind.ElementAccessExpression:
                this.visitElementAccessExpression(node);
                break;
            case ts.SyntaxKind.EnumDeclaration:
                this.visitEnumDeclaration(node);
                break;
            case ts.SyntaxKind.ExportAssignment:
                this.visitExportAssignment(node);
                break;
            case ts.SyntaxKind.ExpressionStatement:
                this.visitExpressionStatement(node);
                break;
            case ts.SyntaxKind.ForStatement:
                this.visitForStatement(node);
                break;
            case ts.SyntaxKind.ForInStatement:
                this.visitForInStatement(node);
                break;
            case ts.SyntaxKind.ForOfStatement:
                this.visitForOfStatement(node);
                break;
            case ts.SyntaxKind.FunctionDeclaration:
                this.visitFunctionDeclaration(node);
                break;
            case ts.SyntaxKind.FunctionExpression:
                this.visitFunctionExpression(node);
                break;
            case ts.SyntaxKind.FunctionType:
                this.visitFunctionType(node);
                break;
            case ts.SyntaxKind.GetAccessor:
                this.visitGetAccessor(node);
                break;
            case ts.SyntaxKind.Identifier:
                this.visitIdentifier(node);
                break;
            case ts.SyntaxKind.IfStatement:
                this.visitIfStatement(node);
                break;
            case ts.SyntaxKind.ImportDeclaration:
                this.visitImportDeclaration(node);
                break;
            case ts.SyntaxKind.ImportEqualsDeclaration:
                this.visitImportEqualsDeclaration(node);
                break;
            case ts.SyntaxKind.IndexSignature:
                this.visitIndexSignatureDeclaration(node);
                break;
            case ts.SyntaxKind.InterfaceDeclaration:
                this.visitInterfaceDeclaration(node);
                break;
            case ts.SyntaxKind.JsxAttribute:
                this.visitJsxAttribute(node);
                break;
            case ts.SyntaxKind.JsxElement:
                this.visitJsxElement(node);
                break;
            case ts.SyntaxKind.JsxExpression:
                this.visitJsxExpression(node);
                break;
            case ts.SyntaxKind.JsxSelfClosingElement:
                this.visitJsxSelfClosingElement(node);
                break;
            case ts.SyntaxKind.JsxSpreadAttribute:
                this.visitJsxSpreadAttribute(node);
                break;
            case ts.SyntaxKind.LabeledStatement:
                this.visitLabeledStatement(node);
                break;
            case ts.SyntaxKind.MethodDeclaration:
                this.visitMethodDeclaration(node);
                break;
            case ts.SyntaxKind.MethodSignature:
                this.visitMethodSignature(node);
                break;
            case ts.SyntaxKind.ModuleDeclaration:
                this.visitModuleDeclaration(node);
                break;
            case ts.SyntaxKind.NamedImports:
                this.visitNamedImports(node);
                break;
            case ts.SyntaxKind.NamespaceImport:
                this.visitNamespaceImport(node);
                break;
            case ts.SyntaxKind.NewExpression:
                this.visitNewExpression(node);
                break;
            case ts.SyntaxKind.ObjectBindingPattern:
                this.visitBindingPattern(node);
                break;
            case ts.SyntaxKind.ObjectLiteralExpression:
                this.visitObjectLiteralExpression(node);
                break;
            case ts.SyntaxKind.Parameter:
                this.visitParameterDeclaration(node);
                break;
            case ts.SyntaxKind.PostfixUnaryExpression:
                this.visitPostfixUnaryExpression(node);
                break;
            case ts.SyntaxKind.PrefixUnaryExpression:
                this.visitPrefixUnaryExpression(node);
                break;
            case ts.SyntaxKind.PropertyAccessExpression:
                this.visitPropertyAccessExpression(node);
                break;
            case ts.SyntaxKind.PropertyAssignment:
                this.visitPropertyAssignment(node);
                break;
            case ts.SyntaxKind.PropertyDeclaration:
                this.visitPropertyDeclaration(node);
                break;
            case ts.SyntaxKind.PropertySignature:
                this.visitPropertySignature(node);
                break;
            case ts.SyntaxKind.RegularExpressionLiteral:
                this.visitRegularExpressionLiteral(node);
                break;
            case ts.SyntaxKind.ReturnStatement:
                this.visitReturnStatement(node);
                break;
            case ts.SyntaxKind.SetAccessor:
                this.visitSetAccessor(node);
                break;
            case ts.SyntaxKind.SourceFile:
                this.visitSourceFile(node);
                break;
            case ts.SyntaxKind.StringLiteral:
                this.visitStringLiteral(node);
                break;
            case ts.SyntaxKind.SwitchStatement:
                this.visitSwitchStatement(node);
                break;
            case ts.SyntaxKind.TemplateExpression:
                this.visitTemplateExpression(node);
                break;
            case ts.SyntaxKind.ThrowStatement:
                this.visitThrowStatement(node);
                break;
            case ts.SyntaxKind.TryStatement:
                this.visitTryStatement(node);
                break;
            case ts.SyntaxKind.TupleType:
                this.visitTupleType(node);
                break;
            case ts.SyntaxKind.TypeAliasDeclaration:
                this.visitTypeAliasDeclaration(node);
                break;
            case ts.SyntaxKind.TypeAssertionExpression:
                this.visitTypeAssertionExpression(node);
                break;
            case ts.SyntaxKind.TypeLiteral:
                this.visitTypeLiteral(node);
                break;
            case ts.SyntaxKind.TypeReference:
                this.visitTypeReference(node);
                break;
            case ts.SyntaxKind.VariableDeclaration:
                this.visitVariableDeclaration(node);
                break;
            case ts.SyntaxKind.VariableStatement:
                this.visitVariableStatement(node);
                break;
            case ts.SyntaxKind.WhileStatement:
                this.visitWhileStatement(node);
                break;
            case ts.SyntaxKind.WithStatement:
                this.visitWithStatement(node);
                break;
            default:
                console.warn(`unknown node type: ${node}`);
                break;
        }
    }
    visitChildren(node) {
        ts.forEachChild(node, (child) => this.visitNode(child));
    }
    visitAnyKeyword(node) {
        this.visitChildren(node);
    }
    visitArrayLiteralExpression(node) {
        this.visitChildren(node);
    }
    visitArrayType(node) {
        this.visitChildren(node);
    }
    visitArrowFunction(node) {
        this.visitChildren(node);
    }
    visitBinaryExpression(node) {
        this.visitChildren(node);
    }
    visitBindingElement(node) {
        this.visitChildren(node);
    }
    visitBindingPattern(node) {
        this.visitChildren(node);
    }
    visitBlock(node) {
        this.visitChildren(node);
    }
    visitBreakStatement(node) {
        this.visitChildren(node);
    }
    visitCallExpression(node) {
        this.visitChildren(node);
    }
    visitCallSignature(node) {
        this.visitChildren(node);
    }
    visitCaseClause(node) {
        this.visitChildren(node);
    }
    visitClassDeclaration(node) {
        this.visitChildren(node);
    }
    visitClassExpression(node) {
        this.visitChildren(node);
    }
    visitCatchClause(node) {
        this.visitChildren(node);
    }
    visitConditionalExpression(node) {
        this.visitChildren(node);
    }
    visitConstructSignature(node) {
        this.visitChildren(node);
    }
    visitConstructorDeclaration(node) {
        this.visitChildren(node);
    }
    visitConstructorType(node) {
        this.visitChildren(node);
    }
    visitContinueStatement(node) {
        this.visitChildren(node);
    }
    visitDebuggerStatement(node) {
        this.visitChildren(node);
    }
    visitDefaultClause(node) {
        this.visitChildren(node);
    }
    visitDoStatement(node) {
        this.visitChildren(node);
    }
    visitElementAccessExpression(node) {
        this.visitChildren(node);
    }
    visitEnumDeclaration(node) {
        this.visitChildren(node);
    }
    visitExportAssignment(node) {
        this.visitChildren(node);
    }
    visitExpressionStatement(node) {
        this.visitChildren(node);
    }
    visitForStatement(node) {
        this.visitChildren(node);
    }
    visitForInStatement(node) {
        this.visitChildren(node);
    }
    visitForOfStatement(node) {
        this.visitChildren(node);
    }
    visitFunctionDeclaration(node) {
        this.visitChildren(node);
    }
    visitFunctionExpression(node) {
        this.visitChildren(node);
    }
    visitFunctionType(node) {
        this.visitChildren(node);
    }
    visitGetAccessor(node) {
        this.visitChildren(node);
    }
    visitIdentifier(node) {
        this.visitChildren(node);
    }
    visitIfStatement(node) {
        this.visitChildren(node);
    }
    visitImportDeclaration(node) {
        this.visitChildren(node);
    }
    visitImportEqualsDeclaration(node) {
        this.visitChildren(node);
    }
    visitIndexSignatureDeclaration(node) {
        this.visitChildren(node);
    }
    visitInterfaceDeclaration(node) {
        this.visitChildren(node);
    }
    visitJsxAttribute(node) {
        this.visitChildren(node);
    }
    visitJsxElement(node) {
        this.visitChildren(node);
    }
    visitJsxExpression(node) {
        this.visitChildren(node);
    }
    visitJsxSelfClosingElement(node) {
        this.visitChildren(node);
    }
    visitJsxSpreadAttribute(node) {
        this.visitChildren(node);
    }
    visitLabeledStatement(node) {
        this.visitChildren(node);
    }
    visitMethodDeclaration(node) {
        this.visitChildren(node);
    }
    visitMethodSignature(node) {
        this.visitChildren(node);
    }
    visitModuleDeclaration(node) {
        this.visitChildren(node);
    }
    visitNamedImports(node) {
        this.visitChildren(node);
    }
    visitNamespaceImport(node) {
        this.visitChildren(node);
    }
    visitNewExpression(node) {
        this.visitChildren(node);
    }
    visitObjectLiteralExpression(node) {
        this.visitChildren(node);
    }
    visitParameterDeclaration(node) {
        this.visitChildren(node);
    }
    visitPostfixUnaryExpression(node) {
        this.visitChildren(node);
    }
    visitPrefixUnaryExpression(node) {
        this.visitChildren(node);
    }
    visitPropertyAccessExpression(node) {
        this.visitChildren(node);
    }
    visitPropertyAssignment(node) {
        this.visitChildren(node);
    }
    visitPropertyDeclaration(node) {
        this.visitChildren(node);
    }
    visitPropertySignature(node) {
        this.visitChildren(node);
    }
    visitRegularExpressionLiteral(node) {
        this.visitChildren(node);
    }
    visitReturnStatement(node) {
        this.visitChildren(node);
    }
    visitSetAccessor(node) {
        this.visitChildren(node);
    }
    visitSourceFile(node) {
        this.visitChildren(node);
    }
    visitStringLiteral(node) {
        this.visitChildren(node);
    }
    visitSwitchStatement(node) {
        this.visitChildren(node);
    }
    visitTemplateExpression(node) {
        this.visitChildren(node);
    }
    visitThrowStatement(node) {
        this.visitChildren(node);
    }
    visitTryStatement(node) {
        this.visitChildren(node);
    }
    visitTupleType(node) {
        this.visitChildren(node);
    }
    visitTypeAliasDeclaration(node) {
        this.visitChildren(node);
    }
    visitTypeAssertionExpression(node) {
        this.visitChildren(node);
    }
    visitTypeLiteral(node) {
        this.visitChildren(node);
    }
    visitTypeReference(node) {
        this.visitChildren(node);
    }
    visitVariableDeclaration(node) {
        this.visitChildren(node);
    }
    visitVariableStatement(node) {
        this.visitChildren(node);
    }
    visitWhileStatement(node) {
        this.visitChildren(node);
    }
    visitWithStatement(node) {
        this.visitChildren(node);
    }
}
exports.Visitor = Visitor;

//# sourceMappingURL=typescript-visitor.js.map
