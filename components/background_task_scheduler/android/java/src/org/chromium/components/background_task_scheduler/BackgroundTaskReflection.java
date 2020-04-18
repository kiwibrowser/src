// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.background_task_scheduler;

import android.support.annotation.Nullable;

import org.chromium.base.Log;

import java.lang.reflect.Constructor;

/**
 * BakgroundTaskReflection contains functionality to construct {@link BackgroundTask} instances from
 * their classname and to inspect whether a particular {@link BackgroundTask} has a parameterless
 * constructor, which is required for any {@link BackgroundTask}.
 */
final class BackgroundTaskReflection {
    private static final String TAG = "BkgrdTaskReflect";

    /**
     * Uses reflection to find the given class and instantiates a {@link BackgroundTask} if the
     * class is valid.
     *
     * @param backgroundTaskClassName the full class name to look for.
     * @return a new {@link BackgroundTask} instance if successful or null when a failure occurs.
     */
    @Nullable
    static BackgroundTask getBackgroundTaskFromClassName(String backgroundTaskClassName) {
        if (backgroundTaskClassName == null) return null;

        Class<?> clazz;
        try {
            clazz = Class.forName(backgroundTaskClassName);
        } catch (ClassNotFoundException e) {
            Log.w(TAG, "Unable to find BackgroundTask class with name " + backgroundTaskClassName);
            return null;
        }

        if (!BackgroundTask.class.isAssignableFrom(clazz)) {
            Log.w(TAG, "Class " + clazz + " is not a BackgroundTask");
            return null;
        }

        try {
            return (BackgroundTask) clazz.newInstance();
        } catch (InstantiationException e) {
            Log.w(TAG, "Unable to instantiate class (InstExc) " + clazz);
            return null;
        } catch (IllegalAccessException e) {
            Log.w(TAG, "Unable to instantiate class (IllAccExc) " + clazz);
            return null;
        }
    }

    /**
     * Inspects all public constructors of the given class, and returns true if one of them is
     * parameterless.
     *
     * @param clazz the class to inspect.
     * @return whether the class has a parameterless constructor.
     */
    static boolean hasParameterlessPublicConstructor(Class<? extends BackgroundTask> clazz) {
        for (Constructor<?> constructor : clazz.getConstructors()) {
            if (constructor.getParameterTypes().length == 0) return true;
        }
        return false;
    }

    private BackgroundTaskReflection() {
        // No instantiation.
    }
}
