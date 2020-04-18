// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.test.util.browser.tabmodel;

import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabModel.TabLaunchType;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tabmodel.TabModelSelectorBase;
import org.chromium.chrome.browser.tabmodel.TabModelUtils;
import org.chromium.content_public.browser.LoadUrlParams;

/**
 * Mock of a basic {@link TabModelSelector}. It supports 2 {@link TabModel}: standard and incognito.
 */
public class MockTabModelSelector extends TabModelSelectorBase {
    // Offsetting the id compared to the index helps greatly when debugging.
    public static final int ID_OFFSET = 100000;
    public static final int INCOGNITO_ID_OFFSET = 200000;
    private static int sCurTabOffset;

    public MockTabModelSelector(
            int tabCount, int incognitoTabCount, MockTabModel.MockTabModelDelegate delegate) {
        super();
        initialize(false, new MockTabModel(false, delegate), new MockTabModel(true, delegate));
        for (int i = 0; i < tabCount; i++) {
            addMockTab();
        }
        if (tabCount > 0) TabModelUtils.setIndex(getModelAt(0), 0);

        for (int i = 0; i < incognitoTabCount; i++) {
            addMockIncognitoTab();
        }
        if (incognitoTabCount > 0) TabModelUtils.setIndex(getModelAt(1), 0);
    }

    private static int nextIdOffset() {
        return sCurTabOffset++;
    }

    public Tab addMockTab() {
        return ((MockTabModel) getModelAt(0)).addTab(ID_OFFSET + nextIdOffset());
    }

    public Tab addMockIncognitoTab() {
        return ((MockTabModel) getModelAt(1)).addTab(INCOGNITO_ID_OFFSET + nextIdOffset());
    }

    @Override
    public Tab openNewTab(LoadUrlParams loadUrlParams, TabLaunchType type, Tab parent,
            boolean incognito) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void closeAllTabs() {
        throw new UnsupportedOperationException();
    }

    @Override
    public int getTotalTabCount() {
        throw new UnsupportedOperationException();
    }
}
