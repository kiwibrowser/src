// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.metrics.RecordHistogram;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.download.ui.DownloadHistoryItemWrapper;
import org.chromium.chrome.browser.download.ui.DownloadHistoryItemWrapper.OfflineItemWrapper;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.offline_items_collection.OfflineItem;
import org.chromium.components.offline_items_collection.OfflineItem.Progress;
import org.chromium.components.offline_items_collection.OfflineItemProgressUnit;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Tests of {@link DownloadUtils}.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
public class DownloadUtilsTest {
    private static final int MILLIS_PER_SECOND = 1000;
    private static final String OFFLINE_ITEM_TITLE = "Some Web Page Title.mhtml";
    private static final String OFFLINE_ITEM_DESCRIPTION = "Our web page";
    private static final String FILE_PATH = "/fake_sd_card/Download/Some Web Page Title.mhtml";
    private static final String MULTIPART_RELATED = "multipart/related";
    private static final String ITEM_ID = "42";

    @Before
    public void setUp() throws Exception {
        RecordHistogram.setDisabledForTests(true);
    }

    @After
    public void tearDown() throws Exception {
        RecordHistogram.setDisabledForTests(false);
    }

    /**
     * Test {@link DownloadUtils#getAbbrieviatedFileName()} method.
     */
    @Test
    @SmallTest
    @Feature({"Download"})
    public void testGetAbbrieviatedFileName() {
        Assert.assertEquals("123.pdf", DownloadUtils.getAbbreviatedFileName("123.pdf", 10));
        Assert.assertEquals(
                "1" + DownloadUtils.ELLIPSIS, DownloadUtils.getAbbreviatedFileName("123.pdf", 1));
        Assert.assertEquals(
                "12" + DownloadUtils.ELLIPSIS, DownloadUtils.getAbbreviatedFileName("1234567", 2));
        Assert.assertEquals("123" + DownloadUtils.ELLIPSIS + ".pdf",
                DownloadUtils.getAbbreviatedFileName("1234567.pdf", 7));
    }

    @Test
    @SmallTest
    @Feature({"Download"})
    public void testFormatRemainingTime() {
        final Context context = InstrumentationRegistry.getTargetContext();
        Assert.assertEquals("0 secs left", DownloadUtils.formatRemainingTime(context, 0));
        Assert.assertEquals(
                "1 sec left", DownloadUtils.formatRemainingTime(context, MILLIS_PER_SECOND));
        Assert.assertEquals("1 min left",
                DownloadUtils.formatRemainingTime(
                        context, DownloadUtils.SECONDS_PER_MINUTE * MILLIS_PER_SECOND));
        Assert.assertEquals(
                "2 mins left", DownloadUtils.formatRemainingTime(context, 149 * MILLIS_PER_SECOND));
        Assert.assertEquals(
                "3 mins left", DownloadUtils.formatRemainingTime(context, 150 * MILLIS_PER_SECOND));
        Assert.assertEquals("1 hour left",
                DownloadUtils.formatRemainingTime(
                        context, DownloadUtils.SECONDS_PER_HOUR * MILLIS_PER_SECOND));
        Assert.assertEquals("2 hours left",
                DownloadUtils.formatRemainingTime(
                        context, 149 * DownloadUtils.SECONDS_PER_MINUTE * MILLIS_PER_SECOND));
        Assert.assertEquals("3 hours left",
                DownloadUtils.formatRemainingTime(
                        context, 150 * DownloadUtils.SECONDS_PER_MINUTE * MILLIS_PER_SECOND));
        Assert.assertEquals("1 day left",
                DownloadUtils.formatRemainingTime(
                        context, DownloadUtils.SECONDS_PER_DAY * MILLIS_PER_SECOND));
        Assert.assertEquals("2 days left",
                DownloadUtils.formatRemainingTime(
                        context, 59 * DownloadUtils.SECONDS_PER_HOUR * MILLIS_PER_SECOND));
        Assert.assertEquals("3 days left",
                DownloadUtils.formatRemainingTime(
                        context, 60 * DownloadUtils.SECONDS_PER_HOUR * MILLIS_PER_SECOND));
    }

    @Test
    @SmallTest
    @Feature({"Download"})
    public void testFormatBytesReceived() {
        final Context context = InstrumentationRegistry.getTargetContext();
        Assert.assertEquals("Downloaded 0.0 KB",
                DownloadUtils.getStringForBytes(
                        context, DownloadUtils.BYTES_DOWNLOADED_STRINGS, 0));
        Assert.assertEquals("Downloaded 0.5 KB",
                DownloadUtils.getStringForBytes(
                        context, DownloadUtils.BYTES_DOWNLOADED_STRINGS, 512));
        Assert.assertEquals("Downloaded 1.0 KB",
                DownloadUtils.getStringForBytes(
                        context, DownloadUtils.BYTES_DOWNLOADED_STRINGS, 1024));
        Assert.assertEquals("Downloaded 1.0 MB",
                DownloadUtils.getStringForBytes(
                        context, DownloadUtils.BYTES_DOWNLOADED_STRINGS, 1024 * 1024));
        Assert.assertEquals("Downloaded 1.0 GB",
                DownloadUtils.getStringForBytes(
                        context, DownloadUtils.BYTES_DOWNLOADED_STRINGS, 1024 * 1024 * 1024));
    }

    @Test
    @SmallTest
    @Feature({"Download"})
    public void testFormatRemainingFiles() {
        final Context context = InstrumentationRegistry.getTargetContext();
        Progress progress = new Progress(3, Long.valueOf(5), OfflineItemProgressUnit.FILES);
        Assert.assertEquals(60, progress.getPercentage());
        Assert.assertEquals("2 files left", DownloadUtils.formatRemainingFiles(context, progress));
        progress = new Progress(4, Long.valueOf(5), OfflineItemProgressUnit.FILES);
        Assert.assertEquals(80, progress.getPercentage());
        Assert.assertEquals("1 file left", DownloadUtils.formatRemainingFiles(context, progress));
        progress = new Progress(5, Long.valueOf(5), OfflineItemProgressUnit.FILES);
        Assert.assertEquals(100, progress.getPercentage());
        Assert.assertEquals("0 files left", DownloadUtils.formatRemainingFiles(context, progress));
    }

    @Test
    @SmallTest
    @Feature({"Download"})
    public void testCreateShareIntentHasTitle() {
        // Create an item list with a single item.
        List<DownloadHistoryItemWrapper> items = new ArrayList<DownloadHistoryItemWrapper>();
        OfflineItem offlineItem = new OfflineItem();
        offlineItem.title = OFFLINE_ITEM_TITLE;
        offlineItem.description = OFFLINE_ITEM_DESCRIPTION;
        offlineItem.filePath = FILE_PATH;
        offlineItem.mimeType = MULTIPART_RELATED;
        OfflineItemWrapper itemWrapper =
                OfflineItemWrapper.createOfflineItemWrapperForTest(offlineItem);
        items.add(itemWrapper);

        // Create a map matching the id to a filename.
        Map<String, String> offlineFilePathMap = new HashMap<String, String>();
        offlineFilePathMap.put(ITEM_ID, FILE_PATH);

        // Call the share function.
        Intent shareIntent = DownloadUtils.createShareIntent(items, offlineFilePathMap);

        // Check that the resulting share intent has extra subject set for the title.
        Bundle extras = shareIntent.getExtras();
        Assert.assertNotNull(extras);
        String subject = extras.getString(Intent.EXTRA_SUBJECT);
        Assert.assertEquals(OFFLINE_ITEM_TITLE, subject);
    }
}
