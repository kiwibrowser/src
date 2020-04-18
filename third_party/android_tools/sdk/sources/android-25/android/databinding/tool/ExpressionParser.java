/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.databinding.tool;

import org.antlr.v4.runtime.ANTLRInputStream;
import org.antlr.v4.runtime.BaseErrorListener;
import org.antlr.v4.runtime.CommonTokenStream;
import org.antlr.v4.runtime.ParserRuleContext;
import org.antlr.v4.runtime.RecognitionException;
import org.antlr.v4.runtime.Recognizer;
import org.antlr.v4.runtime.Token;
import org.antlr.v4.runtime.misc.Nullable;
import org.antlr.v4.runtime.tree.ErrorNode;
import org.antlr.v4.runtime.tree.ParseTreeListener;
import org.antlr.v4.runtime.tree.TerminalNode;

import android.databinding.parser.BindingExpressionLexer;
import android.databinding.parser.BindingExpressionParser;
import android.databinding.tool.expr.Expr;
import android.databinding.tool.expr.ExprModel;
import android.databinding.tool.processing.ErrorMessages;
import android.databinding.tool.store.Location;
import android.databinding.tool.util.L;
import android.databinding.tool.util.Preconditions;

import java.util.ArrayList;
import java.util.List;

public class ExpressionParser {
    final ExprModel mModel;
    final ExpressionVisitor visitor;

    public ExpressionParser(ExprModel model) {
        mModel = model;
        visitor = new ExpressionVisitor(mModel);
    }

    public Expr parse(String input, @Nullable Location locationInFile) {
        ANTLRInputStream inputStream = new ANTLRInputStream(input);
        BindingExpressionLexer lexer = new BindingExpressionLexer(inputStream);
        CommonTokenStream tokenStream = new CommonTokenStream(lexer);
        final BindingExpressionParser parser = new BindingExpressionParser(tokenStream);
        parser.addErrorListener(new BaseErrorListener() {
            @Override
            public <T extends Token> void syntaxError(Recognizer<T, ?> recognizer,
                    @Nullable T offendingSymbol, int line, int charPositionInLine, String msg,
                    @Nullable RecognitionException e) {
                L.e(ErrorMessages.SYNTAX_ERROR, msg);
            }
        });
        BindingExpressionParser.BindingSyntaxContext root = parser.bindingSyntax();
        try {
            mModel.setCurrentLocationInFile(locationInFile);
            visitor.setParseTreeListener(new ParseTreeListener() {
                List<ParserRuleContext> mStack = new ArrayList<ParserRuleContext>();
                @Override
                public void visitTerminal(TerminalNode node) {
                }

                @Override
                public void visitErrorNode(ErrorNode node) {
                }

                @Override
                public void enterEveryRule(ParserRuleContext ctx) {
                    mStack.add(ctx);
                    mModel.setCurrentParserContext(ctx);
                }

                @Override
                public void exitEveryRule(ParserRuleContext ctx) {
                    Preconditions.check(ctx == mStack.get(mStack.size() - 1),
                            "Inconsistent exit from context. Received %s, expecting %s",
                            ctx.toInfoString(parser),
                            mStack.get(mStack.size() - 1).toInfoString(parser));
                    mStack.remove(mStack.size() - 1);
                    if (mStack.size() > 0) {
                        mModel.setCurrentParserContext(mStack.get(mStack.size() - 1));
                    } else {
                        mModel.setCurrentParserContext(null);
                    }
                }
            });
            return root.accept(visitor);
        } finally {
            mModel.setCurrentLocationInFile(null);
        }
    }

    public ExprModel getModel() {
        return mModel;
    }
}
