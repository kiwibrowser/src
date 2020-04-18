// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import android.text.TextUtils;

import org.chromium.base.Callback;
import org.chromium.base.CollectionUtil;
import org.chromium.base.ContextUtils;
import org.chromium.base.CpuFeatures;
import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.library_loader.Linker;
import org.chromium.base.process_launcher.ChildConnectionAllocator;
import org.chromium.base.process_launcher.ChildProcessConnection;
import org.chromium.base.process_launcher.ChildProcessConstants;
import org.chromium.base.process_launcher.ChildProcessLauncher;
import org.chromium.base.process_launcher.FileDescriptorInfo;
import org.chromium.content.app.ChromiumLinkerParams;
import org.chromium.content.app.SandboxedProcessService;
import org.chromium.content.common.ContentSwitchUtils;
import org.chromium.content_public.browser.ChildProcessImportance;
import org.chromium.content_public.common.ContentSwitches;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * This is the java counterpart to ChildProcessLauncherHelper. It is owned by native side and
 * has an explicit destroy method.
 * Each public or jni methods should have explicit documentation on what threads they are called.
 */
@JNINamespace("content::internal")
public class ChildProcessLauncherHelper {
    private static final String TAG = "ChildProcLH";

    // Manifest values used to specify the service names.
    private static final String NUM_SANDBOXED_SERVICES_KEY =
            "org.chromium.content.browser.NUM_SANDBOXED_SERVICES";
    private static final String SANDBOXED_SERVICES_NAME =
            "org.chromium.content.app.SandboxedProcessService";
    private static final String NUM_PRIVILEGED_SERVICES_KEY =
            "org.chromium.content.browser.NUM_PRIVILEGED_SERVICES";
    private static final String PRIVILEGED_SERVICES_NAME =
            "org.chromium.content.app.PrivilegedProcessService";

    // A warmed-up connection to a sandboxed service.
    private static SpareChildConnection sSpareSandboxedConnection;

    // Allocator used for sandboxed services.
    private static ChildConnectionAllocator sSandboxedChildConnectionAllocator;
    private static ChildProcessRanking sSandboxedChildConnectionRanking;

    // Map from PID to ChildProcessLauncherHelper.
    private static final Map<Integer, ChildProcessLauncherHelper> sLauncherByPid = new HashMap<>();

    // Allocator used for non-sandboxed services.
    private static ChildConnectionAllocator sPrivilegedChildConnectionAllocator;

    // Used by tests to override the default sandboxed service allocator settings.
    private static ChildConnectionAllocator.ConnectionFactory sSandboxedServiceFactoryForTesting;
    private static int sSandboxedServicesCountForTesting = -1;
    private static String sSandboxedServicesNameForTesting;

    private static BindingManager sBindingManager;

    // Whether the main application is currently brought to the foreground.
    private static boolean sApplicationInForeground = true;

    // TODO(boliu): Generalize these so they work for all connections, not just sandboxed.
    // Whether the connection is managed by the BindingManager.
    private final boolean mUseBindingManager;
    private final ChildProcessRanking mRanking;

    // Whether the created process should be sandboxed.
    private final boolean mSandboxed;

    // The type of process as determined by the command line.
    private final String mProcessType;

    private final ChildProcessLauncher.Delegate mLauncherDelegate =
            new ChildProcessLauncher.Delegate() {
                @Override
                public ChildProcessConnection getBoundConnection(
                        ChildConnectionAllocator connectionAllocator,
                        ChildProcessConnection.ServiceCallback serviceCallback) {
                    if (sSpareSandboxedConnection == null) {
                        return null;
                    }
                    return sSpareSandboxedConnection.getConnection(
                            connectionAllocator, serviceCallback);
                }

                @Override
                public void onBeforeConnectionAllocated(Bundle bundle) {
                    populateServiceBundle(bundle);
                }

                @Override
                public void onBeforeConnectionSetup(Bundle connectionBundle) {
                    // Populate the bundle passed to the service setup call with content specific
                    // parameters.
                    connectionBundle.putInt(
                            ContentChildProcessConstants.EXTRA_CPU_COUNT, CpuFeatures.getCount());
                    connectionBundle.putLong(
                            ContentChildProcessConstants.EXTRA_CPU_FEATURES, CpuFeatures.getMask());
                    if (Linker.isUsed()) {
                        connectionBundle.putBundle(Linker.EXTRA_LINKER_SHARED_RELROS,
                                Linker.getInstance().getSharedRelros());
                    }
                }

                @Override
                public void onConnectionEstablished(ChildProcessConnection connection) {
                    int pid = connection.getPid();
                    assert pid > 0;

                    sLauncherByPid.put(pid, ChildProcessLauncherHelper.this);
                    if (mRanking != null) {
                        mRanking.addConnection(connection, false /* foreground */,
                                1 /* frameDepth */, ChildProcessImportance.MODERATE);
                    }

                    // If the connection fails and pid == 0, the Java-side cleanup was already
                    // handled by DeathCallback. We still have to call back to native for cleanup
                    // there.
                    if (mNativeChildProcessLauncherHelper != 0) {
                        nativeOnChildProcessStarted(
                                mNativeChildProcessLauncherHelper, connection.getPid());
                    }
                    mNativeChildProcessLauncherHelper = 0;
                }

                @Override
                public void onConnectionLost(ChildProcessConnection connection) {
                    assert connection.getPid() > 0;
                    sLauncherByPid.remove(connection.getPid());
                    BindingManager manager = getBindingManager();
                    if (mUseBindingManager && manager != null) {
                        manager.dropRecency(connection);
                    }
                    if (mRanking != null) {
                        mRanking.removeConnection(connection);
                    }
                }
            };

    private final ChildProcessLauncher mLauncher;

    private long mNativeChildProcessLauncherHelper;

    // This is the current computed importance from all the inputs from setPriority.
    // The initial value is MODERATE since a newly created connection has moderate bindings.
    private @ChildProcessImportance int mEffectiveImportance = ChildProcessImportance.MODERATE;
    private boolean mForeground;

    @CalledByNative
    private static FileDescriptorInfo makeFdInfo(
            int id, int fd, boolean autoClose, long offset, long size) {
        assert LauncherThread.runningOnLauncherThread();
        ParcelFileDescriptor pFd;
        if (autoClose) {
            // Adopt the FD, it will be closed when we close the ParcelFileDescriptor.
            pFd = ParcelFileDescriptor.adoptFd(fd);
        } else {
            try {
                pFd = ParcelFileDescriptor.fromFd(fd);
            } catch (IOException e) {
                Log.e(TAG, "Invalid FD provided for process connection, aborting connection.", e);
                return null;
            }
        }
        return new FileDescriptorInfo(id, pFd, offset, size);
    }

    @VisibleForTesting
    @CalledByNative
    public static ChildProcessLauncherHelper createAndStart(
            long nativePointer, String[] commandLine, FileDescriptorInfo[] filesToBeMapped) {
        assert LauncherThread.runningOnLauncherThread();
        String processType =
                ContentSwitchUtils.getSwitchValue(commandLine, ContentSwitches.SWITCH_PROCESS_TYPE);

        boolean sandboxed = true;
        if (!ContentSwitches.SWITCH_RENDERER_PROCESS.equals(processType)) {
            if (ContentSwitches.SWITCH_GPU_PROCESS.equals(processType)) {
                sandboxed = false;
            } else {
                // We only support sandboxed utility processes now.
                assert ContentSwitches.SWITCH_UTILITY_PROCESS.equals(processType);

                // Remove sandbox restriction on network service process.
                if (ContentSwitches.NETWORK_SANDBOX_TYPE.equals(ContentSwitchUtils.getSwitchValue(
                            commandLine, ContentSwitches.SWITCH_SERVICE_SANDBOX_TYPE))) {
                    sandboxed = false;
                }
            }
        }

        IBinder binderCallback = ContentSwitches.SWITCH_GPU_PROCESS.equals(processType)
                ? new GpuProcessCallback()
                : null;

        ChildProcessLauncherHelper helper = new ChildProcessLauncherHelper(
                nativePointer, commandLine, filesToBeMapped, sandboxed, binderCallback);
        helper.start();
        return helper;
    }

    /**
     * Creates a ready to use sandboxed child process. Should be called early during startup so the
     * child process is created while other startup work is happening.
     * @param context the application context used for the connection.
     */
    public static void warmUp(final Context context) {
        assert ThreadUtils.runningOnUiThread();
        LauncherThread.post(new Runnable() {
            @Override
            public void run() {
                warmUpOnLauncherThread(context);
            }
        });
    }

    private static void warmUpOnLauncherThread(Context context) {
        if (sSpareSandboxedConnection != null && !sSpareSandboxedConnection.isEmpty()) {
            return;
        }

        Bundle serviceBundle = populateServiceBundle(new Bundle());
        ChildConnectionAllocator allocator = getConnectionAllocator(context, true /* sandboxed */);
        sSpareSandboxedConnection = new SpareChildConnection(context, allocator, serviceBundle);
    }

    /**
     * Starts the moderate binding management that adjust a process priority in response to various
     * signals (app sent to background/foreground for example).
     * Note: WebAPKs and non WebAPKs share the same moderate binding pool, so the size of the
     * shared moderate binding pool is always set based on the number of sandboxes processes
     * used by Chrome.
     * @param context Android's context.
     */
    public static void startModerateBindingManagement(final Context context) {
        assert ThreadUtils.runningOnUiThread();
        LauncherThread.post(new Runnable() {
            @Override
            public void run() {
                ChildConnectionAllocator allocator =
                        getConnectionAllocator(context, true /* sandboxed */);
                sBindingManager = new BindingManager(
                        context, allocator.getNumberOfServices(), false /* onTesting */);
            }
        });
    }

    // May return null.
    private static BindingManager getBindingManager() {
        assert LauncherThread.runningOnLauncherThread();
        return sBindingManager;
    }

    /**
     * Called when the embedding application is sent to background.
     */
    public static void onSentToBackground() {
        assert ThreadUtils.runningOnUiThread();
        sApplicationInForeground = false;
        LauncherThread.post(new Runnable() {
            @Override
            public void run() {
                BindingManager manager = getBindingManager();
                if (manager != null) {
                    manager.onSentToBackground();
                }
            }
        });
    }

    /**
     * Called when the embedding application is brought to foreground.
     */
    public static void onBroughtToForeground() {
        assert ThreadUtils.runningOnUiThread();
        sApplicationInForeground = true;
        LauncherThread.post(new Runnable() {
            @Override
            public void run() {
                BindingManager manager = getBindingManager();
                if (manager != null) {
                    manager.onBroughtToForeground();
                }
            }
        });
    }

    @VisibleForTesting
    public static void setSandboxServicesSettingsForTesting(
            ChildConnectionAllocator.ConnectionFactory factory, int serviceCount,
            String serviceName) {
        sSandboxedServiceFactoryForTesting = factory;
        sSandboxedServicesCountForTesting = serviceCount;
        sSandboxedServicesNameForTesting = serviceName;
    }

    @VisibleForTesting
    static ChildConnectionAllocator getConnectionAllocator(Context context, boolean sandboxed) {
        assert LauncherThread.runningOnLauncherThread();
        final String packageName = ChildProcessCreationParams.getPackageNameForService();
        boolean bindToCaller = ChildProcessCreationParams.getBindToCallerCheck();
        boolean bindAsExternalService =
                sandboxed && ChildProcessCreationParams.getIsSandboxedServiceExternal();

        if (!sandboxed) {
            if (sPrivilegedChildConnectionAllocator == null) {
                sPrivilegedChildConnectionAllocator =
                        ChildConnectionAllocator.create(context, LauncherThread.getHandler(), null,
                                packageName, PRIVILEGED_SERVICES_NAME, NUM_PRIVILEGED_SERVICES_KEY,
                                bindToCaller, bindAsExternalService, true /* useStrongBinding */);
            }
            return sPrivilegedChildConnectionAllocator;
        }

        if (sSandboxedChildConnectionAllocator == null) {
            Log.w(TAG,
                    "Create a new ChildConnectionAllocator with package name = %s,"
                            + " sandboxed = true",
                    packageName);
            Runnable freeSlotRunnable = () -> {
                ChildProcessConnection lowestRank =
                        sSandboxedChildConnectionRanking.getLowestRankedConnection();
                if (lowestRank != null) {
                    lowestRank.kill();
                }
            };

            ChildConnectionAllocator connectionAllocator = null;
            if (sSandboxedServicesCountForTesting != -1) {
                // Testing case where allocator settings are overriden.
                String serviceName = !TextUtils.isEmpty(sSandboxedServicesNameForTesting)
                        ? sSandboxedServicesNameForTesting
                        : SandboxedProcessService.class.getName();
                connectionAllocator = ChildConnectionAllocator.createForTest(freeSlotRunnable,
                        packageName, serviceName, sSandboxedServicesCountForTesting, bindToCaller,
                        bindAsExternalService, false /* useStrongBinding */);
            } else {
                connectionAllocator = ChildConnectionAllocator.create(context,
                        LauncherThread.getHandler(), freeSlotRunnable, packageName,
                        SANDBOXED_SERVICES_NAME, NUM_SANDBOXED_SERVICES_KEY, bindToCaller,
                        bindAsExternalService, false /* useStrongBinding */);
            }
            if (sSandboxedServiceFactoryForTesting != null) {
                connectionAllocator.setConnectionFactoryForTesting(
                        sSandboxedServiceFactoryForTesting);
            }
            sSandboxedChildConnectionAllocator = connectionAllocator;
            sSandboxedChildConnectionRanking = new ChildProcessRanking(
                    sSandboxedChildConnectionAllocator.getNumberOfServices());
        }
        return sSandboxedChildConnectionAllocator;
    }

    private ChildProcessLauncherHelper(long nativePointer, String[] commandLine,
            FileDescriptorInfo[] filesToBeMapped, boolean sandboxed, IBinder binderCallback) {
        assert LauncherThread.runningOnLauncherThread();

        mNativeChildProcessLauncherHelper = nativePointer;
        mUseBindingManager = sandboxed;
        mSandboxed = sandboxed;

        ChildConnectionAllocator connectionAllocator =
                getConnectionAllocator(ContextUtils.getApplicationContext(), sandboxed);
        mLauncher = new ChildProcessLauncher(LauncherThread.getHandler(), mLauncherDelegate,
                commandLine, filesToBeMapped, connectionAllocator,
                binderCallback == null ? null : Arrays.asList(binderCallback));
        mProcessType =
                ContentSwitchUtils.getSwitchValue(commandLine, ContentSwitches.SWITCH_PROCESS_TYPE);

        if (sandboxed) {
            mRanking = sSandboxedChildConnectionRanking;
        } else {
            mRanking = null;
        }
    }

    private void start() {
        mLauncher.start(true /* doSetupConnection */, true /* queueIfNoFreeConnection */);
    }

    /**
     * @return The type of process as specified in the command line at
     * {@link ContentSwitches#SWITCH_PROCESS_TYPE}.
     */
    public String getProcessType() {
        return TextUtils.isEmpty(mProcessType) ? "" : mProcessType;
    }

    public int getPid() {
        assert LauncherThread.runningOnLauncherThread();
        return mLauncher.getPid();
    }

    // Called on client (UI or IO) thread.
    @CalledByNative
    private boolean hasOomProtectionBindings() {
        ChildProcessConnection connection = mLauncher.getConnection();
        // Here we are accessing the connection from a thread other than the launcher thread, but it
        // does not change once it's been set. So it is safe to test whether it's null here and
        // access it afterwards.
        if (connection == null) {
            return false;
        }

        return !connection.isWaivedBoundOnlyOrWasWhenDied();
    }

    // Called on client (UI or IO) thread.
    @CalledByNative
    private boolean isApplicationInForeground() {
        return sApplicationInForeground;
    }

    // Called on client (UI or IO) thread.
    @CalledByNative
    private boolean isKilledByUs() {
        ChildProcessConnection connection = mLauncher.getConnection();
        // Here we are accessing the connection from a thread other than the launcher thread, but it
        // does not change once it's been set. So it is safe to test whether it's null here and
        // access it afterwards.
        if (connection == null) {
            return false;
        }

        // We consider the process to be child protected if it has a strong or moderate binding and
        // the app is in the foreground.
        return connection.isKilledByUs();
    }

    @CalledByNative
    private void setPriority(int pid, boolean foreground, long frameDepth,
            boolean boostForPendingViews, @ChildProcessImportance int importance) {
        assert LauncherThread.runningOnLauncherThread();
        assert mLauncher.getPid() == pid;
        if (getByPid(pid) == null) {
            // Child already disconnected. Ignore any trailing calls.
            return;
        }

        ChildProcessConnection connection = mLauncher.getConnection();
        if (ChildProcessCreationParams.getIgnoreVisibilityForImportance()) {
            foreground = false;
            boostForPendingViews = false;
        }

        @ChildProcessImportance
        int newEffectiveImportance;
        if ((foreground && frameDepth == 0) || importance == ChildProcessImportance.IMPORTANT) {
            newEffectiveImportance = ChildProcessImportance.IMPORTANT;
        } else if ((foreground && frameDepth > 0)
                || importance == ChildProcessImportance.MODERATE) {
            newEffectiveImportance = ChildProcessImportance.MODERATE;
        } else {
            newEffectiveImportance = ChildProcessImportance.NORMAL;
        }

        // Add first and remove second.
        if (foreground && !mForeground) {
            BindingManager manager = getBindingManager();
            if (mUseBindingManager && manager != null) {
                manager.increaseRecency(connection);
            }
        }
        mForeground = foreground;

        if (mEffectiveImportance != newEffectiveImportance) {
            switch (newEffectiveImportance) {
                case ChildProcessImportance.NORMAL:
                    // Nothing to add.
                    break;
                case ChildProcessImportance.MODERATE:
                    connection.addModerateBinding();
                    break;
                case ChildProcessImportance.IMPORTANT:
                    connection.addStrongBinding();
                    break;
                case ChildProcessImportance.COUNT:
                    assert false;
                    break;
                default:
                    assert false;
            }
        }

        if (mRanking != null) {
            mRanking.updateConnection(connection, foreground, frameDepth, importance);
        }

        if (mEffectiveImportance != newEffectiveImportance) {
            switch (mEffectiveImportance) {
                case ChildProcessImportance.NORMAL:
                    // Nothing to remove.
                    break;
                case ChildProcessImportance.MODERATE:
                    connection.removeModerateBinding();
                    break;
                case ChildProcessImportance.IMPORTANT:
                    connection.removeStrongBinding();
                    break;
                case ChildProcessImportance.COUNT:
                    assert false;
                    break;
                default:
                    assert false;
            }
        }

        mEffectiveImportance = newEffectiveImportance;
    }

    @CalledByNative
    static void stop(int pid) {
        assert LauncherThread.runningOnLauncherThread();
        Log.d(TAG, "stopping child connection: pid=%d", pid);
        ChildProcessLauncherHelper launcher = getByPid(pid);
        // launcher can be null for single process.
        if (launcher != null) {
            // Can happen for single process.
            launcher.mLauncher.stop();
        }
    }

    // Can be called on a number of threads, including launcher, and binder.
    private static native void nativeOnChildProcessStarted(
            long nativeChildProcessLauncherHelper, int pid);

    private static boolean sLinkerInitialized;
    private static long sLinkerLoadAddress;
    private static void initLinker() {
        assert LauncherThread.runningOnLauncherThread();
        if (sLinkerInitialized) return;
        if (Linker.isUsed()) {
            sLinkerLoadAddress = Linker.getInstance().getBaseLoadAddress();
            if (sLinkerLoadAddress == 0) {
                Log.i(TAG, "Shared RELRO support disabled!");
            }
        }
        sLinkerInitialized = true;
    }

    private static ChromiumLinkerParams getLinkerParamsForNewConnection() {
        assert LauncherThread.runningOnLauncherThread();

        initLinker();
        assert sLinkerInitialized;
        if (sLinkerLoadAddress == 0) return null;

        // Always wait for the shared RELROs in service processes.
        final boolean waitForSharedRelros = true;
        if (Linker.areTestsEnabled()) {
            Linker linker = Linker.getInstance();
            return new ChromiumLinkerParams(sLinkerLoadAddress, waitForSharedRelros,
                    linker.getTestRunnerClassNameForTesting());
        } else {
            return new ChromiumLinkerParams(sLinkerLoadAddress, waitForSharedRelros);
        }
    }

    private static Bundle populateServiceBundle(Bundle bundle) {
        ChildProcessCreationParams creationParams = ChildProcessCreationParams.get();
        if (creationParams != null) {
            creationParams.addIntentExtras(bundle);
        }
        bundle.putBoolean(ChildProcessConstants.EXTRA_BIND_TO_CALLER,
                ChildProcessCreationParams.getBindToCallerCheck());
        ChromiumLinkerParams linkerParams = getLinkerParamsForNewConnection();
        if (linkerParams != null) linkerParams.populateBundle(bundle);
        return bundle;
    }

    public static ChildProcessLauncherHelper getByPid(int pid) {
        return sLauncherByPid.get(pid);
    }

    /**
     * Groups all currently tracked processes by type and returns a map of type -> list of PIDs.
     *
     * @param callback The callback to notify with the process information.  {@code callback} will
     *                 run on the same thread this method is called on.  That thread must support a
     *                 {@link android.os.Looper}.
     */
    public static void getProcessIdsByType(Callback < Map < String, List<Integer>>> callback) {
        final Handler responseHandler = new Handler();
        LauncherThread.post(() -> {
            Map<String, List<Integer>> map = new HashMap<>();
            CollectionUtil.forEach(sLauncherByPid, entry -> {
                String type = entry.getValue().getProcessType();
                List<Integer> pids = map.get(type);
                if (pids == null) {
                    pids = new ArrayList<>();
                    map.put(type, pids);
                }
                pids.add(entry.getKey());
            });

            responseHandler.post(() -> callback.onResult(map));
        });
    }

    // Testing only related methods.

    @VisibleForTesting
    public static Map<Integer, ChildProcessLauncherHelper> getAllProcessesForTesting() {
        return sLauncherByPid;
    }

    @VisibleForTesting
    public static ChildProcessLauncherHelper createAndStartForTesting(String[] commandLine,
            FileDescriptorInfo[] filesToBeMapped, boolean sandboxed, IBinder binderCallback,
            boolean doSetupConnection) {
        ChildProcessLauncherHelper launcherHelper = new ChildProcessLauncherHelper(
                0L, commandLine, filesToBeMapped, sandboxed, binderCallback);
        launcherHelper.mLauncher.start(doSetupConnection, true /* queueIfNoFreeConnection */);
        return launcherHelper;
    }

    /** @return the count of services set-up and working. */
    @VisibleForTesting
    static int getConnectedServicesCountForTesting() {
        int count = sPrivilegedChildConnectionAllocator == null
                ? 0
                : sPrivilegedChildConnectionAllocator.allocatedConnectionsCountForTesting();
        return count + getConnectedSandboxedServicesCountForTesting();
    }

    @VisibleForTesting
    public static int getConnectedSandboxedServicesCountForTesting() {
        return sSandboxedChildConnectionAllocator == null
                ? 0
                : sSandboxedChildConnectionAllocator.allocatedConnectionsCountForTesting();
    }

    @VisibleForTesting
    public ChildProcessConnection getChildProcessConnection() {
        return mLauncher.getConnection();
    }

    @VisibleForTesting
    public ChildConnectionAllocator getChildConnectionAllocatorForTesting() {
        return mLauncher.getConnectionAllocator();
    }

    @VisibleForTesting
    public static ChildProcessConnection getWarmUpConnectionForTesting() {
        return sSpareSandboxedConnection == null ? null : sSpareSandboxedConnection.getConnection();
    }

    @VisibleForTesting
    public static boolean crashProcessForTesting(int pid) {
        if (sLauncherByPid.get(pid) == null) return false;

        ChildProcessConnection connection = sLauncherByPid.get(pid).mLauncher.getConnection();
        if (connection == null) return false;
        try {
            connection.crashServiceForTesting();
            return true;
        } catch (RemoteException ex) {
            return false;
        }
    }
}
