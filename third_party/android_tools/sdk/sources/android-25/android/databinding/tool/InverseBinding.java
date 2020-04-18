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

import android.databinding.tool.expr.Expr;
import android.databinding.tool.expr.ExprModel;
import android.databinding.tool.expr.FieldAccessExpr;
import android.databinding.tool.processing.ErrorMessages;
import android.databinding.tool.processing.Scope;
import android.databinding.tool.processing.scopes.LocationScopeProvider;
import android.databinding.tool.reflection.ModelAnalyzer;
import android.databinding.tool.reflection.ModelClass;
import android.databinding.tool.store.Location;
import android.databinding.tool.store.SetterStore;
import android.databinding.tool.store.SetterStore.BindingGetterCall;
import android.databinding.tool.store.SetterStore.BindingSetterCall;
import android.databinding.tool.util.L;
import android.databinding.tool.util.Preconditions;
import android.databinding.tool.writer.FlagSet;
import android.databinding.tool.writer.KCode;
import android.databinding.tool.writer.LayoutBinderWriterKt;

import kotlin.jvm.functions.Function2;

import java.util.ArrayList;
import java.util.List;

public class InverseBinding implements LocationScopeProvider {

    private final String mName;
    private final Expr mExpr;
    private final BindingTarget mTarget;
    private BindingGetterCall mGetterCall;
    private final ArrayList<FieldAccessExpr> mChainedExpressions = new ArrayList<FieldAccessExpr>();

    public InverseBinding(BindingTarget target, String name, Expr expr) {
        mTarget = target;
        mName = name;
        mExpr = expr;
    }

    @Override
    public List<Location> provideScopeLocation() {
        if (mExpr != null) {
            return mExpr.getLocations();
        } else {
            return mChainedExpressions.get(0).getLocations();
        }
    }

    void setGetterCall(BindingGetterCall getterCall) {
        mGetterCall = getterCall;
    }

    public void addChainedExpression(FieldAccessExpr expr) {
        mChainedExpressions.add(expr);
    }

    public boolean isOnBinder() {
        return mTarget.getResolvedType().isViewDataBinding();
    }

    private SetterStore.BindingGetterCall getGetterCall() {
        if (mGetterCall == null) {
            if (mExpr != null) {
                mExpr.getResolvedType(); // force resolve of ObservableFields
            }
            try {
                Scope.enter(mTarget);
                Scope.enter(this);
                resolveGetterCall();
                if (mGetterCall == null) {
                    L.e(ErrorMessages.CANNOT_FIND_GETTER_CALL, mName,
                            mExpr == null ? "Unknown" : mExpr.getResolvedType(),
                            mTarget.getResolvedType());
                }
            } finally {
                Scope.exit();
                Scope.exit();
            }
        }
        return mGetterCall;
    }

    private void resolveGetterCall() {
        ModelClass viewType = mTarget.getResolvedType();
        final SetterStore setterStore = SetterStore.get(ModelAnalyzer.getInstance());
        final ModelClass resolvedType = mExpr == null ? null : mExpr.getResolvedType();
        mGetterCall = setterStore.getGetterCall(mName, viewType, resolvedType,
                getModel().getImports());
    }

    public BindingTarget getTarget() {
        return mTarget;
    }

    public KCode toJavaCode(String bindingComponent, final FlagSet flagField) {
        final String targetViewName = LayoutBinderWriterKt.getFieldName(getTarget());
        KCode code = new KCode();
        // A chained expression will have substituted its chained value for the expression
        // unless the attribute has no expression. Therefore, chaining and expressions are
        // mutually exclusive.
        Preconditions.check((mExpr == null) != mChainedExpressions.isEmpty(),
                "Chained expressions are only against unbound attributes.");
        if (mExpr != null) {
            code.app("", mExpr.toInverseCode(new KCode(getGetterCall().toJava(bindingComponent,
                    targetViewName))));
        } else { // !mChainedExpressions.isEmpty())
            final String fieldName = flagField.getLocalName();
            FlagSet flagSet = new FlagSet();
            for (FieldAccessExpr expr : mChainedExpressions) {
                flagSet = flagSet.or(new FlagSet(expr.getId()));
            }
            final FlagSet allFlags = flagSet;
            code.nl(new KCode("synchronized(this) {"));
            code.tab(LayoutBinderWriterKt
                    .mapOr(flagField, flagSet, new Function2<String, Integer, KCode>() {
                        @Override
                        public KCode invoke(String suffix, Integer index) {
                            return new KCode(fieldName)
                                    .app(suffix)
                                    .app(" |= ")
                                    .app(LayoutBinderWriterKt.binaryCode(allFlags, index))
                                    .app(";");
                        }
                    }));
            code.nl(new KCode("}"));
            code.nl(new KCode("requestRebind()"));
        }
        return code;
    }

    public String getBindingAdapterInstanceClass() {
        return getGetterCall().getBindingAdapterInstanceClass();
    }

    /**
     * The min api level in which this binding should be executed.
     * <p>
     * This should be the minimum value among the dependencies of this binding.
     */
    public int getMinApi() {
        final BindingGetterCall getterCall = getGetterCall();
        return Math.max(getterCall.getMinApi(), getterCall.getEvent().getMinApi());
    }

    public BindingSetterCall getEventSetter() {
        final BindingGetterCall getterCall = getGetterCall();
        return getterCall.getEvent();
    }

    public String getName() {
        return mName;
    }

    public String getEventAttribute() {
        return getGetterCall().getEventAttribute();
    }

    public ExprModel getModel() {
        if (mExpr != null) {
            return mExpr.getModel();
        }
        return mChainedExpressions.get(0).getModel();
    }
}
