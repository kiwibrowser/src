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

package android.databinding.tool.expr;

import android.databinding.tool.reflection.ModelAnalyzer;
import android.databinding.tool.reflection.ModelClass;
import android.databinding.tool.writer.KCode;

import java.util.ArrayList;
import java.util.BitSet;
import java.util.List;

public class TernaryExpr extends Expr {
    TernaryExpr(Expr pred, Expr ifTrue, Expr ifFalse) {
        super(pred, ifTrue, ifFalse);
    }

    public Expr getPred() {
        return getChildren().get(0);
    }

    public Expr getIfTrue() {
        return getChildren().get(1);
    }

    public Expr getIfFalse() {
        return getChildren().get(2);
    }

    @Override
    protected String computeUniqueKey() {
        return "?:" + super.computeUniqueKey();
    }

    @Override
    public String getInvertibleError() {
        if (getPred().isDynamic()) {
            return "The condition of a ternary operator must be constant: " +
                    getPred().toFullCode();
        }
        final String trueInvertible = getIfTrue().getInvertibleError();
        if (trueInvertible != null) {
            return trueInvertible;
        } else {
            return getIfFalse().getInvertibleError();
        }
    }

    @Override
    protected ModelClass resolveType(ModelAnalyzer modelAnalyzer) {
        final Expr ifTrue = getIfTrue();
        final Expr ifFalse = getIfFalse();
        if (isNullLiteral(ifTrue)) {
            return ifFalse.getResolvedType();
        } else if (isNullLiteral(ifFalse)) {
            return ifTrue.getResolvedType();
        }
        return modelAnalyzer.findCommonParentOf(getIfTrue().getResolvedType(),
                getIfFalse().getResolvedType());
    }

    private static boolean isNullLiteral(Expr expr) {
        final ModelClass type = expr.getResolvedType();
        return (type.isObject() && (expr instanceof SymbolExpr) &&
                "null".equals(((SymbolExpr)expr).getText()));
    }

    @Override
    protected List<Dependency> constructDependencies() {
        List<Dependency> deps = new ArrayList<Dependency>();
        Expr predExpr = getPred();
        final Dependency pred = new Dependency(this, predExpr);
        pred.setMandatory(true);
        deps.add(pred);

        Expr ifTrueExpr = getIfTrue();
        if (ifTrueExpr.isDynamic()) {
            deps.add(new Dependency(this, ifTrueExpr, predExpr, true));
        }
        Expr ifFalseExpr = getIfFalse();
        if (ifFalseExpr.isDynamic()) {
            deps.add(new Dependency(this, ifFalseExpr, predExpr, false));
        }
        return deps;
    }

    @Override
    protected BitSet getPredicateInvalidFlags() {
        return getPred().getInvalidFlags();
    }

    @Override
    protected KCode generateCode(boolean expand) {
        return new KCode()
                .app("", getPred().toCode(expand))
                .app(" ? ", getIfTrue().toCode(expand))
                .app(" : ", getIfFalse().toCode(expand));

    }

    @Override
    public KCode toInverseCode(KCode variable) {
        return new KCode()
                .app("if (", getPred().toCode(true))
                .app(") {")
                .tab(getIfTrue().toInverseCode(variable))
                .nl(new KCode("} else {"))
                .tab(getIfFalse().toInverseCode(variable))
                .nl(new KCode("}"));
    }

    @Override
    public boolean isConditional() {
        return true;
    }
}
