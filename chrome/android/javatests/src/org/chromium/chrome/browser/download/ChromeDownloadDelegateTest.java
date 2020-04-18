// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import android.content.Context;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

import java.util.concurrent.Callable;

/**
 * Tests for ChromeDownloadDelegate class.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class ChromeDownloadDelegateTest {
    @Rule
    public ChromeActivityTestRule<ChromeActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeActivity.class);

    @Before
    public void setUp() throws InterruptedException {
        mActivityTestRule.startMainActivityOnBlankPage();
    }

    /**
     * Mock class for test.
     */
    static class MockChromeDownloadDelegate extends ChromeDownloadDelegate {
        public MockChromeDownloadDelegate(Context context, Tab tab) {
            super(context, tab);
        }

        @Override
        protected void onDownloadStartNoStream(DownloadInfo downloadInfo) {
        }
    }

    /**
     * Test to make sure {@link ChromeDownloadDelegate#shouldInterceptContextMenuDownload}
     * returns true only for ".dd" or ".dm" extensions with http/https scheme.
     */
    @Test
    @SmallTest
    @Feature({"Download"})
    @RetryOnFailure
    public void testShouldInterceptContextMenuDownload() throws InterruptedException {
        final Tab tab = mActivityTestRule.getActivity().getActivityTab();
        mActivityTestRule.loadUrl("about:blank");
        ChromeDownloadDelegate delegate = ThreadUtils.runOnUiThreadBlockingNoException(
                (Callable<ChromeDownloadDelegate>) ()
                        -> new MockChromeDownloadDelegate(
                                InstrumentationRegistry.getTargetContext(), tab));
        Assert.assertFalse(delegate.shouldInterceptContextMenuDownload("file://test/test.html"));
        Assert.assertFalse(delegate.shouldInterceptContextMenuDownload("http://test/test.html"));
        Assert.assertFalse(delegate.shouldInterceptContextMenuDownload("ftp://test/test.dm"));
        Assert.assertFalse(delegate.shouldInterceptContextMenuDownload("data://test.dd"));
        Assert.assertFalse(delegate.shouldInterceptContextMenuDownload("http://test.dd"));
        Assert.assertFalse(delegate.shouldInterceptContextMenuDownload("http://test/test.dd"));
        Assert.assertTrue(delegate.shouldInterceptContextMenuDownload("https://test/test.dm"));
    }

    @Test
    @SmallTest
    @Feature({"Download"})
    public void testGetFileExtension() {
        Assert.assertEquals("ext", ChromeDownloadDelegate.getFileExtension("", "file.ext"));
        Assert.assertEquals("ext", ChromeDownloadDelegate.getFileExtension("http://file.ext", ""));
        Assert.assertEquals(
                "txt", ChromeDownloadDelegate.getFileExtension("http://file.ext", "file.txt"));
        Assert.assertEquals(
                "txt", ChromeDownloadDelegate.getFileExtension("http://file.ext", "file name.txt"));
    }
}
