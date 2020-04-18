// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.testing.local;

import android.os.AsyncTask;

import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.shadows.ShadowAsyncTask;

import java.util.concurrent.Executor;

/**
 * Forces async tasks to execute with the default executor.
 * This works around Robolectric not working out of the box with custom executors.
 */
@Implements(AsyncTask.class)
public class CustomShadowAsyncTask<Params, Progress, Result>
        extends ShadowAsyncTask<Params, Progress, Result> {
    @SafeVarargs
    @Override
    @Implementation
    public final AsyncTask<Params, Progress, Result> executeOnExecutor(
            Executor executor, Params... params) {
        return super.execute(params);
    }
}
