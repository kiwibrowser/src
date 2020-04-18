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

import android.databinding.tool.writer.KCode;

public class StaticIdentifierExpr extends IdentifierExpr {

    StaticIdentifierExpr(String name) {
        super(name);
    }

    @Override
    public boolean isObservable() {
        return false;
    }

    @Override
    public boolean isDynamic() {
        return false;
    }

    @Override
    public String getInvertibleError() {
        return "Class " + getResolvedType().toJavaCode() +
                " may not be the target of a two-way binding expression";
    }

    @Override
    public KCode toInverseCode(KCode value) {
        throw new IllegalStateException("StaticIdentifierExpr is not invertible.");
    }
    @Override
    protected KCode generateCode(boolean expand) {
        return new KCode(getResolvedType().toJavaCode());
    }
}
