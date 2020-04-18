/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.ex.camera2.portability.util;

import com.android.ex.camera2.portability.debug.Log;

import java.lang.reflect.Method;

/**
 * Mirrors hidden class {@link android.os.SystemProperties} (available since API Level 1).
 */
public final class SystemProperties {
    private static final Log.Tag TAG = new Log.Tag("SysProps");

    /**
     * Gets system properties set by <code>adb shell setprop <em>key</em> <em>value</em></code>
     *
     * @param key the property key.
     * @param defaultValue the value to return if the property is undefined or empty (this parameter
     *            may be {@code null}).
     * @return the system property value or the default value.
     */
    public static String get(String key, String defaultValue) {
        try {
            final Class<?> systemProperties = Class.forName("android.os.SystemProperties");
            final Method get = systemProperties.getMethod("get", String.class, String.class);
            return (String) get.invoke(null, key, defaultValue);
        } catch (Exception e) {
            // This should never happen
            Log.e(TAG, "Exception while getting system property: ", e);
            return defaultValue;
        }
    }

    private SystemProperties() {
    }
}
