// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.test;

import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.android_webview.AwContents;
import org.chromium.base.test.util.Feature;
import org.chromium.net.test.util.TestWebServer;

import java.io.File;

/**
 * Test suite for the HTTP cache.
 */
@RunWith(AwJUnit4ClassRunner.class)
public class HttpCacheTest {
    @Rule
    public AwActivityTestRule mActivityTestRule = new AwActivityTestRule() {
        @Override
        public boolean needsBrowserProcessStarted() {
            return false;
        }
    };

    @Test
    @SmallTest
    @Feature({"AndroidWebView"})
    public void testHttpCacheIsInsideCacheDir() throws Exception {
        File webViewCacheDir = new File(InstrumentationRegistry.getInstrumentation()
                                                .getTargetContext()
                                                .getCacheDir()
                                                .getPath(),
                "org.chromium.android_webview");
        deleteDirectory(webViewCacheDir);

        mActivityTestRule.startBrowserProcess();
        final TestAwContentsClient contentClient = new TestAwContentsClient();
        final AwTestContainerView testContainerView =
                mActivityTestRule.createAwTestContainerViewOnMainSync(contentClient);
        final AwContents awContents = testContainerView.getAwContents();

        TestWebServer httpServer = null;
        try {
            httpServer = TestWebServer.start();
            final String pageUrl = "/page.html";
            final String pageHtml = "<body>Hello, World!</body>";
            final String fullPageUrl = httpServer.setResponse(pageUrl, pageHtml, null);
            mActivityTestRule.loadUrlSync(
                    awContents, contentClient.getOnPageFinishedHelper(), fullPageUrl);
            Assert.assertEquals(1, httpServer.getRequestCount(pageUrl));
        } finally {
            if (httpServer != null) {
                httpServer.shutdown();
            }
        }

        Assert.assertTrue(webViewCacheDir.isDirectory());
        Assert.assertTrue(webViewCacheDir.list().length > 0);
    }

    private void deleteDirectory(File dir) throws Exception {
        if (!dir.exists()) return;
        Assert.assertTrue(dir.isDirectory());
        Process rmrf = Runtime.getRuntime().exec("rm -rf " + dir.getAbsolutePath());
        rmrf.waitFor();
        Assert.assertFalse(dir.exists());
    }
}
