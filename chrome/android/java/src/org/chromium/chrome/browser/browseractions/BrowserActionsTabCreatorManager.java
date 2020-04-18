// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.browseractions;

import android.content.Context;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.TabState;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabDelegateFactory;
import org.chromium.chrome.browser.tabmodel.TabCreatorManager;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabModel.TabLaunchType;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.WindowAndroid;

/**
 * The manager returns a {@link BrowserActionsTabCreator} to create Tabs for Browser Actions.
 */
public class BrowserActionsTabCreatorManager implements TabCreatorManager {
    private final BrowserActionsTabCreator mTabCreator;

    public BrowserActionsTabCreatorManager() {
        mTabCreator = new BrowserActionsTabCreator();
    }

    /**
     * This class creates various kinds of new tabs specific for Browser Actions.
     * The created tabs are not bound with {@link ChromeActivity}.
     */
    public class BrowserActionsTabCreator extends TabCreator {
        private TabModel mTabModel;

        @Override
        public boolean createsTabsAsynchronously() {
            return false;
        }

        @Override
        public Tab createNewTab(LoadUrlParams loadUrlParams, TabLaunchType type, Tab parent) {
            assert type == TabLaunchType.FROM_BROWSER_ACTIONS
                    || type
                            == TabLaunchType.FROM_RESTORE
                : "tab launch type should be FROM_BROWSER_ACTIONS or FROM_RESTORE";
            Context context = ContextUtils.getApplicationContext();
            WindowAndroid windowAndroid = new WindowAndroid(context);
            Tab tab = Tab.createTabForLazyLoad(
                    false, windowAndroid, type, Tab.INVALID_TAB_ID, loadUrlParams);
            tab.initialize(null, null, new TabDelegateFactory(), true, false);
            mTabModel.addTab(tab, -1, type);
            return tab;
        }

        @Override
        public Tab createFrozenTab(TabState state, int id, int index) {
            Context context = ContextUtils.getApplicationContext();
            WindowAndroid windowAndroid = new WindowAndroid(context);
            Tab tab = Tab.createFrozenTabFromState(
                    id, false, windowAndroid, Tab.INVALID_TAB_ID, state);
            tab.initialize(null, null, new TabDelegateFactory(), true, false);
            mTabModel.addTab(tab, index, TabLaunchType.FROM_RESTORE);
            return tab;
        }

        @Override
        public Tab launchUrl(String url, TabLaunchType type) {
            throw new UnsupportedOperationException("Browser Actions does not support launchUrl");
        }

        @Override
        public boolean createTabWithWebContents(
                Tab parent, WebContents webContents, int parentId, TabLaunchType type, String url) {
            throw new UnsupportedOperationException(
                    "Browser Actions does not support createTabWithWebContents");
        }

        /**
         * Sets the tab model to use when creating tabs.
         * @param model The new {@link TabModel} to use.
         */
        public void setTabModel(TabModel model) {
            mTabModel = model;
        }
    }

    @Override
    public TabCreator getTabCreator(boolean incognito) {
        if (incognito) {
            throw new IllegalStateException(
                    "Browser Actions does not support background incognito tabs");
        }
        return mTabCreator;
    }
}
