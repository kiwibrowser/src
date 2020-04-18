// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.background_task_scheduler;

import android.content.Context;
import android.os.Bundle;
import android.support.annotation.IntDef;
import android.support.annotation.NonNull;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * TaskInfo represents a request to run a specific {@link BackgroundTask} given the required
 * parameters, such as whether a special type of network is available.
 */
public class TaskInfo {
    /**
     * Specifies information regarding one-off tasks.
     * This is part of a {@link TaskInfo} iff the task is NOT a periodic task.
     */
    public static class OneOffInfo {
        private final long mWindowStartTimeMs;
        private final long mWindowEndTimeMs;
        private final boolean mHasWindowStartTimeConstraint;

        private OneOffInfo(long windowStartTimeMs, long windowEndTimeMs,
                boolean hasWindowStartTimeConstraint) {
            mWindowStartTimeMs = windowStartTimeMs;
            mWindowEndTimeMs = windowEndTimeMs;
            mHasWindowStartTimeConstraint = hasWindowStartTimeConstraint;
        }

        /**
         * @return the start of the window that the task can begin executing as a delta in
         * milliseconds from now.
         */
        public long getWindowStartTimeMs() {
            return mWindowStartTimeMs;
        }

        /**
         * @return the end of the window that the task can begin executing as a delta in
         * milliseconds from now.
         */
        public long getWindowEndTimeMs() {
            return mWindowEndTimeMs;
        }

        /**
         * @return whether this one-off task has a window start time constraint.
         */
        public boolean hasWindowStartTimeConstraint() {
            return mHasWindowStartTimeConstraint;
        }

        @Override
        public String toString() {
            return "{windowStartTimeMs: " + mWindowStartTimeMs + ", windowEndTimeMs: "
                    + mWindowEndTimeMs + "}";
        }
    }

    /**
     * Specifies information regarding periodic tasks.
     * This is part of a {@link TaskInfo} iff the task is a periodic task.
     */
    public static class PeriodicInfo {
        private final long mIntervalMs;
        private final long mFlexMs;
        private final boolean mHasFlex;

        private PeriodicInfo(long intervalMs, long flexMs, boolean hasFlex) {
            mIntervalMs = intervalMs;
            mFlexMs = flexMs;
            mHasFlex = hasFlex;
        }

        /**
         * @return the interval between occurrences of this task in milliseconds.
         */
        public long getIntervalMs() {
            return mIntervalMs;
        }

        /**
         * @return the flex time for this task. The task can execute at any time in a window of flex
         * length at the end of the period. It is reported in milliseconds.
         */
        public long getFlexMs() {
            return mFlexMs;
        }

        /**
         * @return true whether this task has defined a flex time. False otherwise.
         */
        public boolean hasFlex() {
            return mHasFlex;
        }

        @Override
        public String toString() {
            StringBuilder sb = new StringBuilder("{");
            sb.append("{");
            sb.append("intervalMs: ").append(mIntervalMs);
            if (mHasFlex) {
                sb.append(", flexMs: ").append(mFlexMs);
            }
            sb.append("}");
            return sb.toString();
        }
    }

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({NETWORK_TYPE_NONE, NETWORK_TYPE_ANY, NETWORK_TYPE_UNMETERED})
    public @interface NetworkType {}

    /**
     * This task has no requirements for network connectivity. Default.
     *
     * @see NetworkType
     */
    public static final int NETWORK_TYPE_NONE = 0;
    /**
     * This task requires network connectivity.
     *
     * @see NetworkType
     */
    public static final int NETWORK_TYPE_ANY = 1;
    /**
     * This task requires network connectivity that is unmetered.
     *
     * @see NetworkType
     */
    public static final int NETWORK_TYPE_UNMETERED = 2;

    /**
     * The task ID should be unique across all tasks. A list of such unique IDs exists in
     * {@link TaskIds}.
     */
    private final int mTaskId;

    /**
     * The {@link BackgroundTask} to invoke when this task is run.
     */
    @NonNull
    private final Class<? extends BackgroundTask> mBackgroundTaskClass;

    /**
     * The extras to provide to the {@link BackgroundTask} when it is run.
     */
    @NonNull
    private final Bundle mExtras;

    /**
     * The type of network the task requires to run.
     */
    @NetworkType
    private final int mRequiredNetworkType;

    /**
     * Whether the task requires charging to run.
     */
    private final boolean mRequiresCharging;

    /**
     * Whether or not to persist this task across device reboots.
     */
    private final boolean mIsPersisted;

    /**
     * Whether this task should override any preexisting tasks with the same task id.
     */
    private final boolean mUpdateCurrent;

    /**
     * Whether this task is periodic.
     */
    private final boolean mIsPeriodic;

    /**
     * Task information regarding one-off tasks. Non-null iff {@link #mIsPeriodic} is false.
     */
    private final OneOffInfo mOneOffInfo;

    /**
     * Task information regarding periodic tasks. Non-null iff {@link #mIsPeriodic} is true.
     */
    private final PeriodicInfo mPeriodicInfo;

    private TaskInfo(Builder builder) {
        mTaskId = builder.mTaskId;
        mBackgroundTaskClass = builder.mBackgroundTaskClass;
        mExtras = builder.mExtras == null ? new Bundle() : builder.mExtras;
        mRequiredNetworkType = builder.mRequiredNetworkType;
        mRequiresCharging = builder.mRequiresCharging;
        mIsPersisted = builder.mIsPersisted;
        mUpdateCurrent = builder.mUpdateCurrent;
        mIsPeriodic = builder.mIsPeriodic;
        if (mIsPeriodic) {
            mOneOffInfo = null;
            mPeriodicInfo =
                    new PeriodicInfo(builder.mIntervalMs, builder.mFlexMs, builder.mHasFlex);
        } else {
            mOneOffInfo = new OneOffInfo(builder.mWindowStartTimeMs, builder.mWindowEndTimeMs,
                    builder.mHasWindowStartTimeConstraint);
            mPeriodicInfo = null;
        }
    }

    /**
     * @return the unique ID of this task.
     */
    public int getTaskId() {
        return mTaskId;
    }

    /**
     * @return the {@link BackgroundTask} class that will be instantiated for this task.
     */
    @NonNull
    public Class<? extends BackgroundTask> getBackgroundTaskClass() {
        return mBackgroundTaskClass;
    }

    /**
     * @return the extras that will be provided to the {@link BackgroundTask}.
     */
    @NonNull
    public Bundle getExtras() {
        return mExtras;
    }

    /**
     * @return the type of network the task requires to run.
     */
    @NetworkType
    public int getRequiredNetworkType() {
        return mRequiredNetworkType;
    }

    /**
     * @return whether the task requires charging to run.
     */
    public boolean requiresCharging() {
        return mRequiresCharging;
    }

    /**
     * @return whether or not to persist this task across device reboots.
     */
    public boolean isPersisted() {
        return mIsPersisted;
    }

    /**
     * @return whether this task should override any preexisting tasks with the same task id.
     */
    public boolean shouldUpdateCurrent() {
        return mUpdateCurrent;
    }

    /**
     * @return Whether or not this task is a periodic task.
     */
    public boolean isPeriodic() {
        return mIsPeriodic;
    }

    /**
    * This is part of a {@link TaskInfo} iff the task is NOT a periodic task, i.e.
    * {@link TaskInfo#isPeriodic()} returns false.
    *
    * @return the specific data that is only available for one-off tasks.
    */
    public OneOffInfo getOneOffInfo() {
        return mOneOffInfo;
    }

    /**
     * This is part of a {@link TaskInfo} iff the task is a periodic task, i.e.
     * {@link TaskInfo#isPeriodic()} returns true.
     *
     * @return the specific data that is only available for periodic tasks.
     */
    public PeriodicInfo getPeriodicInfo() {
        return mPeriodicInfo;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("{");
        sb.append("taskId: ").append(mTaskId);
        sb.append(", backgroundTaskClass: ").append(mBackgroundTaskClass);
        sb.append(", extras: ").append(mExtras);
        sb.append(", requiredNetworkType: ").append(mRequiredNetworkType);
        sb.append(", requiresCharging: ").append(mRequiresCharging);
        sb.append(", isPersisted: ").append(mIsPersisted);
        sb.append(", updateCurrent: ").append(mUpdateCurrent);
        sb.append(", isPeriodic: ").append(mIsPeriodic);
        if (isPeriodic()) {
            sb.append(", periodicInfo: ").append(mPeriodicInfo);
        } else {
            sb.append(", oneOffInfo: ").append(mOneOffInfo);
        }
        sb.append("}");
        return sb.toString();
    }

    /**
     * Schedule a one-off task to execute within a deadline. If windowEndTimeMs is 0, the task will
     * run as soon as possible. For executing a task within a time window, see
     * {@link #createOneOffTask(int, Class, long, long)}.
     *
     * @param taskId the unique task ID for this task. Should be listed in {@link TaskIds}.
     * @param backgroundTaskClass the {@link BackgroundTask} class that will be instantiated for
     * this task.
     * @param windowEndTimeMs the end of the window that the task can begin executing as a delta in
     * milliseconds from now.
     * @return the builder which can be used to continue configuration and {@link Builder#build()}.
     * @see TaskIds
     */
    public static Builder createOneOffTask(
            int taskId, Class<? extends BackgroundTask> backgroundTaskClass, long windowEndTimeMs) {
        return new Builder(taskId, backgroundTaskClass, false).setWindowEndTimeMs(windowEndTimeMs);
    }

    /**
     * Schedule a one-off task to execute within a time window. For executing a task within a
     * deadline, see {@link #createOneOffTask(int, Class, long)},
     *
     * @param taskId the unique task ID for this task. Should be listed in {@link TaskIds}.
     * @param backgroundTaskClass the {@link BackgroundTask} class that will be instantiated for
     * this task.
     * @param windowStartTimeMs the start of the window that the task can begin executing as a delta
     * in milliseconds from now.
     * @param windowEndTimeMs the end of the window that the task can begin executing as a delta in
     * milliseconds from now.
     * @return the builder which can be used to continue configuration and {@link Builder#build()}.
     * @see TaskIds
     */
    public static Builder createOneOffTask(int taskId,
            Class<? extends BackgroundTask> backgroundTaskClass, long windowStartTimeMs,
            long windowEndTimeMs) {
        return new Builder(taskId, backgroundTaskClass, false)
                .setWindowStartTimeMs(windowStartTimeMs)
                .setWindowEndTimeMs(windowEndTimeMs);
    }

    /**
     * Schedule a periodic task that will recur at the specified interval, without the need to
     * be rescheduled. The task will continue to recur until
     * {@link BackgroundTaskScheduler#cancel(Context, int)} is invoked with the task ID from this
     * {@link TaskInfo}.
     *
     * @param taskId the unique task ID for this task. Should be listed in {@link TaskIds}.
     * @param backgroundTaskClass the {@link BackgroundTask} class that will be instantiated for
     * this task.
     * @param intervalMs the interval between occurrences of this task in milliseconds.
     * @return the builder which can be used to continue configuration and {@link Builder#build()}.
     * @see TaskIds
     */
    public static Builder createPeriodicTask(
            int taskId, Class<? extends BackgroundTask> backgroundTaskClass, long intervalMs) {
        return new Builder(taskId, backgroundTaskClass, true).setIntervalMs(intervalMs);
    }

    /**
     * Schedule a periodic task that will recur at the specified interval, without the need to
     * be rescheduled. The task will continue to recur until
     * {@link BackgroundTaskScheduler#cancel(Context, int)} is invoked with the task ID from this
     * {@link TaskInfo}.
     * The flex time specifies how close to the end of the interval you are willing to execute.
     * Instead of executing at the exact interval, the task will execute at the interval or up to
     * flex milliseconds before.
     *
     * @param taskId the unique task ID for this task. Should be listed in {@link TaskIds}.
     * @param backgroundTaskClass the {@link BackgroundTask} class that will be instantiated for
     * this task.
     * @param intervalMs the interval between occurrences of this task in milliseconds.
     * @param flexMs the flex time for this task. The task can execute at any time in a window of
     * flex
     * length at the end of the period. It is reported in milliseconds.
     * @return the builder which can be used to continue configuration and {@link Builder#build()}.
     * @see TaskIds
     */
    public static Builder createPeriodicTask(int taskId,
            Class<? extends BackgroundTask> backgroundTaskClass, long intervalMs, long flexMs) {
        return new Builder(taskId, backgroundTaskClass, true)
                .setIntervalAndFlexMs(intervalMs, flexMs);
    }

    /**
     * A helper builder to provide a way to build {@link TaskInfo}. To create a {@link Builder}
     * use one of the create* class method on {@link TaskInfo}.
     *
     * @see #createOneOffTask(int, Class, long)
     * @see #createOneOffTask(int, Class, long, long)
     * @see #createPeriodicTask(int, Class, long)
     * @see #createPeriodicTask(int, Class, long, long)
     */
    public static final class Builder {
        private final int mTaskId;
        @NonNull
        private final Class<? extends BackgroundTask> mBackgroundTaskClass;
        private final boolean mIsPeriodic;
        private Bundle mExtras;
        @NetworkType
        private int mRequiredNetworkType;
        private boolean mRequiresCharging;
        private boolean mIsPersisted;
        private boolean mUpdateCurrent;

        // Data about one-off tasks.
        private long mWindowStartTimeMs;
        private long mWindowEndTimeMs;
        private boolean mHasWindowStartTimeConstraint;

        // Data about periodic tasks.
        private long mIntervalMs;
        private long mFlexMs;
        private boolean mHasFlex;

        Builder(int taskId, @NonNull Class<? extends BackgroundTask> backgroundTaskClass,
                boolean isPeriodic) {
            mTaskId = taskId;
            mBackgroundTaskClass = backgroundTaskClass;
            mIsPeriodic = isPeriodic;
        }

        Builder setWindowStartTimeMs(long windowStartTimeMs) {
            assert !mIsPeriodic;
            mWindowStartTimeMs = windowStartTimeMs;
            mHasWindowStartTimeConstraint = true;
            return this;
        }

        Builder setWindowEndTimeMs(long windowEndTimeMs) {
            assert !mIsPeriodic;
            mWindowEndTimeMs = windowEndTimeMs;
            return this;
        }

        Builder setIntervalMs(long intervalMs) {
            assert mIsPeriodic;
            mIntervalMs = intervalMs;
            return this;
        }

        Builder setIntervalAndFlexMs(long intervalMs, long flexMs) {
            assert mIsPeriodic;
            mIntervalMs = intervalMs;
            mFlexMs = flexMs;
            mHasFlex = true;
            return this;
        }

        /**
         * Set the optional extra values necessary for this task. Must only ever contain simple
         * values supported by {@link android.os.BaseBundle}. All other values are thrown away.
         * If the extras for this builder are not set, or set to null, the resulting
         * {@link TaskInfo} will have an empty bundle (i.e. not null).
         *
         * @param bundle the bundle of extra values necessary for this task.
         * @return this {@link Builder}.
         */
        public Builder setExtras(Bundle bundle) {
            mExtras = bundle;
            return this;
        }

        /**
         * Set the type of network the task requires to run.
         *
         * @param networkType the {@link NetworkType} required for this task.
         * @return this {@link Builder}.
         */
        public Builder setRequiredNetworkType(@NetworkType int networkType) {
            mRequiredNetworkType = networkType;
            return this;
        }

        /**
         * Set whether the task requires charging to run.
         *
         * @param requiresCharging true if this task requires charging.
         * @return this {@link Builder}.
         */
        public Builder setRequiresCharging(boolean requiresCharging) {
            mRequiresCharging = requiresCharging;
            return this;
        }

        /**
         * Set whether or not to persist this task across device reboots.
         *
         * @param isPersisted true if this task should be persisted across reboots.
         * @return this {@link Builder}.
         */
        public Builder setIsPersisted(boolean isPersisted) {
            mIsPersisted = isPersisted;
            return this;
        }

        /**
         * Set whether this task should override any preexisting tasks with the same task id.
         *
         * @param updateCurrent true if this task should overwrite a currently existing task with
         *                      the same ID, if it exists.
         * @return this {@link Builder}.
         */
        public Builder setUpdateCurrent(boolean updateCurrent) {
            mUpdateCurrent = updateCurrent;
            return this;
        }

        /**
         * Build the {@link TaskInfo object} specified by this builder.
         *
         * @return the {@link TaskInfo} object.
         */
        public TaskInfo build() {
            return new TaskInfo(this);
        }
    }
}
