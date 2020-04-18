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

import static android.databinding.tool.reflection.Callable.DYNAMIC;
import static android.databinding.tool.reflection.Callable.STATIC;

import android.databinding.tool.processing.Scope;
import android.databinding.tool.reflection.Callable;
import android.databinding.tool.reflection.Callable.Type;
import android.databinding.tool.reflection.ModelAnalyzer;
import android.databinding.tool.reflection.ModelClass;
import android.databinding.tool.reflection.ModelMethod;
import android.databinding.tool.util.L;
import android.databinding.tool.writer.KCode;

import java.util.ArrayList;
import java.util.List;


public class MethodCallExpr extends Expr {
    final String mName;

    Callable mGetter;

    static List<Expr> concat(Expr e, List<Expr> list) {
        List<Expr> merged = new ArrayList<Expr>();
        merged.add(e);
        merged.addAll(list);
        return merged;
    }

    MethodCallExpr(Expr target, String name, List<Expr> args) {
        super(concat(target, args));
        mName = name;
    }

    @Override
    public void updateExpr(ModelAnalyzer modelAnalyzer) {
        try {
            Scope.enter(this);
            resolveType(modelAnalyzer);
            super.updateExpr(modelAnalyzer);
        } finally {
            Scope.exit();
        }
    }

    @Override
    protected KCode generateCode(boolean expand) {
        KCode code = new KCode()
        .app("", getTarget().toCode(expand))
        .app(".")
        .app(getGetter().name)
        .app("(");
        boolean first = true;
        for (Expr arg : getArgs()) {
            if (first) {
                first = false;
            } else {
                code.app(", ");
            }
            code.app("", arg.toCode(expand));
        }
        code.app(")");
        return code;
    }

    @Override
    protected ModelClass resolveType(ModelAnalyzer modelAnalyzer) {
        if (mGetter == null) {
            List<ModelClass> args = new ArrayList<ModelClass>();
            for (Expr expr : getArgs()) {
                args.add(expr.getResolvedType());
            }

            Expr target = getTarget();
            boolean isStatic = target instanceof StaticIdentifierExpr;
            ModelMethod method = target.getResolvedType().getMethod(mName, args, isStatic);
            if (method == null) {
                String message = "cannot find method '" + mName + "' in class " +
                        target.getResolvedType().toJavaCode();
                IllegalArgumentException e = new IllegalArgumentException(message);
                L.e(e, "cannot find method %s in class %s", mName,
                        target.getResolvedType().toJavaCode());
                throw e;
            }
            if (!isStatic && method.isStatic()) {
                // found a static method on an instance. Use class instead
                target.getParents().remove(this);
                getChildren().remove(target);
                StaticIdentifierExpr staticId = getModel()
                        .staticIdentifierFor(target.getResolvedType());
                getChildren().add(staticId);
                staticId.getParents().add(this);
                // make sure we update this in case we access it below
                target = getTarget();
            }
            int flags = DYNAMIC;
            if (method.isStatic()) {
                flags |= STATIC;
            }
            mGetter = new Callable(Type.METHOD, method.getName(), null, method.getReturnType(args),
                    method.getParameterTypes().length, flags);
        }
        return mGetter.resolvedType;
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

    @Override
    protected String computeUniqueKey() {
        return join(getTarget().computeUniqueKey(), mName,
                super.computeUniqueKey());
    }

    public Expr getTarget() {
        return getChildren().get(0);
    }

    public String getName() {
        return mName;
    }

    public List<Expr> getArgs() {
        return getChildren().subList(1, getChildren().size());
    }

    public Callable getGetter() {
        return mGetter;
    }

    @Override
    public String getInvertibleError() {
        return "Method calls may not be used in two-way expressions";
    }
}
