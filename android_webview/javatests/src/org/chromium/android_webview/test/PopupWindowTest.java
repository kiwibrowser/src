// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.test;

import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;
import android.webkit.JavascriptInterface;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.android_webview.AwContents;
import org.chromium.android_webview.test.AwActivityTestRule.PopupInfo;
import org.chromium.android_webview.test.util.CommonResources;
import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content.browser.test.util.DOMUtils;
import org.chromium.content.browser.test.util.TestCallbackHelperContainer;
import org.chromium.content_public.browser.SelectionPopupController;
import org.chromium.net.test.util.TestWebServer;

/**
 * Tests for pop up window flow.
 */
@RunWith(AwJUnit4ClassRunner.class)
public class PopupWindowTest {
    @Rule
    public AwActivityTestRule mActivityTestRule = new AwActivityTestRule();

    private TestAwContentsClient mParentContentsClient;
    private AwTestContainerView mParentContainerView;
    private AwContents mParentContents;
    private TestWebServer mWebServer;

    private static final String POPUP_TITLE = "Popup Window";

    @Before
    public void setUp() throws Exception {
        mParentContentsClient = new TestAwContentsClient();
        mParentContainerView =
                mActivityTestRule.createAwTestContainerViewOnMainSync(mParentContentsClient);
        mParentContents = mParentContainerView.getAwContents();
        mWebServer = TestWebServer.start();
    }

    @After
    public void tearDown() throws Exception {
        if (mWebServer != null) {
            mWebServer.shutdown();
        }
    }

    @Test
    @SmallTest
    @Feature({"AndroidWebView"})
    public void testPopupWindow() throws Throwable {
        final String popupPath = "/popup.html";
        final String parentPageHtml = CommonResources.makeHtmlPageFrom("", "<script>"
                        + "function tryOpenWindow() {"
                        + "  var newWindow = window.open('" + popupPath + "');"
                        + "}</script>");

        final String popupPageHtml = CommonResources.makeHtmlPageFrom(
                "<title>" + POPUP_TITLE + "</title>",
                "This is a popup window");

        mActivityTestRule.triggerPopup(mParentContents, mParentContentsClient, mWebServer,
                parentPageHtml, popupPageHtml, popupPath, "tryOpenWindow()");
        AwContents popupContents =
                mActivityTestRule.connectPendingPopup(mParentContents).popupContents;
        Assert.assertEquals(POPUP_TITLE, mActivityTestRule.getTitleOnUiThread(popupContents));
    }

    @Test
    @SmallTest
    @Feature({"AndroidWebView"})
    public void testJavascriptInterfaceForPopupWindow() throws Throwable {
        // android.webkit.cts.WebViewTest#testJavascriptInterfaceForClientPopup
        final String popupPath = "/popup.html";
        final String parentPageHtml = CommonResources.makeHtmlPageFrom("",
                "<script>"
                        + "function tryOpenWindow() {"
                        + "  var newWindow = window.open('" + popupPath + "');"
                        + "}</script>");

        final String popupPageHtml = CommonResources.makeHtmlPageFrom(
                "<title>" + POPUP_TITLE + "</title>", "This is a popup window");

        mActivityTestRule.triggerPopup(mParentContents, mParentContentsClient, mWebServer,
                parentPageHtml, popupPageHtml, popupPath, "tryOpenWindow()");
        PopupInfo popupInfo = mActivityTestRule.createPopupContents(mParentContents);
        TestAwContentsClient popupContentsClient = popupInfo.popupContentsClient;
        final AwContents popupContents = popupInfo.popupContents;

        class DummyJavaScriptInterface {
            @JavascriptInterface
            public int test() {
                return 42;
            }
        }
        final DummyJavaScriptInterface obj = new DummyJavaScriptInterface();

        InstrumentationRegistry.getInstrumentation().runOnMainSync(
                () -> popupContents.addJavascriptInterface(obj, "dummy"));

        mActivityTestRule.loadPopupContents(mParentContents, popupInfo, null);

        AwActivityTestRule.pollInstrumentationThread(() -> {
            String ans = mActivityTestRule.executeJavaScriptAndWaitForResult(
                    popupContents, popupContentsClient, "dummy.test()");

            return ans.equals("42");
        });
    }

    @Test
    @SmallTest
    @Feature({"AndroidWebView"})
    public void testDefaultUserAgentsInParentAndChildWindows() throws Throwable {
        mActivityTestRule.getAwSettingsOnUiThread(mParentContents).setJavaScriptEnabled(true);
        mActivityTestRule.loadUrlSync(
                mParentContents, mParentContentsClient.getOnPageFinishedHelper(), "about:blank");
        String parentUserAgent = mActivityTestRule.executeJavaScriptAndWaitForResult(
                mParentContents, mParentContentsClient, "navigator.userAgent");

        final String popupPath = "/popup.html";
        final String myUserAgentString = "myUserAgent";
        final String parentPageHtml = CommonResources.makeHtmlPageFrom("",
                "<script>"
                        + "function tryOpenWindow() {"
                        + "  var newWindow = window.open('" + popupPath + "');"
                        + "}</script>");

        final String popupPageHtml = "<html><head><script>"
                + "document.title = navigator.userAgent;"
                + "</script><body><div id='a'></div></body></html>";

        mActivityTestRule.triggerPopup(mParentContents, mParentContentsClient, mWebServer,
                parentPageHtml, popupPageHtml, popupPath, "tryOpenWindow()");

        PopupInfo popupInfo = mActivityTestRule.createPopupContents(mParentContents);
        TestAwContentsClient popupContentsClient = popupInfo.popupContentsClient;
        final AwContents popupContents = popupInfo.popupContents;

        mActivityTestRule.loadPopupContents(mParentContents, popupInfo, null);

        final String childUserAgent = mActivityTestRule.executeJavaScriptAndWaitForResult(
                popupContents, popupContentsClient, "navigator.userAgent");

        Assert.assertEquals(parentUserAgent, childUserAgent);
    }

    @Test
    @SmallTest
    @Feature({"AndroidWebView"})
    public void testOverrideUserAgentInOnCreateWindow() throws Throwable {
        final String popupPath = "/popup.html";
        final String myUserAgentString = "myUserAgent";
        final String parentPageHtml = CommonResources.makeHtmlPageFrom("",
                "<script>"
                        + "function tryOpenWindow() {"
                        + "  var newWindow = window.open('" + popupPath + "');"
                        + "}</script>");

        final String popupPageHtml = "<html><head><script>"
                + "document.title = navigator.userAgent;"
                + "</script><body><div id='a'></div></body></html>";

        mActivityTestRule.triggerPopup(mParentContents, mParentContentsClient, mWebServer,
                parentPageHtml, popupPageHtml, popupPath, "tryOpenWindow()");

        PopupInfo popupInfo = mActivityTestRule.createPopupContents(mParentContents);
        TestAwContentsClient popupContentsClient = popupInfo.popupContentsClient;
        final AwContents popupContents = popupInfo.popupContents;

        // Override the user agent string for the popup window.
        mActivityTestRule.loadPopupContents(
                mParentContents, popupInfo, new AwActivityTestRule.OnCreateWindowHandler() {
                    @Override
                    public boolean onCreateWindow(AwContents awContents) {
                        awContents.getSettings().setUserAgentString(myUserAgentString);
                        return true;
                    }
                });

        CriteriaHelper.pollUiThread(Criteria.equals(
                myUserAgentString, () -> mActivityTestRule.getTitleOnUiThread(popupContents)));
    }

    @Test
    @SmallTest
    @Feature({"AndroidWebView"})
    public void testOnPageFinishedCalledOnDomModificationAfterNavigation() throws Throwable {
        final String popupPath = "/popup.html";
        final String parentPageHtml = CommonResources.makeHtmlPageFrom("",
                "<script>"
                        + "function tryOpenWindow() {"
                        + "  window.popupWindow = window.open('" + popupPath + "');"
                        + "  window.popupWindow.console = {};"
                        + "}"
                        + "function modifyDomOfPopup() {"
                        + "  window.popupWindow.document.body.innerHTML = 'Hello from the parent!';"
                        + "}</script>");

        final String popupPageHtml = CommonResources.makeHtmlPageFrom(
                "<title>" + POPUP_TITLE + "</title>",
                "This is a popup window");

        mActivityTestRule.triggerPopup(mParentContents, mParentContentsClient, mWebServer,
                parentPageHtml, popupPageHtml, popupPath, "tryOpenWindow()");
        PopupInfo popupInfo = mActivityTestRule.connectPendingPopup(mParentContents);
        Assert.assertEquals(
                POPUP_TITLE, mActivityTestRule.getTitleOnUiThread(popupInfo.popupContents));

        TestCallbackHelperContainer.OnPageFinishedHelper onPageFinishedHelper =
                popupInfo.popupContentsClient.getOnPageFinishedHelper();
        final int onPageFinishedCallCount = onPageFinishedHelper.getCallCount();

        mActivityTestRule.executeJavaScriptAndWaitForResult(
                mParentContents, mParentContentsClient, "modifyDomOfPopup()");
        // Test that |waitForCallback| does not time out.
        onPageFinishedHelper.waitForCallback(onPageFinishedCallCount);
    }

    @Test
    @SmallTest
    @Feature({"AndroidWebView"})
    @RetryOnFailure
    public void testPopupWindowTextHandle() throws Throwable {
        final String popupPath = "/popup.html";
        final String parentPageHtml = CommonResources.makeHtmlPageFrom("", "<script>"
                        + "function tryOpenWindow() {"
                        + "  var newWindow = window.open('" + popupPath + "');"
                        + "}</script>");

        final String popupPageHtml = CommonResources.makeHtmlPageFrom(
                "<title>" + POPUP_TITLE + "</title>",
                "<span id=\"plain_text\" class=\"full_view\">This is a popup window.</span>");

        mActivityTestRule.triggerPopup(mParentContents, mParentContentsClient, mWebServer,
                parentPageHtml, popupPageHtml, popupPath, "tryOpenWindow()");
        PopupInfo popupInfo = mActivityTestRule.connectPendingPopup(mParentContents);
        final AwContents popupContents = popupInfo.popupContents;
        TestAwContentsClient popupContentsClient = popupInfo.popupContentsClient;
        Assert.assertEquals(POPUP_TITLE, mActivityTestRule.getTitleOnUiThread(popupContents));

        AwActivityTestRule.enableJavaScriptOnUiThread(popupContents);

        // Now long press on some texts and see if the text handles show up.
        DOMUtils.longPressNode(popupContents.getWebContents(), "plain_text");
        SelectionPopupController controller =
                SelectionPopupController.fromWebContents(popupContents.getWebContents());
        assertWaitForSelectActionBarStatus(true, controller);
        Assert.assertTrue(ThreadUtils.runOnUiThreadBlocking(() -> controller.hasSelection()));

        // Now hide the select action bar. This should hide the text handles and
        // clear the selection.
        hideSelectActionMode(controller);

        assertWaitForSelectActionBarStatus(false, controller);
        String jsGetSelection = "window.getSelection().toString()";
        // Test window.getSelection() returns empty string "" literally.
        Assert.assertEquals("\"\"",
                mActivityTestRule.executeJavaScriptAndWaitForResult(
                        popupContents, popupContentsClient, jsGetSelection));
    }

    // Copied from imeTest.java.
    private void assertWaitForSelectActionBarStatus(
            boolean show, final SelectionPopupController controller) {
        CriteriaHelper.pollUiThread(
                Criteria.equals(show, () -> controller.isSelectActionBarShowing()));
    }

    private void hideSelectActionMode(final SelectionPopupController controller) {
        InstrumentationRegistry.getInstrumentation().runOnMainSync(
                () -> controller.destroySelectActionMode());
    }
}
