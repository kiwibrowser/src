// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.testing.local;

import static org.junit.Assert.fail;

import android.os.AsyncTask;

import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.shadows.ShadowApplication;
import org.robolectric.shadows.ShadowAsyncTask;

import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * Executes async tasks on a background thread and waits on the result on the main thread.
 * This is useful for users of AsyncTask on Roboelectric who check if the code is actually being
 * run on a background thread (i.e. through the use of {@link ThreadUtils#runningOnUiThread()}).
 * @param <Params>     type for execute function parameters
 * @param <Progress>   type for reporting Progress
 * @param <Result>     type for reporting result
 */
@Implements(AsyncTask.class)
public class BackgroundShadowAsyncTask<Params, Progress, Result> extends
        ShadowAsyncTask<Params, Progress, Result> {

    private static final ExecutorService sExecutorService = Executors.newSingleThreadExecutor();

    @Override
    @Implementation
    @SafeVarargs
    public final AsyncTask<Params, Progress, Result> execute(final Params... params) {
        try {
            return sExecutorService.submit(new Callable<AsyncTask<Params, Progress, Result>>() {
                @Override
                public AsyncTask<Params, Progress, Result> call() throws Exception {
                    return BackgroundShadowAsyncTask.super.execute(params);
                }
            }).get();
        } catch (Exception ex) {
            fail(ex.getMessage());
            return null;
        }
    }

    @Override
    @Implementation
    public final Result get() {
        try {
            runBackgroundTasks();
            return BackgroundShadowAsyncTask.super.get();
        } catch (Exception e) {
            return null;
        }
    }

    public static void runBackgroundTasks() throws Exception {
        sExecutorService.submit(new Runnable() {
            @Override
            public void run() {
                ShadowApplication.runBackgroundTasks();
            }
        }).get();
    }
}
