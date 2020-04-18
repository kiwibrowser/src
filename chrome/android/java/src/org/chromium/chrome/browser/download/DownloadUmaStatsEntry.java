// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import org.chromium.base.Log;

/**
 * SharedPreferences entries for for helping report UMA stats. A download may require several
 * browser sessions to complete, we need to store them in SharedPreferences in case browser is
 * killed.
 * In the first version, there are 5 fields: useDownloadManager, isPaused, downloadStartTime,
 * numInterruptions, id.
 * In the 2nd version, there are 2 more fieds: lastBytesReceived and bytesWasted
 */
public class DownloadUmaStatsEntry {
    private static final String TAG = "DownloadUmaStats";
    public final String id;
    public final long downloadStartTime;
    public final boolean useDownloadManager;
    public int numInterruptions;
    public boolean isPaused;
    public long lastBytesReceived;
    public long bytesWasted;

    DownloadUmaStatsEntry(String id, long downloadStartTime, int numInterruptions,
            boolean isPaused, boolean useDownloadManager, long lastBytesReceived,
            long bytesWasted) {
        this.id = id;
        this.downloadStartTime = downloadStartTime;
        this.numInterruptions = numInterruptions;
        this.isPaused = isPaused;
        this.useDownloadManager = useDownloadManager;
        this.lastBytesReceived = lastBytesReceived;
        this.bytesWasted = bytesWasted;
    }

    /**
     * Parse the UMA entry from a String object in SharedPrefs.
     * For first versions, there are only 5 fields, the latest version have 7 fields (
     * lastBytesReceived and mBytesWasted).
     *
     * @param sharedPrefString String from SharedPreference.
     * @return a DownloadUmaStatsEntry object.
     */
    static DownloadUmaStatsEntry parseFromString(String sharedPrefString) {
        String[] values = sharedPrefString.split(",", 7);
        if (values.length == 5 || values.length == 7) {
            try {
                boolean useDownloadManager = "1".equals(values[0]);
                boolean isPaused = "1".equals(values[1]);
                long downloadStartTime = Long.parseLong(values[2]);
                int numInterruptions = Integer.parseInt(values[3]);
                String id = values[4];
                long lastReceived = 0;
                long wasted = 0;
                if (values.length == 7) {
                    lastReceived = Long.parseLong(values[5].trim());
                    wasted = Long.parseLong(values[6].trim());
                }
                return new DownloadUmaStatsEntry(
                        id, downloadStartTime, numInterruptions, isPaused, useDownloadManager,
                        lastReceived, wasted);
            } catch (NumberFormatException nfe) {
                Log.w(TAG, "Exception while parsing UMA entry:" + sharedPrefString);
            }
        }
        return null;
    }

    /**
     * @return a string for the DownloadUmaStatsEntry instance to be inserted into SharedPrefs.
     */
    String getSharedPreferenceString() {
        return (useDownloadManager ? "1" : "0") + "," + (isPaused ? "1" : "0") + ","
                + downloadStartTime + "," + numInterruptions + "," + id + "," + lastBytesReceived
                + "," + bytesWasted;
    }

    /**
     * Build a DownloadItem from this object.
     * @return a DownloadItem built from this object.
     */
    DownloadItem buildDownloadItem() {
        DownloadItem item = new DownloadItem(useDownloadManager, null);
        item.setStartTime(downloadStartTime);
        if (useDownloadManager) {
            item.setSystemDownloadId(Long.parseLong(id));
        } else {
            DownloadInfo info = new DownloadInfo.Builder().setDownloadGuid(id)
                    .setBytesReceived(lastBytesReceived).build();
            item.setDownloadInfo(info);
        }
        return item;
    }
}
