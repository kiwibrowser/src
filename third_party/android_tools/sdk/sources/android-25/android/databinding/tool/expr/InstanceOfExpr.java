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

public class InstanceOfExpr extends Expr {
    final String mTypeStr;
    ModelClass mType;

    InstanceOfExpr(Expr left, String type) {
        super(left);
        mTypeStr = type;
    }

    @Override
    protected String computeUniqueKey() {
        return join("instanceof", super.computeUniqueKey(), mTypeStr);
    }

    @Override
    protected KCode generateCode(boolean expand) {
        return new KCode()
                .app("", getExpr().toCode(expand))
                .app(" instanceof ")
                .app(getType().toJavaCode());
    }

    @Override
    protected ModelClass resolveType(ModelAnalyzer modelAnalyzer) {
        mType = modelAnalyzer.findClass(mTypeStr, getModel().getImports());
        return modelAnalyzer.loadPrimitive("boolean");
    }

    @Override
    protected List<Dependency> constructDependencies() {
        return constructDynamicChildrenDependencies();
    }

    public Expr getExpr() {
        return getChildren().get(0);
    }

    public ModelClass getType() {
        return mType;
    }

    @Override
    public String getInvertibleError() {
        return "two-way binding can't target a value with the 'instanceof' operator";
    }
}
