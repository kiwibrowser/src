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

import android.databinding.tool.expr.ArgListExpr;
import android.databinding.tool.expr.Expr;
import android.databinding.tool.expr.ExprModel;
import android.databinding.tool.reflection.ModelClass;
import android.databinding.tool.store.SetterStore;
import android.databinding.tool.util.L;
import android.databinding.tool.writer.LayoutBinderWriterKt;

import java.lang.reflect.Array;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Multiple binding expressions can be evaluated using a single adapter. In those cases,
 * we replace the Binding with a MergedBinding.
 */
public class MergedBinding extends Binding {
    private final SetterStore.MultiAttributeSetter mMultiAttributeSetter;
    public MergedBinding(ExprModel model, SetterStore.MultiAttributeSetter multiAttributeSetter,
            BindingTarget target, Iterable<Binding> bindings) {
        super(target, createMergedName(bindings), createArgListExpr(model, bindings));
        mMultiAttributeSetter = multiAttributeSetter;
    }

    @Override
    public void resolveListeners() {
        ModelClass[] parameters = mMultiAttributeSetter.getParameterTypes();
        List<Expr> children = getExpr().getChildren();
        final Expr expr = getExpr();
        for (int i = 0; i < children.size(); i++) {
            final Expr child = children.get(i);
            child.resolveListeners(parameters[i], expr);
        }
    }

    private static Expr createArgListExpr(ExprModel model, final Iterable<Binding> bindings) {
        List<Expr> args = new ArrayList<Expr>();
        for (Binding binding : bindings) {
            args.add(binding.getExpr());
        }
        Expr expr = model.argListExpr(args);
        expr.setBindingExpression(true);
        return expr;
    }

    private static String createMergedName(Iterable<Binding> bindings) {
        StringBuilder sb = new StringBuilder();
        for (Binding binding : bindings) {
            sb.append(binding.getName());
        }
        return sb.toString();
    }

    public Expr[] getComponentExpressions() {
        ArgListExpr args = (ArgListExpr) getExpr();
        return args.getChildren().toArray(new Expr[args.getChildren().size()]);
    }

    public String[] getAttributes() {
        return mMultiAttributeSetter.attributes;
    }

    @Override
    public String getBindingAdapterInstanceClass() {
        return mMultiAttributeSetter.getBindingAdapterInstanceClass();
    }

    @Override
    public boolean requiresOldValue() {
        return mMultiAttributeSetter.requiresOldValue();
    }

    @Override
    public int getMinApi() {
        return 1;
    }

    @Override
    public String toJavaCode(String targetViewName, String bindingComponent) {
        final ArgListExpr args = (ArgListExpr) getExpr();
        final List<String> newValues = new ArrayList<String>();
        for (Expr expr : args.getChildren()) {
            newValues.add(expr.toCode().generate());
        }
        final List<String> oldValues;
        if (requiresOldValue()) {
            oldValues = new ArrayList<String>();
            for (Expr expr : args.getChildren()) {
                oldValues.add("this." + LayoutBinderWriterKt.getOldValueName(expr));
            }
        } else {
            oldValues = Arrays.asList(new String[args.getChildren().size()]);
        }
        final String[] expressions = concat(oldValues, newValues, String.class);
        L.d("merged binding arg: %s", args.getUniqueKey());
        return mMultiAttributeSetter.toJava(bindingComponent, targetViewName, expressions);
    }

    private static <T> T[] concat(List<T> l1, List<T> l2, Class<T> klass) {
        List<T> result = new ArrayList<T>();
        result.addAll(l1);
        result.addAll(l2);
        return result.toArray((T[]) Array.newInstance(klass, result.size()));
    }
}
