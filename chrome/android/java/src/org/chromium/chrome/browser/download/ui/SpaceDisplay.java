// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.ui;

import android.content.Context;
import android.os.AsyncTask;
import android.os.Environment;
import android.os.StatFs;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.ObserverList;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.download.DownloadUtils;
import org.chromium.chrome.browser.widget.MaterialProgressBar;

import java.io.File;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.RejectedExecutionException;

/** A View that manages the display of space used by the downloads. */
public class SpaceDisplay extends RecyclerView.AdapterDataObserver {
    /** Observes changes to the SpaceDisplay. */
    public static interface Observer {
        /** Called when the display has had its values updated. */
        void onSpaceDisplayUpdated(SpaceDisplay spaceDisplay);
    }

    private static final String TAG = "download_ui";

    private static final int[] USED_STRINGS = {
        R.string.download_manager_ui_space_used_kb,
        R.string.download_manager_ui_space_used_mb,
        R.string.download_manager_ui_space_used_gb
    };
    private static final int[] OTHER_STRINGS = {
        R.string.download_manager_ui_space_other_kb,
        R.string.download_manager_ui_space_other_mb,
        R.string.download_manager_ui_space_other_gb
    };

    private static class StorageSizeTask extends AsyncTask<Void, Void, Long> {
        /**
         * If true, the task gets the total size of storage.  If false, it fetches how much
         * space is free.
         */
        private boolean mFetchTotalSize;

        StorageSizeTask(boolean fetchTotalSize) {
            mFetchTotalSize = fetchTotalSize;
        }

        @Override
        protected Long doInBackground(Void... params) {
            File downloadDirectory = Environment.getExternalStoragePublicDirectory(
                    Environment.DIRECTORY_DOWNLOADS);

            // Create the downloads directory, if necessary.
            if (!downloadDirectory.exists()) {
                try {
                    // mkdirs() can fail, so we still need to check if the directory exists
                    // later.
                    downloadDirectory.mkdirs();
                } catch (SecurityException e) {
                    Log.e(TAG, "SecurityException when creating download directory.", e);
                }
            }

            // Determine how much space is available on the storage device where downloads
            // reside.  If the downloads directory doesn't exist, it is likely that the user
            // doesn't have an SD card installed.
            long blocks = 0;
            if (downloadDirectory.exists()) {
                StatFs statFs = new StatFs(downloadDirectory.getPath());
                if (mFetchTotalSize) {
                    blocks = ApiCompatibilityUtils.getBlockCount(statFs);
                } else {
                    blocks = ApiCompatibilityUtils.getAvailableBlocks(statFs);
                }

                return blocks * ApiCompatibilityUtils.getBlockSize(statFs);
            } else {
                Log.e(TAG, "Download directory doesn't exist.");
                return 0L;
            }
        }
    };

    private final ObserverList<Observer> mObservers = new ObserverList<>();
    private final AsyncTask<Void, Void, Long> mFileSystemBytesTask;
    private AsyncTask<Void, Void, Long> mFreeBytesTask;

    private DownloadHistoryAdapter mHistoryAdapter;
    private View mView;
    private View mViewContainer;
    private TextView mSpaceUsedByDownloadsTextView;
    private TextView mSpaceFreeAndOtherAppsTextView;
    private MaterialProgressBar mSpaceBar;
    private long mFreeBytes;

    SpaceDisplay(final ViewGroup parent, DownloadHistoryAdapter historyAdapter) {
        mHistoryAdapter = historyAdapter;
        mViewContainer = LayoutInflater.from(ContextUtils.getApplicationContext())
                                 .inflate(R.layout.download_manager_ui_space_widget, parent, false);
        mView = mViewContainer.findViewById(R.id.space_widget_content);
        mSpaceUsedByDownloadsTextView = (TextView) mView.findViewById(R.id.size_downloaded);
        mSpaceFreeAndOtherAppsTextView =
                (TextView) mView.findViewById(R.id.size_free_and_other_apps);
        mSpaceBar = (MaterialProgressBar) mView.findViewById(R.id.space_bar);
        mFileSystemBytesTask =
                new StorageSizeTask(true).executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    /** @return The view container of space display view. */
    public View getViewContainer() {
        return mViewContainer;
    }

    /** Returns the view. */
    public View getView() {
        return mView;
    }

    @Override
    public void onChanged() {
        // Record how much the user has downloaded relative to the size of their storage.
        try {
            long bytesUsedByDownloads = Math.max(0, mHistoryAdapter.getTotalDownloadSize());
            RecordHistogram.recordPercentageHistogram("Android.DownloadManager.SpaceUsed",
                    computePercentage(bytesUsedByDownloads, mFileSystemBytesTask.get()));
        } catch (ExecutionException | InterruptedException e) {
            // Can't record what we don't have.
        }

        // Determine how much space is free now, then update the display.
        if (mFreeBytesTask == null) {
            mFreeBytesTask = new StorageSizeTask(false) {
                @Override
                protected void onPostExecute(Long bytes) {
                    mFreeBytes = bytes.longValue();
                    mFreeBytesTask = null;
                    update();
                }
            };

            try {
                mFreeBytesTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
            } catch (RejectedExecutionException e) {
                mFreeBytesTask = null;
            }
        }
    }

    @VisibleForTesting
    public void addObserverForTests(Observer observer) {
        mObservers.addObserver(observer);
    }

    private void update() {
        long fileSystemBytes = 0;

        try {
            fileSystemBytes = mFileSystemBytesTask.get();
        } catch (ExecutionException | InterruptedException e) {
            // Can't do anything here.
        }

        // Indicate how much space has been used by everything on the device via the progress bar.
        long bytesUsedTotal = Math.max(0, fileSystemBytes - mFreeBytes);
        long bytesUsedByDownloads = Math.max(0, mHistoryAdapter.getTotalDownloadSize());
        long bytesUsedByOtherApps = Math.max(0, bytesUsedTotal - bytesUsedByDownloads);

        // Describe how much space has been used by downloads in text.
        Context context = mSpaceUsedByDownloadsTextView.getContext();
        mSpaceUsedByDownloadsTextView.setText(
                DownloadUtils.getStringForBytes(context, USED_STRINGS, bytesUsedByDownloads));

        String spaceFree = DownloadUtils.getStringForAvailableBytes(context, mFreeBytes);
        String spaceUsedByOtherApps =
                DownloadUtils.getStringForBytes(context, OTHER_STRINGS, bytesUsedByOtherApps);
        mSpaceFreeAndOtherAppsTextView.setText(
                context.getResources().getString(R.string.download_manager_ui_space_free_and_other,
                        spaceFree, spaceUsedByOtherApps));

        // Set a minimum size for the download size so that it shows up in the progress bar.
        long threePercentOfSystem = fileSystemBytes == 0 ? 0 : fileSystemBytes / 100 * 3;
        long fudgedBytesUsedByDownloads = Math.max(bytesUsedByDownloads, threePercentOfSystem);
        long fudgedBytesUsedByOtherApps = Math.max(bytesUsedByOtherApps, threePercentOfSystem);

        // Indicate how much space has been used as a progress bar.  The percentage used by
        // downloads is shown by the non-overlapped area of the primary and secondary progressbar.
        int percentageUsedTotal = computePercentage(
                fudgedBytesUsedByDownloads + fudgedBytesUsedByOtherApps, fileSystemBytes);
        int percentageDownloaded = computePercentage(fudgedBytesUsedByDownloads, fileSystemBytes);
        mSpaceBar.setProgress(percentageUsedTotal);
        mSpaceBar.setSecondaryProgress(percentageDownloaded);

        for (Observer observer : mObservers) observer.onSpaceDisplayUpdated(this);
    }

    private int computePercentage(long numerator, long denominator) {
        if (denominator == 0) return 0;
        return (int) Math.min(100.0f, Math.max(0.0f, 100.0f * numerator / denominator));
    }
}
