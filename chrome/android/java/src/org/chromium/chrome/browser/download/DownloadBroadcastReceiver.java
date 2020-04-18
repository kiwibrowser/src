// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

/**
 * This {@link BroadcastReceiver} handles clicks to download notifications and their action buttons.
 * Clicking on an in-progress or failed download will open the download manager. Clicking on
 * a complete, successful download will open the file. Clicking on the resume button of a paused
 * download will relaunch the browser process and try to resume the download from where it is
 * stopped.
 */
public class DownloadBroadcastReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(final Context context, Intent intent) {
        performDownloadOperation(context, intent);
    }

    /**
     * Called to perform a download operation. This will call the DownloadNotificationService
     * to start the browser process asynchronously, and resume or cancel the download afterwards.
     * @param context Context of the receiver.
     * @param intent Intent retrieved from the notification.
     */
    private void performDownloadOperation(final Context context, Intent intent) {
        if (DownloadNotificationService.isDownloadOperationIntent(intent)) {
            DownloadNotificationService.startDownloadNotificationService(context, intent);
        }
    }
}
