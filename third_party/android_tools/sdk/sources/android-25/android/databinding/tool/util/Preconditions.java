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

/**
 * Simple Preconditions utility class, similar to guava's but reports errors via L
 */
public class Preconditions {
    public static void check(boolean value, String error, Object... args) {
        if (!value) {
            L.e(error, args);
        }
    }

    public static void checkNotNull(Object value, String error, Object... args) {
        if (value == null) {
            L.e(error, args);
        }
    }

    public static void checkNull(Object value, String error, Object... args) {
        if (value != null) {
            L.e(error, args);
        }
    }
}
