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

public class BracketExpr extends Expr {

    public enum BracketAccessor {
        ARRAY,
        LIST,
        MAP,
    }

    private BracketAccessor mAccessor;

    BracketExpr(Expr target, Expr arg) {
        super(target, arg);
    }

    @Override
    protected ModelClass resolveType(ModelAnalyzer modelAnalyzer) {
        ModelClass targetType = getTarget().getResolvedType();
        if (targetType.isArray()) {
            mAccessor = BracketAccessor.ARRAY;
        } else if (targetType.isList()) {
            mAccessor = BracketAccessor.LIST;
        } else if (targetType.isMap()) {
            mAccessor = BracketAccessor.MAP;
        } else {
            throw new IllegalArgumentException("Cannot determine variable type used in [] " +
                    "expression. Cast the value to List, Map, " +
                    "or array. Type detected: " + targetType.toJavaCode());
        }
        return targetType.getComponentType();
    }

    @Override
    protected List<Dependency> constructDependencies() {
        final List<Dependency> dependencies = constructDynamicChildrenDependencies();
        for (Dependency dependency : dependencies) {
            if (dependency.getOther() == getTarget()) {
                dependency.setMandatory(true);
            }
        }
        return dependencies;
    }

    protected String computeUniqueKey() {
        final String targetKey = getTarget().computeUniqueKey();
        return addTwoWay(join(targetKey, "$", getArg().computeUniqueKey(), "$"));
    }

    @Override
    public String getInvertibleError() {
        return null;
    }

    public Expr getTarget() {
        return getChildren().get(0);
    }

    public Expr getArg() {
        return getChildren().get(1);
    }

    public BracketAccessor getAccessor() {
        return mAccessor;
    }

    public boolean argCastsInteger() {
        return mAccessor != BracketAccessor.MAP && getArg().getResolvedType().isObject();
    }

    @Override
    protected KCode generateCode(boolean expand) {
        String cast = argCastsInteger() ? "(Integer) " : "";
        switch (getAccessor()) {
            case ARRAY: {
                return new KCode().
                        app("getFromArray(", getTarget().toCode()).
                        app(", ").
                        app(cast, getArg().toCode()).app(")");
            }
            case LIST: {
                ModelClass listType = ModelAnalyzer.getInstance().findClass(java.util.List.class).
                        erasure();
                ModelClass targetType = getTarget().getResolvedType().erasure();
                if (listType.isAssignableFrom(targetType)) {
                    return new KCode().
                            app("getFromList(", getTarget().toCode()).
                            app(", ").
                            app(cast, getArg().toCode()).
                            app(")");
                } else {
                    return new KCode().
                            app("", getTarget().toCode()).
                            app(".get(").
                            app(cast, getArg().toCode()).
                            app(")");
                }
            }
            case MAP:
                return new KCode().
                        app("", getTarget().toCode()).
                        app(".get(", getArg().toCode()).
                        app(")");
        }
        throw new IllegalStateException("Invalid BracketAccessor type");
    }

    @Override
    public KCode toInverseCode(KCode value) {
        String cast = argCastsInteger() ? "(Integer) " : "";
        return new KCode().
                app("setTo(", getTarget().toCode(true)).
                app(", ").
                app(cast, getArg().toCode(true)).
                app(", ", value).app(");");
    }
}
