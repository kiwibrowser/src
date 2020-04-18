// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import static org.chromium.chrome.browser.download.DownloadSnackbarController.INVALID_NOTIFICATION_ID;

import android.app.Notification;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;

import com.google.ipc.invalidation.util.Preconditions;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.browser.download.DownloadNotificationService2.DownloadStatus;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

import javax.annotation.Nullable;

/**
 * Manager to stop and start the foreground service associated with downloads.
 */
public class DownloadForegroundServiceManager {
    private static class DownloadUpdate {
        int mNotificationId;
        Notification mNotification;
        DownloadNotificationService2.DownloadStatus mDownloadStatus;
        Context mContext;

        DownloadUpdate(int notificationId, Notification notification,
                DownloadNotificationService2.DownloadStatus downloadStatus, Context context) {
            mNotificationId = notificationId;
            mNotification = notification;
            mDownloadStatus = downloadStatus;
            mContext = context;
        }
    }

    private static final String TAG = "DownloadFg";
    // Delay to ensure start/stop foreground doesn't happen too quickly (b/74236718).
    private static final int WAIT_TIME_MS = 200;

    // Used to track existing notifications for UMA stats.
    private final List<Integer> mExistingNotifications = new ArrayList<>();

    // Variables used to ensure start/stop foreground doesn't happen too quickly (b/74236718).
    private final Handler mHandler = new Handler();
    private final Runnable mMaybeStopServiceRunnable = new Runnable() {
        @Override
        public void run() {
            Log.w(TAG, "Checking if delayed stopAndUnbindService needs to be resolved.");
            mStopServiceDelayed = false;
            processDownloadUpdateQueue(false /* not isProcessingPending */);
            mHandler.removeCallbacks(mMaybeStopServiceRunnable);
            Log.w(TAG, "Done checking if delayed stopAndUnbindService needs to be resolved.");
        }
    };
    private boolean mStopServiceDelayed = false;

    private int mPinnedNotificationId = INVALID_NOTIFICATION_ID;

    // This is true when context.bindService has been called and before context.unbindService.
    private boolean mIsServiceBound;
    // This is non-null when onServiceConnected has been called (aka service is active).
    private DownloadForegroundService mBoundService;

    @VisibleForTesting
    final Map<Integer, DownloadUpdate> mDownloadUpdateQueue = new HashMap<>();

    public DownloadForegroundServiceManager() {}

    public void updateDownloadStatus(Context context,
            DownloadNotificationService2.DownloadStatus downloadStatus, int notificationId,
            Notification notification) {
        if (downloadStatus != DownloadNotificationService2.DownloadStatus.IN_PROGRESS) {
            Log.w(TAG,
                    "updateDownloadStatus status: " + downloadStatus + ", id: " + notificationId);
        }
        mDownloadUpdateQueue.put(notificationId,
                new DownloadUpdate(notificationId, notification, downloadStatus, context));
        processDownloadUpdateQueue(false /* not isProcessingPending */);

        // Record instance where notification is being launched for the first time.
        if (!mExistingNotifications.contains(notificationId)) {
            mExistingNotifications.add(notificationId);
            if (Build.VERSION.SDK_INT < 24) {
                DownloadNotificationUmaHelper.recordNotificationFlickerCountHistogram(
                        DownloadNotificationUmaHelper.LaunchType.LAUNCH);
            }
        }
    }

    /**
     * Process the notification queue for all cases and initiate any needed actions.
     * In the happy path, the logic should be:
     * bindAndStartService -> startOrUpdateForegroundService -> stopAndUnbindService.
     * @param isProcessingPending Whether the call was made to process pending notifications that
     *                            have accumulated in the queue during the startup process or if it
     *                            was made based on during a basic update.
     */
    @VisibleForTesting
    void processDownloadUpdateQueue(boolean isProcessingPending) {
        DownloadUpdate downloadUpdate = findInterestingDownloadUpdate();
        if (downloadUpdate == null) return;

        // When nothing has been initialized, just bind the service.
        if (!mIsServiceBound) {
            // If the download update is not active at the onset, don't even start the service!
            if (!isActive(downloadUpdate.mDownloadStatus)) {
                cleanDownloadUpdateQueue();
                return;
            }
            startAndBindService(downloadUpdate.mContext);
            return;
        }

        // Skip everything that happens while waiting for startup.
        if (mBoundService == null) return;

        // In the pending case, start foreground with specific notificationId and notification.
        if (isProcessingPending) {
            Log.w(TAG, "Starting service with type " + downloadUpdate.mDownloadStatus);
            startOrUpdateForegroundService(
                    downloadUpdate.mNotificationId, downloadUpdate.mNotification);

            // Post a delayed task to eventually check to see if service needs to be stopped.
            postMaybeStopServiceRunnable();
        }

        // If the selected downloadUpdate is not active, there are no active downloads left.
        // Stop the foreground service.
        // In the pending case, this will stop the foreground immediately after it was started.
        if (!isActive(downloadUpdate.mDownloadStatus)) {
            // Only stop the service if not waiting for delay (ie. WAIT_TIME_MS has transpired).
            if (!mStopServiceDelayed) {
                stopAndUnbindService(downloadUpdate.mDownloadStatus);
                cleanDownloadUpdateQueue();
            } else {
                Log.w(TAG, "Delaying call to stopAndUnbindService.");
            }
            return;
        }

        // Make sure the pinned notification is still active, if not, update.
        if (mDownloadUpdateQueue.get(mPinnedNotificationId) == null
                || !isActive(mDownloadUpdateQueue.get(mPinnedNotificationId).mDownloadStatus)) {
            startOrUpdateForegroundService(
                    downloadUpdate.mNotificationId, downloadUpdate.mNotification);
        }

        // Clear out inactive download updates in queue if there is at least one active download.
        cleanDownloadUpdateQueue();
    }

    /** Helper code to process download update queue. */

    @Nullable
    private DownloadUpdate findInterestingDownloadUpdate() {
        Iterator<Map.Entry<Integer, DownloadUpdate>> entries =
                mDownloadUpdateQueue.entrySet().iterator();
        while (entries.hasNext()) {
            Map.Entry<Integer, DownloadUpdate> entry = entries.next();
            // Return an active entry if possible.
            if (isActive(entry.getValue().mDownloadStatus)) return entry.getValue();
            // If there are no active entries, just return the last entry.
            if (!entries.hasNext()) return entry.getValue();
        }
        // If there's no entries, return null.
        return null;
    }

    private boolean isActive(DownloadNotificationService2.DownloadStatus downloadStatus) {
        return downloadStatus == DownloadNotificationService2.DownloadStatus.IN_PROGRESS;
    }

    private void cleanDownloadUpdateQueue() {
        Iterator<Map.Entry<Integer, DownloadUpdate>> entries =
                mDownloadUpdateQueue.entrySet().iterator();
        while (entries.hasNext()) {
            Map.Entry<Integer, DownloadUpdate> entry = entries.next();
            // Remove entry that is not active or pinned.
            if (!isActive(entry.getValue().mDownloadStatus)
                    && entry.getValue().mNotificationId != mPinnedNotificationId) {
                entries.remove();
            }
        }
    }

    /** Helper code to bind service. */

    @VisibleForTesting
    void startAndBindService(Context context) {
        Log.w(TAG, "startAndBindService");
        mIsServiceBound = true;
        startAndBindServiceInternal(context);
    }

    @VisibleForTesting
    void startAndBindServiceInternal(Context context) {
        DownloadForegroundService.startDownloadForegroundService(context);
        context.bindService(new Intent(context, DownloadForegroundService.class), mConnection,
                Context.BIND_AUTO_CREATE);
    }

    private final ServiceConnection mConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName className, IBinder service) {
            Log.w(TAG, "onServiceConnected");
            if (!(service instanceof DownloadForegroundService.LocalBinder)) {
                Log.w(TAG,
                        "Not from DownloadNotificationService, do not connect."
                                + " Component name: " + className);
                return;
            }
            mBoundService = ((DownloadForegroundService.LocalBinder) service).getService();
            DownloadForegroundServiceObservers.addObserver(
                    DownloadNotificationServiceObserver.class);
            processDownloadUpdateQueue(true /* isProcessingPending */);
        }

        @Override
        public void onServiceDisconnected(ComponentName componentName) {
            Log.w(TAG, "onServiceDisconnected");
            mBoundService = null;
        }
    };

    /** Helper code to start or update foreground service. */

    @VisibleForTesting
    void startOrUpdateForegroundService(int notificationId, Notification notification) {
        Log.w(TAG, "startOrUpdateForegroundService id: " + notificationId);
        if (mBoundService != null && notificationId != INVALID_NOTIFICATION_ID
                && notification != null) {
            // If there was an originally pinned notification, get its id and notification.
            DownloadUpdate downloadUpdate = mDownloadUpdateQueue.get(mPinnedNotificationId);
            Notification oldNotification =
                    (downloadUpdate == null) ? null : downloadUpdate.mNotification;

            boolean killOldNotification = downloadUpdate != null
                    && downloadUpdate.mDownloadStatus == DownloadStatus.CANCELLED;

            // Start service and handle notifications.
            mBoundService.startOrUpdateForegroundService(notificationId, notification,
                    mPinnedNotificationId, oldNotification, killOldNotification);

            // After the service has been started and the notification handled, change stored id.
            mPinnedNotificationId = notificationId;
        }
    }

    /** Helper code to stop and unbind service. */

    @VisibleForTesting
    void stopAndUnbindService(DownloadNotificationService2.DownloadStatus downloadStatus) {
        Log.w(TAG, "stopAndUnbindService status: " + downloadStatus);
        Preconditions.checkNotNull(mBoundService);
        mIsServiceBound = false;

        @DownloadForegroundService.StopForegroundNotification
        int stopForegroundNotification;
        if (downloadStatus == DownloadNotificationService2.DownloadStatus.CANCELLED) {
            stopForegroundNotification = DownloadForegroundService.StopForegroundNotification.KILL;
        } else if (downloadStatus == DownloadNotificationService2.DownloadStatus.PAUSED) {
            stopForegroundNotification =
                    DownloadForegroundService.StopForegroundNotification.DETACH_OR_PERSIST;
        } else {
            stopForegroundNotification =
                    DownloadForegroundService.StopForegroundNotification.DETACH_OR_ADJUST;
        }

        DownloadUpdate downloadUpdate = mDownloadUpdateQueue.get(mPinnedNotificationId);
        Notification oldNotification =
                (downloadUpdate == null) ? null : downloadUpdate.mNotification;

        boolean notificationHandledProperly = stopAndUnbindServiceInternal(
                stopForegroundNotification, mPinnedNotificationId, oldNotification);

        mBoundService = null;

        // Only if the notification was handled properly (ie. detached or killed), reset stored ID.
        if (notificationHandledProperly) mPinnedNotificationId = INVALID_NOTIFICATION_ID;
    }

    @VisibleForTesting
    boolean stopAndUnbindServiceInternal(
            @DownloadForegroundService.StopForegroundNotification int stopForegroundStatus,
            int pinnedNotificationId, Notification pinnedNotification) {
        boolean notificationHandledProperly = mBoundService.stopDownloadForegroundService(
                stopForegroundStatus, pinnedNotificationId, pinnedNotification);
        ContextUtils.getApplicationContext().unbindService(mConnection);

        if (notificationHandledProperly) {
            DownloadForegroundServiceObservers.removeObserver(
                    DownloadNotificationServiceObserver.class);
        }

        return notificationHandledProperly;
    }

    /** Helper code for testing. */

    @VisibleForTesting
    void setBoundService(DownloadForegroundService service) {
        mBoundService = service;
    }

    // Allow testing methods to skip posting the delayed runnable.
    @VisibleForTesting
    void postMaybeStopServiceRunnable() {
        mHandler.removeCallbacks(mMaybeStopServiceRunnable);
        mHandler.postDelayed(mMaybeStopServiceRunnable, WAIT_TIME_MS);
        mStopServiceDelayed = true;
    }
}
