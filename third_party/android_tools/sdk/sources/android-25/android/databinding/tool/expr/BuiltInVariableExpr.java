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
import java.util.List;

public class BuiltInVariableExpr extends IdentifierExpr {
    private final String mAccessCode;

    BuiltInVariableExpr(String name, String type, String accessCode) {
        super(name);
        super.setUserDefinedType(type);
        this.mAccessCode = accessCode;
    }

    @Override
    public boolean isDynamic() {
        return false;
    }

    @Override
    protected ModelClass resolveType(ModelAnalyzer modelAnalyzer) {
        ModelClass modelClass = super.resolveType(modelAnalyzer);
        return modelClass;
    }

    @Override
    protected List<Dependency> constructDependencies() {
        return new ArrayList<Dependency>();
    }

    @Override
    protected KCode generateCode(boolean expand) {
        if (mAccessCode == null) {
            return new KCode().app(mName);
        } else {
            return new KCode().app(mAccessCode);
        }
    }

    public boolean isDeclared() {
        return false;
    }

    @Override
    public String getInvertibleError() {
        return "Built-in variables may not be the target of two-way binding";
    }
}
