/*
 * Copyright (C) 2008 The Android Open Source Project
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

package org.apache.harmony.luni.internal.util;

/**
 * This class provides a way to add an implementation specific way to
 * access the current timezone.
 */
public abstract class TimezoneGetter {

    private static TimezoneGetter instance;

    /**
     * Retrieves the singleton instance of this class.
     *
     * @return TimezoneGetter the single instance of this class.
     */
    public static TimezoneGetter getInstance() {
        return instance;
    }

    /**
     * Sets the singleton instance of this class.
     *
     * @param instance
     *            TimezoneGetter the single instance of this class.
     */
    public static void setInstance(TimezoneGetter getter) {
        if (instance != null) {
            throw new UnsupportedOperationException("TimezoneGetter instance already set");
        }
        instance = getter;
    }

    /**
     * Retrieves the ID of the current time zone.
     *
     * @return String the ID of the current time zone.
     */
    public abstract String getId();
}
