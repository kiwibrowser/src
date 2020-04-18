// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextual_suggestions;

import static org.junit.Assert.assertEquals;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowSystemClock;

import org.chromium.base.metrics.test.ShadowRecordHistogram;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabObserver;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabModel.TabSelectionType;
import org.chromium.chrome.browser.tabmodel.TabModelObserver;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.util.test.ShadowUrlUtilities;

import java.util.ArrayList;
import java.util.List;

/** Unit tests for PageViewTimer. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE, shadows = {ShadowRecordHistogram.class, ShadowUrlUtilities.class})
public final class PageViewTimerTest {
    private static final String STARTING_URL = "http://starting.url";
    private static final String DIFFERENT_URL = "http://different.url";
    private static final int SAMPLE_PAGE_VIEW_TIME = 5678;
    private static final String PAGE_VIEW_TIME_METRIC = "ContextualSuggestions.PageViewTime";

    @Mock
    private TabModelSelector mTabModelSelector;
    @Mock
    private TabModel mTabModel;
    @Mock
    private Tab mTab;
    @Mock
    private Tab mTab2;
    @Captor
    private ArgumentCaptor<TabObserver> mTabObserverCaptor;
    @Captor
    private ArgumentCaptor<TabModelObserver> mTabModelObserverCaptor;

    private PageViewTimer mTimer;

    private TabObserver getTabObserver() {
        return mTabObserverCaptor.getValue();
    }

    private TabModelObserver getTabModelObserver() {
        return mTabModelObserverCaptor.getValue();
    }

    @Before
    public void setUp() {
        ShadowRecordHistogram.reset();
        MockitoAnnotations.initMocks(this);

        // Default tab setup.
        doReturn(false).when(mTab).isIncognito();
        doReturn(true).when(mTab).isLoading();
        doReturn(77).when(mTab).getId();
        doReturn(STARTING_URL).when(mTab).getUrl();

        // Default tab model selector setup.
        List<TabModel> tabModels = new ArrayList<>();
        tabModels.add(mTabModel);
        doReturn(tabModels).when(mTabModelSelector).getModels();
        doReturn(mTab).when(mTabModelSelector).getCurrentTab();
    }

    @Test
    public void createPageViewTimer_withNoTab_noCrash() {
        doReturn(null).when(mTabModelSelector).getCurrentTab();
        PageViewTimer timer = createPageViewTimer();
    }

    @Test
    public void selectTab_didFirstVisuallyNonEmptyPaint_reported() {
        selectTab_showContent_stopTimer(
                showContentByDidFirstVisuallyNonEmptyPaint(mTab), stopTimerByClosingTab(mTab), 1);
    }

    @Test
    public void selectTab_onPageLoadFinished_close_reported() {
        selectTab_showContent_stopTimer(
                showContentByOnPageLoadFinished(mTab), stopTimerByClosingTab(mTab), 1);
    }

    @Test
    public void selectTab_onLoadStopped_close_reported() {
        selectTab_showContent_stopTimer(
                showContentByOnLoadStopped(mTab), stopTimerByClosingTab(mTab), 1);
    }

    @Test
    public void selectTab_whenLoaded_close_reported() {
        doReturn(false).when(mTab).isLoading();
        selectTab_showContent_stopTimer(doNothingToShowContent(), stopTimerByClosingTab(mTab), 1);
    }

    @Test
    public void selectTab_notLoaded_close_ignored() {
        selectTab_showContent_stopTimer(doNothingToShowContent(), stopTimerByClosingTab(mTab), 0);
    }

    @Test
    public void selectTab_didFirstVisuallyNonEmptyPaint_onUpdateUrl_reported() {
        selectTab_showContent_stopTimer(showContentByDidFirstVisuallyNonEmptyPaint(mTab),
                stopTimerByOnUpdateUrl(mTab, DIFFERENT_URL), 1);
    }

    @Test
    public void selectTab_onPageLoadFinished_onUpdateUrl_reported() {
        selectTab_showContent_stopTimer(showContentByOnPageLoadFinished(mTab),
                stopTimerByOnUpdateUrl(mTab, DIFFERENT_URL), 1);
    }

    @Test
    public void selectTab_onLoadStopped_onUpdateUrl_reported() {
        selectTab_showContent_stopTimer(
                showContentByOnLoadStopped(mTab), stopTimerByOnUpdateUrl(mTab, DIFFERENT_URL), 1);
    }

    @Test
    public void selectTab_whenLoaded_onUpdateUrl_reported() {
        doReturn(false).when(mTab).isLoading();
        selectTab_showContent_stopTimer(
                doNothingToShowContent(), stopTimerByOnUpdateUrl(mTab, DIFFERENT_URL), 1);
    }

    @Test
    public void selectTab_notLoaded_onUpdateUrl_ignored() {
        selectTab_showContent_stopTimer(
                doNothingToShowContent(), stopTimerByOnUpdateUrl(mTab, DIFFERENT_URL), 0);
    }

    @Test
    public void selectTab_whenLoaded_onUpdateUrlToSame_ignored() {
        doReturn(false).when(mTab).isLoading();
        selectTab_showContent_stopTimer(
                doNothingToShowContent(), stopTimerByOnUpdateUrl(mTab, STARTING_URL), 0);
    }

    @Test
    public void selectTab_whenNonHttpOrHttpsUrlLoaded_tabRemoved_ignored() {
        doReturn(false).when(mTab).isLoading();
        doReturn("chrome://snippets-internals").when(mTab).getUrl();
        selectTab_showContent_stopTimer(doNothingToShowContent(), stopTimerByRemovingTab(mTab), 0);
    }

    @Test
    public void selectTab_whenLoaded_switchTabs_reported() {
        doReturn(false).when(mTab).isLoading();
        selectTab_showContent_stopTimer(
                doNothingToShowContent(), stopTimerBySwitchingTab(mTab, mTab2), 1);
    }

    @Test
    public void selectTab_notLoaded_switchTabs_ignored() {
        selectTab_showContent_stopTimer(
                doNothingToShowContent(), stopTimerBySwitchingTab(mTab, mTab2), 0);
    }

    @Test
    public void selectTab_whenLoaded_tabRemoved_reported() {
        doReturn(false).when(mTab).isLoading();
        selectTab_showContent_stopTimer(doNothingToShowContent(), stopTimerByRemovingTab(mTab), 1);
    }

    @Test
    public void selectTab_notLoaded_tabRemoved_ignored() {
        selectTab_showContent_stopTimer(doNothingToShowContent(), stopTimerByRemovingTab(mTab), 0);
    }

    @Test
    public void selectTab_whenLoaded_closeChrome_reported() {
        doReturn(false).when(mTab).isLoading();
        selectTab_showContent_stopTimer(doNothingToShowContent(), stopTimerByClosingChrome(), 1);
    }

    @Test
    public void selectTab_notLoaded_closeChrome_reported() {
        selectTab_showContent_stopTimer(doNothingToShowContent(), stopTimerByClosingChrome(), 0);
    }

    private Runnable doNothingToShowContent() {
        return () -> {};
    }

    private Runnable showContentByDidFirstVisuallyNonEmptyPaint(Tab tab) {
        return () -> getTabObserver().didFirstVisuallyNonEmptyPaint(tab);
    }
    private Runnable showContentByOnPageLoadFinished(Tab tab) {
        return () -> getTabObserver().onPageLoadFinished(tab);
    }

    private Runnable showContentByOnLoadStopped(Tab tab) {
        return () -> getTabObserver().onLoadStopped(tab, /*toDifferentDocument=*/false);
    }

    private Runnable stopTimerByClosingTab(Tab tab) {
        return () -> getTabModelObserver().willCloseTab(tab, /*animate=*/false);
    }

    private Runnable stopTimerByOnUpdateUrl(Tab tab, String url) {
        return () -> getTabObserver().onUpdateUrl(tab, url);
    }

    private Runnable stopTimerByRemovingTab(Tab tab) {
        return () -> getTabModelObserver().tabRemoved(tab);
    }

    private Runnable stopTimerBySwitchingTab(Tab fromTab, Tab toTab) {
        return () -> switchTabs(fromTab, toTab);
    }

    private Runnable stopTimerByClosingChrome() {
        return () -> mTimer.destroy();
    }

    private void selectTab_showContent_stopTimer(
            Runnable showContentRunnable, Runnable stopTimerRunnable, int expectedSamples) {
        mTimer = createPageViewTimer();
        switchTabs(null, mTab);
        showContentRunnable.run();
        ShadowSystemClock.sleep(SAMPLE_PAGE_VIEW_TIME);
        stopTimerRunnable.run();
        assertEquals(expectedSamples,
                ShadowRecordHistogram.getHistogramValueCountForTesting(
                        PAGE_VIEW_TIME_METRIC, SAMPLE_PAGE_VIEW_TIME));
    }

    public PageViewTimer createPageViewTimer() {
        PageViewTimer timer = new PageViewTimer(mTabModelSelector);
        verify(mTabModel, times(1)).addObserver(mTabModelObserverCaptor.capture());
        return timer;
    }

    private void switchTabs(Tab fromTab, Tab toTab) {
        getTabModelObserver().didSelectTab(toTab, TabSelectionType.FROM_USER, 0);
        if (fromTab != null) {
            verify(fromTab, times(1)).removeObserver(eq(getTabObserver()));
        }
        if (toTab != null) {
            verify(toTab, times(1)).addObserver(mTabObserverCaptor.capture());
        }
    }
}
