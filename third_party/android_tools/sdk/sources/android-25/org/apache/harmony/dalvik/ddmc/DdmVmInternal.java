/*
 * Copyright (C) 2007 The Android Open Source Project
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

package org.apache.harmony.dalvik.ddmc;

/**
 * Declarations for some VM-internal DDM stuff.
 */
public class DdmVmInternal {

    /* do not instantiate */
    private DdmVmInternal() {}

    /**
     * Enable thread notification.
     *
     * This is built into the VM, since that's where threads get managed.
     */
    native public static void threadNotify(boolean enable);

    /**
     * Enable heap info updates.
     *
     * This is built into the VM, since that's where the heap is managed.
     *
     * @param when when to send the next HPIF chunk
     * @return true on success.  false if 'when' is bad or if there was
     *         an internal error.
     */
    native public static boolean heapInfoNotify(int when);

    /**
     * Enable heap segment updates for the java (isNative == false) or
     * native (isNative == true) heap.
     *
     * This is built into the VM, since that's where the heap is managed.
     */
    native public static boolean heapSegmentNotify(int when, int what,
        boolean isNative);

    /**
     * Get status info for all threads.  This is for the THST chunk.
     *
     * Returns a byte array with the THST data, or null if something
     * went wrong.
     */
    native public static byte[] getThreadStats();

    /**
     * Get a stack trace for the specified thread ID.  The ID can be found
     * in the data from getThreadStats.
     */
    native public static StackTraceElement[] getStackTraceById(int threadId);

    /**
     * Enable or disable "recent allocation" tracking.
     */
    native public static void enableRecentAllocations(boolean enable);

    /*
     * Return a boolean indicating whether or not the "recent allocation"
     * feature is currently enabled.
     */
    native public static boolean getRecentAllocationStatus();

    /**
     * Fill a buffer with data on recent heap allocations.
     */
    native public static byte[] getRecentAllocations();
}

