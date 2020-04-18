// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.init;

import org.chromium.base.ThreadUtils;

import java.util.LinkedList;

/**
 * Allows to chain multiple tasks on the UI thread, with the next task posted when one completes.
 *
 * Threading:
 * - Can construct and call {@link add()} on any thread. Note that this is not synchronized.
 * - Can call {@link start()} on any thread, blocks if called from the UI thread and coalesceTasks
 *   is true.
 * - {@link cancel()} must be called from the UI thread.
 */
public class ChainedTasks {
    private LinkedList<Runnable> mTasks = new LinkedList<>();
    private volatile boolean mFinalized;

    private final Runnable mRunAndPost = new Runnable() {
        @Override
        public void run() {
            if (mTasks.isEmpty()) return;
            mTasks.pop().run();
            ThreadUtils.postOnUiThread(this);
        }
    };

    /**
     * Adds a task to the list of tasks to run. Cannot be called once {@link start()} has been
     * called.
     */
    public void add(Runnable task) {
        if (mFinalized) throw new IllegalStateException("Must not call add() after start()");
        mTasks.add(task);
    }

    /**
     * Cancels the remaining tasks. Must be called from the UI thread.
     */
    public void cancel() {
        if (!ThreadUtils.runningOnUiThread()) {
            throw new IllegalStateException("Must call cancel() from the UI thread.");
        }
        mFinalized = true;
        mTasks.clear();
    }

    /**
     * Posts or runs all the tasks, one by one.
     *
     * @param coalesceTasks if false, posts the tasks. Otherwise run them in a single task. If
     * called on the UI thread, run in the current task.
     */
    public void start(final boolean coalesceTasks) {
        if (mFinalized) throw new IllegalStateException("Cannot call start() several times");
        mFinalized = true;
        if (coalesceTasks) {
            ThreadUtils.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    for (Runnable task : mTasks) task.run();
                    mTasks.clear();
                }
            });
        } else {
            ThreadUtils.postOnUiThread(mRunAndPost);
        }
    }
}
