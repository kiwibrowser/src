/*
 * Copyright (C) 2016 The Android Open Source Project
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
 * limitations under the License.Wifi
 */

package com.android.server.wifi;

import android.os.SystemClock;
/**
 * Wrapper class for time operations. Allows replacement of clock operations for testing.
 */
public class Clock {

    /**
     * Get the current time of the clock in milliseconds.
     *
     * @return Current time in milliseconds.
     */
    public long currentTimeMillis() {
        return System.currentTimeMillis();
    }

    /**
     * Returns milliseconds since boot, including time spent in sleep.
     *
     * @return Current time since boot in milliseconds.
     */
    public long elapsedRealtime() {
        return SystemClock.elapsedRealtime();
    }

    /**
     * Returns the current timestamp of the most precise timer available on the local system, in
     * nanoseconds.
     *
     * @return Current time in nanoseconds.
     */
    public long nanoTime() {
        return System.nanoTime();
    }

    /**
     * Returns nanoseconds since boot, including time spent in sleep.
     *
     * @return Current time since boot in nanoseconds.
     */
    public long elapsedRealtimeNanos() {
        return SystemClock.elapsedRealtimeNanos();
    }
}
