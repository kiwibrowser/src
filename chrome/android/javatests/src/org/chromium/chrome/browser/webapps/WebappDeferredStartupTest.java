// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.webapps;

import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.DeferredStartupHandler;
import org.chromium.chrome.browser.tab.EmptyTabObserver;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.EmptyTabModelSelectorObserver;
import org.chromium.chrome.browser.tabmodel.TabModelSelectorBase;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

/**
 * Tests that when WebappActivity#onDeferredStartup() is run, the activity tab has finished loading.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class WebappDeferredStartupTest {
    static class PageLoadFinishedTabObserver extends EmptyTabObserver {
        private boolean mIsPageLoadFinished;

        @Override
        public void onPageLoadFinished(Tab tab) {
            mIsPageLoadFinished = true;
        }

        public boolean isPageLoadFinished() {
            return mIsPageLoadFinished;
        }
    }

    static class NewTabCreatedTabModelSelectorObserver extends EmptyTabModelSelectorObserver {
        public NewTabCreatedTabModelSelectorObserver(PageLoadFinishedTabObserver observer) {
            mObserver = observer;
        }

        @Override
        public void onNewTabCreated(Tab tab) {
            tab.addObserver(mObserver);
        }

        private PageLoadFinishedTabObserver mObserver;
    }

    static class PageIsLoadedDeferredStartupHandler extends DeferredStartupHandler {
        public PageIsLoadedDeferredStartupHandler(PageLoadFinishedTabObserver observer,
                CallbackHelper helper) {
            mObserver = observer;
            mHelper = helper;
        }

        @Override
        public void queueDeferredTasksOnIdleHandler() {
            Assert.assertTrue("Page is yet to finish loading.", mObserver.isPageLoadFinished());

            mHelper.notifyCalled();
        }

        private CallbackHelper mHelper;
        private PageLoadFinishedTabObserver mObserver;
    }

    @Rule
    public final WebappActivityTestRule mActivityTestRule = new WebappActivityTestRule();

    @Test
    @SmallTest
    @Feature({"Webapps"})
    public void testPageIsLoadedOnDeferredStartup() throws Exception {
        PageLoadFinishedTabObserver tabObserver = new PageLoadFinishedTabObserver();
        TabModelSelectorBase
                .setObserverForTests(new NewTabCreatedTabModelSelectorObserver(tabObserver));
        CallbackHelper helper = new CallbackHelper();
        PageIsLoadedDeferredStartupHandler handler = new PageIsLoadedDeferredStartupHandler(
                tabObserver, helper);
        DeferredStartupHandler.setInstanceForTests(handler);
        mActivityTestRule.startWebappActivity();
        helper.waitForCallback(0);
    }
}
