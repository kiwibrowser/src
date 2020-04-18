// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.browseractions;

import org.chromium.base.Callback;
import org.chromium.chrome.browser.compositor.layouts.content.TabContentManager;
import org.chromium.chrome.browser.tabmodel.TabPersistencePolicy;
import org.chromium.chrome.browser.tabmodel.TabPersistentStore;
import org.chromium.chrome.browser.tabmodel.TabbedModeTabPersistencePolicy;

import java.io.File;
import java.util.List;
import java.util.concurrent.Executor;

import javax.annotation.Nullable;

/**
 * Handles the Browser Actions Tab specific behaviors of tab persistence.
 */
public class BrowserActionsTabPersistencePolicy implements TabPersistencePolicy {
    @Override
    public File getOrCreateStateDirectory() {
        return TabbedModeTabPersistencePolicy.getOrCreateTabbedModeStateDirectory();
    }

    @Override
    public String getStateFileName() {
        return TabPersistentStore.getStateFileName("_browser_actions");
    }

    @Override
    public boolean shouldMergeOnStartup() {
        return false;
    }

    @Override
    @Nullable
    public List<String> getStateToBeMergedFileNames() {
        return null;
    }

    @Override
    public boolean performInitialization(Executor executor) {
        return false;
    }

    @Override
    public void waitForInitializationToFinish() {}

    @Override
    public boolean isMergeInProgress() {
        return false;
    }

    @Override
    public void setMergeInProgress(boolean isStarted) {
        assert false : "Merge not supported in Browser Actions";
    }

    @Override
    public void cancelCleanupInProgress() {}

    @Override
    public void cleanupUnusedFiles(Callback<List<String>> filesToDelete) {}

    @Override
    public void setTabContentManager(TabContentManager cache) {}

    @Override
    public void notifyStateLoaded(int tabCountAtStartup) {}

    @Override
    public void destroy() {}
}
