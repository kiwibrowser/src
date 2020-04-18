// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import android.app.DownloadManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.support.test.filters.SmallTest;

import org.hamcrest.Matchers;
import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.ContextUtils;
import org.chromium.base.test.util.AdvancedMockContext;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.base.test.util.UrlUtils;
import org.chromium.chrome.browser.download.DownloadManagerDelegate.DownloadQueryCallback;
import org.chromium.chrome.browser.download.DownloadManagerDelegate.DownloadQueryResult;
import org.chromium.chrome.browser.download.OMADownloadHandler.OMAInfo;
import org.chromium.chrome.browser.test.ChromeBrowserTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.net.test.EmbeddedTestServer;

import java.io.ByteArrayInputStream;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * Tests for OMADownloadHandler class.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
public class OMADownloadHandlerTest {
    @Rule
    public final ChromeBrowserTestRule mBrowserTestRule = new ChromeBrowserTestRule();

    private static final String PENDING_OMA_DOWNLOADS = "PendingOMADownloads";
    private static final String INSTALL_NOTIFY_URI = "http://test/test";

    private Context getTestContext() {
        return new AdvancedMockContext(InstrumentationRegistry.getTargetContext());
    }

    /**
     * Mock implementation of the DownloadSnackbarController.
     */
    static class MockDownloadSnackbarController extends DownloadSnackbarController {
        public boolean mSucceeded;
        public boolean mFailed;

        public MockDownloadSnackbarController() {
            super(null);
        }

        public void waitForSnackbarControllerToFinish(final boolean success) {
            CriteriaHelper.pollInstrumentationThread(
                    new Criteria("Failed while waiting for all calls to complete.") {
                        @Override
                        public boolean isSatisfied() {
                            return success ? mSucceeded : mFailed;
                        }
                    });
        }

        @Override
        public void onDownloadSucceeded(DownloadInfo downloadInfo, int notificationId,
                long downloadId, boolean canBeResolved, boolean usesAndroidDownloadManager) {
            mSucceeded = true;
        }

        @Override
        public void onDownloadFailed(String errorMessage, boolean showAllDownloads) {
            mFailed = true;
        }
    }

    private static class OMADownloadHandlerForTest extends OMADownloadHandler {
        public String mNofityURI;
        public long mDownloadId;

        public OMADownloadHandlerForTest(Context context, DownloadManagerDelegate delegate,
                DownloadSnackbarController downloadSnackbarController) {
            super(context, delegate, downloadSnackbarController);
        }

        @Override
        protected boolean sendNotification(
                OMAInfo omaInfo, DownloadInfo downloadInfo, long downloadId, String statusMessage) {
            mNofityURI = omaInfo.getValue(OMA_INSTALL_NOTIFY_URI);
            return true;
        }

        @Override
        public void onDownloadEnqueued(
                boolean result, int failureReason, DownloadItem downloadItem, long downloadId) {
            super.onDownloadEnqueued(result, failureReason, downloadItem, downloadId);
            mDownloadId = downloadId;
        }
    }

    /** Helper class to verify the result of {@DownloadManagerDelegate.queryDownloadResult}. */
    private static class DownloadQueryResultVerifier implements DownloadQueryCallback {
        private int mExpectedDownloadStatus;

        public boolean mQueryCompleted;

        public DownloadQueryResultVerifier(int expectedDownloadStatus) {
            mExpectedDownloadStatus = expectedDownloadStatus;
        }

        @Override
        public void onQueryCompleted(DownloadQueryResult result, boolean showNotifications) {
            mQueryCompleted = true;
            Assert.assertEquals(mExpectedDownloadStatus, result.downloadStatus);
        }
    }

    private void waitForQueryCompletion(final DownloadQueryResultVerifier verifier) {
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return verifier.mQueryCompleted;
            }
        });
    }

    /**
     * Test to make sure {@link OMADownloadHandler#getSize} returns the right size for OMAInfo.
     */
    @Test
    @SmallTest
    @Feature({"Download"})
    public void testGetSize() {
        OMADownloadHandler.OMAInfo info = new OMADownloadHandler.OMAInfo();
        Assert.assertEquals(OMADownloadHandler.getSize(info), 0);

        info.addAttributeValue("size", "100");
        Assert.assertEquals(OMADownloadHandler.getSize(info), 100);

        info.addAttributeValue("size", "100,000");
        Assert.assertEquals(OMADownloadHandler.getSize(info), 100000);

        info.addAttributeValue("size", "100000");
        Assert.assertEquals(OMADownloadHandler.getSize(info), 100000);
    }

    /**
     * Test to make sure {@link OMADownloadHandler.OMAInfo#getDrmType} returns the right DRM type.
     */
    @Test
    @SmallTest
    @Feature({"Download"})
    public void testGetDrmType() {
        OMADownloadHandler.OMAInfo info = new OMADownloadHandler.OMAInfo();
        Assert.assertEquals(info.getDrmType(), null);

        info.addAttributeValue("type", "text/html");
        Assert.assertEquals(info.getDrmType(), null);

        info.addAttributeValue("type", OMADownloadHandler.OMA_DRM_MESSAGE_MIME);
        Assert.assertEquals(info.getDrmType(), OMADownloadHandler.OMA_DRM_MESSAGE_MIME);

        // Test that only the first DRM MIME type is returned.
        info.addAttributeValue("type", OMADownloadHandler.OMA_DRM_CONTENT_MIME);
        Assert.assertEquals(info.getDrmType(), OMADownloadHandler.OMA_DRM_MESSAGE_MIME);
    }

    /**
     * Test to make sure {@link OMADownloadHandler#getOpennableType} returns the right MIME type.
     */
    @Test
    @SmallTest
    @Feature({"Download"})
    public void testGetOpennableType() {
        PackageManager pm = InstrumentationRegistry.getContext().getPackageManager();
        OMADownloadHandler.OMAInfo info = new OMADownloadHandler.OMAInfo();
        Assert.assertEquals(OMADownloadHandler.getOpennableType(pm, info), null);

        info.addAttributeValue(OMADownloadHandler.OMA_TYPE, "application/octet-stream");
        info.addAttributeValue(OMADownloadHandler.OMA_TYPE,
                OMADownloadHandler.OMA_DRM_MESSAGE_MIME);
        info.addAttributeValue(OMADownloadHandler.OMA_TYPE, "text/html");
        Assert.assertEquals(OMADownloadHandler.getOpennableType(pm, info), null);

        info.addAttributeValue(OMADownloadHandler.OMA_OBJECT_URI, "http://www.test.com/test.html");
        Assert.assertEquals(OMADownloadHandler.getOpennableType(pm, info), "text/html");

        // Test that only the first opennable type is returned.
        info.addAttributeValue(OMADownloadHandler.OMA_TYPE, "image/png");
        Assert.assertEquals(OMADownloadHandler.getOpennableType(pm, info), "text/html");
    }

    /**
     * Test to make sure {@link OMADownloadHandler#parseDownloadDescriptor} returns the
     * correct OMAInfo if the input is valid.
     */
    @Test
    @SmallTest
    @Feature({"Download"})
    public void testParseValidDownloadDescriptor() {
        String downloadDescriptor =
                "<media xmlns=\"http://www.openmobilealliance.org/xmlns/dd\">\r\n"
                + "<DDVersion>1.0</DDVersion>\r\n"
                + "<name>test.dm</name>\r\n"
                + "<size>1,000</size>\r\n"
                + "<type>image/jpeg</type>\r\n"
                + "<garbage>this is just garbage</garbage>\r\n"
                + "<type>application/vnd.oma.drm.message</type>\r\n"
                + "<vendor>testvendor</vendor>\r\n"
                + "<description>testjpg</description>\r\n"
                + "<objectURI>http://test/test.dm</objectURI>\r\n"
                + "<nextURL>http://nexturl.html</nextURL>\r\n"
                + "</media>";
        OMADownloadHandler.OMAInfo info = OMADownloadHandler.parseDownloadDescriptor(
                new ByteArrayInputStream(ApiCompatibilityUtils.getBytesUtf8(downloadDescriptor)));
        Assert.assertFalse(info.isEmpty());
        Assert.assertEquals(
                info.getValue(OMADownloadHandler.OMA_OBJECT_URI), "http://test/test.dm");
        Assert.assertEquals(info.getValue(OMADownloadHandler.OMA_DD_VERSION), "1.0");
        Assert.assertEquals(info.getValue(OMADownloadHandler.OMA_NAME), "test.dm");
        Assert.assertEquals(info.getValue(OMADownloadHandler.OMA_SIZE), "1,000");
        Assert.assertEquals(info.getValue(OMADownloadHandler.OMA_VENDOR), "testvendor");
        Assert.assertEquals(info.getValue(OMADownloadHandler.OMA_DESCRIPTION), "testjpg");
        Assert.assertEquals(info.getValue(OMADownloadHandler.OMA_NEXT_URL), "http://nexturl.html");
        List<String> types = info.getTypes();
        Assert.assertThat(types,
                Matchers.containsInAnyOrder("image/jpeg", OMADownloadHandler.OMA_DRM_MESSAGE_MIME));
    }

    /**
     * Test that {@link OMADownloadHandler#parseDownloadDescriptor} returns empty result on invalid
     * input.
     */
    @Test
    @SmallTest
    @Feature({"Download"})
    public void testParseInvalidDownloadDescriptor() {
        String downloadDescriptor =
                "<media xmlns=\"http://www.openmobilealliance.org/xmlns/dd\">\r\n"
                + "</media>";
        OMADownloadHandler.OMAInfo info = OMADownloadHandler.parseDownloadDescriptor(
                new ByteArrayInputStream(ApiCompatibilityUtils.getBytesUtf8(downloadDescriptor)));
        Assert.assertTrue(info.isEmpty());

        downloadDescriptor =
                "<media xmlns=\"http://www.openmobilealliance.org/xmlns/dd\">\r\n"
                + "<DDVersion>1.0</DDVersion>\r\n"
                + "<name>"
                + "<size>1,000</size>\r\n"
                + "test.dm"
                + "</name>\r\n"
                + "</media>";
        info = OMADownloadHandler.parseDownloadDescriptor(
                new ByteArrayInputStream(ApiCompatibilityUtils.getBytesUtf8(downloadDescriptor)));
        Assert.assertNull(info);

        downloadDescriptor =
                "garbage"
                + "<media xmlns=\"http://www.openmobilealliance.org/xmlns/dd\">\r\n"
                + "<DDVersion>1.0</DDVersion>\r\n"
                + "</media>";
        info = OMADownloadHandler.parseDownloadDescriptor(
                new ByteArrayInputStream(ApiCompatibilityUtils.getBytesUtf8(downloadDescriptor)));
        Assert.assertNull(info);
    }

    /**
     * Test to make sure {@link DownloadManagerDelegate#queryDownloadResult} will report correctly
     * about the status of completed downloads and removed downloads.
     */
    @Test
    @MediumTest
    @Feature({"Download"})
    public void testQueryDownloadResult() {
        Context context = getTestContext();
        DownloadManager manager =
                (DownloadManager) getTestContext().getSystemService(Context.DOWNLOAD_SERVICE);
        long downloadId1 = manager.addCompletedDownload("test", "test", false, "text/html",
                UrlUtils.getIsolatedTestFilePath("chrome/test/data/android/download/download.txt"),
                4, true);

        DownloadItem downloadItem = new DownloadItem(true, new DownloadInfo.Builder().build());
        downloadItem.setSystemDownloadId(downloadId1);

        DownloadManagerDelegate downloadManagerDelegate = new DownloadManagerDelegate(context);
        DownloadQueryResultVerifier verifier =
                new DownloadQueryResultVerifier(DownloadManagerService.DOWNLOAD_STATUS_COMPLETE);
        downloadManagerDelegate.queryDownloadResult(downloadItem, false, verifier);
        waitForQueryCompletion(verifier);

        manager.remove(downloadId1);
        downloadItem.setSystemDownloadId(downloadId1);
        verifier =
                new DownloadQueryResultVerifier(DownloadManagerService.DOWNLOAD_STATUS_CANCELLED);
        downloadManagerDelegate.queryDownloadResult(downloadItem, false, verifier);
        waitForQueryCompletion(verifier);
    }

    /**
     * Test to make sure {@link OMADownloadHandler#clearPendingOMADownloads} will clear the OMA
     * notifications and pass the notification URI to {@link OMADownloadHandler}.
     */
    @Test
    @MediumTest
    @RetryOnFailure
    @Feature({"Download"})
    public void testClearPendingOMADownloads() {
        Context context = getTestContext();
        DownloadManager manager =
                (DownloadManager) getTestContext().getSystemService(Context.DOWNLOAD_SERVICE);
        long downloadId1 = manager.addCompletedDownload("test", "test", false, "text/html",
                UrlUtils.getIsolatedTestFilePath("chrome/test/data/android/download/download.txt"),
                4, true);

        DownloadManagerDelegate downloadManagerDelegate = new DownloadManagerDelegate(context);
        final MockDownloadSnackbarController snackbarController =
                new MockDownloadSnackbarController();
        final OMADownloadHandlerForTest omaHandler =
                new OMADownloadHandlerForTest(context, downloadManagerDelegate, snackbarController);

        // Write a few pending downloads into shared preferences.
        Set<String> pendingOmaDownloads = new HashSet<>();
        pendingOmaDownloads.add(String.valueOf(downloadId1) + "," + INSTALL_NOTIFY_URI);
        DownloadManagerService.storeDownloadInfo(ContextUtils.getAppSharedPreferences(),
                PENDING_OMA_DOWNLOADS, pendingOmaDownloads, false /* forceCommit */);

        pendingOmaDownloads = DownloadManagerService.getStoredDownloadInfo(
                ContextUtils.getAppSharedPreferences(), PENDING_OMA_DOWNLOADS);
        Assert.assertEquals(1, pendingOmaDownloads.size());

        omaHandler.clearPendingOMADownloads();

        // Wait for OMADownloadHandler to clear the pending downloads.
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return snackbarController.mSucceeded;
            }
        });

        // The pending downloads set in the shared prefs should be empty now.
        pendingOmaDownloads = DownloadManagerService.getStoredDownloadInfo(
                ContextUtils.getAppSharedPreferences(), PENDING_OMA_DOWNLOADS);
        Assert.assertEquals(0, pendingOmaDownloads.size());
        Assert.assertEquals(omaHandler.mNofityURI, INSTALL_NOTIFY_URI);

        manager.remove(downloadId1);
    }

    /**
     * Test that calling {@link OMADownloadHandler#enqueueDownloadManagerRequest} for an
     * OMA download will enqueue a new DownloadManager request and insert an entry into the
     * SharedPrefs.
     */
    @Test
    @MediumTest
    @Feature({"Download"})
    public void testEnqueueOMADownloads() throws InterruptedException {
        EmbeddedTestServer testServer =
                EmbeddedTestServer.createAndStartServer(InstrumentationRegistry.getContext());
        Context context = getTestContext();

        OMADownloadHandler.OMAInfo omaInfo = new OMAInfo();
        omaInfo.addAttributeValue(OMADownloadHandler.OMA_NAME, "test.gzip");
        omaInfo.addAttributeValue(OMADownloadHandler.OMA_OBJECT_URI,
                testServer.getURL("/chrome/test/data/android/download/test.gzip"));
        omaInfo.addAttributeValue(OMADownloadHandler.OMA_INSTALL_NOTIFY_URI, INSTALL_NOTIFY_URI);

        try {
            DownloadInfo info = new DownloadInfo.Builder().build();
            final DownloadManagerDelegate downloadManagerDelegate =
                    new DownloadManagerDelegate(context);
            final MockDownloadSnackbarController snackbarController =
                    new MockDownloadSnackbarController();
            final OMADownloadHandlerForTest omaHandler = new OMADownloadHandlerForTest(
                    context, downloadManagerDelegate, snackbarController) {
                @Override
                public void onReceive(Context context, Intent intent) {
                    // Ignore all the broadcasts.
                }
            };

            omaHandler.clearPendingOMADownloads();
            omaHandler.downloadOMAContent(0, info, omaInfo);
            CriteriaHelper.pollUiThread(new Criteria() {
                @Override
                public boolean isSatisfied() {
                    return omaHandler.mDownloadId != 0;
                }
            });

            Set<String> downloads = DownloadManagerService.getStoredDownloadInfo(
                    ContextUtils.getAppSharedPreferences(), PENDING_OMA_DOWNLOADS);
            Assert.assertEquals(1, downloads.size());
            OMADownloadHandler.OMAEntry entry =
                    OMADownloadHandler.OMAEntry.parseOMAEntry((String) (downloads.toArray()[0]));
            Assert.assertEquals(entry.mDownloadId, omaHandler.mDownloadId);
            Assert.assertEquals(entry.mInstallNotifyURI, INSTALL_NOTIFY_URI);
            DownloadManager manager =
                    (DownloadManager) getTestContext().getSystemService(Context.DOWNLOAD_SERVICE);
            manager.remove(omaHandler.mDownloadId);
        } finally {
            testServer.stopAndDestroyServer();
        }
    }
}
