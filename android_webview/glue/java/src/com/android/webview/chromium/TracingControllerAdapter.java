// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.android.webview.chromium;

import android.annotation.SuppressLint;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.webkit.TracingConfig;
import android.webkit.TracingController;

import org.chromium.android_webview.AwTracingController;
import org.chromium.base.ThreadUtils;
import org.chromium.base.TraceRecordMode;

import java.io.IOException;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Collection;
import java.util.concurrent.Callable;
import java.util.concurrent.Executor;

/**
 * Chromium implementation of TracingController -- forwards calls to
 * the chromium internal implementation and makes sure the calls happen on the
 * UI thread. Translates predefined categories and posts callbacks.
 */
@SuppressLint({"NewApi", // TracingController is new in API level 28.
        "Override"}) // Remove this once lint is targeting API level 28.
public class TracingControllerAdapter extends TracingController {
    private final AwTracingController mAwTracingController;
    private final WebViewChromiumFactoryProvider mFactory;

    public TracingControllerAdapter(
            WebViewChromiumFactoryProvider factory, AwTracingController controller) {
        mFactory = factory;
        mAwTracingController = controller;
    }

    @Override
    public void start(@NonNull TracingConfig tracingConfig) {
        if (tracingConfig == null) {
            throw new IllegalArgumentException("tracingConfig cannot be null");
        }

        int result = 0;
        if (checkNeedsPost()) {
            result = mFactory.runOnUiThreadBlocking(new Callable<Integer>() {
                @Override
                public Integer call() {
                    return startOnUI(tracingConfig);
                }
            });
        } else {
            result = startOnUI(tracingConfig);
        }

        if (result != AwTracingController.RESULT_SUCCESS) {
            // make sure to throw on the original calling thread.
            switch (result) {
                case AwTracingController.RESULT_ALREADY_TRACING:
                    throw new IllegalStateException(
                            "cannot start tracing: tracing is already enabled");
                case AwTracingController.RESULT_INVALID_CATEGORIES:
                    throw new IllegalArgumentException(
                            "category patterns starting with '-' or containing ','"
                            + " are not allowed");
                case AwTracingController.RESULT_INVALID_MODE:
                    throw new IllegalArgumentException("invalid tracing mode");
            }
        }
    }

    @Override
    public boolean stop(@Nullable OutputStream outputStream, @NonNull Executor executor) {
        if (checkNeedsPost()) {
            return mFactory.runOnUiThreadBlocking(new Callable<Boolean>() {
                @Override
                public Boolean call() {
                    return stopOnUI(outputStream, executor);
                }
            });
        }
        return stopOnUI(outputStream, executor);
    }

    @Override
    public boolean isTracing() {
        if (checkNeedsPost()) {
            return mFactory.runOnUiThreadBlocking(new Callable<Boolean>() {
                @Override
                public Boolean call() {
                    return mAwTracingController.isTracing();
                }
            });
        }
        return mAwTracingController.isTracing();
    }

    private int convertAndroidTracingMode(int tracingMode) {
        switch (tracingMode) {
            case TracingConfig.RECORD_UNTIL_FULL:
                return TraceRecordMode.RECORD_UNTIL_FULL;
            case TracingConfig.RECORD_CONTINUOUSLY:
                return TraceRecordMode.RECORD_CONTINUOUSLY;
        }
        return TraceRecordMode.RECORD_CONTINUOUSLY;
    }

    private boolean categoryIsSet(int bitmask, int categoryMask) {
        return (bitmask & categoryMask) == categoryMask;
    }

    private Collection<Integer> collectPredefinedCategories(int bitmask) {
        ArrayList<Integer> predefinedIndices = new ArrayList<Integer>();
        // CATEGORIES_NONE is skipped on purpose.
        if (categoryIsSet(bitmask, TracingConfig.CATEGORIES_ALL)) {
            predefinedIndices.add(AwTracingController.CATEGORIES_ALL);
        }
        if (categoryIsSet(bitmask, TracingConfig.CATEGORIES_ANDROID_WEBVIEW)) {
            predefinedIndices.add(AwTracingController.CATEGORIES_ANDROID_WEBVIEW);
        }
        if (categoryIsSet(bitmask, TracingConfig.CATEGORIES_WEB_DEVELOPER)) {
            predefinedIndices.add(AwTracingController.CATEGORIES_WEB_DEVELOPER);
        }
        if (categoryIsSet(bitmask, TracingConfig.CATEGORIES_INPUT_LATENCY)) {
            predefinedIndices.add(AwTracingController.CATEGORIES_INPUT_LATENCY);
        }
        if (categoryIsSet(bitmask, TracingConfig.CATEGORIES_RENDERING)) {
            predefinedIndices.add(AwTracingController.CATEGORIES_RENDERING);
        }
        if (categoryIsSet(bitmask, TracingConfig.CATEGORIES_JAVASCRIPT_AND_RENDERING)) {
            predefinedIndices.add(AwTracingController.CATEGORIES_JAVASCRIPT_AND_RENDERING);
        }
        if (categoryIsSet(bitmask, TracingConfig.CATEGORIES_FRAME_VIEWER)) {
            predefinedIndices.add(AwTracingController.CATEGORIES_FRAME_VIEWER);
        }
        return predefinedIndices;
    }

    private int startOnUI(TracingConfig tracingConfig) {
        return mAwTracingController.start(
                collectPredefinedCategories(tracingConfig.getPredefinedCategories()),
                tracingConfig.getCustomIncludedCategories(),
                convertAndroidTracingMode(tracingConfig.getTracingMode()));
    }

    public boolean stopOnUI(@Nullable OutputStream outputStream, @NonNull Executor executor) {
        if (outputStream == null) {
            return mAwTracingController.stopAndFlush((OutputStream) null);
        }

        final OutputStream localOutputStream = outputStream;
        return mAwTracingController.stopAndFlush(new OutputStream() {
            @Override
            public void write(byte[] chunk) {
                executor.execute(() -> {
                    try {
                        localOutputStream.write(chunk);
                    } catch (IOException e) {
                        throw new RuntimeException(e);
                    }
                });
            }
            @Override
            public void close() {
                executor.execute(() -> {
                    try {
                        localOutputStream.close();
                    } catch (IOException e) {
                        throw new RuntimeException(e);
                    }
                });
            }
            @Override
            public void write(int b) { /* should not be called */
            }
            @Override
            public void flush() { /* should not be called */
            }
            @Override
            public void write(byte[] b, int off, int len) { /* should not be called */
            }
        });
    }

    private static boolean checkNeedsPost() {
        return !ThreadUtils.runningOnUiThread();
    }
}
