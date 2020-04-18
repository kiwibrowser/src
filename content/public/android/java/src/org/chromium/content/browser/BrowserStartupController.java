// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.content.Context;
import android.os.Handler;
import android.os.StrictMode;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.ResourceExtractor;
import org.chromium.base.ThreadUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.base.library_loader.LoaderErrors;
import org.chromium.base.library_loader.ProcessInitException;
import org.chromium.content.app.ContentMain;

import java.util.ArrayList;
import java.util.List;

/**
 * This class controls how C++ browser main loop is started and ensures it happens only once.
 *
 * It supports kicking off the startup sequence in an asynchronous way. Startup can be called as
 * many times as needed (for instance, multiple activities for the same application), but the
 * browser process will still only be initialized once. All requests to start the browser will
 * always get their callback executed; if the browser process has already been started, the callback
 * is called immediately, else it is called when initialization is complete.
 *
 * All communication with this class must happen on the main thread.
 *
 * This is a singleton, and stores a reference to the application context.
 */
@JNINamespace("content")
public class BrowserStartupController {

    /**
     * This provides the interface to the callbacks for successful or failed startup
     */
    public interface StartupCallback {
        void onSuccess();
        void onFailure();
    }

    private static final String TAG = "cr.BrowserStartup";

    // Helper constants for {@link #executeEnqueuedCallbacks(int, boolean)}.
    @VisibleForTesting
    static final int STARTUP_SUCCESS = -1;
    @VisibleForTesting
    static final int STARTUP_FAILURE = 1;

    private static BrowserStartupController sInstance;

    private static boolean sShouldStartGpuProcessOnBrowserStartup;

    private static void setShouldStartGpuProcessOnBrowserStartup(boolean enable) {
        sShouldStartGpuProcessOnBrowserStartup = enable;
    }

    @VisibleForTesting
    @CalledByNative
    static void browserStartupComplete(int result) {
        if (sInstance != null) {
            sInstance.executeEnqueuedCallbacks(result);
        }
    }

    @CalledByNative
    static boolean shouldStartGpuProcessOnBrowserStartup() {
        return sShouldStartGpuProcessOnBrowserStartup;
    }

    // A list of callbacks that should be called when the async startup of the browser process is
    // complete.
    private final List<StartupCallback> mAsyncStartupCallbacks;

    // Whether the async startup of the browser process has started.
    private boolean mHasStartedInitializingBrowserProcess;

    // Whether tasks that occur after resource extraction have been completed.
    private boolean mPostResourceExtractionTasksCompleted;

    private boolean mHasCalledContentStart;

    // Whether the async startup of the browser process is complete.
    private boolean mStartupDone;

    // This field is set after startup has been completed based on whether the startup was a success
    // or not. It is used when later requests to startup come in that happen after the initial set
    // of enqueued callbacks have been executed.
    private boolean mStartupSuccess;

    private int mLibraryProcessType;

    private TracingControllerAndroid mTracingController;

    BrowserStartupController(int libraryProcessType) {
        mAsyncStartupCallbacks = new ArrayList<>();
        mLibraryProcessType = libraryProcessType;
        ThreadUtils.postOnUiThread(new Runnable() {
            @Override
            public void run() {
                addStartupCompletedObserver(new StartupCallback() {
                    @Override
                    public void onSuccess() {
                        assert mTracingController == null;
                        Context context = ContextUtils.getApplicationContext();
                        mTracingController = new TracingControllerAndroid(context);
                        mTracingController.registerReceiver(context);
                    }

                    @Override
                    public void onFailure() {
                        // Startup failed.
                    }
                });
            }
        });
    }

    /**
     * Get BrowserStartupController instance, create a new one if no existing.
     *
     * @param libraryProcessType the type of process the shared library is loaded. it must be
     *                           LibraryProcessType.PROCESS_BROWSER or
     *                           LibraryProcessType.PROCESS_WEBVIEW.
     * @return BrowserStartupController instance.
     */
    public static BrowserStartupController get(int libraryProcessType) {
        assert ThreadUtils.runningOnUiThread() : "Tried to start the browser on the wrong thread.";
        ThreadUtils.assertOnUiThread();
        if (sInstance == null) {
            assert LibraryProcessType.PROCESS_BROWSER == libraryProcessType
                    || LibraryProcessType.PROCESS_WEBVIEW == libraryProcessType;
            sInstance = new BrowserStartupController(libraryProcessType);
        }
        assert sInstance.mLibraryProcessType == libraryProcessType : "Wrong process type";
        return sInstance;
    }

    @VisibleForTesting
    public static BrowserStartupController overrideInstanceForTest(
            BrowserStartupController controller) {
        sInstance = controller;
        return sInstance;
    }

    /**
     * Start the browser process asynchronously. This will set up a queue of UI thread tasks to
     * initialize the browser process.
     * <p/>
     * Note that this can only be called on the UI thread.
     *
     * @param startGpuProcess Whether to start the GPU process if it is not started.
     * @param callback the callback to be called when browser startup is complete.
     */
    public void startBrowserProcessesAsync(boolean startGpuProcess, final StartupCallback callback)
            throws ProcessInitException {
        assert ThreadUtils.runningOnUiThread() : "Tried to start the browser on the wrong thread.";
        if (mStartupDone) {
            // Browser process initialization has already been completed, so we can immediately post
            // the callback.
            postStartupCompleted(callback);
            return;
        }

        // Browser process has not been fully started yet, so we defer executing the callback.
        mAsyncStartupCallbacks.add(callback);

        if (!mHasStartedInitializingBrowserProcess) {
            // This is the first time we have been asked to start the browser process. We set the
            // flag that indicates that we have kicked off starting the browser process.
            mHasStartedInitializingBrowserProcess = true;

            setShouldStartGpuProcessOnBrowserStartup(startGpuProcess);
            prepareToStartBrowserProcess(false, new Runnable() {
                @Override
                public void run() {
                    ThreadUtils.assertOnUiThread();
                    if (mHasCalledContentStart) return;
                    if (contentStart() > 0) {
                        // Failed. The callbacks may not have run, so run them.
                        enqueueCallbackExecution(STARTUP_FAILURE);
                    }
                }
            });
        }
    }

    /**
     * Start the browser process synchronously. If the browser is already being started
     * asynchronously then complete startup synchronously
     *
     * <p/>
     * Note that this can only be called on the UI thread.
     *
     * @param singleProcess true iff the browser should run single-process, ie. keep renderers in
     *                      the browser process
     * @throws ProcessInitException
     */
    public void startBrowserProcessesSync(boolean singleProcess) throws ProcessInitException {
        // If already started skip to checking the result
        if (!mStartupDone) {
            if (!mHasStartedInitializingBrowserProcess || !mPostResourceExtractionTasksCompleted) {
                prepareToStartBrowserProcess(singleProcess, null);
            }

            boolean startedSuccessfully = true;
            if (!mHasCalledContentStart) {
                if (contentStart() > 0) {
                    // Failed. The callbacks may not have run, so run them.
                    enqueueCallbackExecution(STARTUP_FAILURE);
                    startedSuccessfully = false;
                }
            }
            if (startedSuccessfully) {
                flushStartupTasks();
            }
        }

        // Startup should now be complete
        assert mStartupDone;
        if (!mStartupSuccess) {
            throw new ProcessInitException(LoaderErrors.LOADER_ERROR_NATIVE_STARTUP_FAILED);
        }
    }

    /**
     * Wrap ContentMain.start() for testing.
     */
    @VisibleForTesting
    int contentStart() {
        assert !mHasCalledContentStart;
        mHasCalledContentStart = true;
        return ContentMain.start();
    }

    @VisibleForTesting
    void flushStartupTasks() {
        nativeFlushStartupTasks();
    }

    /**
     * @return Whether the browser process completed successfully.
     */
    public boolean isStartupSuccessfullyCompleted() {
        ThreadUtils.assertOnUiThread();
        return mStartupDone && mStartupSuccess;
    }

    public void addStartupCompletedObserver(StartupCallback callback) {
        ThreadUtils.assertOnUiThread();
        if (mStartupDone) {
            postStartupCompleted(callback);
        } else {
            mAsyncStartupCallbacks.add(callback);
        }
    }

    private void executeEnqueuedCallbacks(int startupResult) {
        assert ThreadUtils.runningOnUiThread() : "Callback from browser startup from wrong thread.";
        mStartupDone = true;
        mStartupSuccess = (startupResult <= 0);
        for (StartupCallback asyncStartupCallback : mAsyncStartupCallbacks) {
            if (mStartupSuccess) {
                asyncStartupCallback.onSuccess();
            } else {
                asyncStartupCallback.onFailure();
            }
        }
        // We don't want to hold on to any objects after we do not need them anymore.
        mAsyncStartupCallbacks.clear();
    }

    // Queue the callbacks to run. Since running the callbacks clears the list it is safe to call
    // this more than once.
    private void enqueueCallbackExecution(final int startupFailure) {
        new Handler().post(new Runnable() {
            @Override
            public void run() {
                executeEnqueuedCallbacks(startupFailure);
            }
        });
    }

    private void postStartupCompleted(final StartupCallback callback) {
        new Handler().post(new Runnable() {
            @Override
            public void run() {
                if (mStartupSuccess) {
                    callback.onSuccess();
                } else {
                    callback.onFailure();
                }
            }
        });
    }

    @VisibleForTesting
    void prepareToStartBrowserProcess(
            final boolean singleProcess, final Runnable completionCallback)
                    throws ProcessInitException {
        Log.i(TAG, "Initializing chromium process, singleProcess=%b", singleProcess);

        // This strictmode exception is to cover the case where the browser process is being started
        // asynchronously but not in the main browser flow.  The main browser flow will trigger
        // library loading earlier and this will be a no-op, but in the other cases this will need
        // to block on loading libraries.
        // This applies to tests and ManageSpaceActivity, which can be launched from Settings.
        StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskReads();
        try {
            // Normally Main.java will have already loaded the library asynchronously, we only need
            // to load it here if we arrived via another flow, e.g. bookmark access & sync setup.
            LibraryLoader.getInstance().ensureInitialized(mLibraryProcessType);
        } finally {
            StrictMode.setThreadPolicy(oldPolicy);
        }

        Runnable postResourceExtraction = new Runnable() {
            @Override
            public void run() {
                if (!mPostResourceExtractionTasksCompleted) {
                    // TODO(yfriedman): Remove dependency on a command line flag for this.
                    DeviceUtilsImpl.addDeviceSpecificUserAgentSwitch(
                            ContextUtils.getApplicationContext());
                    nativeSetCommandLineFlags(
                            singleProcess, nativeIsPluginEnabled() ? getPlugins() : null);
                    mPostResourceExtractionTasksCompleted = true;
                }

                if (completionCallback != null) completionCallback.run();
            }
        };

        if (completionCallback == null) {
            // If no continuation callback is specified, then force the resource extraction
            // to complete.
            ResourceExtractor.get().waitForCompletion();
            postResourceExtraction.run();
        } else {
            ResourceExtractor.get().addCompletionCallback(postResourceExtraction);
        }
    }

    /**
     * Initialization needed for tests. Mainly used by content browsertests.
     */
    public void initChromiumBrowserProcessForTests() {
        ResourceExtractor resourceExtractor = ResourceExtractor.get();
        resourceExtractor.startExtractingResources();
        resourceExtractor.waitForCompletion();
        nativeSetCommandLineFlags(false, null);
    }

    private String getPlugins() {
        return PepperPluginManager.getPlugins(ContextUtils.getApplicationContext());
    }

    private static native void nativeSetCommandLineFlags(
            boolean singleProcess, String pluginDescriptor);

    // Is this an official build of Chrome? Only native code knows for sure. Official build
    // knowledge is needed very early in process startup.
    private static native boolean nativeIsOfficialBuild();

    private static native boolean nativeIsPluginEnabled();

    private static native void nativeFlushStartupTasks();
}
