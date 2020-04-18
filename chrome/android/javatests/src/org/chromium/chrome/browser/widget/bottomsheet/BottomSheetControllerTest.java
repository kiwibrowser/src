// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget.bottomsheet;

import static org.junit.Assert.assertEquals;

import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.support.test.filters.SmallTest;
import android.view.ViewGroup;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.Restriction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.EmptyTabModelObserver;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabModel.TabSelectionType;
import org.chromium.chrome.browser.widget.FadingBackgroundView;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheet.BottomSheetContent;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheet.ContentPriority;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.chrome.test.util.ChromeTabUtils;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.ui.test.util.UiRestriction;

import java.util.concurrent.TimeoutException;

/**
 * This class contains tests for the logic that shows and hides the bottom sheet as a result of
 * different browser events. These tests use a bottom sheet and controller different from the ones
 * created by the activity that are used by different experiments.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add(ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE)
@Restriction(UiRestriction.RESTRICTION_TYPE_PHONE) // TODO(mdjones): Remove this (crbug.com/837838).
public class BottomSheetControllerTest {
    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();

    private BottomSheet mBottomSheet;
    private BottomSheetController mSheetController;
    private TestBottomSheetContent mLowPriorityContent;
    private TestBottomSheetContent mHighPriorityContent;

    @Before
    public void setUp() throws Exception {
        mActivityTestRule.startMainActivityOnBlankPage();
        final ChromeTabbedActivity activity = mActivityTestRule.getActivity();

        ThreadUtils.runOnUiThreadBlocking(() -> {
            ViewGroup coordinator = activity.findViewById(org.chromium.chrome.R.id.coordinator);
            mBottomSheet = activity.getLayoutInflater()
                                   .inflate(org.chromium.chrome.R.layout.bottom_sheet, coordinator)
                                   .findViewById(org.chromium.chrome.R.id.bottom_sheet)
                                   .findViewById(org.chromium.chrome.R.id.bottom_sheet);
            mBottomSheet.init(coordinator, activity);

            FadingBackgroundView scrim = coordinator.findViewById(R.id.fading_focus_target);

            mSheetController = new BottomSheetController(activity, activity.getTabModelSelector(),
                    activity.getCompositorViewHolder().getLayoutManager(), scrim,
                    activity.getContextualSearchManager(), mBottomSheet);

            mLowPriorityContent = new TestBottomSheetContent(
                    mActivityTestRule.getActivity(), ContentPriority.LOW);
            mHighPriorityContent = new TestBottomSheetContent(
                    mActivityTestRule.getActivity(), ContentPriority.HIGH);
        });
    }

    @Test
    @SmallTest
    @Feature({"BottomSheetController"})
    public void testSheetPeek() throws InterruptedException, TimeoutException {
        requestContentInSheet(mLowPriorityContent, true);
        assertEquals("The bottom sheet should be peeking.", BottomSheet.SHEET_STATE_PEEK,
                mBottomSheet.getSheetState());
        assertEquals("The bottom sheet is showing incorrect content.", mLowPriorityContent,
                mBottomSheet.getCurrentSheetContent());
    }

    @Test
    @SmallTest
    @Feature({"BottomSheetController"})
    public void testSheetPriorityInPeekState() throws InterruptedException, TimeoutException {
        requestContentInSheet(mLowPriorityContent, true);
        requestContentInSheet(mHighPriorityContent, true);
        assertEquals("The bottom sheet is showing incorrect content.", mHighPriorityContent,
                mBottomSheet.getCurrentSheetContent());
    }

    @Test
    @SmallTest
    @Feature({"BottomSheetController"})
    public void testSheetPriorityInExpandedState() throws InterruptedException, TimeoutException {
        requestContentInSheet(mLowPriorityContent, true);
        expandSheet();
        requestContentInSheet(mHighPriorityContent, false);
        assertEquals("The bottom sheet is showing incorrect content.", mLowPriorityContent,
                mBottomSheet.getCurrentSheetContent());
    }

    @Test
    @MediumTest
    @Feature({"BottomSheetController"})
    public void testSheetPeekAfterTabSwitcher() throws InterruptedException, TimeoutException {
        requestContentInSheet(mLowPriorityContent, true);
        enterAndExitTabSwitcher();
        assertEquals("The bottom sheet should be peeking.", BottomSheet.SHEET_STATE_PEEK,
                mBottomSheet.getSheetState());
        assertEquals("The bottom sheet is showing incorrect content.", mLowPriorityContent,
                mBottomSheet.getCurrentSheetContent());
    }

    @Test
    @MediumTest
    @Feature({"BottomSheetController"})
    public void testSheetHiddenAfterTabSwitcher() throws InterruptedException, TimeoutException {
        // Open a second tab and then reselect the original activity tab.
        Tab tab1 = mActivityTestRule.getActivity().getActivityTab();
        ChromeTabUtils.newTabFromMenu(
                InstrumentationRegistry.getInstrumentation(), mActivityTestRule.getActivity());
        Tab tab2 = mActivityTestRule.getActivity().getActivityTab();

        requestContentInSheet(mLowPriorityContent, true);

        // Enter the tab switcher and select a different tab.
        ThreadUtils.runOnUiThreadBlocking(() -> {
            mActivityTestRule.getActivity().getLayoutManager().showOverview(false);
            mBottomSheet.endAnimations();
            assertEquals("The bottom sheet should be hidden.", BottomSheet.SHEET_STATE_HIDDEN,
                    mBottomSheet.getSheetState());
            mActivityTestRule.getActivity().getTabModelSelector().getCurrentModel().setIndex(
                    0, TabSelectionType.FROM_USER);
            mActivityTestRule.getActivity().getLayoutManager().hideOverview(false);
            mBottomSheet.endAnimations();
        });

        assertEquals("The bottom sheet still should be hidden.", BottomSheet.SHEET_STATE_HIDDEN,
                mBottomSheet.getSheetState());
        assertEquals("The bottom sheet is showing incorrect content.", null,
                mBottomSheet.getCurrentSheetContent());
    }

    @Test
    @MediumTest
    @Feature({"BottomSheetController"})
    public void testOpenTabInBackground() throws InterruptedException, TimeoutException {
        requestContentInSheet(mLowPriorityContent, true);
        expandSheet();
        openNewTabInBackground();

        assertEquals("The bottom sheet should be expanded.", BottomSheet.SHEET_STATE_HALF,
                mBottomSheet.getSheetState());
        assertEquals("The bottom sheet is showing incorrect content.", mLowPriorityContent,
                mBottomSheet.getCurrentSheetContent());
    }

    @Test
    @MediumTest
    @Feature({"BottomSheetController"})
    public void testSwitchTabs() throws InterruptedException, TimeoutException {
        requestContentInSheet(mLowPriorityContent, true);

        assertEquals("The bottom sheet should be peeking.", BottomSheet.SHEET_STATE_PEEK,
                mBottomSheet.getSheetState());

        openNewTabInForeground();

        assertEquals("The bottom sheet should be hidden.", BottomSheet.SHEET_STATE_HIDDEN,
                mBottomSheet.getSheetState());
        assertEquals("The bottom sheet is showing incorrect content.", null,
                mBottomSheet.getCurrentSheetContent());
    }

    @Test
    @MediumTest
    @Feature({"BottomSheetController"})
    public void testSwitchTabsMultipleTimes() throws InterruptedException, TimeoutException {
        ChromeTabbedActivity activity = mActivityTestRule.getActivity();
        final int originalTabIndex =
                activity.getTabModelSelector().getCurrentModel().indexOf(activity.getActivityTab());
        requestContentInSheet(mLowPriorityContent, true);

        assertEquals("The bottom sheet should be peeking.", BottomSheet.SHEET_STATE_PEEK,
                mBottomSheet.getSheetState());

        openNewTabInForeground();

        assertEquals("The bottom sheet should be hidden.", BottomSheet.SHEET_STATE_HIDDEN,
                mBottomSheet.getSheetState());
        assertEquals("The bottom sheet is showing incorrect content.", null,
                mBottomSheet.getCurrentSheetContent());

        ThreadUtils.runOnUiThreadBlocking(() -> {
            activity.getTabModelSelector().getCurrentModel().setIndex(
                    originalTabIndex, TabModel.TabSelectionType.FROM_USER);
        });

        // Request content be shown again.
        requestContentInSheet(mLowPriorityContent, true);
        expandSheet();

        openNewTabInBackground();

        assertEquals("The bottom sheet should be expanded.", BottomSheet.SHEET_STATE_HALF,
                mBottomSheet.getSheetState());
        assertEquals("The bottom sheet is showing incorrect content.", mLowPriorityContent,
                mBottomSheet.getCurrentSheetContent());
    }

    /**
     * Request content be shown in the bottom sheet and end animations.
     * @param content The content to show.
     * @param expectContentChange If the content is expected to change, setting this to true will
     *                            cause the method to wait for
     *                            BottomSheetObserver#onSheetContentChanged.
     */
    private void requestContentInSheet(BottomSheetContent content, boolean expectContentChange)
            throws InterruptedException, TimeoutException {
        CallbackHelper contentChangedHelper = new CallbackHelper();
        mBottomSheet.addObserver(new EmptyBottomSheetObserver() {
            @Override
            public void onSheetContentChanged(BottomSheetContent content) {
                contentChangedHelper.notifyCalled();
            }
        });
        int currentCallCount = contentChangedHelper.getCallCount();
        ThreadUtils.runOnUiThreadBlocking(
                () -> { mSheetController.requestShowContent(content, false); });

        if (expectContentChange) contentChangedHelper.waitForCallback(currentCallCount, 1);
    }

    /**
     * Expand the bottom sheet to a non-peek height. If the sheet has no content, an assert is
     * thrown.
     */
    private void expandSheet() {
        ThreadUtils.runOnUiThreadBlocking(
                () -> mBottomSheet.setSheetState(BottomSheet.SHEET_STATE_HALF, false));
    }

    /**
     * Enter and immediately exit the tab switcher. This function will assert that the sheet is not
     * showing in the tab switcher.
     */
    private void enterAndExitTabSwitcher() {
        ThreadUtils.runOnUiThreadBlocking(() -> {
            mActivityTestRule.getActivity().getLayoutManager().showOverview(false);
            mBottomSheet.endAnimations();
            assertEquals("The bottom sheet should be hidden.", BottomSheet.SHEET_STATE_HIDDEN,
                    mBottomSheet.getSheetState());
            mActivityTestRule.getActivity().getLayoutManager().hideOverview(false);
            mBottomSheet.endAnimations();
        });
    }

    /**
     * Open a new tab behind the active tab and wait for the tab selection event.
     */
    private void openNewTabInBackground() throws InterruptedException, TimeoutException {
        CallbackHelper tabSelectedHelper = new CallbackHelper();
        mActivityTestRule.getActivity().getTabModelSelector().getCurrentModel().addObserver(
                new EmptyTabModelObserver() {
                    @Override
                    public void didSelectTab(Tab tab, TabModel.TabSelectionType type, int lastId) {
                        tabSelectedHelper.notifyCalled();
                    }
                });

        int previousCallCount = tabSelectedHelper.getCallCount();

        ThreadUtils.runOnUiThreadBlocking(() -> {
            mActivityTestRule.getActivity().getTabCreator(false).createNewTab(
                    new LoadUrlParams("about:blank"),
                    TabModel.TabLaunchType.FROM_LONGPRESS_BACKGROUND, null);
        });

        tabSelectedHelper.waitForCallback(previousCallCount, 1);
        ThreadUtils.runOnUiThreadBlocking(() -> mBottomSheet.endAnimations());
    }

    /**
     * Open a new tab in front of the active tab and wait for it to be completely loaded.
     */
    private void openNewTabInForeground() throws InterruptedException, TimeoutException {
        ChromeTabUtils.fullyLoadUrlInNewTab(InstrumentationRegistry.getInstrumentation(),
                mActivityTestRule.getActivity(), "about:blank", false);
        ThreadUtils.runOnUiThreadBlocking(() -> mBottomSheet.endAnimations());
    }
}
