// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.background_task_scheduler;

/**
 * Helper class to allow external code (typically Chrome-specific BackgroundTaskScheduler code) to
 * report UMA.
 */
public final class BackgroundTaskSchedulerExternalUma {
    private BackgroundTaskSchedulerExternalUma() {}

    /**
     * Reports metrics for when a NativeBackgroundTask loads the native library.
     * @param taskId An id from {@link TaskIds}.
     */
    public static void reportTaskStartedNative(int taskId) {
        BackgroundTaskSchedulerUma.getInstance().reportTaskStartedNative(taskId);
    }
}