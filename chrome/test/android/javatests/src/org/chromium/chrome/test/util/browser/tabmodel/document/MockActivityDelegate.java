// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.test.util.browser.tabmodel.document;

import static junit.framework.Assert.assertTrue;

import android.app.Activity;
import android.content.Intent;

import org.chromium.chrome.browser.tabmodel.document.ActivityDelegate;
import org.chromium.chrome.browser.tabmodel.document.DocumentTabModel.Entry;

import java.util.ArrayList;
import java.util.List;

/**
 * Mocks out calls to the ActivityManager by a DocumentTabModel.
 */
public class MockActivityDelegate extends ActivityDelegate {
    private final List<Entry> mRegularTasks = new ArrayList<Entry>();
    private final List<Entry> mIncognitoTasks = new ArrayList<Entry>();

    /**
     * Creates a MockActivityDelegate.
     */
    public MockActivityDelegate() {
        super(null, null);
    }

    @Override
    public boolean isDocumentActivity(Activity activity) {
        return true;
    }

    @Override
    public boolean isValidActivity(boolean isIncognito, Intent intent) {
        return true;
    }

    @Override
    public void moveTaskToFront(boolean isIncognito, int tabId) {
    }

    @Override
    public List<Entry> getTasksFromRecents(boolean isIncognito) {
        return isIncognito ? mIncognitoTasks : mRegularTasks;
    }

    @Override
    public boolean isIncognitoDocumentAccessibleToUser() {
        return false;
    }

    @Override
    protected boolean isActivityDestroyed(Activity activity) {
        return true;
    }

    @Override
    public void finishAndRemoveTask(boolean isIncognito, int tabId) {
        List<Entry> tasks = getTasksFromRecents(isIncognito);
        for (int i = 0; i < tasks.size(); i++) {
            if (tasks.get(i).tabId == tabId) {
                tasks.remove(i);
                return;
            }
        }
    }

    @Override
    public void finishAllDocumentActivities() {
        mRegularTasks.clear();
        mIncognitoTasks.clear();
    }

    /**
     * Adds a task to the recents list.
     * @param isIncognito Whether the task is an incognito task.
     * @param tabId ID of the task.
     * @param initialUrl Initial URL for the task.
     */
    public void addTask(boolean isIncognito, int tabId, String initialUrl) {
        getTasksFromRecents(isIncognito).add(new Entry(tabId, initialUrl));
    }

    /**
     * Removes a task from the recents list.
     * @param tabId ID of the task.
     */
    public void removeTask(boolean isIncognito, int tabId) {
        boolean found = false;
        List<Entry> tasks = getTasksFromRecents(isIncognito);
        for (int i = 0; i < tasks.size() && !found; i++) {
            if (tasks.get(i).tabId == tabId) found = true;
        }
        assertTrue(found);
        finishAndRemoveTask(isIncognito, tabId);
    }
}