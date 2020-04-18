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
package android.databinding.tool.reflection;

public class Callable {

    public enum Type {
        METHOD,
        FIELD
    }

    public static final int DYNAMIC = 1;
    public static final int CAN_BE_INVALIDATED = 1 << 1;
    public static final int STATIC = 1 << 2;

    public final Type type;

    public final String name;

    public final String setterName;

    public final ModelClass resolvedType;

    private final int mFlags;

    private final int mParameterCount;

    public Callable(Type type, String name, String setterName, ModelClass resolvedType,
                    int parameterCount, int flags) {
        this.type = type;
        this.name = name;
        this.resolvedType = resolvedType;
        mParameterCount = parameterCount;
        this.setterName = setterName;
        mFlags = flags;
    }

    public String getTypeCodeName() {
        return resolvedType.toJavaCode();
    }

    public int getParameterCount() {
        return mParameterCount;
    }

    public boolean isDynamic() {
        return (mFlags & DYNAMIC) != 0;
    }

    public boolean isStatic() {
        return (mFlags & STATIC) != 0;
    }

    public boolean canBeInvalidated() {
        return (mFlags & CAN_BE_INVALIDATED) != 0;
    }

    public int getMinApi() {
        return 1;
    }

    @Override
    public String toString() {
        return "Callable{" +
                "type=" + type +
                ", name='" + name + '\'' +
                ", resolvedType=" + resolvedType +
                ", isDynamic=" + isDynamic() +
                ", canBeInvalidated=" + canBeInvalidated() +
                ", static=" + isStatic() +
                '}';
    }
}
