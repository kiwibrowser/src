// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextual_suggestions;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import android.os.SystemClock;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.LargeTest;
import android.support.test.filters.MediumTest;
import android.support.v7.widget.RecyclerView;
import android.view.View;
import android.view.View.OnLayoutChangeListener;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.MethodRule;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.params.MethodParamAnnotationRule;
import org.chromium.base.test.params.ParameterAnnotations;
import org.chromium.base.test.params.ParameterProvider;
import org.chromium.base.test.params.ParameterSet;
import org.chromium.base.test.params.ParameterizedRunner;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.Restriction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.contextual_suggestions.ContextualSuggestionsModel.PropertyKey;
import org.chromium.chrome.browser.feature_engagement.TrackerFactory;
import org.chromium.chrome.browser.fullscreen.FullscreenManagerTestUtils;
import org.chromium.chrome.browser.modelutil.ListObservable;
import org.chromium.chrome.browser.modelutil.ListObservable.ListObserver;
import org.chromium.chrome.browser.multiwindow.MultiWindowTestHelper;
import org.chromium.chrome.browser.multiwindow.MultiWindowUtils;
import org.chromium.chrome.browser.ntp.ContextMenuManager;
import org.chromium.chrome.browser.ntp.snippets.SnippetArticleViewHolder;
import org.chromium.chrome.browser.test.ScreenShooter;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheet;
import org.chromium.chrome.browser.widget.bottomsheet.EmptyBottomSheetObserver;
import org.chromium.chrome.test.BottomSheetTestRule;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4RunnerDelegate;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.chrome.test.util.ChromeTabUtils;
import org.chromium.chrome.test.util.RenderTestRule;
import org.chromium.chrome.test.util.ViewUtils;
import org.chromium.chrome.test.util.browser.ChromeModernDesign;
import org.chromium.chrome.test.util.browser.Features;
import org.chromium.chrome.test.util.browser.Features.DisableFeatures;
import org.chromium.chrome.test.util.browser.Features.EnableFeatures;
import org.chromium.chrome.test.util.browser.RecyclerViewTestUtils;
import org.chromium.chrome.test.util.browser.compositor.layouts.DisableChromeAnimations;
import org.chromium.components.feature_engagement.FeatureConstants;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content.browser.test.util.TestWebContentsObserver;
import org.chromium.content_public.browser.GestureListenerManager;
import org.chromium.content_public.browser.GestureStateListener;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.content_public.browser.WebContents;
import org.chromium.net.test.EmbeddedTestServer;
import org.chromium.ui.test.util.UiRestriction;

import java.util.Arrays;
import java.util.Locale;
import java.util.concurrent.TimeoutException;

/**
 * Tests related to displaying contextual suggestions in a bottom sheet.
 */
@RunWith(ParameterizedRunner.class)
@ParameterAnnotations.UseRunnerDelegate(ChromeJUnit4RunnerDelegate.class)
@Restriction(UiRestriction.RESTRICTION_TYPE_PHONE)
@CommandLineFlags.Add(ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE)
@EnableFeatures(ChromeFeatureList.CONTEXTUAL_SUGGESTIONS_BOTTOM_SHEET)
@ChromeModernDesign.Enable
public class ContextualSuggestionsTest {
    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();
    @Rule
    public ContextualSuggestionsDependenciesRule mContextualSuggestionsDeps =
            new ContextualSuggestionsDependenciesRule();
    @Rule
    public TestRule mChromeModernDesignStateRule = new ChromeModernDesign.Processor();
    @Rule
    public TestRule mFeaturesProcessor = new Features.InstrumentationProcessor();
    @Rule
    public ScreenShooter mScreenShooter = new ScreenShooter();
    @Rule
    public TestRule mDisableChromeAnimations = new DisableChromeAnimations();
    @Rule
    public RenderTestRule mRenderTestRule = new RenderTestRule();

    @Rule
    public MethodRule mMethodParamAnnotationProcessor = new MethodParamAnnotationRule();
    public static class SlimPeekUIParams implements ParameterProvider {
        @Override
        public Iterable<ParameterSet> getParameters() {
            return Arrays.asList(new ParameterSet().value(false).name("DisableSlimPeekUI"),
                    new ParameterSet().value(true).name("EnableSlimPeekUI"));
        }
    }

    private static final String TEST_PAGE =
            "/chrome/test/data/android/contextual_suggestions/contextual_suggestions_test.html";

    private FakeContextualSuggestionsSource mFakeSource;
    private EmbeddedTestServer mTestServer;
    private ContextualSuggestionsCoordinator mCoordinator;
    private ContextualSuggestionsMediator mMediator;
    private ContextualSuggestionsModel mModel;
    private BottomSheet mBottomSheet;

    // Used in multi-instance test.
    private ContextualSuggestionsCoordinator mCoordinator2;
    private ContextualSuggestionsMediator mMediator2;
    private ContextualSuggestionsModel mModel2;
    private BottomSheet mBottomSheet2;

    private CallbackHelper mToolbarShadowVisibleCallback = new CallbackHelper();

    @ParameterAnnotations.UseMethodParameterBefore(SlimPeekUIParams.class)
    public void setupSlimPeekUI(boolean slimPeekUIEnabled) {
        if (slimPeekUIEnabled) {
            mRenderTestRule.setVariantPrefix("slim-peek");
            Features.getInstance().enable(ChromeFeatureList.CONTEXTUAL_SUGGESTIONS_SLIM_PEEK_UI);
        } else {
            Features.getInstance().disable(ChromeFeatureList.CONTEXTUAL_SUGGESTIONS_SLIM_PEEK_UI);
        }
    }

    @Before
    public void setUp() throws Exception {
        mFakeSource = new FakeContextualSuggestionsSource();
        mContextualSuggestionsDeps.getFactory().suggestionsSource = mFakeSource;
        FetchHelper.setDisableDelayForTesting(true);
        ContextualSuggestionsMediator.setOverrideBrowserControlsHiddenForTesting(true);

        FakeEnabledStateMonitor stateMonitor = new FakeEnabledStateMonitor();
        mContextualSuggestionsDeps.getFactory().enabledStateMonitor = new FakeEnabledStateMonitor();

        mTestServer = EmbeddedTestServer.createAndStartServer(InstrumentationRegistry.getContext());
        mActivityTestRule.startMainActivityWithURL(mTestServer.getURL(TEST_PAGE));

        ThreadUtils.runOnUiThreadBlocking(() -> {
            mCoordinator =
                    mActivityTestRule.getActivity().getContextualSuggestionsCoordinatorForTesting();
            mMediator = mCoordinator.getMediatorForTesting();
            mModel = mCoordinator.getModelForTesting();
            stateMonitor.setObserver(mMediator);

            mModel.addObserver((source, propertyKey) -> {
                if (propertyKey == PropertyKey.TOOLBAR_SHADOW_VISIBILITY
                        && mModel.getToolbarShadowVisibility()) {
                    mToolbarShadowVisibleCallback.notifyCalled();
                }
            });
        });

        mBottomSheet = mActivityTestRule.getActivity().getBottomSheet();
    }

    @After
    public void tearDown() {
        mTestServer.stopAndDestroyServer();
        FetchHelper.setDisableDelayForTesting(false);
        ContextualSuggestionsMediator.setOverrideBrowserControlsHiddenForTesting(false);
    }

    @Test
    @MediumTest
    @Feature({"ContextualSuggestions"})
    public void testOpenContextualSuggestionsBottomSheet() {
        assertEquals("Sheet should still be hidden.", BottomSheet.SHEET_STATE_HIDDEN,
                mBottomSheet.getSheetState());

        ThreadUtils.runOnUiThreadBlocking(() -> {
            mMediator.showContentInSheetForTesting(true, true);
            mBottomSheet.endAnimations();
        });

        assertEquals("Title text should be set.",
                FakeContextualSuggestionsSource.TEST_TOOLBAR_TITLE, mModel.getTitle());

        assertEquals("Cluster list should be set.",
                (int) FakeContextualSuggestionsSource.TOTAL_ITEM_COUNT,
                mModel.getClusterList().getItemCount());

        ContextualSuggestionsBottomSheetContent content =
                (ContextualSuggestionsBottomSheetContent) mBottomSheet.getCurrentSheetContent();
        RecyclerView recyclerView = (RecyclerView) content.getContentView();
        assertEquals("RecyclerView should be empty.", 0, recyclerView.getChildCount());

        assertEquals("Sheet should be peeked.", BottomSheet.SHEET_STATE_PEEK,
                mBottomSheet.getSheetState());
        assertNull("RecyclerView should still be empty.", recyclerView.getAdapter());

        openSheet();

        assertEquals("RecyclerView should have content.",
                (int) FakeContextualSuggestionsSource.TOTAL_ITEM_COUNT,
                recyclerView.getAdapter().getItemCount());
    }

    @Test
    @MediumTest
    @Feature({"ContextualSuggestions"})
    public void testScrollPageToTrigger() throws InterruptedException, TimeoutException {
        ContextualSuggestionsMediator.setOverrideBrowserControlsHiddenForTesting(false);
        mMediator.setTargetScrollPercentageForTesting(0f);
        assertEquals("Sheet should be hidden.", BottomSheet.SHEET_STATE_HIDDEN,
                mBottomSheet.getSheetState());

        CallbackHelper fullyPeekedCallback = new CallbackHelper();
        mBottomSheet.addObserver(new EmptyBottomSheetObserver() {
            @Override
            public void onSheetFullyPeeked() {
                fullyPeekedCallback.notifyCalled();
            }
        });

        // Scroll the base page, hiding then reshowing the browser controls.
        FullscreenManagerTestUtils.disableBrowserOverrides();
        FullscreenManagerTestUtils.waitForBrowserControlsToBeMoveable(
                mActivityTestRule, mActivityTestRule.getActivity().getActivityTab());

        // Assert that the sheet is now visible.
        fullyPeekedCallback.waitForCallback(0);
        assertEquals("Sheet should be peeked.", BottomSheet.SHEET_STATE_PEEK,
                mBottomSheet.getSheetState());
        assertTrue("Bottom sheet should contain suggestions content.",
                mBottomSheet.getCurrentSheetContent()
                                instanceof ContextualSuggestionsBottomSheetContent);
    }

    @Test
    @MediumTest
    @Feature({"ContextualSuggestions"})
    public void testCloseFromPeek() throws InterruptedException, TimeoutException {
        forceShowSuggestions();
        simulateClickOnCloseButton();
    }

    @Test
    @MediumTest
    @Feature({"ContextualSuggestions"})
    public void testCloseFromOpen() throws InterruptedException, TimeoutException {
        forceShowSuggestions();
        openSheet();
        simulateClickOnCloseButton();
    }

    @Test
    @MediumTest
    @Feature({"ContextualSuggestions"})
    public void testTriggerMultipleTimes() throws InterruptedException, TimeoutException {
        // Show one time and close.
        forceShowSuggestions();
        openSheet();
        simulateClickOnCloseButton();

        // Show a second time.
        ThreadUtils.runOnUiThreadBlocking(
                () -> mMediator.requestSuggestions("http://www.testurl.com"));
        forceShowSuggestions();
        openSheet();
    }

    @Test
    @MediumTest
    @Feature({"ContextualSuggestions"})
    public void testTriggerAfterClose() throws InterruptedException, TimeoutException {
        int firstTabId = mActivityTestRule.getActivity().getActivityTab().getId();
        mActivityTestRule.loadUrlInNewTab(mTestServer.getURL(TEST_PAGE));
        int secondTabId = mActivityTestRule.getActivity().getActivityTab().getId();

        // Show suggestions on the second tab and simulate clicking close button.
        forceShowSuggestions();
        openSheet();
        simulateClickOnCloseButton();

        // Switch to the first tab and verify that the suggestions can be shown.
        ChromeTabUtils.switchTabInCurrentTabModel(mActivityTestRule.getActivity(), firstTabId);
        forceShowSuggestions();

        // Switch to the second tab.
        ChromeTabUtils.switchTabInCurrentTabModel(mActivityTestRule.getActivity(), secondTabId);
        ThreadUtils.runOnUiThreadBlocking(() -> mBottomSheet.endAnimations());

        // Simulate reverse scroll.
        FullscreenManagerTestUtils.disableBrowserOverrides();
        FullscreenManagerTestUtils.waitForBrowserControlsToBeMoveable(
                mActivityTestRule, mActivityTestRule.getActivity().getActivityTab());

        // Verify that the model is empty.
        assertEquals("Model should be empty.", 0, mModel.getClusterList().getItemCount());
        assertEquals("Bottom sheet should be hidden.", BottomSheet.SHEET_STATE_HIDDEN,
                mBottomSheet.getSheetState());

        // Switch to the first tab and verify that the suggestions can still be shown.
        ChromeTabUtils.switchTabInCurrentTabModel(mActivityTestRule.getActivity(), firstTabId);
        forceShowSuggestions();
    }

    @Test
    @MediumTest
    @Feature({"ContextualSuggestions"})
    public void testOpenSuggestion() throws InterruptedException, TimeoutException {
        forceShowSuggestions();
        openSheet();

        SnippetArticleViewHolder holder = getFirstSuggestionViewHolder();
        String expectedUrl = holder.getUrl();

        TestWebContentsObserver webContentsObserver = new TestWebContentsObserver(
                mActivityTestRule.getActivity().getActivityTab().getWebContents());

        int callCount = webContentsObserver.getOnPageStartedHelper().getCallCount();

        ThreadUtils.runOnUiThreadBlocking(() -> {
            holder.itemView.performClick();
        });

        webContentsObserver.getOnPageStartedHelper().waitForCallback(callCount);

        ThreadUtils.runOnUiThreadBlocking(() -> mBottomSheet.endAnimations());

        assertEquals("Tab URL should match snippet URL", expectedUrl,
                mActivityTestRule.getActivity().getActivityTab().getUrl());
        assertFalse("Sheet should be closed.", mBottomSheet.isSheetOpen());
    }

    @Test
    @MediumTest
    @Feature({"ContextualSuggestions"})
    public void testOpenArticleInNewTab() throws Exception {
        forceShowSuggestions();
        openSheet();

        SnippetArticleViewHolder holder = getFirstSuggestionViewHolder();
        String expectedUrl = holder.getUrl();

        ChromeTabUtils.invokeContextMenuAndOpenInANewTab(mActivityTestRule, holder.itemView,
                ContextMenuManager.ID_OPEN_IN_NEW_TAB, false, expectedUrl);

        assertEquals("Sheet should still be opened.", BottomSheet.SHEET_STATE_FULL,
                mBottomSheet.getSheetState());
    }

    @Test
    @MediumTest
    @Feature({"ContextualSuggestions"})
    public void testOpenSuggestionInNewTabIncognito() throws Exception {
        forceShowSuggestions();
        openSheet();

        SnippetArticleViewHolder holder = getFirstSuggestionViewHolder();
        String expectedUrl = holder.getUrl();

        ChromeTabUtils.invokeContextMenuAndOpenInANewTab(mActivityTestRule, holder.itemView,
                ContextMenuManager.ID_OPEN_IN_INCOGNITO_TAB, true, expectedUrl);

        ThreadUtils.runOnUiThreadBlocking(() -> mBottomSheet.endAnimations());

        assertFalse("Sheet should be closed.", mBottomSheet.isSheetOpen());
    }

    @Test
    @MediumTest
    @Feature({"ContextualSuggestions"})
    public void testMenuButton() throws Exception {
        forceShowSuggestions();
        View title =
                mBottomSheet.getCurrentSheetContent().getToolbarView().findViewById(R.id.title);
        View menuButton =
                mBottomSheet.getCurrentSheetContent().getToolbarView().findViewById(R.id.more);

        CallbackHelper layoutHelper = new CallbackHelper();
        title.addOnLayoutChangeListener(new OnLayoutChangeListener() {
            @Override
            public void onLayoutChange(View v, int left, int top, int right, int bottom,
                    int oldLeft, int oldTop, int oldRight, int oldBottom) {
                layoutHelper.notifyCalled();
            }
        });
        layoutHelper.waitForCallback(0);

        int titleWidth = title.getWidth();
        // Menu button is not visible on peek state.
        assertEquals(View.GONE, menuButton.getVisibility());
        assertEquals(0, menuButton.getWidth());

        // Menu button is visible after sheet is opened.
        openSheet();
        assertEquals(View.VISIBLE, menuButton.getVisibility());

        // Title view should be resized.
        ViewUtils.waitForStableView(menuButton);
        assertNotEquals(0, menuButton.getWidth());
        assertEquals(titleWidth, title.getWidth() + menuButton.getWidth());
    }

    @Test
    @MediumTest
    @Feature({"ContextualSuggestions"})
    public void testShadowVisibleOnScroll() throws InterruptedException, TimeoutException {
        forceShowSuggestions();
        openSheet();

        assertFalse("Shadow shouldn't be visible.", mModel.getToolbarShadowVisibility());

        int currentCallCount = mToolbarShadowVisibleCallback.getCallCount();

        ThreadUtils.runOnUiThreadBlocking(() -> {
            RecyclerView view =
                    (RecyclerView) mBottomSheet.getCurrentSheetContent().getContentView();
            view.scrollToPosition(5);
        });

        mToolbarShadowVisibleCallback.waitForCallback(currentCallCount);

        assertTrue("Shadow should be visible.", mModel.getToolbarShadowVisibility());
    }

    @Test
    @MediumTest
    @Feature({"ContextualSuggestions"})
    public void testInProductHelp() throws InterruptedException, TimeoutException {
        FakeTracker tracker = new FakeTracker(FeatureConstants.CONTEXTUAL_SUGGESTIONS_FEATURE);
        TrackerFactory.setTrackerForTests(tracker);
        forceShowSuggestions();

        assertTrue(
                "Help bubble should be showing.", mMediator.getHelpBubbleForTesting().isShowing());

        ThreadUtils.runOnUiThreadBlocking(() -> mMediator.getHelpBubbleForTesting().dismiss());

        Assert.assertEquals("Help bubble should be dimissed.", 1,
                tracker.mDimissedCallbackHelper.getCallCount());
    }

    @Test
    @LargeTest
    @Feature({"ContextualSuggestions"})
    public void testMultiInstanceMode() throws Exception {
        ChromeTabbedActivity activity1 = mActivityTestRule.getActivity();
        forceShowSuggestions();
        openSheet();

        MultiWindowUtils.getInstance().setIsInMultiWindowModeForTesting(true);
        ChromeTabbedActivity activity2 = MultiWindowTestHelper.createSecondChromeTabbedActivity(
                activity1, new LoadUrlParams(mTestServer.getURL(TEST_PAGE)));
        ChromeActivityTestRule.waitForActivityNativeInitializationComplete(activity2);

        CallbackHelper itemRangeInsertedCallback = new CallbackHelper();
        ThreadUtils.runOnUiThreadBlocking(() -> {
            mCoordinator2 = activity2.getContextualSuggestionsCoordinatorForTesting();
            mMediator2 = mCoordinator2.getMediatorForTesting();
            mModel2 = mCoordinator2.getModelForTesting();
            mBottomSheet2 = activity2.getBottomSheet();

            mModel2.mClusterListObservable.addObserver(new ListObserver() {
                @Override
                public void onItemRangeInserted(ListObservable source, int index, int count) {
                    itemRangeInsertedCallback.notifyCalled();
                }

                @Override
                public void onItemRangeRemoved(ListObservable source, int index, int count) {}

                @Override
                public void onItemRangeChanged(
                        ListObservable source, int index, int count, Object payload) {}

            });

            mMediator2.onEnabledStateChanged(true);
        });

        assertNotEquals("There should be two coordinators.", mCoordinator, mCoordinator2);
        assertNotEquals("There should be two mediators.", mMediator, mMediator2);
        assertNotEquals("There should be two models.", mModel, mModel2);
        assertEquals("There should have been two requests to create a ContextualSuggestionsSource",
                2,
                mContextualSuggestionsDeps.getFactory()
                        .createSuggestionsSourceCallback.getCallCount());

        itemRangeInsertedCallback.waitForCallback(0);
        assertEquals("Second model has incorrect number of items.",
                (int) FakeContextualSuggestionsSource.TOTAL_ITEM_COUNT,
                mModel2.getClusterList().getItemCount());

        ThreadUtils.runOnUiThreadBlocking(() -> {
            mMediator2.showContentInSheetForTesting(true, true);
            mBottomSheet2.endAnimations();

            ContextualSuggestionsBottomSheetContent content1 =
                    (ContextualSuggestionsBottomSheetContent) mBottomSheet.getCurrentSheetContent();
            ContextualSuggestionsBottomSheetContent content2 =
                    (ContextualSuggestionsBottomSheetContent)
                            mBottomSheet2.getCurrentSheetContent();
            assertNotEquals("There should be two bottom sheet contents", content1, content2);
        });

        assertEquals("Sheet in the second activity should be peeked.", BottomSheet.SHEET_STATE_PEEK,
                mBottomSheet2.getSheetState());
        assertEquals("Sheet in the first activity should be open.", BottomSheet.SHEET_STATE_FULL,
                mBottomSheet.getSheetState());

        ThreadUtils.runOnUiThreadBlocking(
                () -> mBottomSheet2.setSheetState(BottomSheet.SHEET_STATE_FULL, false));

        SnippetArticleViewHolder holder = getFirstSuggestionViewHolder(mBottomSheet2);
        String expectedUrl = holder.getUrl();
        ChromeTabUtils.invokeContextMenuAndOpenInOtherWindow(activity2, activity1, holder.itemView,
                ContextMenuManager.ID_OPEN_IN_NEW_WINDOW, false, expectedUrl);

        ThreadUtils.runOnUiThreadBlocking(() -> {
            mBottomSheet.endAnimations();
            mBottomSheet2.endAnimations();
        });

        assertTrue("Sheet in second activity should be opened.", mBottomSheet2.isSheetOpen());
        assertFalse("Sheet in first activity should be closed.", mBottomSheet.isSheetOpen());
    }

    @Test
    @MediumTest
    @Feature({"ContextualSuggestions", "UiCatalogue"})
    @DisableFeatures(FeatureConstants.CONTEXTUAL_SUGGESTIONS_FEATURE)
    @ParameterAnnotations.UseMethodParameter(SlimPeekUIParams.class)
    public void testCaptureContextualSuggestionsBottomSheet(boolean slimPeekUIEnabled)
            throws InterruptedException, TimeoutException {
        String postfix = slimPeekUIEnabled ? " (slim)" : "";

        forceShowSuggestions();
        BottomSheetTestRule.waitForWindowUpdates();
        mScreenShooter.shoot("Contextual suggestions: peeking" + postfix);

        ThreadUtils.runOnUiThreadBlocking(
                () -> mBottomSheet.setSheetState(BottomSheet.SHEET_STATE_HALF, false));
        BottomSheetTestRule.waitForWindowUpdates();
        mScreenShooter.shoot("Contextual suggestions: half height, images loading" + postfix);

        ThreadUtils.runOnUiThreadBlocking(() -> mFakeSource.runImageFetchCallbacks());
        BottomSheetTestRule.waitForWindowUpdates();
        mScreenShooter.shoot("Contextual suggestions: half height, images loaded" + postfix);

        ThreadUtils.runOnUiThreadBlocking(
                () -> mBottomSheet.setSheetState(BottomSheet.SHEET_STATE_FULL, false));
        BottomSheetTestRule.waitForWindowUpdates();
        mScreenShooter.shoot("Contextual suggestions: full height" + postfix);

        ThreadUtils.runOnUiThreadBlocking(() -> {
            RecyclerView view =
                    (RecyclerView) mBottomSheet.getCurrentSheetContent().getContentView();
            view.scrollToPosition(5);
        });
        BottomSheetTestRule.waitForWindowUpdates();
        mScreenShooter.shoot("Contextual suggestions: scrolled" + postfix);
    }

    @Test
    @MediumTest
    @Feature({"ContextualSuggestions", "RenderTest"})
    @DisableFeatures(FeatureConstants.CONTEXTUAL_SUGGESTIONS_FEATURE)
    @ParameterAnnotations.UseMethodParameter(SlimPeekUIParams.class)
    public void testRender(boolean slimPeekUIEnabled) throws Exception {
        // Force suggestions to populate in the bottom sheet, then render the peeking bar.
        forceShowSuggestions();
        BottomSheetTestRule.waitForWindowUpdates();
        mRenderTestRule.render(
                mBottomSheet.getCurrentSheetContent().getToolbarView(), "peeking_bar");

        // Open the sheet to cause the suggestions to be bound in the RecyclerView, then capture
        // a suggestion with its thumbnail loading.
        ThreadUtils.runOnUiThreadBlocking(
                () -> mBottomSheet.setSheetState(BottomSheet.SHEET_STATE_FULL, false));
        BottomSheetTestRule.waitForWindowUpdates();
        mRenderTestRule.render(getFirstSuggestionViewHolder().itemView, "suggestion_image_loading");

        // Run the image fetch callback so images load, then capture a suggestion with its
        // thumbnail loaded.
        ThreadUtils.runOnUiThreadBlocking(() -> mFakeSource.runImageFetchCallbacks());
        BottomSheetTestRule.waitForWindowUpdates();
        mRenderTestRule.render(getFirstSuggestionViewHolder().itemView, "suggestion_image_loaded");

        // Render a thumbnail with an offline badge.
        ThreadUtils.runOnUiThreadBlocking(
                () -> getSuggestionViewHolder(2).setOfflineBadgeVisibilityForTesting(true));
        mRenderTestRule.render(getSuggestionViewHolder(2).itemView, "suggestion_offline");

        // Render the full suggestions sheet.
        mRenderTestRule.render(mBottomSheet, "full_height");

        // Scroll the suggestions and render the full suggestions sheet.
        ThreadUtils.runOnUiThreadBlocking(() -> {
            RecyclerView view =
                    (RecyclerView) mBottomSheet.getCurrentSheetContent().getContentView();
            view.scrollToPosition(5);
        });
        BottomSheetTestRule.waitForWindowUpdates();
        mRenderTestRule.render(mBottomSheet, "full_height_scrolled");
    }

    @Test
    @MediumTest
    @Feature({"ContextualSuggestions"})
    public void testPeekWithPageScrollPercentage() throws Exception {
        CallbackHelper scrollChangedCallback = new CallbackHelper();
        GestureStateListener gestureStateListener = new GestureStateListener() {
            @Override
            public void onScrollOffsetOrExtentChanged(int scrollOffsetY, int scrollExtentY) {
                scrollChangedCallback.notifyCalled();
            }
        };
        WebContents webContents = mActivityTestRule.getWebContents();
        GestureListenerManager.fromWebContents(webContents).addListener(gestureStateListener);
        View view = webContents.getViewAndroidDelegate().getContainerView();

        // Verify that suggestions are not shown before scroll.
        ThreadUtils.runOnUiThreadBlocking(
                () -> mMediator.showContentInSheetForTesting(false, true));
        assertEquals("Bottom sheet should be hidden before scroll.", BottomSheet.SHEET_STATE_HIDDEN,
                mBottomSheet.getSheetState());

        // Scroll the page to 30% and verify that the suggestions are not shown. The pixel to scroll
        // is hard coded (approximately) based on the html height of the TEST_PAGE.
        int callCount = scrollChangedCallback.getCallCount();
        ThreadUtils.runOnUiThreadBlocking(() -> view.scrollBy(0, 3000));
        scrollChangedCallback.waitForCallback(callCount);

        // Simulate call to show content without browser controls being hidden.
        ThreadUtils.runOnUiThreadBlocking(
                () -> mMediator.showContentInSheetForTesting(false, true));
        assertEquals("Bottom sheet should be hidden on 30% scroll percentage.",
                BottomSheet.SHEET_STATE_HIDDEN, mBottomSheet.getSheetState());

        // Scroll the page to approximately 60% and verify that the suggestions are shown.
        callCount = scrollChangedCallback.getCallCount();
        ThreadUtils.runOnUiThreadBlocking(() -> view.scrollBy(0, 3000));
        scrollChangedCallback.waitForCallback(callCount);

        // Simulate call to show content without browser controls being hidden.
        ThreadUtils.runOnUiThreadBlocking(
                () -> mMediator.showContentInSheetForTesting(false, true));
        assertEquals("Bottom sheet should be shown on >=50% scroll percentage.",
                BottomSheet.SHEET_STATE_PEEK, mBottomSheet.getSheetState());

        GestureListenerManager.fromWebContents(webContents).removeListener(gestureStateListener);
    }

    @Test
    @MediumTest
    @Feature({"ContextualSuggestions"})
    public void testPeekDelay() throws Exception {
        // Close the suggestions from setUp().
        ThreadUtils.runOnUiThreadBlocking(() -> {
            mMediator.clearState();
            mBottomSheet.endAnimations();
        });

        // Request suggestions with fetch time baseline set for testing.
        long startTime = SystemClock.uptimeMillis();
        FetchHelper.setFetchTimeBaselineMillisForTesting(startTime);
        ThreadUtils.runOnUiThreadBlocking(
                () -> mMediator.requestSuggestions("http://www.testurl.com"));
        assertEquals("Bottom sheet should be hidden before delay.", BottomSheet.SHEET_STATE_HIDDEN,
                mBottomSheet.getSheetState());

        // Simulate user scroll by calling showContentInSheet until the sheet is peeked.
        CriteriaHelper.pollUiThread(() -> {
            mMediator.showContentInSheetForTesting(true, false);
            mBottomSheet.endAnimations();
            return mBottomSheet.getSheetState() == BottomSheet.SHEET_STATE_PEEK;
        });

        // Verify that suggestions is shown after the expected delay.
        long duration = SystemClock.uptimeMillis() - startTime;
        long expected = FakeContextualSuggestionsSource.TEST_PEEK_DELAY_SECONDS * 1000;
        assertTrue(String.format(Locale.US,
                        "The peek delay should be greater than %d ms, but was %d ms.",
                        expected, duration),
                duration >= expected);
    }

    @Test
    @MediumTest
    @Feature({"ContextualSuggestions"})
    public void testPeekCount() throws Exception {
        forceShowSuggestions();

        // Opening the sheet and setting it back to peek state shouldn't affect the peek count.
        setSheetOffsetForState(BottomSheet.SHEET_STATE_FULL);
        setSheetOffsetForState(BottomSheet.SHEET_STATE_PEEK);

        // Hide and peek the bottom sheet for (TEST_PEEK_COUNT - 1) number of times, since
        // #forceShowSuggestions() has already peeked the bottom sheet once.
        for (int i = 1; i < FakeContextualSuggestionsSource.TEST_PEEK_COUNT; ++i) {
            setSheetOffsetForState(BottomSheet.SHEET_STATE_HIDDEN);

            // Verify that the suggestions are not cleared.
            assertEquals("Model has incorrect number of items.",
                    (int) FakeContextualSuggestionsSource.TOTAL_ITEM_COUNT,
                    mModel.getClusterList().getItemCount());
            assertNotNull("Bottom sheet contents should not be null.",
                    mBottomSheet.getCurrentSheetContent());

            setSheetOffsetForState(BottomSheet.SHEET_STATE_PEEK);
        }

        // Hide the sheet and verify that the suggestions are cleared.
        setSheetOffsetForState(BottomSheet.SHEET_STATE_HIDDEN);
        assertEquals("Model should be empty.", 0, mModel.getClusterList().getItemCount());
        assertNull("Bottom sheet contents should be null.", mBottomSheet.getCurrentSheetContent());
    }

    private void forceShowSuggestions() throws InterruptedException, TimeoutException {
        assertEquals("Model has incorrect number of items.",
                (int) FakeContextualSuggestionsSource.TOTAL_ITEM_COUNT,
                mModel.getClusterList().getItemCount());

        ThreadUtils.runOnUiThreadBlocking(() -> {
            mMediator.showContentInSheetForTesting(true, true);
            mBottomSheet.endAnimations();

            assertEquals("Sheet should be peeked.", BottomSheet.SHEET_STATE_PEEK,
                    mBottomSheet.getSheetState());
            assertTrue("Bottom sheet should contain suggestions content.",
                    mBottomSheet.getCurrentSheetContent()
                                    instanceof ContextualSuggestionsBottomSheetContent);
        });
    }

    private void simulateClickOnCloseButton() {
        ThreadUtils.runOnUiThreadBlocking(() -> {
            mBottomSheet.getCurrentSheetContent()
                    .getToolbarView()
                    .findViewById(R.id.close_button)
                    .performClick();
            mBottomSheet.endAnimations();
        });

        assertEquals("Sheet should be hidden.", BottomSheet.SHEET_STATE_HIDDEN,
                mBottomSheet.getSheetState());
        assertNull("Bottom sheet contents should be null.", mBottomSheet.getCurrentSheetContent());
    }

    private void openSheet() {
        ThreadUtils.runOnUiThreadBlocking(
                () -> mBottomSheet.setSheetState(BottomSheet.SHEET_STATE_FULL, false));
    }

    private void setSheetOffsetForState(@BottomSheet.SheetState int state) {
        ThreadUtils.runOnUiThreadBlocking(() -> {
            mBottomSheet.setSheetOffsetFromBottomForTesting(
                    mBottomSheet.getSheetHeightForState(state));
            mBottomSheet.endAnimations();
        });
    }

    private SnippetArticleViewHolder getFirstSuggestionViewHolder() {
        return getFirstSuggestionViewHolder(mBottomSheet);
    }

    private SnippetArticleViewHolder getFirstSuggestionViewHolder(BottomSheet bottomSheet) {
        return getSuggestionViewHolder(bottomSheet, 0);
    }

    private SnippetArticleViewHolder getSuggestionViewHolder(int index) {
        return getSuggestionViewHolder(mBottomSheet, index);
    }

    private SnippetArticleViewHolder getSuggestionViewHolder(BottomSheet bottomSheet, int index) {
        ContextualSuggestionsBottomSheetContent content =
                (ContextualSuggestionsBottomSheetContent) bottomSheet.getCurrentSheetContent();
        RecyclerView recyclerView = (RecyclerView) content.getContentView();

        RecyclerViewTestUtils.waitForStableRecyclerView(recyclerView);

        return (SnippetArticleViewHolder) recyclerView.findViewHolderForAdapterPosition(index);
    }
}
