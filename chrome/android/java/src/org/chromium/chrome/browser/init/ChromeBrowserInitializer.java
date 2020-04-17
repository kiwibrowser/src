// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.init;

import android.app.Activity;
import android.content.Context;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.Process;
import android.os.StrictMode;

import com.squareup.leakcanary.LeakCanary;

import org.chromium.base.ActivityState;
import org.chromium.base.ApplicationStatus;
import org.chromium.base.ApplicationStatus.ActivityStateListener;
import org.chromium.base.CommandLine;
import org.chromium.base.ContentUriUtils;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.PathUtils;
import org.chromium.base.ResourceExtractor;
import org.chromium.base.SysUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.base.TraceEvent;
import org.chromium.base.annotations.RemovableInRelease;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.base.library_loader.ProcessInitException;
import org.chromium.base.memory.MemoryPressureUma;
import org.chromium.chrome.browser.AppHooks;
import org.chromium.chrome.browser.ChromeApplication;
import org.chromium.chrome.browser.ChromeStrictMode;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.ClassRegister;
import org.chromium.chrome.browser.FileProviderHelper;
import org.chromium.chrome.browser.crash.LogcatExtractionRunnable;
import org.chromium.chrome.browser.download.DownloadManagerService;
import org.chromium.chrome.browser.services.GoogleServicesManager;
import org.chromium.chrome.browser.tabmodel.document.DocumentTabModelImpl;
import org.chromium.chrome.browser.webapps.ActivityAssigner;
import org.chromium.chrome.browser.webapps.ChromeWebApkHost;
import org.chromium.components.crash.browser.CrashDumpManager;
import org.chromium.content.browser.BrowserStartupController;
import org.chromium.content_public.browser.DeviceUtils;
import org.chromium.content_public.browser.SpeechRecognition;
import org.chromium.net.NetworkChangeNotifier;
import org.chromium.policy.CombinedPolicyProvider;

import java.io.File;
import java.util.Locale;

/**
 * Application level delegate that handles start up tasks.
 * {@link AsyncInitializationActivity} classes should override the {@link BrowserParts}
 * interface for any additional initialization tasks for the initialization to work as intended.
 */
public class ChromeBrowserInitializer {
    private static final String TAG = "BrowserInitializer";
    private static ChromeBrowserInitializer sChromeBrowserInitializer;

    private final Handler mHandler;
    private final ChromeApplication mApplication;
    private final Locale mInitialLocale = Locale.getDefault();

    private boolean mPreInflationStartupComplete;
    private boolean mPostInflationStartupComplete;
    private boolean mNativeInitializationComplete;

    // Public to allow use in ChromeBackupAgent
    public static final String PRIVATE_DATA_DIRECTORY_SUFFIX = "chrome";

    /**
     * A callback to be executed when there is a new version available in Play Store.
     */
    public interface OnNewVersionAvailableCallback extends Runnable {
        /**
         * Set the update url to get the new version available.
         * @param updateUrl The url to be used.
         */
        void setUpdateUrl(String updateUrl);
    }

    /**
     * This class is an application specific object that orchestrates the app initialization.
     * @param context The context to get the application context from.
     * @return The singleton instance of {@link ChromeBrowserInitializer}.
     */
    public static ChromeBrowserInitializer getInstance(Context context) {
        if (sChromeBrowserInitializer == null) {
            sChromeBrowserInitializer = new ChromeBrowserInitializer(context);
        }
        return sChromeBrowserInitializer;
    }

    private ChromeBrowserInitializer(Context context) {
        mApplication = (ChromeApplication) context.getApplicationContext();
        mHandler = new Handler(Looper.getMainLooper());
        initLeakCanary();
    }

    @RemovableInRelease
    private void initLeakCanary() {
        // Watch that Activity objects are not retained after their onDestroy() has been called.
        // This is a no-op in release builds.
        LeakCanary.install(mApplication);
    }

    /**
     * @return whether native initialization is complete.
     */
    public boolean hasNativeInitializationCompleted() {
        return mNativeInitializationComplete;
    }

    /**
     * Initializes the Chrome browser process synchronously.
     *
     * @throws ProcessInitException if there is a problem with the native library.
     */
    public void handleSynchronousStartup() throws ProcessInitException {
        handleSynchronousStartupInternal(false);
    }

    /**
     * Initializes the Chrome browser process synchronously with GPU process warmup.
     */
    public void handleSynchronousStartupWithGpuWarmUp() throws ProcessInitException {
        handleSynchronousStartupInternal(true);
    }

    private void handleSynchronousStartupInternal(final boolean startGpuProcess)
            throws ProcessInitException {
        ThreadUtils.checkUiThread();

        BrowserParts parts = new EmptyBrowserParts() {
            @Override
            public boolean shouldStartGpuProcess() {
                return startGpuProcess;
            }
        };
        handlePreNativeStartup(parts);
        handlePostNativeStartup(false, parts);
    }

    /**
     * Execute startup tasks that can be done without native libraries. See {@link BrowserParts} for
     * a list of calls to be implemented.
     * @param parts The delegate for the {@link ChromeBrowserInitializer} to communicate
     *              initialization tasks.
     */
    public void handlePreNativeStartup(final BrowserParts parts) {
        ThreadUtils.checkUiThread();

        ProcessInitializationHandler.getInstance().initializePreNative();
        preInflationStartup();
        parts.preInflationStartup();
        if (parts.isActivityFinishing()) return;

        preInflationStartupDone();
        parts.setContentViewAndLoadLibrary();
        postInflationStartup();
        parts.postInflationStartup();
    }

    /**
     * This is needed for device class manager which depends on commandline args that are
     * initialized in preInflationStartup()
     */
    private void preInflationStartupDone() {
        // Domain reliability uses significant enough memory that we should disable it on low memory
        // devices for now.
        // TODO(zbowling): remove this after domain reliability is refactored. (crbug.com/495342)
        if (true || SysUtils.isLowEndDevice()) {
            CommandLine.getInstance().appendSwitch(ChromeSwitches.DISABLE_DOMAIN_RELIABILITY);
        }
        CommandLine.getInstance().appendSwitch(ChromeSwitches.FORCE_CRASH_DUMP_UPLOAD);
    }

    /**
     * Pre-load shared prefs to avoid being blocked on the disk access async task in the future.
     * Running in an AsyncTask as pre-loading itself may cause I/O.
     */
    private void warmUpSharedPrefs() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            new AsyncTask<Void, Void, Void>() {
                @Override
                protected Void doInBackground(Void... params) {
                    ContextUtils.getAppSharedPreferences();
                    DocumentTabModelImpl.warmUpSharedPrefs(mApplication);
                    ActivityAssigner.warmUpSharedPrefs(mApplication);
                    DownloadManagerService.warmUpSharedPrefs(mApplication);
                    return null;
                }
            }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
        } else {
            ContextUtils.getAppSharedPreferences();
            DocumentTabModelImpl.warmUpSharedPrefs(mApplication);
            ActivityAssigner.warmUpSharedPrefs(mApplication);
            DownloadManagerService.warmUpSharedPrefs(mApplication);
        }
    }

    private void preInflationStartup() {
        ThreadUtils.assertOnUiThread();
        if (mPreInflationStartupComplete) return;
        PathUtils.setPrivateDataDirectorySuffix(PRIVATE_DATA_DIRECTORY_SUFFIX);

        // Ensure critical files are available, so they aren't blocked on the file-system
        // behind long-running accesses in next phase.
        // Don't do any large file access here!
        ChromeStrictMode.configureStrictMode();
        ChromeWebApkHost.init();

        warmUpSharedPrefs();

        DeviceUtils.addDeviceSpecificUserAgentSwitch(mApplication);
        ApplicationStatus.registerStateListenerForAllActivities(
                createActivityStateListener());

        mPreInflationStartupComplete = true;
    }

    private void postInflationStartup() {
        ThreadUtils.assertOnUiThread();
        if (mPostInflationStartupComplete) return;

        // Check to see if we need to extract any new resources from the APK. This could
        // be on first run when we need to extract all the .pak files we need, or after
        // the user has switched locale, in which case we want new locale resources.
        ResourceExtractor.get().startExtractingResources();

        mPostInflationStartupComplete = true;
    }

    /**
     * Execute startup tasks that require native libraries to be loaded. See {@link BrowserParts}
     * for a list of calls to be implemented.
     * @param isAsync Whether this call should synchronously wait for the browser process to be
     *                fully initialized before returning to the caller.
     * @param delegate The delegate for the {@link ChromeBrowserInitializer} to communicate
     *                 initialization tasks.
     */
    public void handlePostNativeStartup(final boolean isAsync, final BrowserParts delegate)
            throws ProcessInitException {
        assert ThreadUtils.runningOnUiThread() : "Tried to start the browser on the wrong thread";
        final ChainedTasks tasks = new ChainedTasks();
        tasks.add(new Runnable() {
            @Override
            public void run() {
                ProcessInitializationHandler.getInstance().initializePostNative();
            }
        });

        tasks.add(new Runnable() {
            @Override
            public void run() {
                initNetworkChangeNotifier(mApplication.getApplicationContext());
            }
        });

        tasks.add(new Runnable() {
            @Override
            public void run() {
                // This is not broken down as a separate task, since this:
                // 1. Should happen as early as possible
                // 2. Only submits asynchronous work
                // 3. Is thus very cheap (profiled at 0.18ms on a Nexus 5 with Lollipop)
                // It should also be in a separate task (and after) initNetworkChangeNotifier, as
                // this posts a task to the UI thread that would interfere with preconneciton
                // otherwise. By preconnecting afterwards, we make sure that this task has run.
                delegate.maybePreconnect();

                onStartNativeInitialization();
            }
        });

        tasks.add(new Runnable() {
            @Override
            public void run() {
                if (delegate.isActivityDestroyed()) return;
                delegate.initializeCompositor();
            }
        });

        tasks.add(new Runnable() {
            @Override
            public void run() {
                if (delegate.isActivityDestroyed()) return;
                delegate.initializeState();
            }
        });

        tasks.add(new Runnable() {
            @Override
            public void run() {
                onFinishNativeInitialization();
            }
        });

        tasks.add(new Runnable() {
            @Override
            public void run() {
                if (delegate.isActivityDestroyed()) return;
                delegate.finishNativeInitialization();
            }
        });

        if (isAsync) {
            // We want to start this queue once the C++ startup tasks have run; allow the
            // C++ startup to run asynchonously, and set it up to start the Java queue once
            // it has finished.
            startChromeBrowserProcessesAsync(
                    delegate.shouldStartGpuProcess(),
                    new BrowserStartupController.StartupCallback() {
                        @Override
                        public void onFailure() {
                            delegate.onStartupFailure();
                        }

                        @Override
                        public void onSuccess() {
                            tasks.start(false);
                        }
                    });
        } else {
            startChromeBrowserProcessesSync();
            tasks.start(true);
        }
    }

    private void startChromeBrowserProcessesAsync(
            boolean startGpuProcess,
            BrowserStartupController.StartupCallback callback) throws ProcessInitException {
        try {
            TraceEvent.begin("ChromeBrowserInitializer.startChromeBrowserProcessesAsync");
            BrowserStartupController.get(LibraryProcessType.PROCESS_BROWSER)
                    .startBrowserProcessesAsync(startGpuProcess, callback);
        } finally {
            TraceEvent.end("ChromeBrowserInitializer.startChromeBrowserProcessesAsync");
        }
    }

    private void startChromeBrowserProcessesSync() throws ProcessInitException {
        try {
            TraceEvent.begin("ChromeBrowserInitializer.startChromeBrowserProcessesSync");
            ThreadUtils.assertOnUiThread();
            StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskReads();
            LibraryLoader.getInstance().ensureInitialized(LibraryProcessType.PROCESS_BROWSER);
            StrictMode.setThreadPolicy(oldPolicy);
            LibraryLoader.getInstance().asyncPrefetchLibrariesToMemory();
            BrowserStartupController.get(LibraryProcessType.PROCESS_BROWSER)
                    .startBrowserProcessesSync(false);
            GoogleServicesManager.get(mApplication);
        } finally {
            TraceEvent.end("ChromeBrowserInitializer.startChromeBrowserProcessesSync");
        }
    }

    public static void initNetworkChangeNotifier(Context context) {
        ThreadUtils.assertOnUiThread();
        TraceEvent.begin("NetworkChangeNotifier.init");
        // Enable auto-detection of network connectivity state changes.
        NetworkChangeNotifier.init();
        NetworkChangeNotifier.setAutoDetectConnectivityState(true);
        TraceEvent.end("NetworkChangeNotifier.init");
    }

    private void onStartNativeInitialization() {
        ThreadUtils.assertOnUiThread();
        if (mNativeInitializationComplete) return;
        // The policies are used by browser startup, so we need to register the policy providers
        // before starting the browser process.
        AppHooks.get().registerPolicyProviders(CombinedPolicyProvider.get());

        SpeechRecognition.initialize(mApplication);
        ClassRegister.get().registerContentClassFactory();
    }

    private void onFinishNativeInitialization() {
        if (mNativeInitializationComplete) return;

        mNativeInitializationComplete = true;
        ContentUriUtils.setFileProviderUtil(new FileProviderHelper());

        // When a minidump is detected, extract and append a logcat to it, then upload it to the
        // crash server. Note that the logcat extraction might fail. This is ok; in that case, the
        // minidump will be found and uploaded upon the next browser launch.
        CrashDumpManager.registerUploadCallback(new CrashDumpManager.UploadMinidumpCallback() {
            @Override
            public void tryToUploadMinidump(File minidump) {
                AsyncTask.THREAD_POOL_EXECUTOR.execute(new LogcatExtractionRunnable(minidump));
            }
        });

        MemoryPressureUma.initializeForBrowser();
    }

    private ActivityStateListener createActivityStateListener() {
        return new ActivityStateListener() {
            @Override
            public void onActivityStateChange(Activity activity, int newState) {
                if (newState == ActivityState.CREATED || newState == ActivityState.DESTROYED) {
                    // Android destroys Activities at some point after a locale change, but doesn't
                    // kill the process.  This can lead to a bug where Chrome is halfway RTL, where
                    // stale natively-loaded resources are not reloaded (http://crbug.com/552618).
                    if (!mInitialLocale.equals(Locale.getDefault())) {
                        Log.e(TAG, "Killing process because of locale change.");
                        Process.killProcess(Process.myPid());
                    }
                }
            }
        };
    }

    /**
     * For unit testing of clients.
     * @param initializer The (dummy or mocked) initializer to use.
     */
    public static void setForTesting(ChromeBrowserInitializer initializer) {
        sChromeBrowserInitializer = initializer;
    }
}
