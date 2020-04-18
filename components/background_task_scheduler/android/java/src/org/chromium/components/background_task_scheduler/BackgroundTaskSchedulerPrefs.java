// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.background_task_scheduler;

import android.content.SharedPreferences;
import android.os.Build;

import org.chromium.base.ContextUtils;
import org.chromium.base.TraceEvent;

import java.util.HashSet;
import java.util.Set;

/**
 * Class handling shared preference entries for BackgroundTaskScheduler.
 */
public class BackgroundTaskSchedulerPrefs {
    private static final String TAG = "BTSPrefs";
    private static final String KEY_SCHEDULED_TASKS = "bts_scheduled_tasks";
    private static final String KEY_LAST_SDK_VERSION = "bts_last_sdk_version";

    /**
     * Class abstracting conversions between a string kept in shared preferences and actual values
     * held there.
     */
    private static class ScheduledTaskPreferenceEntry {
        private static final String ENTRY_SEPARATOR = ":";
        private String mBackgroundTaskClass;
        private int mTaskId;

        /** Creates a scheduled task shared preference entry from task info. */
        public static ScheduledTaskPreferenceEntry createForTaskInfo(TaskInfo taskInfo) {
            return new ScheduledTaskPreferenceEntry(
                    taskInfo.getBackgroundTaskClass().getName(), taskInfo.getTaskId());
        }

        /**
         * Parses a preference entry from input string.
         *
         * @param entry An input string to parse.
         * @return A parsed entry or null if the input is not valid.
         */
        public static ScheduledTaskPreferenceEntry parseEntry(String entry) {
            if (entry == null) return null;

            String[] entryParts = entry.split(ENTRY_SEPARATOR);
            if (entryParts.length != 2 || entryParts[0].isEmpty() || entryParts[1].isEmpty()) {
                return null;
            }
            int taskId = 0;
            try {
                taskId = Integer.parseInt(entryParts[1]);
            } catch (NumberFormatException e) {
                return null;
            }
            return new ScheduledTaskPreferenceEntry(entryParts[0], taskId);
        }

        public ScheduledTaskPreferenceEntry(String className, int taskId) {
            mBackgroundTaskClass = className;
            mTaskId = taskId;
        }

        /**
         * Converts a task info to a shared preference entry in the format:
         * BACKGROUND_TASK_CLASS_NAME:TASK_ID.
         */
        @Override
        public String toString() {
            return mBackgroundTaskClass + ENTRY_SEPARATOR + mTaskId;
        }

        /** Gets the name of background task class in this entry. */
        public String getBackgroundTaskClass() {
            return mBackgroundTaskClass;
        }

        /** Gets the ID of the task in this entry. */
        public int getTaskId() {
            return mTaskId;
        }
    }

    /** Adds a task to scheduler's preferences, so that it can be rescheduled with OS upgrade. */
    public static void addScheduledTask(TaskInfo taskInfo) {
        try (TraceEvent te = TraceEvent.scoped("BackgroundTaskSchedulerPrefs.addScheduledTask",
                     Integer.toString(taskInfo.getTaskId()))) {
            SharedPreferences prefs = ContextUtils.getAppSharedPreferences();
            Set<String> scheduledTasks =
                    prefs.getStringSet(KEY_SCHEDULED_TASKS, new HashSet<String>(1));
            String prefsEntry = ScheduledTaskPreferenceEntry.createForTaskInfo(taskInfo).toString();
            if (scheduledTasks.contains(prefsEntry)) return;

            // Set returned from getStringSet() should not be modified.
            scheduledTasks = new HashSet<>(scheduledTasks);
            scheduledTasks.add(prefsEntry);
            updateScheduledTasks(prefs, scheduledTasks);
        }
    }

    /** Removes a task from scheduler's preferences. */
    public static void removeScheduledTask(int taskId) {
        try (TraceEvent te = TraceEvent.scoped("BackgroundTaskSchedulerPrefs.removeScheduledTask",
                     Integer.toString(taskId))) {
            SharedPreferences prefs = ContextUtils.getAppSharedPreferences();
            Set<String> scheduledTasks = getScheduledTaskEntries(prefs);

            String entryToRemove = null;
            for (String entry : scheduledTasks) {
                ScheduledTaskPreferenceEntry parsed =
                        ScheduledTaskPreferenceEntry.parseEntry(entry);
                if (parsed != null && parsed.getTaskId() == taskId) {
                    entryToRemove = entry;
                    break;
                }
            }

            // Entry matching taskId was not found.
            if (entryToRemove == null) return;

            // Set returned from getStringSet() should not be modified.
            scheduledTasks = new HashSet<>(scheduledTasks);
            scheduledTasks.remove(entryToRemove);
            updateScheduledTasks(prefs, scheduledTasks);
        }
    }

    /** Gets a set of scheduled task class names. */
    public static Set<String> getScheduledTasks() {
        try (TraceEvent te = TraceEvent.scoped("BackgroundTaskSchedulerPrefs.getScheduledTasks")) {
            SharedPreferences prefs = ContextUtils.getAppSharedPreferences();
            Set<String> scheduledTask = getScheduledTaskEntries(prefs);
            Set<String> scheduledTasksClassNames = new HashSet<>(scheduledTask.size());
            for (String entry : scheduledTask) {
                ScheduledTaskPreferenceEntry parsed =
                        ScheduledTaskPreferenceEntry.parseEntry(entry);
                if (parsed != null) {
                    scheduledTasksClassNames.add(parsed.getBackgroundTaskClass());
                }
            }
            return scheduledTasksClassNames;
        }
    }

    /** Gets a set of scheduled task IDs. */
    public static Set<Integer> getScheduledTaskIds() {
        try (TraceEvent te =
                        TraceEvent.scoped("BackgroundTaskSchedulerPrefs.getScheduledTaskIds")) {
            SharedPreferences prefs = ContextUtils.getAppSharedPreferences();
            Set<String> scheduledTasks = getScheduledTaskEntries(prefs);
            Set<Integer> scheduledTaskIds = new HashSet<>(scheduledTasks.size());
            for (String entry : scheduledTasks) {
                ScheduledTaskPreferenceEntry parsed =
                        ScheduledTaskPreferenceEntry.parseEntry(entry);
                if (parsed != null) {
                    scheduledTaskIds.add(parsed.getTaskId());
                }
            }
            return scheduledTaskIds;
        }
    }

    /** Removes all scheduled tasks from shared preferences store. */
    public static void removeAllTasks() {
        try (TraceEvent te = TraceEvent.scoped("BackgroundTaskSchedulerPrefs.removeAllTasks")) {
            ContextUtils.getAppSharedPreferences().edit().remove(KEY_SCHEDULED_TASKS).apply();
        }
    }

    /** Gets the last SDK version on which this instance ran. Defaults to current SDK version. */
    public static int getLastSdkVersion() {
        try (TraceEvent te = TraceEvent.scoped("BackgroundTaskSchedulerPrefs.getLastSdkVersion")) {
            int sdkInt = ContextUtils.getAppSharedPreferences().getInt(
                    KEY_LAST_SDK_VERSION, Build.VERSION.SDK_INT);
            return sdkInt;
        }
    }

    /** Gets the last SDK version on which this instance ran. */
    public static void setLastSdkVersion(int sdkVersion) {
        try (TraceEvent te = TraceEvent.scoped("BackgroundTaskSchedulerPrefs.setLastSdkVersion",
                     Integer.toString(sdkVersion))) {
            ContextUtils.getAppSharedPreferences()
                    .edit()
                    .putInt(KEY_LAST_SDK_VERSION, sdkVersion)
                    .apply();
        }
    }

    private static void updateScheduledTasks(SharedPreferences prefs, Set<String> tasks) {
        SharedPreferences.Editor editor = prefs.edit();
        editor.putStringSet(KEY_SCHEDULED_TASKS, tasks);
        editor.apply();
    }

    /** Gets the entries for scheduled tasks from shared preferences. */
    private static Set<String> getScheduledTaskEntries(SharedPreferences prefs) {
        return prefs.getStringSet(KEY_SCHEDULED_TASKS, new HashSet<String>(1));
    }
}
