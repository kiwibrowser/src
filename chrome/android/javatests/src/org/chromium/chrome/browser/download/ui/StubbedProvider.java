// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.ui;

import static junit.framework.Assert.assertEquals;
import static junit.framework.Assert.assertNull;

import android.os.Environment;
import android.os.Handler;
import android.os.Looper;

import org.chromium.base.Callback;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.chrome.browser.download.DownloadInfo;
import org.chromium.chrome.browser.download.DownloadItem;
import org.chromium.chrome.browser.widget.ThumbnailProvider;
import org.chromium.chrome.browser.widget.selection.SelectionDelegate;
import org.chromium.components.download.DownloadState;
import org.chromium.components.offline_items_collection.ContentId;
import org.chromium.components.offline_items_collection.LegacyHelpers;
import org.chromium.components.offline_items_collection.OfflineContentProvider;
import org.chromium.components.offline_items_collection.OfflineItem;
import org.chromium.components.offline_items_collection.OfflineItem.Progress;
import org.chromium.components.offline_items_collection.OfflineItemFilter;
import org.chromium.components.offline_items_collection.OfflineItemProgressUnit;
import org.chromium.components.offline_items_collection.OfflineItemState;
import org.chromium.components.offline_items_collection.VisualsCallback;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

/** Stubs out backends used by the Download Home UI. */
public class StubbedProvider implements BackendProvider {

    /** Stubs out the DownloadManagerService. */
    public class StubbedDownloadDelegate implements DownloadDelegate {
        public final CallbackHelper addCallback = new CallbackHelper();
        public final CallbackHelper removeCallback = new CallbackHelper();
        public final CallbackHelper checkExternalCallback = new CallbackHelper();
        public final CallbackHelper removeDownloadCallback = new CallbackHelper();

        public final List<DownloadItem> regularItems = new ArrayList<>();
        public final List<DownloadItem> offTheRecordItems = new ArrayList<>();
        private DownloadHistoryAdapter mAdapter;

        @Override
        public void addDownloadHistoryAdapter(DownloadHistoryAdapter adapter) {
            addCallback.notifyCalled();
            assertNull(mAdapter);
            mAdapter = adapter;
        }

        @Override
        public void removeDownloadHistoryAdapter(DownloadHistoryAdapter adapter) {
            removeCallback.notifyCalled();
            mAdapter = null;
        }

        @Override
        public void getAllDownloads(final boolean isOffTheRecord) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    mAdapter.onAllDownloadsRetrieved(
                            isOffTheRecord ? offTheRecordItems : regularItems, isOffTheRecord);
                }
            });
        }

        @Override
        public void broadcastDownloadAction(DownloadItem downloadItem, String action) {}

        @Override
        public void checkForExternallyRemovedDownloads(boolean isOffTheRecord) {
            checkExternalCallback.notifyCalled();
        }

        @Override
        public void removeDownload(
                final String guid, final boolean isOffTheRecord, boolean externallyRemoved) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    mAdapter.onDownloadItemRemoved(guid, isOffTheRecord);
                    removeDownloadCallback.notifyCalled();
                }
            });
        }

        @Override
        public boolean isDownloadOpenableInBrowser(boolean isOffTheRecord, String mimeType) {
            return false;
        }

        @Override
        public void updateLastAccessTime(String downloadGuid, boolean isOffTheRecord) {}
    }

    /** Stubs out the OfflineContentProvider. */
    public class StubbedOfflineContentProvider implements OfflineContentProvider {
        public final CallbackHelper addCallback = new CallbackHelper();
        public final CallbackHelper removeCallback = new CallbackHelper();
        public final CallbackHelper deleteItemCallback = new CallbackHelper();
        public final ArrayList<OfflineItem> items = new ArrayList<>();
        public OfflineContentProvider.Observer observer;

        @Override
        public void addObserver(OfflineContentProvider.Observer addedObserver) {
            // Immediately indicate that the delegate has loaded.
            observer = addedObserver;
            addCallback.notifyCalled();
        }

        @Override
        public void removeObserver(OfflineContentProvider.Observer removedObserver) {
            assertEquals(observer, removedObserver);
            observer = null;
            removeCallback.notifyCalled();
        }

        @Override
        public void getAllItems(Callback<ArrayList<OfflineItem>> callback) {
            mHandler.post(() -> callback.onResult(items));
        }

        @Override
        public void removeItem(ContentId id) {
            for (OfflineItem item : items) {
                if (item.id.equals(id)) {
                    items.remove(item);
                    break;
                }
            }

            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    observer.onItemRemoved(id);
                    deleteItemCallback.notifyCalled();
                }
            });
        }

        @Override
        public void openItem(ContentId id) {}
        @Override
        public void pauseDownload(ContentId id) {}
        @Override
        public void resumeDownload(ContentId id, boolean hasUserGesture) {}
        @Override
        public void cancelDownload(ContentId id) {}

        @Override
        public void getItemById(ContentId id, Callback<OfflineItem> callback) {
            mHandler.post(() -> callback.onResult(null));
        }

        @Override
        public void getVisualsForItem(ContentId id, VisualsCallback callback) {}
    }

    /** Stubs out all attempts to get thumbnails for files. */
    public static class StubbedThumbnailProvider implements ThumbnailProvider {
        @Override
        public void destroy() {}

        @Override
        public void getThumbnail(ThumbnailRequest request) {}

        @Override
        public void removeThumbnailsFromDisk(String contentId) {}

        @Override
        public void cancelRetrieval(ThumbnailRequest request) {}
    }

    /** Stubs out the UIDelegate. */
    public class StubbedUIDelegate implements UIDelegate {
        @Override
        public void deleteItem(DownloadHistoryItemWrapper item) {
            mHandler.post(() -> item.removePermanently());
        }

        @Override
        public void shareItem(DownloadHistoryItemWrapper item) {}
    }

    private static final long ONE_GIGABYTE = 1024L * 1024L * 1024L;

    private final Handler mHandler;
    private final StubbedDownloadDelegate mDownloadDelegate;
    private final StubbedOfflineContentProvider mOfflineContentProvider;
    private final SelectionDelegate<DownloadHistoryItemWrapper> mSelectionDelegate;
    private final StubbedThumbnailProvider mStubbedThumbnailProvider;
    private UIDelegate mUIDelegate;

    public StubbedProvider() {
        mHandler = new Handler(Looper.getMainLooper());
        mDownloadDelegate = new StubbedDownloadDelegate();
        mOfflineContentProvider = new StubbedOfflineContentProvider();
        mSelectionDelegate = new DownloadItemSelectionDelegate();
        mStubbedThumbnailProvider = new StubbedThumbnailProvider();
        mUIDelegate = new StubbedUIDelegate();
    }

    public void setUIDelegate(UIDelegate delegate) {
        mUIDelegate = delegate;
    }

    @Override
    public StubbedDownloadDelegate getDownloadDelegate() {
        return mDownloadDelegate;
    }

    @Override
    public StubbedOfflineContentProvider getOfflineContentProvider() {
        return mOfflineContentProvider;
    }

    @Override
    public SelectionDelegate<DownloadHistoryItemWrapper> getSelectionDelegate() {
        return mSelectionDelegate;
    }

    @Override
    public StubbedThumbnailProvider getThumbnailProvider() {
        return mStubbedThumbnailProvider;
    }

    @Override
    public UIDelegate getUIDelegate() {
        return mUIDelegate;
    }

    @Override
    public void destroy() {}

    /** See {@link #createDownloadItem(int, String, boolean, int, int)}. */
    public static DownloadItem createDownloadItem(int which, String date) throws Exception {
        return createDownloadItem(which, date, false, DownloadState.COMPLETE, 100);
    }

    /** Creates a new DownloadItem with pre-defined values. */
    public static DownloadItem createDownloadItem(
            int which, String date, boolean isIncognito, int state, int percent) throws Exception {
        DownloadInfo.Builder builder = null;
        if (which == 0) {
            builder = new DownloadInfo.Builder()
                    .setUrl("https://google.com")
                    .setBytesReceived(1)
                    .setFileName("first_file.jpg")
                    .setFilePath("/storage/fake_path/Downloads/first_file.jpg")
                    .setDownloadGuid("first_guid")
                    .setMimeType("image/jpeg");
        } else if (which == 1) {
            builder = new DownloadInfo.Builder()
                    .setUrl("https://one.com")
                    .setBytesReceived(10)
                    .setFileName("second_file.gif")
                    .setFilePath("/storage/fake_path/Downloads/second_file.gif")
                    .setDownloadGuid("second_guid")
                    .setMimeType("image/gif");
        } else if (which == 2) {
            builder = new DownloadInfo.Builder()
                    .setUrl("https://is.com")
                    .setBytesReceived(100)
                    .setFileName("third_file")
                    .setFilePath("/storage/fake_path/Downloads/third_file")
                    .setDownloadGuid("third_guid")
                    .setMimeType("text/plain");
        } else if (which == 3) {
            builder = new DownloadInfo.Builder()
                    .setUrl("https://the.com")
                    .setBytesReceived(5)
                    .setFileName("four.webm")
                    .setFilePath("/storage/fake_path/Downloads/four.webm")
                    .setDownloadGuid("fourth_guid")
                    .setMimeType("video/webm");
        } else if (which == 4) {
            builder = new DownloadInfo.Builder()
                    .setUrl("https://loneliest.com")
                    .setBytesReceived(50)
                    .setFileName("five.mp3")
                    .setFilePath("/storage/fake_path/Downloads/five.mp3")
                    .setDownloadGuid("fifth_guid")
                    .setMimeType("audio/mp3");
        } else if (which == 5) {
            builder = new DownloadInfo.Builder()
                    .setUrl("https://number.com")
                    .setBytesReceived(500)
                    .setFileName("six.mp3")
                    .setFilePath("/storage/fake_path/Downloads/six.mp3")
                    .setDownloadGuid("sixth_guid")
                    .setMimeType("audio/mp3");
        } else if (which == 6) {
            builder =
                    new DownloadInfo.Builder()
                            .setUrl("https://sigh.com")
                            .setBytesReceived(ONE_GIGABYTE)
                            .setFileName("huge_image.png")
                            .setFilePath(Environment.getExternalStorageDirectory().getAbsolutePath()
                                    + "/fake_path/Downloads/huge_image.png")
                            .setDownloadGuid("seventh_guid")
                            .setMimeType("image/png");
        } else if (which == 7) {
            builder = new DownloadInfo.Builder()
                    .setUrl("https://sleepy.com")
                    .setBytesReceived(ONE_GIGABYTE / 2)
                    .setFileName("sleep.pdf")
                    .setFilePath("/storage/fake_path/Downloads/sleep.pdf")
                    .setDownloadGuid("eighth_guid")
                    .setMimeType("application/pdf");
        } else if (which == 8) {
            // This is a duplicate of item 7 above with a different GUID.
            builder = new DownloadInfo.Builder()
                    .setUrl("https://sleepy.com")
                    .setBytesReceived(ONE_GIGABYTE / 2)
                    .setFileName("sleep.pdf")
                    .setFilePath("/storage/fake_path/Downloads/sleep.pdf")
                    .setDownloadGuid("ninth_guid")
                    .setMimeType("application/pdf");
        } else if (which == 9) {
            builder = new DownloadInfo.Builder()
                    .setUrl("https://totallynew.com")
                    .setBytesReceived(ONE_GIGABYTE / 10)
                    .setFileName("forserious.jpg")
                    .setFilePath(null)
                    .setDownloadGuid("tenth_guid")
                    .setMimeType("image/jpg");
        } else if (which == 10) {
            // Duplicate version of #9, but the file path has been set.
            builder = new DownloadInfo.Builder()
                    .setUrl("https://totallynew.com")
                    .setBytesReceived(ONE_GIGABYTE / 10)
                    .setFileName("forserious.jpg")
                    .setFilePath("/storage/fake_path/Downloads/forserious.jpg")
                    .setDownloadGuid("tenth_guid")
                    .setMimeType("image/jpg");
        } else {
            return null;
        }

        builder.setIsOffTheRecord(isIncognito);
        builder.setProgress(new Progress(100, 100L, OfflineItemProgressUnit.PERCENTAGE));
        builder.setState(state);

        DownloadItem item = new DownloadItem(false, builder.build());
        item.setStartTime(dateToEpoch(date));
        return item;
    }

    /** Creates a new OfflineItem with pre-defined values. */
    public static OfflineItem createOfflineItem(int which, String date) throws Exception {
        long startTime = dateToEpoch(date);
        int downloadState = OfflineItemState.COMPLETE;
        if (which == 0) {
            return createOfflineItem("offline_guid_1", "https://url.com", downloadState, 0,
                    "page 1", "/data/fake_path/Downloads/first_file", startTime, 1000);
        } else if (which == 1) {
            return createOfflineItem("offline_guid_2", "http://stuff_and_things.com", downloadState,
                    0, "page 2", "/data/fake_path/Downloads/file_two", startTime, 10000);
        } else if (which == 2) {
            return createOfflineItem("offline_guid_3", "https://url.com", downloadState, 100,
                    "page 3", "/data/fake_path/Downloads/3_file", startTime, 100000);
        } else if (which == 3) {
            return createOfflineItem("offline_guid_4", "https://thangs.com", downloadState, 1024,
                    "page 4", "/data/fake_path/Downloads/4", startTime, ONE_GIGABYTE * 5L);
        } else {
            return null;
        }
    }

    private static OfflineItem createOfflineItem(String guid, String url, int state,
            long downloadProgressBytes, String title, String targetPath, long startTime,
            long totalSize) {
        OfflineItem offlineItem = new OfflineItem();
        offlineItem.id = new ContentId(LegacyHelpers.LEGACY_OFFLINE_PAGE_NAMESPACE, guid);
        offlineItem.pageUrl = url;
        offlineItem.state = state;
        offlineItem.receivedBytes = downloadProgressBytes;
        offlineItem.title = title;
        offlineItem.filePath = targetPath;
        offlineItem.creationTimeMs = startTime;
        offlineItem.totalSizeBytes = totalSize;
        offlineItem.filter = OfflineItemFilter.FILTER_PAGE;
        return offlineItem;
    }

    /** Converts a date string to a timestamp. */
    private static long dateToEpoch(String dateStr) throws Exception {
        return new SimpleDateFormat("yyyyMMdd HH:mm", Locale.getDefault()).parse(dateStr).getTime();
    }

}
