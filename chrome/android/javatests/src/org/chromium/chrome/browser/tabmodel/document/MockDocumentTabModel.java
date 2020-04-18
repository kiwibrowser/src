// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.tabmodel.document;

import org.junit.Assert;

import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabList;
import org.chromium.chrome.browser.tabmodel.TabModelObserver;

/**
 * Mocks out all of the DocumentTabModel calls to fail.  Override specific ones as needed.
 */
public class MockDocumentTabModel implements DocumentTabModel {
    private boolean mIsIncognito;

    public MockDocumentTabModel(boolean isIncognito) {
        mIsIncognito = isIncognito;
    }

    @Override
    public Profile getProfile() {
        Assert.fail();
        return null;
    }

    @Override
    public boolean closeTab(Tab tab) {
        Assert.fail();
        return false;
    }

    @Override
    public boolean closeTab(Tab tab, boolean animate, boolean uponExit, boolean canUndo) {
        Assert.fail();
        return false;
    }

    @Override
    public Tab getNextTabIfClosed(int id) {
        Assert.fail();
        return null;
    }

    @Override
    public void closeAllTabs() {
        Assert.fail();
    }

    @Override
    public void closeAllTabs(boolean allowDelegation, boolean uponExit) {
        Assert.fail();
    }

    @Override
    public boolean supportsPendingClosures() {
        Assert.fail();
        return false;
    }

    @Override
    public void commitAllTabClosures() {
        Assert.fail();
    }

    @Override
    public void commitTabClosure(int tabId) {
        Assert.fail();
    }

    @Override
    public void cancelTabClosure(int tabId) {
        Assert.fail();
    }

    @Override
    public TabList getComprehensiveModel() {
        Assert.fail();
        return null;
    }

    @Override
    public void setIndex(int i, TabSelectionType type) {
        Assert.fail();
    }

    @Override
    public void moveTab(int id, int newIndex) {
        Assert.fail();
    }

    @Override
    public void destroy() {
        Assert.fail();
    }

    @Override
    public void addTab(Tab tab, int index, TabLaunchType type) {
        Assert.fail();
    }

    @Override
    public void removeTab(Tab tab) {
        Assert.fail();
    }

    @Override
    public void addObserver(TabModelObserver observer) {
        Assert.fail();
    }

    @Override
    public void removeObserver(TabModelObserver observer) {
        Assert.fail();
    }

    @Override
    public boolean isIncognito() {
        return mIsIncognito;
    }

    @Override
    public boolean isCurrentModel() {
        return false;
    }

    @Override
    public int index() {
        Assert.fail();
        return 0;
    }

    @Override
    public int getCount() {
        Assert.fail();
        return 0;
    }

    @Override
    public Tab getTabAt(int index) {
        Assert.fail();
        return null;
    }

    @Override
    public int indexOf(Tab tab) {
        Assert.fail();
        return 0;
    }

    @Override
    public boolean isClosurePending(int tabId) {
        Assert.fail();
        return false;
    }

    @Override
    public String getInitialUrlForDocument(int tabId) {
        Assert.fail();
        return null;
    }

    @Override
    public void openMostRecentlyClosedTab() {}
}
