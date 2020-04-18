// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.test;

import static org.chromium.android_webview.test.AwActivityTestRule.WAIT_TIMEOUT_MS;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.LargeTest;
import android.support.test.filters.SmallTest;
import android.util.Pair;
import android.view.KeyEvent;
import android.view.View;
import android.webkit.JavascriptInterface;

import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.android_webview.AwContents;
import org.chromium.android_webview.AwSettings;
import org.chromium.android_webview.AwSwitches;
import org.chromium.android_webview.renderer_priority.RendererPriority;
import org.chromium.android_webview.test.TestAwContentsClient.OnDownloadStartHelper;
import org.chromium.android_webview.test.util.CommonResources;
import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.parameter.SkipCommandLineParameterization;
import org.chromium.content_public.common.ContentSwitches;
import org.chromium.content_public.common.ContentUrlConstants;
import org.chromium.net.test.EmbeddedTestServer;
import org.chromium.net.test.util.TestWebServer;

import java.io.InputStream;
import java.net.URL;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * AwContents tests.
 */
@RunWith(AwJUnit4ClassRunner.class)
public class AwContentsTest {
    @Rule
    public AwActivityTestRule mActivityTestRule = new AwActivityTestRule();

    private TestAwContentsClient mContentsClient = new TestAwContentsClient();

    @Test
    @SmallTest
    @Feature({"AndroidWebView"})
    public void testCreateDestroy() throws Throwable {
        // NOTE this test runs on UI thread, so we cannot call any async methods.
        mActivityTestRule.runOnUiThread(() -> mActivityTestRule.createAwTestContainerView(
                mContentsClient)
                .getAwContents()
                .destroy());
    }

    @Test
    @SmallTest
    @Feature({"AndroidWebView"})
    public void testCreateLoadPageDestroy() throws Throwable {
        AwTestContainerView awTestContainerView =
                mActivityTestRule.createAwTestContainerViewOnMainSync(mContentsClient);
        mActivityTestRule.loadDataSync(awTestContainerView.getAwContents(),
                mContentsClient.getOnPageFinishedHelper(), CommonResources.ABOUT_HTML, "text/html",
                false);

        mActivityTestRule.destroyAwContentsOnMainSync(awTestContainerView.getAwContents());
        // It should be safe to call destroy multiple times.
        mActivityTestRule.destroyAwContentsOnMainSync(awTestContainerView.getAwContents());
    }

    @Test
    @LargeTest
    @Feature({"AndroidWebView"})
    public void testCreateLoadDestroyManyTimes() throws Throwable {
        for (int i = 0; i < 10; ++i) {
            AwTestContainerView testView =
                    mActivityTestRule.createAwTestContainerViewOnMainSync(mContentsClient);
            AwContents awContents = testView.getAwContents();

            mActivityTestRule.loadUrlSync(awContents, mContentsClient.getOnPageFinishedHelper(),
                    ContentUrlConstants.ABOUT_BLANK_DISPLAY_URL);
            mActivityTestRule.destroyAwContentsOnMainSync(awContents);
        }
    }

    @Test
    @LargeTest
    @Feature({"AndroidWebView"})
    public void testCreateLoadDestroyManyAtOnce() throws Throwable {
        AwTestContainerView views[] = new AwTestContainerView[10];

        for (int i = 0; i < views.length; ++i) {
            views[i] = mActivityTestRule.createAwTestContainerViewOnMainSync(mContentsClient);
            mActivityTestRule.loadUrlSync(views[i].getAwContents(),
                    mContentsClient.getOnPageFinishedHelper(),
                    ContentUrlConstants.ABOUT_BLANK_DISPLAY_URL);
        }

        for (int i = 0; i < views.length; ++i) {
            mActivityTestRule.destroyAwContentsOnMainSync(views[i].getAwContents());
            views[i] = null;
        }
    }

    @Test
    @SmallTest
    @Feature({"AndroidWebView"})
    public void testWebViewApisFailGracefullyAfterDestruction() throws Throwable {
        mActivityTestRule.runOnUiThread(() -> {
            AwContents awContents = mActivityTestRule.createAwTestContainerView(mContentsClient)
                    .getAwContents();
            awContents.destroy();

            // The documentation for WebView#destroy() reads "This method should be called
            // after this WebView has been removed from the view system. No other methods
            // may be called on this WebView after destroy".
            // However, some apps do not respect that restriction so we need to ensure that
            // we fail gracefully and do not crash when APIs are invoked after destruction.
            // Due to the large number of APIs we only test a representative selection here.
            awContents.clearHistory();
            Assert.assertNull(awContents.getOriginalUrl());
            Assert.assertNull(awContents.getNavigationHistory());
            awContents.loadUrl("http://www.google.com");
            awContents.findAllAsync("search");
            Assert.assertNull(awContents.getUrl());
            Assert.assertFalse(awContents.canGoBack());
            awContents.disableJavascriptInterfacesInspection();
            awContents.invokeZoomPicker();
            awContents.onResume();
            awContents.stopLoading();
            awContents.onWindowVisibilityChanged(View.VISIBLE);
            awContents.requestFocus();
            awContents.isMultiTouchZoomSupported();
            awContents.setOverScrollMode(View.OVER_SCROLL_NEVER);
            awContents.pauseTimers();
            awContents.onContainerViewScrollChanged(200, 200, 100, 100);
            awContents.computeScroll();
            awContents.onMeasure(100, 100);
            awContents.onDraw(new Canvas());
            awContents.getMostRecentProgress();
            Assert.assertEquals(0, awContents.computeHorizontalScrollOffset());
            Assert.assertEquals(0, awContents.getContentWidthCss());
            awContents.onKeyUp(KeyEvent.KEYCODE_BACK,
                    new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_MENU));
        });
    }

    @Test
    @SmallTest
    @Feature({"AndroidWebView"})
    public void testUseAwSettingsAfterDestroy() throws Throwable {
        AwTestContainerView awTestContainerView =
                mActivityTestRule.createAwTestContainerViewOnMainSync(mContentsClient);
        AwSettings awSettings =
                mActivityTestRule.getAwSettingsOnUiThread(awTestContainerView.getAwContents());
        mActivityTestRule.loadDataSync(awTestContainerView.getAwContents(),
                mContentsClient.getOnPageFinishedHelper(), CommonResources.ABOUT_HTML, "text/html",
                false);
        mActivityTestRule.destroyAwContentsOnMainSync(awTestContainerView.getAwContents());

        // AwSettings should still be usable even after native side is destroyed.
        String newFontFamily = "serif";
        awSettings.setStandardFontFamily(newFontFamily);
        Assert.assertEquals(newFontFamily, awSettings.getStandardFontFamily());
        boolean newBlockNetworkLoads = !awSettings.getBlockNetworkLoads();
        awSettings.setBlockNetworkLoads(newBlockNetworkLoads);
        Assert.assertEquals(newBlockNetworkLoads, awSettings.getBlockNetworkLoads());
    }

    private int callDocumentHasImagesSync(final AwContents awContents)
            throws Throwable, InterruptedException {
        // Set up a container to hold the result object and a semaphore to
        // make the test wait for the result.
        final AtomicInteger val = new AtomicInteger();
        final Semaphore s = new Semaphore(0);
        final Message msg = Message.obtain(new Handler(Looper.getMainLooper()) {
            @Override
            public void handleMessage(Message msg) {
                val.set(msg.arg1);
                s.release();
            }
        });
        InstrumentationRegistry.getInstrumentation().runOnMainSync(
                () -> awContents.documentHasImages(msg));
        Assert.assertTrue(s.tryAcquire(AwActivityTestRule.WAIT_TIMEOUT_MS, TimeUnit.MILLISECONDS));
        int result = val.get();
        return result;
    }

    @Test
    @SmallTest
    @Feature({"AndroidWebView"})
    public void testDocumentHasImages() throws Throwable {
        AwTestContainerView testView =
                mActivityTestRule.createAwTestContainerViewOnMainSync(mContentsClient);
        AwContents awContents = testView.getAwContents();

        final CallbackHelper loadHelper = mContentsClient.getOnPageFinishedHelper();

        final String mime = "text/html";
        final String emptyDoc = "<head/><body/>";
        final String imageDoc = "<head/><body><img/><img/></body>";

        // Make sure a document that does not have images returns 0
        mActivityTestRule.loadDataSync(awContents, loadHelper, emptyDoc, mime, false);
        int result = callDocumentHasImagesSync(awContents);
        Assert.assertEquals(0, result);

        // Make sure a document that does have images returns 1
        mActivityTestRule.loadDataSync(awContents, loadHelper, imageDoc, mime, false);
        result = callDocumentHasImagesSync(awContents);
        Assert.assertEquals(1, result);
    }

    @Test
    @SmallTest
    @Feature({"AndroidWebView"})
    public void testClearCacheMemoryAndDisk() throws Throwable {
        final AwTestContainerView testContainer =
                mActivityTestRule.createAwTestContainerViewOnMainSync(mContentsClient);
        final AwContents awContents = testContainer.getAwContents();

        TestWebServer webServer = TestWebServer.start();
        try {
            final String pagePath = "/clear_cache_test.html";
            List<Pair<String, String>> headers = new ArrayList<Pair<String, String>>();
            // Set Cache-Control headers to cache this request. One century should be long enough.
            headers.add(Pair.create("Cache-Control", "max-age=3153600000"));
            headers.add(Pair.create("Last-Modified", "Wed, 3 Oct 2012 00:00:00 GMT"));
            final String pageUrl = webServer.setResponse(
                    pagePath, "<html><body>foo</body></html>", headers);

            // First load to populate cache.
            mActivityTestRule.clearCacheOnUiThread(awContents, true);
            mActivityTestRule.loadUrlSync(
                    awContents, mContentsClient.getOnPageFinishedHelper(), pageUrl);
            Assert.assertEquals(1, webServer.getRequestCount(pagePath));

            // Load about:blank so next load is not treated as reload by webkit and force
            // revalidate with the server.
            mActivityTestRule.loadUrlSync(awContents, mContentsClient.getOnPageFinishedHelper(),
                    ContentUrlConstants.ABOUT_BLANK_DISPLAY_URL);

            // No clearCache call, so should be loaded from cache.
            mActivityTestRule.loadUrlSync(
                    awContents, mContentsClient.getOnPageFinishedHelper(), pageUrl);
            Assert.assertEquals(1, webServer.getRequestCount(pagePath));

            // Same as above.
            mActivityTestRule.loadUrlSync(awContents, mContentsClient.getOnPageFinishedHelper(),
                    ContentUrlConstants.ABOUT_BLANK_DISPLAY_URL);

            // Clear cache, so should hit server again.
            mActivityTestRule.clearCacheOnUiThread(awContents, true);
            mActivityTestRule.loadUrlSync(
                    awContents, mContentsClient.getOnPageFinishedHelper(), pageUrl);
            Assert.assertEquals(2, webServer.getRequestCount(pagePath));
        } finally {
            webServer.shutdown();
        }
    }

    @Test
    @SmallTest
    @Feature({"AndroidWebView"})
    public void testClearCacheInQuickSuccession() throws Throwable {
        final AwTestContainerView testContainer =
                mActivityTestRule.createAwTestContainerViewOnMainSync(new TestAwContentsClient());
        final AwContents awContents = testContainer.getAwContents();

        InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
            for (int i = 0; i < 10; ++i) {
                awContents.clearCache(true);
            }
        });
    }

    @Test
    @SmallTest
    @Feature({"AndroidWebView"})
    public void testGetFavicon() throws Throwable {
        AwContents.setShouldDownloadFavicons();
        final AwTestContainerView testView =
                mActivityTestRule.createAwTestContainerViewOnMainSync(mContentsClient);
        final AwContents awContents = testView.getAwContents();

        TestWebServer webServer = TestWebServer.start();
        try {
            final String faviconUrl = webServer.setResponseBase64(
                    "/" + CommonResources.FAVICON_FILENAME, CommonResources.FAVICON_DATA_BASE64,
                    CommonResources.getImagePngHeaders(false));
            final String pageUrl = webServer.setResponse("/favicon.html",
                    CommonResources.FAVICON_STATIC_HTML, null);

            // The getFavicon will return the right icon a certain time after
            // the page load completes which makes it slightly hard to test.
            final Bitmap defaultFavicon = awContents.getFavicon();

            mActivityTestRule.getAwSettingsOnUiThread(awContents).setImagesEnabled(true);
            mActivityTestRule.loadUrlSync(
                    awContents, mContentsClient.getOnPageFinishedHelper(), pageUrl);

            mActivityTestRule.pollUiThread(() -> awContents.getFavicon() != null
                    && !awContents.getFavicon().sameAs(defaultFavicon));

            final Object originalFaviconSource = (new URL(faviconUrl)).getContent();
            final Bitmap originalFavicon =
                    BitmapFactory.decodeStream((InputStream) originalFaviconSource);
            Assert.assertNotNull(originalFavicon);

            Assert.assertTrue(awContents.getFavicon().sameAs(originalFavicon));

        } finally {
            webServer.shutdown();
        }
    }

    @Test
    @Feature({"AndroidWebView", "Downloads"})
    @SmallTest
    public void testDownload() throws Throwable {
        AwTestContainerView testView =
                mActivityTestRule.createAwTestContainerViewOnMainSync(mContentsClient);
        AwContents awContents = testView.getAwContents();

        final String data = "download data";
        final String contentDisposition = "attachment;filename=\"download.txt\"";
        final String mimeType = "text/plain";

        List<Pair<String, String>> downloadHeaders = new ArrayList<Pair<String, String>>();
        downloadHeaders.add(Pair.create("Content-Disposition", contentDisposition));
        downloadHeaders.add(Pair.create("Content-Type", mimeType));
        downloadHeaders.add(Pair.create("Content-Length", Integer.toString(data.length())));

        TestWebServer webServer = TestWebServer.start();
        try {
            final String pageUrl = webServer.setResponse(
                    "/download.txt", data, downloadHeaders);
            final OnDownloadStartHelper downloadStartHelper =
                    mContentsClient.getOnDownloadStartHelper();
            final int callCount = downloadStartHelper.getCallCount();
            mActivityTestRule.loadUrlAsync(awContents, pageUrl);
            downloadStartHelper.waitForCallback(callCount);

            Assert.assertEquals(pageUrl, downloadStartHelper.getUrl());
            Assert.assertEquals(contentDisposition, downloadStartHelper.getContentDisposition());
            Assert.assertEquals(mimeType, downloadStartHelper.getMimeType());
            Assert.assertEquals(data.length(), downloadStartHelper.getContentLength());
        } finally {
            webServer.shutdown();
        }
    }

    @Test
    @Feature({"AndroidWebView", "setNetworkAvailable"})
    @SmallTest
    public void testSetNetworkAvailable() throws Throwable {
        AwTestContainerView testView =
                mActivityTestRule.createAwTestContainerViewOnMainSync(mContentsClient);
        AwContents awContents = testView.getAwContents();
        String script = "navigator.onLine";

        AwActivityTestRule.enableJavaScriptOnUiThread(awContents);
        mActivityTestRule.loadUrlSync(awContents, mContentsClient.getOnPageFinishedHelper(),
                ContentUrlConstants.ABOUT_BLANK_DISPLAY_URL);

        // Default to "online".
        Assert.assertEquals("true",
                mActivityTestRule.executeJavaScriptAndWaitForResult(
                        awContents, mContentsClient, script));

        // Forcing "offline".
        AwActivityTestRule.setNetworkAvailableOnUiThread(awContents, false);
        Assert.assertEquals("false",
                mActivityTestRule.executeJavaScriptAndWaitForResult(
                        awContents, mContentsClient, script));

        // Forcing "online".
        AwActivityTestRule.setNetworkAvailableOnUiThread(awContents, true);
        Assert.assertEquals("true",
                mActivityTestRule.executeJavaScriptAndWaitForResult(
                        awContents, mContentsClient, script));
    }


    static class JavaScriptObject {
        private CallbackHelper mCallbackHelper;
        public JavaScriptObject(CallbackHelper callbackHelper) {
            mCallbackHelper = callbackHelper;
        }

        @JavascriptInterface
        public void run() {
            mCallbackHelper.notifyCalled();
        }
    }

    @Test
    @Feature({"AndroidWebView", "Android-JavaBridge"})
    @SmallTest
    public void testJavaBridge() throws Throwable {
        final AwTestContainerView testView =
                mActivityTestRule.createAwTestContainerViewOnMainSync(mContentsClient);
        final CallbackHelper callback = new CallbackHelper();

        InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
            AwContents awContents = testView.getAwContents();
            AwSettings awSettings = awContents.getSettings();
            awSettings.setJavaScriptEnabled(true);
            awContents.addJavascriptInterface(new JavaScriptObject(callback), "bridge");
            awContents.evaluateJavaScriptForTests("window.bridge.run();", null);
        });
        callback.waitForCallback(0, 1, WAIT_TIMEOUT_MS, TimeUnit.MILLISECONDS);
    }

    @Test
    @Feature({"AndroidWebView"})
    @SmallTest
    public void testEscapingOfErrorPage() throws Throwable {
        AwTestContainerView testView =
                mActivityTestRule.createAwTestContainerViewOnMainSync(mContentsClient);
        AwContents awContents = testView.getAwContents();
        String script = "window.failed == true";

        AwActivityTestRule.enableJavaScriptOnUiThread(awContents);
        CallbackHelper onPageFinishedHelper = mContentsClient.getOnPageFinishedHelper();
        int currentCallCount = onPageFinishedHelper.getCallCount();
        mActivityTestRule.loadUrlAsync(awContents,
                "file:///file-that-does-not-exist#<script>window.failed = true;</script>");
        onPageFinishedHelper.waitForCallback(
                currentCallCount, 1, WAIT_TIMEOUT_MS, TimeUnit.MILLISECONDS);

        Assert.assertEquals("false",
                mActivityTestRule.executeJavaScriptAndWaitForResult(
                        awContents, mContentsClient, script));
    }

    @Test
    @Feature({"AndroidWebView"})
    @SmallTest
    public void testCanInjectHeaders() throws Throwable {
        final AwTestContainerView testContainer =
                mActivityTestRule.createAwTestContainerViewOnMainSync(mContentsClient);
        final AwContents awContents = testContainer.getAwContents();

        AwActivityTestRule.enableJavaScriptOnUiThread(awContents);

        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(
                InstrumentationRegistry.getInstrumentation().getContext());

        try {
            String url = testServer.getURL("/echoheader?X-foo");

            final Map<String, String> extraHeaders = new HashMap<String, String>();
            extraHeaders.put("X-foo", "bar");
            mActivityTestRule.loadUrlSync(
                    awContents, mContentsClient.getOnPageFinishedHelper(), url, extraHeaders);

            String xfoo = mActivityTestRule.getJavaScriptResultBodyTextContent(
                    awContents, mContentsClient);
            Assert.assertEquals("bar", xfoo);

            url = testServer.getURL("/echoheader?Referer");

            extraHeaders.clear();
            extraHeaders.put("Referer", "http://www.example.com/");
            mActivityTestRule.loadUrlSync(
                    awContents, mContentsClient.getOnPageFinishedHelper(), url, extraHeaders);

            String referer = mActivityTestRule.getJavaScriptResultBodyTextContent(
                    awContents, mContentsClient);
            Assert.assertEquals("http://www.example.com/", referer);
        } finally {
            testServer.stopAndDestroyServer();
        }
    }

    // This is a meta test that we don't accidentally turn off hardware
    // acceleration in instrumentation tests without notice. Do not add the
    // @DisableHardwareAccelerationForTest annotation for this test.
    @Test
    @Feature({"AndroidWebView"})
    @SmallTest
    public void testHardwareModeWorks() throws Throwable {
        AwTestContainerView testContainer =
                mActivityTestRule.createAwTestContainerViewOnMainSync(mContentsClient);
        Assert.assertTrue(testContainer.isHardwareAccelerated());
        Assert.assertTrue(testContainer.isBackedByHardwareView());
    }

    // TODO(hush): more ssl tests. And put the ssl tests into a separate test
    // class.
    @Test
    @Feature({"AndroidWebView"})
    @SmallTest
    // If the user allows the ssl error, the same ssl error will not trigger
    // the onReceivedSslError callback; If the user denies it, the same ssl
    // error will still trigger the onReceivedSslError callback.
    public void testSslPreferences() throws Throwable {
        final AwTestContainerView testContainer =
                mActivityTestRule.createAwTestContainerViewOnMainSync(mContentsClient);
        final AwContents awContents = testContainer.getAwContents();
        TestWebServer webServer = TestWebServer.startSsl();
        final String pagePath = "/hello.html";
        final String pageUrl =
                webServer.setResponse(pagePath, "<html><body>hello world</body></html>", null);
        final CallbackHelper onReceivedSslErrorHelper =
                mContentsClient.getOnReceivedSslErrorHelper();
        int onSslErrorCallCount = onReceivedSslErrorHelper.getCallCount();

        mActivityTestRule.loadUrlSync(
                awContents, mContentsClient.getOnPageFinishedHelper(), pageUrl);

        Assert.assertEquals(onSslErrorCallCount + 1, onReceivedSslErrorHelper.getCallCount());
        Assert.assertEquals(1, webServer.getRequestCount(pagePath));

        // Now load the page again. This time, we expect no ssl error, because
        // user's decision should be remembered.
        onSslErrorCallCount = onReceivedSslErrorHelper.getCallCount();
        mActivityTestRule.loadUrlSync(
                awContents, mContentsClient.getOnPageFinishedHelper(), pageUrl);
        Assert.assertEquals(onSslErrorCallCount, onReceivedSslErrorHelper.getCallCount());

        // Now clear the ssl preferences then load the same url again. Expect to see
        // onReceivedSslError getting called again.
        awContents.clearSslPreferences();
        onSslErrorCallCount = onReceivedSslErrorHelper.getCallCount();
        mActivityTestRule.loadUrlSync(
                awContents, mContentsClient.getOnPageFinishedHelper(), pageUrl);
        Assert.assertEquals(onSslErrorCallCount + 1, onReceivedSslErrorHelper.getCallCount());

        // Now clear the stored decisions and tell the client to deny ssl errors.
        awContents.clearSslPreferences();
        mContentsClient.setAllowSslError(false);
        onSslErrorCallCount = onReceivedSslErrorHelper.getCallCount();
        mActivityTestRule.loadUrlSync(
                awContents, mContentsClient.getOnPageFinishedHelper(), pageUrl);
        Assert.assertEquals(onSslErrorCallCount + 1, onReceivedSslErrorHelper.getCallCount());

        // Now load the same page again. This time, we still expect onReceivedSslError,
        // because we only remember user's decision if it is "allow".
        onSslErrorCallCount = onReceivedSslErrorHelper.getCallCount();
        mActivityTestRule.loadUrlSync(
                awContents, mContentsClient.getOnPageFinishedHelper(), pageUrl);
        Assert.assertEquals(onSslErrorCallCount + 1, onReceivedSslErrorHelper.getCallCount());
    }

    /**
     * Verifies that Web Notifications and the Push API are not exposed in WebView.
     */
    @Test
    @Feature({"AndroidWebView"})
    @SmallTest
    public void testPushAndNotificationsDisabled() throws Throwable {
        AwTestContainerView testView =
                mActivityTestRule.createAwTestContainerViewOnMainSync(mContentsClient);
        AwContents awContents = testView.getAwContents();

        String script = "window.Notification || window.PushManager";

        AwActivityTestRule.enableJavaScriptOnUiThread(awContents);
        mActivityTestRule.loadUrlSync(awContents, mContentsClient.getOnPageFinishedHelper(),
                ContentUrlConstants.ABOUT_BLANK_DISPLAY_URL);
        Assert.assertEquals("null",
                mActivityTestRule.executeJavaScriptAndWaitForResult(
                        awContents, mContentsClient, script));
    }

    private @RendererPriority int getRendererPriorityOnUiThread(final AwContents awContents)
            throws Exception {
        return ThreadUtils.runOnUiThreadBlocking(() -> awContents.getEffectivePriorityForTesting());
    }

    private void setRendererPriorityOnUiThread(final AwContents awContents,
            final @RendererPriority int priority, final boolean waivedWhenNotVisible)
            throws Throwable {
        mActivityTestRule.runOnUiThread(
                () -> awContents.setRendererPriorityPolicy(priority, waivedWhenNotVisible));
    }

    @Test
    @Feature({"AndroidWebView"})
    @SmallTest
    @SkipCommandLineParameterization
    @CommandLineFlags.Add({
            AwSwitches.WEBVIEW_SANDBOXED_RENDERER, ContentSwitches.RENDER_PROCESS_LIMIT + "=1"})
    public void testForegroundPriorityOneProcess() throws Throwable {
        final AwTestContainerView view1 =
                mActivityTestRule.createAwTestContainerViewOnMainSync(mContentsClient);
        final AwContents contents1 = view1.getAwContents();
        final AwTestContainerView view2 =
                mActivityTestRule.createAwTestContainerViewOnMainSync(mContentsClient);
        final AwContents contents2 = view2.getAwContents();

        mActivityTestRule.loadUrlSync(contents1, mContentsClient.getOnPageFinishedHelper(),
                ContentUrlConstants.ABOUT_BLANK_DISPLAY_URL);
        mActivityTestRule.loadUrlSync(contents2, mContentsClient.getOnPageFinishedHelper(),
                ContentUrlConstants.ABOUT_BLANK_DISPLAY_URL);

        // Process should start out high.
        Assert.assertEquals(RendererPriority.HIGH, getRendererPriorityOnUiThread(contents1));
        Assert.assertEquals(RendererPriority.HIGH, getRendererPriorityOnUiThread(contents2));

        // Set one to low. Process should take max priority of contents, so still high.
        setRendererPriorityOnUiThread(contents1, RendererPriority.LOW, false);
        Assert.assertEquals(RendererPriority.HIGH, getRendererPriorityOnUiThread(contents1));
        Assert.assertEquals(RendererPriority.HIGH, getRendererPriorityOnUiThread(contents2));

        // Set both to low and check.
        setRendererPriorityOnUiThread(contents2, RendererPriority.LOW, false);
        Assert.assertEquals(RendererPriority.LOW, getRendererPriorityOnUiThread(contents1));
        Assert.assertEquals(RendererPriority.LOW, getRendererPriorityOnUiThread(contents2));

        // Set both to waive and check.
        setRendererPriorityOnUiThread(contents1, RendererPriority.WAIVED, false);
        setRendererPriorityOnUiThread(contents2, RendererPriority.WAIVED, false);
        Assert.assertEquals(RendererPriority.WAIVED, getRendererPriorityOnUiThread(contents1));
        Assert.assertEquals(RendererPriority.WAIVED, getRendererPriorityOnUiThread(contents2));

        // Set one to high and check.
        setRendererPriorityOnUiThread(contents1, RendererPriority.HIGH, false);
        Assert.assertEquals(RendererPriority.HIGH, getRendererPriorityOnUiThread(contents1));
        Assert.assertEquals(RendererPriority.HIGH, getRendererPriorityOnUiThread(contents2));

        // Destroy contents with high priority, and process should fall back to low.
        // Destroy posts on UI, but getRendererPriorityOnUiThread posts after, so there should
        // be no flakiness and no need for polling.
        mActivityTestRule.destroyAwContentsOnMainSync(contents1);
        Assert.assertEquals(RendererPriority.WAIVED, getRendererPriorityOnUiThread(contents2));
    }

    @Test
    @Feature({"AndroidWebView"})
    @SmallTest
    @SkipCommandLineParameterization
    @CommandLineFlags.Add({
            AwSwitches.WEBVIEW_SANDBOXED_RENDERER, ContentSwitches.RENDER_PROCESS_LIMIT + "=2"})
    public void testForegroundPriorityTwoProcesses() throws Throwable {
        final AwTestContainerView view1 =
                mActivityTestRule.createAwTestContainerViewOnMainSync(mContentsClient);
        final AwContents contents1 = view1.getAwContents();
        final AwTestContainerView view2 =
                mActivityTestRule.createAwTestContainerViewOnMainSync(mContentsClient);
        final AwContents contents2 = view2.getAwContents();

        mActivityTestRule.loadUrlSync(contents1, mContentsClient.getOnPageFinishedHelper(),
                ContentUrlConstants.ABOUT_BLANK_DISPLAY_URL);
        mActivityTestRule.loadUrlSync(contents2, mContentsClient.getOnPageFinishedHelper(),
                ContentUrlConstants.ABOUT_BLANK_DISPLAY_URL);

        // Process should start out high.
        Assert.assertEquals(RendererPriority.HIGH, getRendererPriorityOnUiThread(contents1));
        Assert.assertEquals(RendererPriority.HIGH, getRendererPriorityOnUiThread(contents2));

        // Set one to low. Other should not be affected.
        setRendererPriorityOnUiThread(contents1, RendererPriority.LOW, false);
        Assert.assertEquals(RendererPriority.LOW, getRendererPriorityOnUiThread(contents1));
        Assert.assertEquals(RendererPriority.HIGH, getRendererPriorityOnUiThread(contents2));
    }

    @Test
    @Feature({"AndroidWebView"})
    @SmallTest
    @SkipCommandLineParameterization
    @CommandLineFlags.Add(AwSwitches.WEBVIEW_SANDBOXED_RENDERER)
    public void testBackgroundPriority() throws Throwable {
        final AwContents awContents =
                mActivityTestRule.createAwTestContainerViewOnMainSync(mContentsClient)
                        .getAwContents();
        Assert.assertEquals(RendererPriority.HIGH, getRendererPriorityOnUiThread(awContents));

        mActivityTestRule.runOnUiThread(() -> awContents.onPause());
        Assert.assertEquals(RendererPriority.HIGH, getRendererPriorityOnUiThread(awContents));

        setRendererPriorityOnUiThread(
                awContents, RendererPriority.HIGH, true /* waivedWhenNotVisible */);
        Assert.assertEquals(RendererPriority.WAIVED, getRendererPriorityOnUiThread(awContents));

        mActivityTestRule.runOnUiThread(() -> awContents.onResume());
        Assert.assertEquals(RendererPriority.HIGH, getRendererPriorityOnUiThread(awContents));
    }

    @Test
    @Feature({"AndroidWebView"})
    @SmallTest
    @CommandLineFlags.Add(AwSwitches.WEBVIEW_SANDBOXED_RENDERER)
    @SkipCommandLineParameterization
    public void testPauseDestroyResume() throws Throwable {
        mActivityTestRule.runOnUiThread(() -> {
            AwContents awContents;
            awContents = mActivityTestRule.createAwTestContainerView(mContentsClient)
                    .getAwContents();
            awContents.pauseTimers();
            awContents.pauseTimers();
            awContents.destroy();
            awContents = mActivityTestRule.createAwTestContainerView(mContentsClient)
                    .getAwContents();
            awContents.resumeTimers();
        });
    }

    /** Regression test for https://crbug.com/732976. Load a data URL, then immediately
     * after that load a javascript URL. The data URL navigation shouldn't be blocked.
     */
    @Test
    @LargeTest
    @Feature({"AndroidWebView"})
    public void testJavaScriptUrlAfterLoadData() throws Throwable {
        AwTestContainerView testView =
                mActivityTestRule.createAwTestContainerViewOnMainSync(mContentsClient);
        final AwContents awContents = testView.getAwContents();
        mActivityTestRule.runOnUiThread(() -> {
            // Run javascript navigation immediately, without waiting for the completion of data
            // URL.
            awContents.loadData("<html>test</html>", "text/html", "utf-8");
            awContents.loadUrl("javascript: void(0)");
        });

        mContentsClient.getOnPageFinishedHelper().waitForCallback(
                0, 1, WAIT_TIMEOUT_MS, TimeUnit.MILLISECONDS);
        Assert.assertEquals("data:text/html,<html>test</html>", awContents.getLastCommittedUrl());

        TestAwContentsClient.AddMessageToConsoleHelper consoleHelper =
                mContentsClient.getAddMessageToConsoleHelper();
        Assert.assertEquals(0, consoleHelper.getMessages().size());
    }
}
