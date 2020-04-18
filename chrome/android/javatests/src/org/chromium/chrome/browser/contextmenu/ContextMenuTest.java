// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextmenu;

import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.LargeTest;
import android.support.test.filters.MediumTest;
import android.support.test.filters.SmallTest;
import android.view.ContextMenu;
import android.view.KeyEvent;

import org.hamcrest.Matchers;
import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.FlakyTest;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.compositor.layouts.LayoutManager;
import org.chromium.chrome.browser.download.DownloadTestRule;
import org.chromium.chrome.browser.download.DownloadTestRule.CustomMainActivityStart;
import org.chromium.chrome.browser.firstrun.FirstRunStatus;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.EmptyTabModelSelectorObserver;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.util.browser.contextmenu.ContextMenuUtils;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content.browser.test.util.DOMUtils;
import org.chromium.content.browser.test.util.TestTouchUtils;
import org.chromium.net.test.EmbeddedTestServer;
import org.chromium.policy.test.annotations.Policies;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.TimeoutException;
import java.util.concurrent.atomic.AtomicReference;

/**
 * Context menu related tests
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE,
        ChromeSwitches.GOOGLE_BASE_URL + "=http://example.com/"})
public class ContextMenuTest implements CustomMainActivityStart {
    @Rule
    public DownloadTestRule mDownloadTestRule = new DownloadTestRule(this);

    private static final String TEST_PATH =
            "/chrome/test/data/android/contextmenu/context_menu_test.html";

    private EmbeddedTestServer mTestServer;
    private String mTestUrl;

    private static final String FILENAME_GIF = "download.gif";
    private static final String FILENAME_PNG = "test_image.png";
    private static final String FILENAME_WEBM = "test.webm";

    private static final String[] TEST_FILES = new String[] {
        FILENAME_GIF, FILENAME_PNG, FILENAME_WEBM
    };

    @Before
    public void setUp() throws Exception {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                FirstRunStatus.setFirstRunFlowComplete(true);
            }
        });
    }

    @After
    public void tearDown() throws Exception {
        mTestServer.stopAndDestroyServer();
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                FirstRunStatus.setFirstRunFlowComplete(false);
            }
        });
        deleteTestFiles();
    }

    @Override
    public void customMainActivityStart() throws InterruptedException {
        mTestServer = EmbeddedTestServer.createAndStartServer(InstrumentationRegistry.getContext());
        mTestUrl = mTestServer.getURL(TEST_PATH);
        deleteTestFiles();
        mDownloadTestRule.startMainActivityWithURL(mTestUrl);
        mDownloadTestRule.assertWaitForPageScaleFactorMatch(0.5f);
    }

    @Test
    @MediumTest
    @Feature({"Browser", "Main"})
    @RetryOnFailure
    public void testCopyLinkURL() throws Throwable {
        Tab tab = mDownloadTestRule.getActivity().getActivityTab();
        ContextMenuUtils.selectContextMenuItem(InstrumentationRegistry.getInstrumentation(),
                mDownloadTestRule.getActivity(), tab, "testLink",
                R.id.contextmenu_copy_link_address);

        assertStringContains("test_link.html", getClipboardText());
    }

    @Test
    @MediumTest
    @Feature({"Browser"})
    @RetryOnFailure
    public void testCopyImageLinkCopiesLinkURL() throws Throwable {
        Tab tab = mDownloadTestRule.getActivity().getActivityTab();
        ContextMenuUtils.selectContextMenuItem(InstrumentationRegistry.getInstrumentation(),
                mDownloadTestRule.getActivity(), tab, "testImageLink",
                R.id.contextmenu_copy_link_address);

        assertStringContains("test_link.html", getClipboardText());
    }

    @Test
    @MediumTest
    @Feature({"Browser"})
    @RetryOnFailure
    public void testLongPressOnImage() throws InterruptedException, TimeoutException {
        checkOpenImageInNewTab(
                "testImage", "/chrome/test/data/android/contextmenu/test_image.png");
    }

    /**
     * @MediumTest
     * @Feature({"Browser"})
     * @CommandLineFlags.Add(ChromeSwitches.DISABLE_DOCUMENT_MODE)
    */
    @Test
    @FlakyTest(message = "http://crbug.com/606939")
    public void testLongPressOnImageLink() throws InterruptedException, TimeoutException {
        checkOpenImageInNewTab(
                "testImageLink", "/chrome/test/data/android/contextmenu/test_image.png");
    }

    private void checkOpenImageInNewTab(String domId, final String expectedPath)
            throws InterruptedException, TimeoutException {
        final Tab activityTab = mDownloadTestRule.getActivity().getActivityTab();

        final CallbackHelper newTabCallback = new CallbackHelper();
        final AtomicReference<Tab> newTab = new AtomicReference<>();
        mDownloadTestRule.getActivity().getTabModelSelector().addObserver(
                new EmptyTabModelSelectorObserver() {
                    @Override
                    public void onNewTabCreated(Tab tab) {
                        super.onNewTabCreated(tab);

                        if (tab.getParentId() != activityTab.getId()) return;
                        newTab.set(tab);
                        newTabCallback.notifyCalled();

                        mDownloadTestRule.getActivity().getTabModelSelector().removeObserver(this);
                    }
                });

        int callbackCount = newTabCallback.getCallCount();

        ContextMenuUtils.selectContextMenuItem(InstrumentationRegistry.getInstrumentation(),
                mDownloadTestRule.getActivity(), activityTab, domId,
                R.id.contextmenu_open_image_in_new_tab);

        try {
            newTabCallback.waitForCallback(callbackCount);
        } catch (TimeoutException ex) {
            Assert.fail("New tab never created from context menu press");
        }

        // Only check for the URL matching as the tab will not be fully created in svelte mode.
        final String expectedUrl = mTestServer.getURL(expectedPath);
        CriteriaHelper.pollUiThread(Criteria.equals(expectedUrl, new Callable<String>() {
            @Override
            public String call() {
                return newTab.get().getUrl();
            }
        }));
    }

    @Test
    @MediumTest
    @Feature({"Browser"})
    @RetryOnFailure
    public void testDismissContextMenuOnBack() throws InterruptedException, TimeoutException {
        Tab tab = mDownloadTestRule.getActivity().getActivityTab();
        ContextMenu menu = ContextMenuUtils.openContextMenu(tab, "testImage");
        Assert.assertNotNull("Context menu was not properly created", menu);
        CriteriaHelper.pollUiThread(new Criteria("Context menu did not have window focus") {
            @Override
            public boolean isSatisfied() {
                return !mDownloadTestRule.getActivity().hasWindowFocus();
            }
        });

        InstrumentationRegistry.getInstrumentation().sendKeyDownUpSync(KeyEvent.KEYCODE_BACK);
        CriteriaHelper.pollUiThread(new Criteria("Activity did not regain focus.") {
            @Override
            public boolean isSatisfied() {
                return mDownloadTestRule.getActivity().hasWindowFocus();
            }
        });
    }

    @Test
    @MediumTest
    @Feature({"Browser"})
    @RetryOnFailure
    public void testDismissContextMenuOnClick() throws InterruptedException, TimeoutException {
        Tab tab = mDownloadTestRule.getActivity().getActivityTab();
        ContextMenu menu = ContextMenuUtils.openContextMenu(tab, "testImage");
        Assert.assertNotNull("Context menu was not properly created", menu);
        CriteriaHelper.pollUiThread(new Criteria("Context menu did not have window focus") {
            @Override
            public boolean isSatisfied() {
                return !mDownloadTestRule.getActivity().hasWindowFocus();
            }
        });

        TestTouchUtils.singleClickView(
                InstrumentationRegistry.getInstrumentation(), tab.getView(), 0, 0);

        CriteriaHelper.pollUiThread(new Criteria("Activity did not regain focus.") {
            @Override
            public boolean isSatisfied() {
                return mDownloadTestRule.getActivity().hasWindowFocus();
            }
        });
    }

    @Test
    @MediumTest
    @Feature({"Browser"})
    @RetryOnFailure
    public void testCopyEmailAddress() throws Throwable {
        Tab tab = mDownloadTestRule.getActivity().getActivityTab();
        ContextMenuUtils.selectContextMenuItem(InstrumentationRegistry.getInstrumentation(),
                mDownloadTestRule.getActivity(), tab, "testEmail", R.id.contextmenu_copy);

        Assert.assertEquals("Copied email address is not correct",
                "someone1@example.com,someone2@example.com", getClipboardText());
    }

    @Test
    @MediumTest
    @Feature({"Browser"})
    @RetryOnFailure
    public void testCopyTelNumber() throws Throwable {
        Tab tab = mDownloadTestRule.getActivity().getActivityTab();
        ContextMenuUtils.selectContextMenuItem(InstrumentationRegistry.getInstrumentation(),
                mDownloadTestRule.getActivity(), tab, "testTel", R.id.contextmenu_copy);

        Assert.assertEquals("Copied tel number is not correct", "10000000000", getClipboardText());
    }

    @Test
    @LargeTest
    @Feature({"Browser"})
    @RetryOnFailure
    public void testSaveDataUrl()
            throws InterruptedException, TimeoutException, SecurityException, IOException {
        saveMediaFromContextMenu("dataUrlIcon", R.id.contextmenu_save_image, FILENAME_GIF);
    }

    @Test
    @LargeTest
    @Feature({"Browser"})
    @RetryOnFailure
    public void testSaveImage()
            throws InterruptedException, TimeoutException, SecurityException, IOException {
        saveMediaFromContextMenu("testImage", R.id.contextmenu_save_image, FILENAME_PNG);
    }

    @Test
    @LargeTest
    @Feature({"Browser"})
    @RetryOnFailure
    public void testSaveVideo()
            throws InterruptedException, TimeoutException, SecurityException, IOException {
        // Click the video to enable playback
        DOMUtils.clickNode(
                mDownloadTestRule.getActivity().getCurrentWebContents(), "videoDOMElement");
        saveMediaFromContextMenu("videoDOMElement", R.id.contextmenu_save_video, FILENAME_WEBM);
    }

    /**
     * Opens a link and image in new tabs and verifies the order of the tabs. Also verifies that
     * the parent page remains in front after opening links in new tabs.
     *
     * This test only applies in tabbed mode. In document mode, Android handles the ordering of the
     * tabs.
     */
    @Test
    @LargeTest
    @Feature({"Browser"})
    @RetryOnFailure
    public void testOpenLinksInNewTabsAndVerifyTabIndexOrdering()
            throws InterruptedException, TimeoutException {
        TabModel tabModel = mDownloadTestRule.getActivity().getCurrentTabModel();
        int numOpenedTabs = tabModel.getCount();
        Tab tab = mDownloadTestRule.getActivity().getActivityTab();
        ContextMenuUtils.selectContextMenuItem(InstrumentationRegistry.getInstrumentation(),
                mDownloadTestRule.getActivity(), tab, "testLink", R.id.contextmenu_open_in_new_tab);
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
        int indexOfLinkPage = numOpenedTabs;
        numOpenedTabs += 1;
        Assert.assertEquals(
                "Number of open tabs does not match", numOpenedTabs, tabModel.getCount());

        // Wait for any new tab animation to finish if we're being driven by the compositor.
        final LayoutManager layoutDriver =
                mDownloadTestRule.getActivity().getCompositorViewHolder().getLayoutManager();
        CriteriaHelper.pollUiThread(
                new Criteria("Background tab animation not finished.") {
                    @Override
                    public boolean isSatisfied() {
                        return layoutDriver.getActiveLayout().shouldDisplayContentOverlay();
                    }
                });

        ContextMenuUtils.selectContextMenuItem(InstrumentationRegistry.getInstrumentation(),
                mDownloadTestRule.getActivity(), tab, "testLink2",
                R.id.contextmenu_open_in_new_tab);
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
        int indexOfLinkPage2 = numOpenedTabs;
        numOpenedTabs += 1;
        Assert.assertEquals(
                "Number of open tabs does not match", numOpenedTabs, tabModel.getCount());

        // Verify the Url is still the same of Parent page.
        Assert.assertEquals(mTestUrl, mDownloadTestRule.getActivity().getActivityTab().getUrl());

        // Verify that the background tabs were opened in the expected order.
        String newTabUrl = mTestServer.getURL(
                "/chrome/test/data/android/contextmenu/test_link.html");
        Assert.assertEquals(newTabUrl, tabModel.getTabAt(indexOfLinkPage).getUrl());

        String imageUrl = mTestServer.getURL(
                "/chrome/test/data/android/contextmenu/test_link2.html");
        Assert.assertEquals(imageUrl, tabModel.getTabAt(indexOfLinkPage2).getUrl());
    }

    @Test
    @SmallTest
    @Feature({"Browser", "ContextMenu"})
    @RetryOnFailure
    public void testContextMenuRetrievesLinkOptions()
            throws TimeoutException, InterruptedException {
        Tab tab = mDownloadTestRule.getActivity().getActivityTab();
        ContextMenu menu = ContextMenuUtils.openContextMenu(tab, "testLink");

        Integer[] expectedItems = {R.id.contextmenu_open_in_new_tab,
                R.id.contextmenu_open_in_incognito_tab, R.id.contextmenu_save_link_as,
                R.id.contextmenu_copy_link_text, R.id.contextmenu_copy_link_address,
                R.id.contextmenu_share_link};
        assertMenuItemsAreEqual(menu, expectedItems);
    }

    @Test
    @SmallTest
    @Feature({"Browser", "ContextMenu"})
    @RetryOnFailure
    public void testContextMenuRetrievesImageOptions()
            throws TimeoutException, InterruptedException {
        Tab tab = mDownloadTestRule.getActivity().getActivityTab();
        ContextMenu menu = ContextMenuUtils.openContextMenu(tab, "testImage");

        Integer[] expectedItems = {R.id.contextmenu_save_image,
                R.id.contextmenu_open_image_in_new_tab, R.id.contextmenu_search_by_image,
                R.id.contextmenu_share_image};
        assertMenuItemsAreEqual(menu, expectedItems);
    }

    @Test
    @SmallTest
    @Feature({"Browser", "ContextMenu"})
    @Policies.Add({ @Policies.Item(key = "DefaultSearchProviderEnabled", string = "false") })
    @RetryOnFailure
    public void testContextMenuRetrievesImageOptions_NoDefaultSearchEngine()
            throws TimeoutException, InterruptedException {
        Tab tab = mDownloadTestRule.getActivity().getActivityTab();
        ContextMenu menu = ContextMenuUtils.openContextMenu(tab, "testImage");

        Integer[] expectedItems = {R.id.contextmenu_save_image,
                R.id.contextmenu_open_image_in_new_tab, R.id.contextmenu_share_image};
        assertMenuItemsAreEqual(menu, expectedItems);
    }

    @Test
    @SmallTest
    @Feature({"Browser", "ContextMenu"})
    @RetryOnFailure
    public void testContextMenuRetrievesImageLinkOptions()
            throws TimeoutException, InterruptedException {
        Tab tab = mDownloadTestRule.getActivity().getActivityTab();
        ContextMenu menu = ContextMenuUtils.openContextMenu(tab, "testImageLink");

        Integer[] expectedItems = {R.id.contextmenu_open_in_new_tab,
                R.id.contextmenu_open_in_incognito_tab, R.id.contextmenu_copy_link_address,
                R.id.contextmenu_save_link_as, R.id.contextmenu_save_image,
                R.id.contextmenu_open_image_in_new_tab, R.id.contextmenu_search_by_image,
                R.id.contextmenu_share_image, R.id.contextmenu_share_link};
        assertMenuItemsAreEqual(menu, expectedItems);
    }

    @Test
    @SmallTest
    @Feature({"Browser", "ContextMenu"})
    @RetryOnFailure
    public void testContextMenuRetrievesVideoOptions()
            throws TimeoutException, InterruptedException {
        Tab tab = mDownloadTestRule.getActivity().getActivityTab();
        DOMUtils.clickNode(
                mDownloadTestRule.getActivity().getCurrentWebContents(), "videoDOMElement");
        ContextMenu menu = ContextMenuUtils.openContextMenu(tab, "videoDOMElement");

        Integer[] expectedItems = {R.id.contextmenu_save_video};
        assertMenuItemsAreEqual(menu, expectedItems);
    }

    /**
     * Takes all the visible items on the menu and compares them to a the list of expected items.
     * @param menu A context menu that is displaying visible items.
     * @param expectedItems A list of items that is expected to appear within a context menu. The
     *                      list does not need to be ordered.
     */
    private void assertMenuItemsAreEqual(ContextMenu menu, Integer... expectedItems) {
        List<Integer> actualItems = new ArrayList<>();
        for (int i = 0; i < menu.size(); i++) {
            if (menu.getItem(i).isVisible()) {
                actualItems.add(menu.getItem(i).getItemId());
            }
        }

        Assert.assertThat(actualItems, Matchers.containsInAnyOrder(expectedItems));
    }

    private void saveMediaFromContextMenu(String mediaDOMElement, int saveMenuID,
            String expectedFilename) throws InterruptedException, TimeoutException,
            SecurityException, IOException {
        // Select "save [image/video]" in that menu.
        Tab tab = mDownloadTestRule.getActivity().getActivityTab();
        int callCount = mDownloadTestRule.getChromeDownloadCallCount();
        ContextMenuUtils.selectContextMenuItem(InstrumentationRegistry.getInstrumentation(),
                mDownloadTestRule.getActivity(), tab, mediaDOMElement, saveMenuID);

        // Wait for the download to complete and see if we got the right file
        Assert.assertTrue(mDownloadTestRule.waitForChromeDownloadToFinish(callCount));
        mDownloadTestRule.checkLastDownload(expectedFilename);
    }

    private String getClipboardText() throws Throwable {
        final AtomicReference<String> clipboardTextRef = new AtomicReference<>();
        mDownloadTestRule.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                ClipboardManager clipMgr =
                        (ClipboardManager) mDownloadTestRule.getActivity().getSystemService(
                                Context.CLIPBOARD_SERVICE);
                ClipData clipData = clipMgr.getPrimaryClip();
                Assert.assertNotNull("Primary clip is null", clipData);
                Assert.assertTrue("Primary clip contains no items.", clipData.getItemCount() > 0);
                clipboardTextRef.set(clipData.getItemAt(0).getText().toString());
            }
        });
        return clipboardTextRef.get();
    }

    private void assertStringContains(String subString, String superString) {
        Assert.assertTrue("String '" + superString + "' does not contain '" + subString + "'",
                superString.contains(subString));
    }

    /**
     * Makes sure there are no files with names identical to the ones this test uses in the
     * downloads directory
     */
    private void deleteTestFiles() {
        mDownloadTestRule.deleteFilesInDownloadDirectory(TEST_FILES);
    }
}
