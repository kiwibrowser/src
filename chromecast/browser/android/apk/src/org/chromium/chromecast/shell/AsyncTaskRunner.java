// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.shell;

import android.os.AsyncTask;

import org.chromium.chromecast.base.Consumer;
import org.chromium.chromecast.base.Scope;
import org.chromium.chromecast.base.Supplier;

import java.util.concurrent.Executor;

/**
 * Runs a task on a worker thread, then run the callback with the result on the UI thread.
 *
 * This is a slightly less verbose way of doing asynchronous work than using android.os.AsyncTask
 * directly.
 *
 * Since this implements Scope, a function that returns the result of doAsync() can easily be used
 * as a ScopeFactory in an Observable#watch() call, which can be used to cancel running tasks if
 * the Observable deactivates.
 */
public class AsyncTaskRunner {
    private final Executor mExecutor;

    public AsyncTaskRunner(Executor executor) {
        mExecutor = executor;
    }

    public AsyncTaskRunner() {
        mExecutor = null;
    }

    public <T> Scope doAsync(Supplier<T> task, Consumer<? super T> callback) {
        AsyncTask<Void, Void, T> asyncTask = new AsyncTask<Void, Void, T>() {
            @Override
            protected T doInBackground(Void... params) {
                return task.get();
            }
            @Override
            protected void onPostExecute(T result) {
                callback.accept(result);
            }
        };
        if (mExecutor != null) {
            asyncTask.executeOnExecutor(mExecutor);
        } else {
            asyncTask.execute();
        }
        return () -> asyncTask.cancel(false);
    }
}
