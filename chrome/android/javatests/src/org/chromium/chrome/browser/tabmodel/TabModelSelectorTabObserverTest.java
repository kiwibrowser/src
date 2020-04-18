// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.tabmodel;

import android.support.test.annotation.UiThreadTest;
import android.support.test.filters.SmallTest;
import android.support.test.rule.UiThreadTestRule;

import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.RuleChain;
import org.junit.runner.RunWith;

import org.chromium.base.ObserverList;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabObserver;
import org.chromium.chrome.browser.tab.TabTestUtils;
import org.chromium.chrome.browser.tabmodel.TabModel.TabLaunchType;
import org.chromium.content_public.browser.LoadUrlParams;

/**
 * Tests for the TabModelSelectorTabObserver.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class TabModelSelectorTabObserverTest {
    // Do not add @Rule to this, it's already added to RuleChain
    private final TabModelSelectorObserverTestRule mTestRule =
            new TabModelSelectorObserverTestRule();

    @Rule
    public final RuleChain mChain = RuleChain.outerRule(mTestRule).around(new UiThreadTestRule());

    @Test
    @UiThreadTest
    @SmallTest
    public void testAddingTab() {
        TestTabModelSelectorTabObserver observer =
                new TestTabModelSelectorTabObserver(mTestRule.getSelector());
        Tab tab = createTestTab(false);
        assertTabDoesNotHaveObserver(tab, observer);
        mTestRule.getNormalTabModel().addTab(tab, 0, TabModel.TabLaunchType.FROM_LINK);
        assertTabHasObserver(tab, observer);
    }

    @Test
    @UiThreadTest
    @SmallTest
    public void testClosingTab() {
        TestTabModelSelectorTabObserver observer =
                new TestTabModelSelectorTabObserver(mTestRule.getSelector());
        Tab tab = createTestTab(false);
        mTestRule.getNormalTabModel().addTab(tab, 0, TabModel.TabLaunchType.FROM_LINK);
        assertTabHasObserver(tab, observer);
        mTestRule.getNormalTabModel().closeTab(tab);
        assertTabDoesNotHaveObserver(tab, observer);
    }

    @Test
    @UiThreadTest
    @SmallTest
    public void testRemovingTab() {
        TestTabModelSelectorTabObserver observer =
                new TestTabModelSelectorTabObserver(mTestRule.getSelector());
        Tab tab = createTestTab(false);
        mTestRule.getNormalTabModel().addTab(tab, 0, TabModel.TabLaunchType.FROM_LINK);
        assertTabHasObserver(tab, observer);
        mTestRule.getNormalTabModel().removeTab(tab);
        assertTabDoesNotHaveObserver(tab, observer);
    }

    @Test
    @UiThreadTest
    @SmallTest
    public void testPreExistingTabs() {
        Tab normalTab1 = createTestTab(false);
        mTestRule.getNormalTabModel().addTab(normalTab1, 0, TabModel.TabLaunchType.FROM_LINK);
        Tab normalTab2 = createTestTab(false);
        mTestRule.getNormalTabModel().addTab(normalTab2, 1, TabModel.TabLaunchType.FROM_LINK);

        Tab incognitoTab1 = createTestTab(true);
        mTestRule.getIncognitoTabModel().addTab(incognitoTab1, 0, TabModel.TabLaunchType.FROM_LINK);
        Tab incognitoTab2 = createTestTab(true);
        mTestRule.getIncognitoTabModel().addTab(incognitoTab2, 1, TabModel.TabLaunchType.FROM_LINK);

        TestTabModelSelectorTabObserver observer =
                new TestTabModelSelectorTabObserver(mTestRule.getSelector());
        assertTabHasObserver(normalTab1, observer);
        assertTabHasObserver(normalTab2, observer);
        assertTabHasObserver(incognitoTab1, observer);
        assertTabHasObserver(incognitoTab2, observer);
    }

    @Test
    @UiThreadTest
    @SmallTest
    public void testDestroyRemovesObserver() {
        Tab normalTab1 = createTestTab(false);
        mTestRule.getNormalTabModel().addTab(normalTab1, 0, TabModel.TabLaunchType.FROM_LINK);
        Tab incognitoTab1 = createTestTab(true);
        mTestRule.getIncognitoTabModel().addTab(incognitoTab1, 0, TabModel.TabLaunchType.FROM_LINK);

        TestTabModelSelectorTabObserver observer =
                new TestTabModelSelectorTabObserver(mTestRule.getSelector());
        assertTabHasObserver(normalTab1, observer);
        assertTabHasObserver(incognitoTab1, observer);

        observer.destroy();
        assertTabDoesNotHaveObserver(normalTab1, observer);
        assertTabDoesNotHaveObserver(incognitoTab1, observer);
    }

    @Test
    @UiThreadTest
    @SmallTest
    public void testObserverAddedBeforeInitialize() {
        TabModelSelectorBase selector = new TabModelSelectorBase() {
            @Override
            public Tab openNewTab(LoadUrlParams loadUrlParams, TabLaunchType type, Tab parent,
                    boolean incognito) {
                return null;
            }
        };
        TestTabModelSelectorTabObserver observer =
                new TestTabModelSelectorTabObserver(mTestRule.getSelector());
        selector.initialize(false, mTestRule.getNormalTabModel(), mTestRule.getIncognitoTabModel());

        Tab normalTab1 = createTestTab(false);
        mTestRule.getNormalTabModel().addTab(normalTab1, 0, TabModel.TabLaunchType.FROM_LINK);
        assertTabHasObserver(normalTab1, observer);

        Tab incognitoTab1 = createTestTab(true);
        mTestRule.getIncognitoTabModel().addTab(incognitoTab1, 0, TabModel.TabLaunchType.FROM_LINK);
        assertTabHasObserver(incognitoTab1, observer);
    }

    private Tab createTestTab(boolean incognito) {
        Tab testTab = new Tab(Tab.INVALID_TAB_ID, incognito, mTestRule.getWindowAndroid());
        testTab.initializeNative();
        return testTab;
    }

    private static class TestTabModelSelectorTabObserver extends TabModelSelectorTabObserver {
        public TestTabModelSelectorTabObserver(TabModelSelectorBase selector) {
            super(selector);
        }
    }

    private void assertTabHasObserver(Tab tab, TabObserver observer) {
        ObserverList.RewindableIterator<TabObserver> tabObservers =
                TabTestUtils.getTabObservers(tab);
        tabObservers.rewind();
        boolean containsObserver = false;
        while (tabObservers.hasNext()) {
            if (tabObservers.next().equals(observer)) {
                containsObserver = true;
                break;
            }
        }
        Assert.assertTrue(containsObserver);
    }

    private void assertTabDoesNotHaveObserver(Tab tab, TabObserver observer) {
        ObserverList.RewindableIterator<TabObserver> tabObservers =
                TabTestUtils.getTabObservers(tab);
        tabObservers.rewind();
        while (tabObservers.hasNext()) {
            Assert.assertNotEquals(tabObservers.next(), observer);
        }
    }
}
