// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import android.app.Notification;
import android.content.Context;
import android.graphics.Bitmap;

import org.chromium.base.ThreadUtils;
import org.chromium.components.offline_items_collection.ContentId;
import org.chromium.components.offline_items_collection.OfflineItem.Progress;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Callable;

/**
 * Mock class to DownloadNotificationService for testing purpose.
 */
public class MockDownloadNotificationService extends DownloadNotificationService {
    private final List<Integer> mNotificationIds = new ArrayList<Integer>();
    private boolean mPaused = false;
    private Context mContext;
    private int mLastNotificationId;
    private boolean mIsForegroundRunning = false;

    void setContext(Context context) {
        mContext = context;
    }

    @Override
    public void stopForegroundInternal(boolean killNotification) {
        if (!useForegroundService()) return;
        if (killNotification) mNotificationIds.clear();
        mIsForegroundRunning = false;
    }

    @Override
    public void startForegroundInternal() {
        mIsForegroundRunning = true;
    }

    public boolean isForegroundRunning() {
        return mIsForegroundRunning;
    }

    @Override
    public void cancelOffTheRecordDownloads() {
        super.cancelOffTheRecordDownloads();
        mPaused = true;
    }

    @Override
    boolean hasDownloadNotificationsInternal(int notificationIdToIgnore) {
        if (!useForegroundService()) return false;
        // Cancelling notifications here is synchronous, so we don't really have to worry about
        // {@code notificationIdToIgnore}, but address it properly anyway.
        if (mNotificationIds.size() == 1 && notificationIdToIgnore != -1) {
            return !mNotificationIds.contains(notificationIdToIgnore);
        }

        return !mNotificationIds.isEmpty();
    }

    @Override
    void cancelSummaryNotification() {}

    @Override
    void updateNotification(int id, Notification notification) {
        if (!mNotificationIds.contains(id)) {
            mNotificationIds.add(id);
            mLastNotificationId = id;
        }
    }

    public boolean isPaused() {
        return mPaused;
    }

    public List<Integer> getNotificationIds() {
        return mNotificationIds;
    }

    @Override
    public void cancelNotification(int notificationId, ContentId id) {
        super.cancelNotification(notificationId, id);
        mNotificationIds.remove(Integer.valueOf(notificationId));
    }

    public int getLastAddedNotificationId() {
        return mLastNotificationId;
    }

    @Override
    public Context getApplicationContext() {
        return mContext == null ? super.getApplicationContext() : mContext;
    }

    @Override
    public int notifyDownloadSuccessful(final ContentId id, final String filePath,
            final String fileName, final long systemDownloadId, final boolean isOffTheRecord,
            final boolean isSupportedMimeType, final boolean isOpenable, final Bitmap icon,
            final String originalUrl, final String referrer) {
        return ThreadUtils.runOnUiThreadBlockingNoException(new Callable<Integer>() {
            @Override
            public Integer call() throws Exception {
                return MockDownloadNotificationService.super.notifyDownloadSuccessful(id, filePath,
                        fileName, systemDownloadId, isOffTheRecord, isSupportedMimeType, isOpenable,
                        icon, originalUrl, referrer);
            }
        });
    }

    @Override
    public void notifyDownloadProgress(final ContentId id, final String fileName,
            final Progress progress, final long bytesReceived, final long timeRemainingInMillis,
            final long startTime, final boolean isOffTheRecord,
            final boolean canDownloadWhileMetered, final boolean isTransient, final Bitmap icon) {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                MockDownloadNotificationService.super.notifyDownloadProgress(id, fileName, progress,
                        bytesReceived, timeRemainingInMillis, startTime, isOffTheRecord,
                        canDownloadWhileMetered, isTransient, icon);
            }
        });
    }

    @Override
    public void notifyDownloadFailed(final ContentId id, final String fileName, final Bitmap icon) {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                MockDownloadNotificationService.super.notifyDownloadFailed(id, fileName, icon);
            }
        });
    }

    @Override
    public void notifyDownloadCanceled(final ContentId id) {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                MockDownloadNotificationService.super.notifyDownloadCanceled(id);
            }
        });
    }
}

