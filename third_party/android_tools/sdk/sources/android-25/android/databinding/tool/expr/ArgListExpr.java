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

import java.util.List;

/**
 * This is a special expression that is created when we have an adapter that has multiple
 * parameters.
 * <p>
 * When it is detected, we create a new binding with this argument list expression and merge N
 * bindings into a new one so that rest of the code generation logic works as expected.
 */
public class ArgListExpr extends Expr {
    private int mId;
    public ArgListExpr(int id, Iterable<Expr> children) {
        super(children);
        mId = id;
    }

    @Override
    protected String computeUniqueKey() {
        return "ArgList[" + mId + "]" + super.computeUniqueKey();
    }

    @Override
    protected KCode generateCode(boolean expand) {
        throw new IllegalStateException("should never try to convert an argument expressions"
                + " into code");
    }

    @Override
    protected ModelClass resolveType(ModelAnalyzer modelAnalyzer) {
        return modelAnalyzer.findClass(Void.class);
    }

    @Override
    protected List<Dependency> constructDependencies() {
        return super.constructDynamicChildrenDependencies();
    }

    @Override
    public boolean canBeEvaluatedToAVariable() {
        return false;
    }

    @Override
    public String getInvertibleError() {
        return "Merged bindings are not invertible.";
    }
}
