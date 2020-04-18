// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.ui;

import android.content.Context;
import android.os.AsyncTask;
import android.os.Environment;
import android.view.View;
import android.widget.TextView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.download.DirectoryOption;
import org.chromium.chrome.browser.download.DownloadUtils;

import java.io.File;

/**
 * The storage summary inside the download home toolbar.
 */
public class StorageSummary {
    private DirectoryOption mDirectoryOption;
    private final TextView mView;

    /**
     * Asynchronous task to query the default download directory option on primary storage.
     * Pass one String parameter as the name of the directory option.
     */
    private static class DefaultDirectoryTask extends AsyncTask<Void, Void, DirectoryOption> {
        @Override
        protected DirectoryOption doInBackground(Void... params) {
            File defaultDownloadDir =
                    Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS);
            DirectoryOption directoryOption = new DirectoryOption("",
                    defaultDownloadDir.getAbsolutePath(), defaultDownloadDir.getUsableSpace(),
                    defaultDownloadDir.getTotalSpace(), DirectoryOption.DEFAULT_OPTION);
            return directoryOption;
        }
    }

    public StorageSummary(TextView view) {
        mView = view;

        if (ChromeFeatureList.isEnabled(ChromeFeatureList.DOWNLOADS_LOCATION_CHANGE)) {
            computeStorage();
        } else {
            mView.setVisibility(View.GONE);
        }
    }

    private void computeStorage() {
        DefaultDirectoryTask task = new DefaultDirectoryTask() {
            @Override
            protected void onPostExecute(DirectoryOption directoryOption) {
                mDirectoryOption = directoryOption;
                update();
            }
        };
        task.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    private void update() {
        if (mDirectoryOption == null) return;

        // Set the storage summary to the view.
        Context context = mView.getContext();
        long usedSpace = mDirectoryOption.totalSpace - mDirectoryOption.availableSpace;
        if (usedSpace < 0) usedSpace = 0;
        String storageSummary = context.getString(R.string.download_manager_ui_space_using,
                DownloadUtils.getStringForBytes(context, usedSpace),
                DownloadUtils.getStringForBytes(context, mDirectoryOption.totalSpace));
        mView.setText(storageSummary);
    }
}
