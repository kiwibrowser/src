// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.annotation.SuppressLint;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Environment;
import android.text.TextUtils;

import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.content.R;
import org.chromium.ui.widget.Toast;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.TimeZone;

/**
 * Controller for Chrome's tracing feature.
 *
 * We don't have any UI per se. Just call startTracing() to start and
 * stopTracing() to stop. We'll report progress to the user with Toasts.
 *
 * If the host application registers this class's BroadcastReceiver, you can
 * also start and stop the tracer with a broadcast intent, as follows:
 * <ul>
 * <li>To start tracing: am broadcast -a org.chromium.content_shell_apk.GPU_PROFILER_START
 * <li>Add "-e file /foo/bar/xyzzy" to log trace data to a specific file.
 * <li>To stop tracing: am broadcast -a org.chromium.content_shell_apk.GPU_PROFILER_STOP
 * </ul>
 * Note that the name of these intents change depending on which application
 * is being traced, but the general form is [app package name].GPU_PROFILER_{START,STOP}.
 */
@JNINamespace("content")
public class TracingControllerAndroid {

    private static final String TAG = "cr.TracingController";

    private static final String ACTION_START = "GPU_PROFILER_START";
    private static final String ACTION_STOP = "GPU_PROFILER_STOP";
    private static final String ACTION_LIST_CATEGORIES = "GPU_PROFILER_LIST_CATEGORIES";
    private static final String FILE_EXTRA = "file";
    private static final String CATEGORIES_EXTRA = "categories";
    private static final String RECORD_CONTINUOUSLY_EXTRA = "continuous";
    private static final String DEFAULT_CHROME_CATEGORIES_PLACE_HOLDER =
            "_DEFAULT_CHROME_CATEGORIES";

    // These strings must match the ones expected by adb_profile_chrome.
    private static final String PROFILER_STARTED_FMT = "Profiler started: %s";
    private static final String PROFILER_FINISHED_FMT =
            "Profiler finished. Results are in %s.";

    private final Context mContext;
    private final TracingBroadcastReceiver mBroadcastReceiver;
    private final TracingIntentFilter mIntentFilter;
    private boolean mIsTracing;

    // We might not want to always show toasts when we start the profiler, especially if
    // showing the toast impacts performance.  This gives us the chance to disable them.
    private boolean mShowToasts = true;

    private String mFilename;

    public TracingControllerAndroid(Context context) {
        mContext = context;
        mBroadcastReceiver = new TracingBroadcastReceiver();
        mIntentFilter = new TracingIntentFilter(context);
    }

    /**
     * Get a BroadcastReceiver that can handle profiler intents.
     */
    public BroadcastReceiver getBroadcastReceiver() {
        return mBroadcastReceiver;
    }

    /**
     * Get an IntentFilter for profiler intents.
     */
    public IntentFilter getIntentFilter() {
        return mIntentFilter;
    }

    /**
     * Register a BroadcastReceiver in the given context.
     */
    public void registerReceiver(Context context) {
        context.registerReceiver(getBroadcastReceiver(), getIntentFilter());
    }

    /**
     * Unregister the GPU BroadcastReceiver in the given context.
     * @param context
     */
    public void unregisterReceiver(Context context) {
        context.unregisterReceiver(getBroadcastReceiver());
    }

    /**
     * Returns true if we're currently profiling.
     */
    public boolean isTracing() {
        return mIsTracing;
    }

    /**
     * Returns the path of the current output file. Null if isTracing() false.
     */
    public String getOutputPath() {
        return mFilename;
    }

    /**
     * Generates a unique filename to be used for tracing in the Downloads directory.
     */
    @CalledByNative
    private static String generateTracingFilePath() {
        String state = Environment.getExternalStorageState();
        if (!Environment.MEDIA_MOUNTED.equals(state)) {
            return null;
        }

        // Generate a hopefully-unique filename using the UTC timestamp.
        // (Not a huge problem if it isn't unique, we'll just append more data.)
        SimpleDateFormat formatter = new SimpleDateFormat(
                "yyyy-MM-dd-HHmmss", Locale.US);
        formatter.setTimeZone(TimeZone.getTimeZone("UTC"));
        File dir = Environment.getExternalStoragePublicDirectory(
                Environment.DIRECTORY_DOWNLOADS);
        File file = new File(
                dir, "chrome-profile-results-" + formatter.format(new Date()));
        return file.getPath();
    }

    /**
     * Start profiling to a new file in the Downloads directory.
     *
     * Calls #startTracing(String, boolean, String, String) with a new timestamped filename.
     * @see #startTracing(String, boolean, String, String)
     */
    public boolean startTracing(boolean showToasts, String categories, String traceOptions) {
        mShowToasts = showToasts;

        String filePath = generateTracingFilePath();
        if (filePath == null) {
            logAndToastError(mContext.getString(R.string.profiler_no_storage_toast));
        }
        return startTracing(filePath, showToasts, categories, traceOptions);
    }

    private void initializeNativeControllerIfNeeded() {
        if (mNativeTracingControllerAndroid == 0) {
            mNativeTracingControllerAndroid = nativeInit();
        }
    }

    /**
     * Start profiling to the specified file. Returns true on success.
     *
     * Only one TracingControllerAndroid can be running at the same time. If another profiler
     * is running when this method is called, it will be cancelled. If this
     * profiler is already running, this method does nothing and returns false.
     *
     * @param filename The name of the file to output the profile data to.
     * @param showToasts Whether or not we want to show toasts during this profiling session.
     * When we are timing the profile run we might not want to incur extra draw overhead of showing
     * notifications about the profiling system.
     * @param categories Which categories to trace. See TracingControllerAndroid::BeginTracing()
     * (in content/public/browser/trace_controller.h) for the format.
     * @param traceOptions Which trace options to use. See
     * TraceOptions::TraceOptions(const std::string& options_string)
     * (in base/trace_event/trace_event_impl.h) for the format.
     */
    public boolean startTracing(String filename, boolean showToasts, String categories,
            String traceOptions) {
        mShowToasts = showToasts;
        if (isTracing()) {
            // Don't need a toast because this shouldn't happen via the UI.
            Log.e(TAG, "Received startTracing, but we're already tracing");
            return false;
        }
        // Lazy initialize the native side, to allow construction before the library is loaded.
        initializeNativeControllerIfNeeded();
        if (!nativeStartTracing(mNativeTracingControllerAndroid, categories,
                traceOptions.toString())) {
            logAndToastError(mContext.getString(R.string.profiler_error_toast));
            return false;
        }

        logForProfiler(String.format(PROFILER_STARTED_FMT, categories));
        showToast(mContext.getString(R.string.profiler_started_toast) + ": " + categories);
        mFilename = filename;
        mIsTracing = true;
        return true;
    }

    /**
     * Stop profiling. This won't take effect until Chrome has flushed its file.
     */
    public void stopTracing() {
        if (isTracing()) {
            nativeStopTracing(mNativeTracingControllerAndroid, mFilename);
        }
    }

    /**
     * Called by native code when the profiler's output file is closed.
     */
    @CalledByNative
    protected void onTracingStopped() {
        if (!isTracing()) {
            // Don't need a toast because this shouldn't happen via the UI.
            Log.e(TAG, "Received onTracingStopped, but we aren't tracing");
            return;
        }

        logForProfiler(String.format(PROFILER_FINISHED_FMT, mFilename));
        showToast(mContext.getString(R.string.profiler_stopped_toast, mFilename));
        mIsTracing = false;
        mFilename = null;
    }

    /**
     * Get known category groups.
     */
    public void getCategoryGroups() {
        // Lazy initialize the native side, to allow construction before the library is loaded.
        initializeNativeControllerIfNeeded();
        if (!nativeGetKnownCategoryGroupsAsync(mNativeTracingControllerAndroid)) {
            Log.e(TAG, "Unable to fetch tracing record groups list.");
        }
    }

    /**
     * Clean up the C++ side of this class.
     * After the call, this class instance shouldn't be used.
     */
    public void destroy() {
        if (mNativeTracingControllerAndroid != 0) {
            nativeDestroy(mNativeTracingControllerAndroid);
            mNativeTracingControllerAndroid = 0;
        }
    }

    private void logAndToastError(String str) {
        Log.e(TAG, str);
        if (mShowToasts) Toast.makeText(mContext, str, Toast.LENGTH_SHORT).show();
    }

    // The |str| string needs to match the ones that adb_chrome_profiler looks for.
    private void logForProfiler(String str) {
        Log.i(TAG, str);
    }

    private void showToast(String str) {
        if (mShowToasts) Toast.makeText(mContext, str, Toast.LENGTH_SHORT).show();
    }

    // TODO(crbug.com/635567): Fix this properly.
    @SuppressLint("ParcelCreator")
    private static class TracingIntentFilter extends IntentFilter {
        TracingIntentFilter(Context context) {
            addAction(context.getPackageName() + "." + ACTION_START);
            addAction(context.getPackageName() + "." + ACTION_STOP);
            addAction(context.getPackageName() + "." + ACTION_LIST_CATEGORIES);
        }
    }

    class TracingBroadcastReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent.getAction().endsWith(ACTION_START)) {
                String categories = intent.getStringExtra(CATEGORIES_EXTRA);
                if (TextUtils.isEmpty(categories)) {
                    categories = nativeGetDefaultCategories();
                } else {
                    categories = categories.replaceFirst(
                            DEFAULT_CHROME_CATEGORIES_PLACE_HOLDER, nativeGetDefaultCategories());
                }
                String traceOptions = intent.getStringExtra(RECORD_CONTINUOUSLY_EXTRA) == null
                        ? "record-until-full" : "record-continuously";
                String filename = intent.getStringExtra(FILE_EXTRA);
                if (filename != null) {
                    startTracing(filename, true, categories, traceOptions);
                } else {
                    startTracing(true, categories, traceOptions);
                }
            } else if (intent.getAction().endsWith(ACTION_STOP)) {
                stopTracing();
            } else if (intent.getAction().endsWith(ACTION_LIST_CATEGORIES)) {
                getCategoryGroups();
            } else {
                Log.e(TAG, "Unexpected intent: %s", intent);
            }
        }
    }

    private long mNativeTracingControllerAndroid;
    private native long nativeInit();
    private native void nativeDestroy(long nativeTracingControllerAndroid);
    private native boolean nativeStartTracing(
            long nativeTracingControllerAndroid, String categories, String traceOptions);
    private native void nativeStopTracing(long nativeTracingControllerAndroid, String filename);
    private native boolean nativeGetKnownCategoryGroupsAsync(long nativeTracingControllerAndroid);
    private native String nativeGetDefaultCategories();
}
