// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.toolbar;

import android.view.View.OnClickListener;
import android.view.ViewGroup;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.modelutil.PropertyModelChangeProcessor;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.EmptyTabModelSelectorObserver;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabModel.TabLaunchType;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tabmodel.TabModelSelectorObserver;
import org.chromium.chrome.browser.tabmodel.TabModelSelectorTabModelObserver;
import org.chromium.chrome.browser.toolbar.TabSwitcherButtonModel.PropertyKey;
import org.chromium.chrome.browser.util.AccessibilityUtil;

import java.util.List;

/**
 * The controller for the tab switcher button. This class handles all interactions that the tab
 * switcher button has with the outside world.
 */
public class TabSwitcherButtonCoordinator {
    /**
     *  The model that handles events from outside the tab switcher button. Normally the coordinator
     *  should acces the mediator which then updates the model. Since this component is very simple
     *  the mediator is omitted.
     */
    private final TabSwitcherButtonModel mTabSwitcherButtonModel;

    private final TabModelSelector mTabModelSelector;
    private final TabModelSelectorObserver mTabModelSelectorObserver;
    private final TabModelSelectorTabModelObserver mTabModelSelectorTabModelObserver;

    /**
     * Build the controller that manages the tab switcher button.
     * @param root The root {@link ViewGroup} for locating the view to inflate.
     * @param onClickListener An {@link OnClickListener} that is triggered when the tab switcher
     *                        button is clicked.
     * @param tabModelSelector A {@link TabModelSelector} that the tab switcher button uses to
     *                         keep its tab count updated.
     */
    public TabSwitcherButtonCoordinator(
            ViewGroup root, OnClickListener onClickListener, TabModelSelector tabModelSelector) {
        mTabSwitcherButtonModel = new TabSwitcherButtonModel();

        final TabSwitcherButtonView view =
                (TabSwitcherButtonView) root.findViewById(R.id.tab_switcher_button);
        PropertyModelChangeProcessor<TabSwitcherButtonModel, TabSwitcherButtonView, PropertyKey>
                processor = new PropertyModelChangeProcessor<>(
                        mTabSwitcherButtonModel, view, new TabSwitcherButtonViewBinder());
        mTabSwitcherButtonModel.addObserver(processor);

        mTabModelSelector = tabModelSelector;

        mTabModelSelectorObserver = new EmptyTabModelSelectorObserver() {
            @Override
            public void onTabModelSelected(TabModel newModel, TabModel oldModel) {
                updateTabCount();
            }

            @Override
            public void onTabStateInitialized() {
                updateTabCount();
            }
        };
        mTabModelSelector.addObserver(mTabModelSelectorObserver);

        mTabModelSelectorTabModelObserver = new TabModelSelectorTabModelObserver(tabModelSelector) {
            @Override
            public void didAddTab(Tab tab, TabLaunchType type) {
                updateTabCount();
            }

            @Override
            public void tabClosureUndone(Tab tab) {
                updateTabCount();
            }

            @Override
            public void didCloseTab(int tabId, boolean incognito) {
                updateTabCount();
            }

            @Override
            public void tabPendingClosure(Tab tab) {
                updateTabCount();
            }

            @Override
            public void allTabsPendingClosure(List<Tab> tabs) {
                updateTabCount();
            }

            @Override
            public void tabRemoved(Tab tab) {
                updateTabCount();
            }
        };

        updateTabCount();

        mTabSwitcherButtonModel.setOnClickListener(onClickListener);
        CharSequence description = root.getResources().getString(R.string.open_tabs);
        mTabSwitcherButtonModel.setOnLongClickListener(
                v -> AccessibilityUtil.showAccessibilityToast(root.getContext(), v, description));
    }

    public void destroy() {
        if (mTabModelSelector != null) mTabModelSelector.removeObserver(mTabModelSelectorObserver);
        if (mTabModelSelectorTabModelObserver != null) mTabModelSelectorTabModelObserver.destroy();
    }

    private void updateTabCount() {
        mTabSwitcherButtonModel.setNumberOfTabs(mTabModelSelector.getCurrentModel().getCount());
    }
}
