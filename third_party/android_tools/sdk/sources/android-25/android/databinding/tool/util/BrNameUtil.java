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

package android.databinding.tool.util;

import android.databinding.tool.reflection.Callable;

/**
 * Central place to convert method/field names to BR observable fields
 */
public class BrNameUtil {
    private static String stripPrefixFromField(String name) {
        if (name.length() >= 2) {
            char firstChar = name.charAt(0);
            char secondChar = name.charAt(1);
            if (name.length() > 2 && firstChar == 'm' && secondChar == '_') {
                char thirdChar = name.charAt(2);
                if (Character.isJavaIdentifierStart(thirdChar)) {
                    return "" + Character.toLowerCase(thirdChar) +
                            name.subSequence(3, name.length());
                }
            } else if ((firstChar == 'm' && Character.isUpperCase(secondChar)) ||
                    (firstChar == '_' && Character.isJavaIdentifierStart(secondChar))) {
                return "" + Character.toLowerCase(secondChar) + name.subSequence(2, name.length());
            }
        }
        return name;
    }

    public static String brKey(Callable callable) {
        if (callable.type == Callable.Type.FIELD) {
            return stripPrefixFromField(callable.name);
        }
        CharSequence propertyName;
        final String name = callable.name;
        if (isGetter(callable) || isSetter(callable)) {
            propertyName = name.subSequence(3, name.length());
        } else if (isBooleanGetter(callable)) {
            propertyName = name.subSequence(2, name.length());
        } else {
            L.e("@Bindable associated with method must follow JavaBeans convention %s", callable);
            return null;
        }
        char firstChar = propertyName.charAt(0);
        return "" + Character.toLowerCase(firstChar) +
                propertyName.subSequence(1, propertyName.length());
    }

    private static boolean isGetter(Callable callable) {
        return prefixes(callable.name, "get") &&
                Character.isJavaIdentifierStart(callable.name.charAt(3)) &&
                callable.getParameterCount() == 0 &&
                !callable.resolvedType.isVoid();
    }

    private static boolean isSetter(Callable callable) {
        return prefixes(callable.name, "set") &&
                Character.isJavaIdentifierStart(callable.name.charAt(3)) &&
                callable.getParameterCount() == 1 &&
                callable.resolvedType.isVoid();
    }

    private static boolean isBooleanGetter(Callable callable) {
        return prefixes(callable.name, "is") &&
                Character.isJavaIdentifierStart(callable.name.charAt(2)) &&
                callable.getParameterCount() == 0 &&
                callable.resolvedType.isBoolean();
    }

    private static boolean prefixes(CharSequence sequence, String prefix) {
        boolean prefixes = false;
        if (sequence.length() > prefix.length()) {
            int count = prefix.length();
            prefixes = true;
            for (int i = 0; i < count; i++) {
                if (sequence.charAt(i) != prefix.charAt(i)) {
                    prefixes = false;
                    break;
                }
            }
        }
        return prefixes;
    }
}
