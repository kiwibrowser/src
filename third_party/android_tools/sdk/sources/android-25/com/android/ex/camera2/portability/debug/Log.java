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

package com.android.ex.camera2.portability.debug;

public class Log {
    /**
     * All Camera logging using this class will use this tag prefix.
     * Additionally, the prefix itself is checked in isLoggable and
     * serves as an override. So, to toggle all logs allowed by the
     * current {@link Configuration}, you can set properties:
     *
     * adb shell setprop log.tag.CAM2PORT_ VERBOSE
     * adb shell setprop log.tag.CAM2PORT_ ""
     */
    public static final String CAMERA_LOGTAG_PREFIX = "CAM2PORT_";
    private static final Log.Tag TAG = new Log.Tag("Log");

    /**
     * This class restricts the length of the log tag to be less than the
     * framework limit and also prepends the common tag prefix defined by
     * {@code CAMERA_LOGTAG_PREFIX}.
     */
    public static final class Tag {

        // The length limit from Android framework is 23.
        private static final int MAX_TAG_LEN = 23 - CAMERA_LOGTAG_PREFIX.length();

        final String mValue;

        public Tag(String tag) {
            final int lenDiff = tag.length() - MAX_TAG_LEN;
            if (lenDiff > 0) {
                w(TAG, "Tag " + tag + " is " + lenDiff + " chars longer than limit.");
            }
            mValue = CAMERA_LOGTAG_PREFIX + (lenDiff > 0 ? tag.substring(0, MAX_TAG_LEN) : tag);
        }

        @Override
        public String toString() {
            return mValue;
        }
    }

    public static void d(Tag tag, String msg) {
        if (isLoggable(tag, android.util.Log.DEBUG)) {
            android.util.Log.d(tag.toString(), msg);
        }
    }

    public static void d(Tag tag, String msg, Throwable tr) {
        if (isLoggable(tag, android.util.Log.DEBUG)) {
            android.util.Log.d(tag.toString(), msg, tr);
        }
    }

    public static void e(Tag tag, String msg) {
        if (isLoggable(tag, android.util.Log.ERROR)) {
            android.util.Log.e(tag.toString(), msg);
        }
    }

    public static void e(Tag tag, String msg, Throwable tr) {
        if (isLoggable(tag, android.util.Log.ERROR)) {
            android.util.Log.e(tag.toString(), msg, tr);
        }
    }

    public static void i(Tag tag, String msg) {
        if (isLoggable(tag, android.util.Log.INFO)) {
            android.util.Log.i(tag.toString(), msg);
        }
    }

    public static void i(Tag tag, String msg, Throwable tr) {
        if (isLoggable(tag, android.util.Log.INFO)) {
            android.util.Log.i(tag.toString(), msg, tr);
        }
    }

    public static void v(Tag tag, String msg) {
        if (isLoggable(tag, android.util.Log.VERBOSE)) {
            android.util.Log.v(tag.toString(), msg);
        }
    }

    public static void v(Tag tag, String msg, Throwable tr) {
        if (isLoggable(tag, android.util.Log.VERBOSE)) {
            android.util.Log.v(tag.toString(), msg, tr);
        }
    }

    public static void w(Tag tag, String msg) {
        if (isLoggable(tag, android.util.Log.WARN)) {
            android.util.Log.w(tag.toString(), msg);
        }
    }

    public static void w(Tag tag, String msg, Throwable tr) {
        if (isLoggable(tag, android.util.Log.WARN)) {
            android.util.Log.w(tag.toString(), msg, tr);
        }
    }

    private static boolean isLoggable(Tag tag, int level) {
        try {
            if (LogHelper.getOverrideLevel() != 0) {
                // Override system log level and output. VERBOSE is smaller than
                // ERROR, so the comparison checks that the override value is smaller
                // than the desired output level. This applies to all tags.
                return LogHelper.getOverrideLevel() <= level;
            } else {
                // The prefix can be used as an override tag to see all camera logs
                return android.util.Log.isLoggable(CAMERA_LOGTAG_PREFIX, level)
                        || android.util.Log.isLoggable(tag.toString(), level);
            }
        } catch (IllegalArgumentException ex) {
            e(TAG, "Tag too long:" + tag);
            return false;
        }
    }
}
