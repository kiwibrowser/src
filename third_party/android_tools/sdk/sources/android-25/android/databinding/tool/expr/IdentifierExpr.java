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

import android.databinding.tool.processing.ErrorMessages;
import android.databinding.tool.reflection.ModelAnalyzer;
import android.databinding.tool.reflection.ModelClass;
import android.databinding.tool.util.Preconditions;
import android.databinding.tool.writer.KCode;
import android.databinding.tool.writer.LayoutBinderWriterKt;

import java.util.ArrayList;
import java.util.List;

public class IdentifierExpr extends Expr {
    String mName;
    String mUserDefinedType;
    private boolean mIsDeclared;

    IdentifierExpr(String name) {
        mName = name;
    }

    public String getName() {
        return mName;
    }

    /**
     * If this is root, its type should be set while parsing the XML document
     * @param userDefinedType The type of this identifier
     */
    public void setUserDefinedType(String userDefinedType) {
        mUserDefinedType = userDefinedType;
    }

    @Override
    protected String computeUniqueKey() {
        return join(mName, super.computeUniqueKey());
    }

    public String getUserDefinedType() {
        return mUserDefinedType;
    }

    @Override
    public boolean isDynamic() {
        return true;
    }

    @Override
    protected ModelClass resolveType(final ModelAnalyzer modelAnalyzer) {
        Preconditions.checkNotNull(mUserDefinedType, ErrorMessages.UNDEFINED_VARIABLE, mName);
        return modelAnalyzer.findClass(mUserDefinedType, getModel().getImports());
    }

    @Override
    protected List<Dependency> constructDependencies() {
        return new ArrayList<Dependency>();
    }

    @Override
    protected String asPackage() {
        return mUserDefinedType == null ? mName : null;
    }

    @Override
    protected KCode generateCode(boolean expand) {
        if (expand) {
            return new KCode(LayoutBinderWriterKt.getFieldName(this));
        } else {
            return new KCode(LayoutBinderWriterKt.getExecutePendingLocalName(this));
        }
    }

    public void setDeclared() {
        mIsDeclared = true;
    }

    public boolean isDeclared() {
        return mIsDeclared;
    }

    @Override
    public String getInvertibleError() {
        return null;
    }

    @Override
    public KCode toInverseCode(KCode value) {
        return new KCode().app(LayoutBinderWriterKt.getSetterName(this)).app("(", value).app(");");
    }
}
