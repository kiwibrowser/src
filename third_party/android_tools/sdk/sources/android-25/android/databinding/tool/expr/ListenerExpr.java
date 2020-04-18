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
import android.databinding.tool.reflection.ModelMethod;
import android.databinding.tool.writer.KCode;
import android.databinding.tool.writer.LayoutBinderWriterKt;

import java.util.ArrayList;
import java.util.List;

/**
 * This wraps an expression, but makes it unique for a particular event listener type.
 * This is used to differentiate listener methods. For example:
 * <pre>
 *     public void onFoo(String str) {...}
 *     public void onFoo(int i) {...}
 * </pre>
 */
public class ListenerExpr extends Expr {
    private final String mName;
    private final ModelClass mListenerType;
    private final ModelMethod mMethod;

    ListenerExpr(Expr expr, String name, ModelClass listenerType, ModelMethod method) {
        super(expr);
        mName = name;
        mListenerType = listenerType;
        mMethod = method;
    }

    @Override
    protected ModelClass resolveType(ModelAnalyzer modelAnalyzer) {
        return mListenerType;
    }

    public ModelMethod getMethod() {
        return mMethod;
    }

    public Expr getChild() {
        return getChildren().get(0);
    }

    public String getName() {
        return mName;
    }

    @Override
    public boolean isDynamic() {
        return getChild().isDynamic();
    }

    @Override
    protected List<Dependency> constructDependencies() {
        final List<Dependency> dependencies = new ArrayList<Dependency>();
        Dependency dependency = new Dependency(this, getChild());
        dependency.setMandatory(true);
        dependencies.add(dependency);
        return dependencies;
    }

    protected String computeUniqueKey() {
        return join(getResolvedType().getCanonicalName(), getChild().computeUniqueKey(), mName);
    }

    @Override
    public KCode generateCode(boolean expand) {
        KCode code = new KCode("(");
        final int minApi = Math.max(mListenerType.getMinApi(), mMethod.getMinApi());
        if (minApi > 1) {
            code.app("(getBuildSdkInt() < " + minApi + ") ? null : ");
        }
        final String fieldName = LayoutBinderWriterKt.getFieldName(this);
        final String listenerClassName = LayoutBinderWriterKt.getListenerClassName(this);
        final KCode value = getChild().toCode();
            code.app("((")
                    .app(fieldName)
                    .app(" == null) ? (")
                    .app(fieldName)
                    .app(" = new ")
                    .app(listenerClassName)
                    .app("()) : ")
                    .app(fieldName)
                    .app(")");
        if (getChild().isDynamic()) {
            code.app(".setValue(", value)
                    .app(")");
        }
        code.app(")");
        return code;
    }

    @Override
    public String getInvertibleError() {
        return "Listeners cannot be the target of a two-way binding";
    }
}
