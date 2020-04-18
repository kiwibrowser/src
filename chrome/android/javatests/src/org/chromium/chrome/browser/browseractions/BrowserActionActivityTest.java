// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.browseractions;

import android.annotation.TargetApi;
import android.app.Activity;
import android.app.Instrumentation;
import android.app.Instrumentation.ActivityMonitor;
import android.app.PendingIntent;
import android.app.ProgressDialog;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.StrictMode;
import android.support.annotation.DrawableRes;
import android.support.customtabs.browseractions.BrowserActionItem;
import android.support.customtabs.browseractions.BrowserActionsIntent;
import android.support.customtabs.browseractions.BrowserServiceFileProvider;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;
import android.util.Pair;
import android.util.SparseArray;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ActivityState;
import org.chromium.base.ApplicationStatus;
import org.chromium.base.Callback;
import org.chromium.base.ContextUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.MinAndroidSdkLevel;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.browseractions.BrowserActionsContextMenuHelper.BrowserActionsTestDelegate;
import org.chromium.chrome.browser.contextmenu.ChromeContextMenuItem;
import org.chromium.chrome.browser.contextmenu.ContextMenuItem;
import org.chromium.chrome.browser.contextmenu.ShareContextMenuItem;
import org.chromium.chrome.browser.customtabs.CustomTabActivityTestRule;
import org.chromium.chrome.browser.customtabs.CustomTabsTestUtils;
import org.chromium.chrome.browser.firstrun.FirstRunStatus;
import org.chromium.chrome.browser.multiwindow.MultiWindowTestHelper;
import org.chromium.chrome.browser.multiwindow.MultiWindowUtils;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.EmptyTabModelObserver;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabModel.TabLaunchType;
import org.chromium.chrome.browser.tabmodel.TabModelObserver;
import org.chromium.chrome.browser.util.IntentUtils;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.net.test.EmbeddedTestServer;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Callable;

/**
 * Instrumentation tests for context menu of a {@link BrowserActionActivity}.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
public class BrowserActionActivityTest {
    private static final String TEST_PAGE = "/chrome/test/data/android/google.html";
    private static final String TEST_PAGE_2 = "/chrome/test/data/android/test.html";
    private static final String TEST_PAGE_3 = "/chrome/test/data/android/simple.html";
    private static final String CUSTOM_ITEM_TITLE_1 = "Custom item with drawable icon";
    private static final String CUSTOM_ITEM_TITLE_2 = "Custom item with vector drawable icon";
    private static final String CUSTOM_ITEM_TITLE_3 = "Custom item wit invalid icon id";
    private static final String CUSTOM_ITEM_TITLE_4 = "Custom item without icon";
    @DrawableRes
    private static final int CUSTOM_ITEM_ICON_BITMAP_DRAWABLE_ID_1 = R.drawable.star_green;
    @DrawableRes
    private static final int CUSTOM_ITEM_ICON_BITMAP_DRAWABLE_ID_2 = R.drawable.star_gray;
    @DrawableRes
    private static final int CUSTOM_ITEM_ICON_VECTOR_DRAWABLE_ID = R.drawable.ic_add;
    @DrawableRes
    private static final int CUSTOM_ITEM_ICON_INVALID_DRAWABLE_ID = -1;

    private final CallbackHelper mOnBrowserActionsMenuShownCallback = new CallbackHelper();
    private final CallbackHelper mOnFinishNativeInitializationCallback = new CallbackHelper();
    private final CallbackHelper mOnOpenTabInBackgroundStartCallback = new CallbackHelper();
    private final CallbackHelper mOnDownloadStartCallback = new CallbackHelper();

    private final CallbackHelper mDidAddTabCallback = new CallbackHelper();

    private SparseArray<PendingIntent> mCustomActions;
    private List<Pair<Integer, List<ContextMenuItem>>> mItems;
    private ProgressDialog mProgressDialog;
    private TestDelegate mTestDelegate;
    private TabModelObserver mTestObserver;
    private EmbeddedTestServer mTestServer;
    private String mTestPage;
    private String mTestPage2;
    private String mTestPage3;
    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();
    @Rule
    public CustomTabActivityTestRule mCustomTabActivityTestRule = new CustomTabActivityTestRule();

    private class TestDelegate implements BrowserActionsTestDelegate {
        @Override
        public void onBrowserActionsMenuShown() {
            mOnBrowserActionsMenuShownCallback.notifyCalled();
        }

        @Override
        public void onFinishNativeInitialization() {
            mOnFinishNativeInitializationCallback.notifyCalled();
        }

        @Override
        public void onOpenTabInBackgroundStart() {
            mOnOpenTabInBackgroundStartCallback.notifyCalled();
        }

        @Override
        public void initialize(SparseArray<PendingIntent> customActions,
                List<Pair<Integer, List<ContextMenuItem>>> items,
                ProgressDialog progressDialog) {
            mCustomActions = customActions;
            mItems = items;
            mProgressDialog = progressDialog;
        }

        @Override
        public void onDownloadStart() {
            mOnDownloadStartCallback.notifyCalled();
        }
    }

    @Before
    public void setUp() throws Exception {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                FirstRunStatus.setFirstRunFlowComplete(true);
            }
        });
        mTestDelegate = new TestDelegate();
        mTestObserver = new EmptyTabModelObserver() {
            @Override
            public void didAddTab(Tab tab, TabLaunchType type) {
                mDidAddTabCallback.notifyCalled();
            }
        };
        Context appContext = InstrumentationRegistry.getInstrumentation()
                                     .getTargetContext()
                                     .getApplicationContext();
        mTestServer = EmbeddedTestServer.createAndStartServer(appContext);
        mTestPage = mTestServer.getURL(TEST_PAGE);
        mTestPage2 = mTestServer.getURL(TEST_PAGE_2);
        mTestPage3 = mTestServer.getURL(TEST_PAGE_3);
    }

    @After
    public void tearDown() throws Exception {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                FirstRunStatus.setFirstRunFlowComplete(false);
            }
        });
        mTestServer.stopAndDestroyServer();
    }

    private void assertDrawableNull(Context context, ContextMenuItem item) {
        item.getDrawableAsync(context, new Callback<Drawable>() {
            @Override
            public void onResult(Drawable drawable) {
                Assert.assertNull(drawable);
            }
        });
    }

    private void assertDrawableNotNull(Context context, ContextMenuItem item) {
        item.getDrawableAsync(context, new Callback<Drawable>() {
            @Override
            public void onResult(Drawable drawable) {
                Assert.assertNotNull(drawable);
            }
        });
    }

    @Test
    @SmallTest
    /**
     * TODO(ltian): move this to a separate test class only for {@link
     * BrowserActionsContextMenuHelper}.
     */
    public void testMenuShownCorrectlyWithFREComplete() throws Exception {
        List<BrowserActionItem> items = createCustomItems();
        BrowserActionActivity activity = startBrowserActionActivity(mTestPage, items, 0);

        // Menu should be shown before native finish loading.
        mOnBrowserActionsMenuShownCallback.waitForCallback(0);
        Assert.assertEquals(0, mOnFinishNativeInitializationCallback.getCallCount());
        Assert.assertEquals(0, mOnOpenTabInBackgroundStartCallback.getCallCount());

        // Let the initialization completes.
        mOnFinishNativeInitializationCallback.waitForCallback(0);
        Assert.assertEquals(1, mOnBrowserActionsMenuShownCallback.getCallCount());
        Assert.assertEquals(1, mOnFinishNativeInitializationCallback.getCallCount());

        Context context = InstrumentationRegistry.getTargetContext();
        Assert.assertEquals(context.getPackageName(), activity.mUntrustedCreatorPackageName);

        // Check menu populated correctly.
        List<Pair<Integer, List<ContextMenuItem>>> menus = mItems;
        Assert.assertEquals(1, menus.size());
        List<ContextMenuItem> contextMenuItems = menus.get(0).second;
        Assert.assertEquals(5 + items.size(), contextMenuItems.size());
        for (int i = 0; i < 4; i++) {
            Assert.assertTrue(contextMenuItems.get(i) instanceof ChromeContextMenuItem);
        }
        Assert.assertTrue(contextMenuItems.get(4) instanceof ShareContextMenuItem);
        Assert.assertTrue(contextMenuItems.get(5) instanceof BrowserActionsCustomContextMenuItem);
        // Load custom items correctly.
        for (int i = 0; i < items.size(); i++) {
            Assert.assertEquals(
                    items.get(i).getTitle(), contextMenuItems.get(5 + i).getTitle(context));
            Assert.assertEquals(items.get(i).getAction(),
                    mCustomActions.get(
                            BrowserActionsContextMenuHelper.CUSTOM_BROWSER_ACTIONS_ID_GROUP.get(
                                    i)));
        }
        assertDrawableNotNull(context, contextMenuItems.get(5));
        assertDrawableNotNull(context, contextMenuItems.get(6));
        assertDrawableNull(context, contextMenuItems.get(7));
        assertDrawableNull(context, contextMenuItems.get(8));
    }

    @Test
    @SmallTest
    public void testMenuShownCorrectlyWithFRENotComplete() throws Exception {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                FirstRunStatus.setFirstRunFlowComplete(false);
            }
        });
        List<BrowserActionItem> items = createCustomItems();
        BrowserActionActivity activity = startBrowserActionActivity(mTestPage, items, 0);

        // Menu should be shown before native finish loading.
        Assert.assertTrue(activity.isStartupDelayed());
        mOnBrowserActionsMenuShownCallback.waitForCallback(0);
        Assert.assertEquals(1, mOnBrowserActionsMenuShownCallback.getCallCount());
        // startupDelayed should still be true because native initalized is skipped.
        Assert.assertTrue(activity.isStartupDelayed());
        Assert.assertEquals(0, mOnFinishNativeInitializationCallback.getCallCount());

        Context context = InstrumentationRegistry.getTargetContext();
        Assert.assertEquals(context.getPackageName(), activity.mUntrustedCreatorPackageName);

        // Check menu populated correctly that only copy, share and custom items are shown.
        List<Pair<Integer, List<ContextMenuItem>>> menus = mItems;
        Assert.assertEquals(1, menus.size());
        List<ContextMenuItem> contextMenuItems = menus.get(0).second;
        Assert.assertEquals(2 + items.size(), contextMenuItems.size());
        Assert.assertTrue(contextMenuItems.get(0) instanceof ChromeContextMenuItem);
        Assert.assertTrue(contextMenuItems.get(1) instanceof ShareContextMenuItem);
        for (int i = 0; i < items.size(); i++) {
            Assert.assertEquals(
                    items.get(i).getTitle(), contextMenuItems.get(2 + i).getTitle(context));
            Assert.assertEquals(items.get(i).getAction(),
                    mCustomActions.get(
                            BrowserActionsContextMenuHelper.CUSTOM_BROWSER_ACTIONS_ID_GROUP.get(
                                    i)));
        }
        assertDrawableNotNull(context, contextMenuItems.get(2));
        assertDrawableNotNull(context, contextMenuItems.get(3));
        assertDrawableNull(context, contextMenuItems.get(4));
        assertDrawableNull(context, contextMenuItems.get(5));
    }

    @Test
    @SmallTest
    public void testCustomIconShownFromProviderCorrectly() throws Exception {
        List<BrowserActionItem> items = createCustomItemsWithProvider();
        startBrowserActionActivity(mTestPage, items, 0);
        mOnBrowserActionsMenuShownCallback.waitForCallback(0);

        List<Pair<Integer, List<ContextMenuItem>>> menus = mItems;
        List<ContextMenuItem> contextMenuItems = menus.get(0).second;
        ContentResolver resolver = InstrumentationRegistry.getTargetContext().getContentResolver();

        Assert.assertTrue(contextMenuItems.get(5) instanceof BrowserActionsCustomContextMenuItem);
        BrowserActionsCustomContextMenuItem customItems =
                (BrowserActionsCustomContextMenuItem) contextMenuItems.get(5);
        CallbackHelper imageShownCallback = new CallbackHelper();
        Context context = InstrumentationRegistry.getTargetContext();
        Callback<Drawable> callback = new Callback<Drawable>() {
            @Override
            public void onResult(Drawable drawable) {
                Assert.assertNotNull(drawable);
                imageShownCallback.notifyCalled();
            }
        };
        customItems.getDrawableAsync(context, callback);
        imageShownCallback.waitForCallback(0);
    }

    @Test
    @SmallTest
    public void testOpenTabInBackgroundWithInitialization() throws Exception {
        testItemHandlingWithInitialzation(
                R.id.browser_actions_open_in_background, mOnOpenTabInBackgroundStartCallback);
    }

    @Test
    @SmallTest
    public void testDownloadWithInitialization() throws Exception {
        testItemHandlingWithInitialzation(
                R.id.browser_actions_save_link_as, mOnDownloadStartCallback);
    }

    private void testItemHandlingWithInitialzation(int itemid, CallbackHelper callback)
            throws Exception {
        final BrowserActionActivity activity = startBrowserActionActivity(mTestPage);
        mOnBrowserActionsMenuShownCallback.waitForCallback(0);
        // Download an url before initialization finishes.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                activity.getHelperForTesting().onItemSelected(itemid, false);
            }
        });

        // If native initialization is not finished, A ProgressDialog should be displayed and
        // chosen item should be pending until initialization is finished.
        if (mOnFinishNativeInitializationCallback.getCallCount() == 0) {
            Assert.assertTrue(mProgressDialog.isShowing());
            mOnFinishNativeInitializationCallback.waitForCallback(0);
        }
        callback.waitForCallback(0);
        Assert.assertEquals(1, mOnFinishNativeInitializationCallback.getCallCount());
        Assert.assertFalse(mProgressDialog.isShowing());
        Assert.assertEquals(1, callback.getCallCount());
    }

    @Test
    @SmallTest
    public void testOpenSingleTabInBackgroundWhenChromeAvailable() throws Exception {
        // Start ChromeTabbedActivity first.
        mActivityTestRule.startMainActivityWithURL(mTestPage);

        // Load Browser Actions menu completely.
        final BrowserActionActivity activity = startBrowserActionActivity(mTestPage2);
        mOnBrowserActionsMenuShownCallback.waitForCallback(0);
        mOnFinishNativeInitializationCallback.waitForCallback(0);
        Assert.assertEquals(1, mActivityTestRule.getActivity().getCurrentTabModel().getCount());
        // No notification should be shown.
        Assert.assertFalse(BrowserActionsService.hasBrowserActionsNotification());

        // Open a tab in the background.
        openTabInBackground(activity);
        // Notification for single tab should be shown.
        Assert.assertEquals(R.string.browser_actions_single_link_open_notification_title,
                BrowserActionsService.getTitleResId());

        // Tabs should always be added at the end of the model.
        Assert.assertEquals(2, mActivityTestRule.getActivity().getCurrentTabModel().getCount());
        Assert.assertEquals(mTestPage,
                mActivityTestRule.getActivity().getCurrentTabModel().getTabAt(0).getUrl());
        Assert.assertEquals(mTestPage2,
                mActivityTestRule.getActivity().getCurrentTabModel().getTabAt(1).getUrl());
        int prevTabId = mActivityTestRule.getActivity().getCurrentTabModel().getTabAt(0).getId();
        int newTabId = mActivityTestRule.getActivity().getCurrentTabModel().getTabAt(1).getId();
        // TODO(ltian): overwrite delegate prevent creating notifcation for test.
        Intent notificationIntent = BrowserActionsService.getNotificationIntent();
        notificationIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

        // Force ChromeTabbedActivity dismissed to make sure it calls onStop then calls onStart next
        // time it is started by an Intent.
        Intent customTabIntent = CustomTabsTestUtils.createMinimalCustomTabIntent(
                InstrumentationRegistry.getTargetContext(), mTestPage);
        mCustomTabActivityTestRule.startCustomTabActivityWithIntent(customTabIntent);

        // The Intent of the Browser Actions notification should not toggle overview mode and should
        mActivityTestRule.startActivityCompletely(notificationIntent);
        Assert.assertFalse(mActivityTestRule.getActivity().getLayoutManager().overviewVisible());
        Assert.assertNotEquals(prevTabId, mActivityTestRule.getActivity().getActivityTab().getId());
        Assert.assertEquals(newTabId, mActivityTestRule.getActivity().getActivityTab().getId());
    }

    @Test
    @SmallTest
    public void testOpenMulitpleTabsInBackgroundWhenChromeAvailable() throws Exception {
        // Start ChromeTabbedActivity first.
        mActivityTestRule.startMainActivityWithURL(mTestPage);

        // Open two tabs in the background.
        final BrowserActionActivity activity1 = startBrowserActionActivity(mTestPage2);
        mOnBrowserActionsMenuShownCallback.waitForCallback(0);
        mOnFinishNativeInitializationCallback.waitForCallback(0);
        Assert.assertEquals(1, mActivityTestRule.getActivity().getCurrentTabModel().getCount());
        Assert.assertFalse(BrowserActionsService.hasBrowserActionsNotification());
        openTabInBackground(activity1);
        Assert.assertEquals(R.string.browser_actions_single_link_open_notification_title,
                BrowserActionsService.getTitleResId());

        final BrowserActionActivity activity2 = startBrowserActionActivity(mTestPage3, 1);
        mOnBrowserActionsMenuShownCallback.waitForCallback(1);
        mOnFinishNativeInitializationCallback.waitForCallback(1);
        Assert.assertEquals(2, mActivityTestRule.getActivity().getCurrentTabModel().getCount());
        openTabInBackground(activity2);
        // Notification title should be shown for multiple tabs.
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return R.string.browser_actions_multi_links_open_notification_title
                        == BrowserActionsService.getTitleResId();
            }
        });

        // Tabs should always be added at the end of the model.
        Assert.assertEquals(3, mActivityTestRule.getActivity().getCurrentTabModel().getCount());
        Assert.assertEquals(mTestPage,
                mActivityTestRule.getActivity().getCurrentTabModel().getTabAt(0).getUrl());
        Assert.assertEquals(mTestPage2,
                mActivityTestRule.getActivity().getCurrentTabModel().getTabAt(1).getUrl());
        Assert.assertEquals(mTestPage3,
                mActivityTestRule.getActivity().getCurrentTabModel().getTabAt(2).getUrl());
        Intent notificationIntent = BrowserActionsService.getNotificationIntent();
        notificationIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

        // Force ChromeTabbedActivity dismissed to make sure it calls onStop then calls onStart next
        // time it is started by an Intent.
        Intent customTabIntent = CustomTabsTestUtils.createMinimalCustomTabIntent(
                InstrumentationRegistry.getTargetContext(), mTestPage);
        mCustomTabActivityTestRule.startCustomTabActivityWithIntent(customTabIntent);

        // The Intent of the Browser Actions notification should toggle overview mode.
        mActivityTestRule.startActivityCompletely(notificationIntent);
        if (mActivityTestRule.getActivity().isTablet()) {
            Assert.assertFalse(
                    mActivityTestRule.getActivity().getLayoutManager().overviewVisible());
        } else {
            Assert.assertTrue(mActivityTestRule.getActivity().getLayoutManager().overviewVisible());
        }
    }

    @Test
    @SmallTest
    public void testOpenSingleTabInBackgroundWhenChromeNotAvailable() throws Exception {
        // Load Browser Actions menu completely.
        final BrowserActionActivity activity = startBrowserActionActivity(mTestPage);
        mOnBrowserActionsMenuShownCallback.waitForCallback(0);
        mOnFinishNativeInitializationCallback.waitForCallback(0);
        // No notification should be shown.
        Assert.assertFalse(BrowserActionsService.hasBrowserActionsNotification());

        BrowserActionsTabModelSelector selector =
                ThreadUtils.runOnUiThreadBlocking(new Callable<BrowserActionsTabModelSelector>() {
                    @Override
                    public BrowserActionsTabModelSelector call() {
                        return BrowserActionsTabModelSelector.getInstance();
                    }
                });
        // Open a tab in the background.
        selector.getModel(false).addObserver(mTestObserver);
        openTabInBackground(activity);
        Assert.assertEquals(R.string.browser_actions_single_link_open_notification_title,
                BrowserActionsService.getTitleResId());
        mDidAddTabCallback.waitForCallback(0);
        // New tab should be added to the BrowserActionTabModelSelector.
        Assert.assertEquals(1, selector.getCurrentModel().getCount());
        Tab newTab = selector.getCurrentModel().getTabAt(0);
        Assert.assertEquals(mTestPage, newTab.getUrl());

        // Browser Actions tabs should be merged into Chrome tab model when Chrome starts.
        Intent notificationIntent = BrowserActionsService.getNotificationIntent();
        notificationIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        mActivityTestRule.startActivityCompletely(notificationIntent);
        TabModel currentModel =
                mActivityTestRule.getActivity().getTabModelSelector().getCurrentModel();
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return currentModel.getCount() == 1;
            }
        });
        Assert.assertEquals(newTab.getId(), currentModel.getTabAt(0).getId());
    }

    @Test
    @SmallTest
    public void testOpenMultipleTabsInBackgroundWhenChromeNotAvailable() throws Exception {
        // Load Browser Actions menu completely and open a tab in the background.
        final BrowserActionActivity activity1 = startBrowserActionActivity(mTestPage);
        mOnBrowserActionsMenuShownCallback.waitForCallback(0);
        mOnFinishNativeInitializationCallback.waitForCallback(0);

        BrowserActionsTabModelSelector selector =
                ThreadUtils.runOnUiThreadBlocking(new Callable<BrowserActionsTabModelSelector>() {
                    @Override
                    public BrowserActionsTabModelSelector call() {
                        return BrowserActionsTabModelSelector.getInstance();
                    }
                });
        selector.getModel(false).addObserver(mTestObserver);
        openTabInBackground(activity1);
        Assert.assertEquals(R.string.browser_actions_single_link_open_notification_title,
                BrowserActionsService.getTitleResId());
        mDidAddTabCallback.waitForCallback(0);
        Assert.assertEquals(1, selector.getCurrentModel().getCount());

        // Load Browser Actions menu again and open another tab in the background.
        final BrowserActionActivity activity2 = startBrowserActionActivity(mTestPage2, 1);
        mOnBrowserActionsMenuShownCallback.waitForCallback(1);
        mOnFinishNativeInitializationCallback.waitForCallback(1);
        openTabInBackground(activity2);
        // Notification title should be shown for multiple tabs.
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return R.string.browser_actions_multi_links_open_notification_title
                        == BrowserActionsService.getTitleResId();
            }
        });

        // BrowserActionTabModelSelector should have two tabs and tabs should be added sequentially.
        mDidAddTabCallback.waitForCallback(1);
        Assert.assertEquals(2, selector.getCurrentModel().getCount());
        Tab newTab1 = selector.getCurrentModel().getTabAt(0);
        Tab newTab2 = selector.getCurrentModel().getTabAt(1);
        Assert.assertEquals(mTestPage, newTab1.getUrl());
        Assert.assertEquals(mTestPage2, newTab2.getUrl());

        // Browser Actions tabs should be merged into Chrome tab model when Chrome starts.
        Intent notificationIntent = BrowserActionsService.getNotificationIntent();
        notificationIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        mActivityTestRule.startActivityCompletely(notificationIntent);
        TabModel currentModel =
                mActivityTestRule.getActivity().getTabModelSelector().getCurrentModel();
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return currentModel.getCount() == 2;
            }
        });
        Assert.assertEquals(newTab1.getId(), currentModel.getTabAt(0).getId());
        Assert.assertEquals(newTab2.getId(), currentModel.getTabAt(1).getId());
        Assert.assertEquals(1, currentModel.index());

        // Tab switcher should be shown on phones.
        if (mActivityTestRule.getActivity().isTablet()) {
            Assert.assertFalse(
                    mActivityTestRule.getActivity().getLayoutManager().overviewVisible());
        } else {
            Assert.assertTrue(mActivityTestRule.getActivity().getLayoutManager().overviewVisible());
        }
    }

    private void openTabInBackground(BrowserActionActivity activity) {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskWrites();
                try {
                    activity.getHelperForTesting().onItemSelected(
                            R.id.browser_actions_open_in_background, false);
                } finally {
                    StrictMode.setThreadPolicy(oldPolicy);
                }
            }
        });
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return BrowserActionsService.hasBrowserActionsNotification()
                        && BrowserActionsService.isBackgroundService();
            }
        });
    }

    @Test
    @SmallTest
    @Feature({"TabPersistentStore", "MultiWindow"})
    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    @MinAndroidSdkLevel(24)
    public void testTabMergingWhenChromeNotAvailable() throws Exception {
        // Start two ChromeTabbedActivitys in MultiWindow mode.
        mActivityTestRule.startMainActivityWithURL(mTestPage);
        ChromeTabbedActivity activity1 = mActivityTestRule.getActivity();
        MultiWindowUtils.getInstance().setIsInMultiWindowModeForTesting(true);
        ChromeTabbedActivity activity2 =
                MultiWindowTestHelper.createSecondChromeTabbedActivity(activity1);
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return activity2.areTabModelsInitialized()
                        && activity2.getTabModelSelector().isTabStateInitialized()
                        && activity2.getActivityTab() != null;
            }
        });
        String cta2ActivityTabUrl = activity2.getActivityTab().getUrl();
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                activity2.getTabCreator(false).createNewTab(
                        new LoadUrlParams(mTestPage2), TabLaunchType.FROM_CHROME_UI, null);
            }
        });

        // Save state and destroy both activities.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                activity1.saveState();
                activity2.saveState();
                activity1.finishAndRemoveTask();
                activity2.finishAndRemoveTask();
            }
        });

        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return ApplicationStatus.getStateForActivity(activity1) == ActivityState.DESTROYED
                        && ApplicationStatus.getStateForActivity(activity2)
                        == ActivityState.DESTROYED;
            }
        });

        // Load Browser Actions menu and open a tab in the background completely.
        final BrowserActionActivity activity3 = startBrowserActionActivity(mTestPage3);
        mOnBrowserActionsMenuShownCallback.waitForCallback(0);
        mOnFinishNativeInitializationCallback.waitForCallback(0);
        openTabInBackground(activity3);

        // Save the Browser Actions tab states and destroy the selector and activity.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                BrowserActionsTabModelSelector selector =
                        BrowserActionsTabModelSelector.getInstance();
                selector.saveState();
                selector.destroy();
            }
        });
        activity3.finish();

        // Relaunch a new ChromeTabbedActivity. Tabs from multi-windows should be merged and
        // Browser Actions tabs should be append at the end.
        Intent intent = createChromeTabbedActivityIntent(ContextUtils.getApplicationContext());
        ChromeTabbedActivity newActivity =
                (ChromeTabbedActivity) InstrumentationRegistry.getInstrumentation()
                        .startActivitySync(intent);
        // Wait for the tab state to be initialized.
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return newActivity.areTabModelsInitialized()
                        && newActivity.getTabModelSelector().isTabStateInitialized();
            }
        });
        TabModel model = newActivity.getTabModelSelector().getModel(false);
        Assert.assertEquals(4, model.getCount());
        Assert.assertEquals(mTestPage, model.getTabAt(0).getUrl());
        Assert.assertEquals(cta2ActivityTabUrl, model.getTabAt(1).getUrl());
        Assert.assertEquals(mTestPage2, model.getTabAt(2).getUrl());
        Assert.assertEquals(mTestPage3, model.getTabAt(3).getUrl());
    }

    // TODO(ltian): create a test util class and change this to a static function in it to share
    // with TabModelMergingTest.
    private Intent createChromeTabbedActivityIntent(Context context) {
        Intent intent = new Intent();
        intent.setClass(context, ChromeTabbedActivity.class);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        return intent;
    }

    private BrowserActionActivity startBrowserActionActivity(String url) throws Exception {
        return startBrowserActionActivity(url, 0);
    }

    private BrowserActionActivity startBrowserActionActivity(String url, int expectedCallCount)
            throws Exception {
        Context context = InstrumentationRegistry.getTargetContext();
        return startBrowserActionActivity(url, new ArrayList<>(), expectedCallCount);
    }

    private BrowserActionActivity startBrowserActionActivity(
            String url, List<BrowserActionItem> items, int expectedCallCount) throws Exception {
        final Instrumentation instrumentation = InstrumentationRegistry.getInstrumentation();
        ActivityMonitor browserActionMonitor =
                new ActivityMonitor(BrowserActionActivity.class.getName(), null, false);
        instrumentation.addMonitor(browserActionMonitor);

        // The BrowserActionActivity shouldn't have started yet.
        Assert.assertEquals(expectedCallCount, mOnBrowserActionsMenuShownCallback.getCallCount());
        Assert.assertEquals(
                expectedCallCount, mOnFinishNativeInitializationCallback.getCallCount());
        Assert.assertEquals(expectedCallCount, mOnOpenTabInBackgroundStartCallback.getCallCount());

        // Fire an Intent to start the BrowserActionActivity.
        sendBrowserActionIntent(url, items);

        Activity activity = instrumentation.waitForMonitorWithTimeout(
                browserActionMonitor, CriteriaHelper.DEFAULT_MAX_TIME_TO_POLL);
        Assert.assertNotNull("Activity didn't start", activity);
        Assert.assertTrue("Wrong activity started", activity instanceof BrowserActionActivity);
        instrumentation.removeMonitor(browserActionMonitor);
        ((BrowserActionActivity) activity)
                .getHelperForTesting()
                .setTestDelegateForTesting(mTestDelegate);
        return (BrowserActionActivity) activity;
    }

    private void sendBrowserActionIntent(String url, List<BrowserActionItem> items) {
        Context context = InstrumentationRegistry.getTargetContext();
        Intent intent = new Intent(BrowserActionsIntent.ACTION_BROWSER_ACTIONS_OPEN);
        intent.setData(Uri.parse(url));
        intent.putExtra(BrowserActionsIntent.EXTRA_TYPE, BrowserActionsIntent.URL_TYPE_NONE);
        PendingIntent pendingIntent = PendingIntent.getActivity(context, 0, new Intent(), 0);
        intent.putExtra(BrowserActionsIntent.EXTRA_APP_ID, pendingIntent);

        ArrayList<Bundle> customItemBundles = new ArrayList<>();
        List<Uri> uris = new ArrayList<>();
        for (BrowserActionItem item : items) {
            Bundle customItemBundle = new Bundle();
            customItemBundle.putString(BrowserActionsIntent.KEY_TITLE, item.getTitle());
            if (item.getIconId() != 0) {
                customItemBundle.putInt(BrowserActionsIntent.KEY_ICON_ID, item.getIconId());
            }
            if (item.getIconUri() != null) {
                Uri uri = item.getIconUri();
                customItemBundle.putParcelable(BrowserActionsIntent.KEY_ICON_URI, uri);
                uris.add(uri);
            }
            customItemBundle.putParcelable(BrowserActionsIntent.KEY_ACTION, item.getAction());
            customItemBundles.add(customItemBundle);
        }
        intent.putParcelableArrayListExtra(
                BrowserActionsIntent.EXTRA_MENU_ITEMS, customItemBundles);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

        intent.setClass(context, BrowserActionActivity.class);
        if (!uris.isEmpty()) {
            BrowserServiceFileProvider.grantReadPermission(
                    intent, uris, InstrumentationRegistry.getContext());
        }
        // Android Test Rule auto adds {@link Intent.FLAG_ACTIVITY_NEW_TASK} which violates {@link
        // BrowserActionsIntent} policy. Add an extra to skip Intent.FLAG_ACTIVITY_NEW_TASK check
        // for test.
        IntentUtils.safeStartActivity(context, intent);
    }

    private PendingIntent createCustomItemAction(String url) {
        Context context = InstrumentationRegistry.getTargetContext();
        Intent customIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
        return PendingIntent.getActivity(context, 0, customIntent, 0);
    }

    private List<BrowserActionItem> createCustomItems() {
        List<BrowserActionItem> items = new ArrayList<>();
        PendingIntent action1 = createCustomItemAction(mTestPage);
        BrowserActionItem item1 = new BrowserActionItem(
                CUSTOM_ITEM_TITLE_1, action1, CUSTOM_ITEM_ICON_BITMAP_DRAWABLE_ID_1);
        PendingIntent action2 = createCustomItemAction(mTestPage);
        BrowserActionItem item2 = new BrowserActionItem(
                CUSTOM_ITEM_TITLE_2, action2, CUSTOM_ITEM_ICON_VECTOR_DRAWABLE_ID);
        PendingIntent action3 = createCustomItemAction(mTestPage);
        BrowserActionItem item3 = new BrowserActionItem(
                CUSTOM_ITEM_TITLE_3, action3, CUSTOM_ITEM_ICON_INVALID_DRAWABLE_ID);
        PendingIntent action4 = createCustomItemAction(mTestPage);
        BrowserActionItem item4 = new BrowserActionItem(CUSTOM_ITEM_TITLE_4, action4);
        items.add(item1);
        items.add(item2);
        items.add(item3);
        items.add(item4);
        return items;
    }

    private List<BrowserActionItem> createCustomItemsWithProvider() {
        List<BrowserActionItem> items = new ArrayList<>();
        PendingIntent action1 = createCustomItemAction(mTestPage);
        Context context = InstrumentationRegistry.getTargetContext();
        Bitmap bm = BitmapFactory.decodeResource(
                context.getResources(), CUSTOM_ITEM_ICON_BITMAP_DRAWABLE_ID_1);

        Uri uri = BrowserServiceFileProvider.generateUri(context, bm, CUSTOM_ITEM_TITLE_1, 1);
        BrowserActionItem item = new BrowserActionItem(CUSTOM_ITEM_TITLE_1, action1, uri);
        items.add(item);
        return items;
    }
}
