// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.test.util.browser.tabmodel.document;

import android.os.StrictMode;
import android.util.Base64;
import android.util.Log;

import org.junit.Assert;

import org.chromium.base.StreamUtil;
import org.chromium.chrome.browser.TabState;
import org.chromium.chrome.browser.tabmodel.document.StorageDelegate;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

/**
 * Mocks out a directory on the file system for TabState storage. Because of the way that
 * {@link TabState} relies on real files and because it's not possible to mock out a
 * {@link FileInputStream}, we manage real files in a temporary directory that get written when
 * {@link #addEncodedTabState} is called.
 */
public class MockStorageDelegate extends StorageDelegate {
    private static final String TAG = "MockStorageDelegate";

    private byte[] mTaskFileBytes;
    private final File mStateDirectory;

    public MockStorageDelegate(File cacheDirectory) throws Exception {
        mStateDirectory = new File(cacheDirectory, "DocumentTabModelTest");
        ensureDirectoryDestroyed();
    }

    @Override
    protected byte[] readMetadataFileBytes(boolean encrypted) {
        if (encrypted) return null;
        return mTaskFileBytes == null ? null : mTaskFileBytes.clone();
    }

    @Override
    public void writeTaskFileBytes(boolean encrypted, byte[] bytes) {
        if (encrypted) return;
        mTaskFileBytes = bytes.clone();
    }

    @Override
    public File getStateDirectory() {
        // This is a test class, allowing StrictMode violations.
        StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskWrites();
        try {
            if (!mStateDirectory.exists() && !mStateDirectory.mkdirs()) {
                Assert.fail("Failed to create state directory.  Tests should fail.");
            }
            return mStateDirectory;
        } finally {
            StrictMode.setThreadPolicy(oldPolicy);
        }
    }

    /**
     * Sets the task file byte buffer to be the decoded format of the given string.
     * @param encoded Base64 encoded task file.
     */
    public void setTaskFileBytesFromEncodedString(String encoded) {
        mTaskFileBytes = Base64.decode(encoded, Base64.DEFAULT);
    }

    /**
     * Adds a TabState to the file system.
     * @param tabId ID of the Tab.
     * @param encodedState Base64 encoded TabState.
     * @return Whether or not the TabState was successfully read.
     */
    public boolean addEncodedTabState(int tabId, boolean encrypted, String encodedState) {
        String filename = TabState.getTabStateFilename(tabId, encrypted);
        File tabStateFile = new File(getStateDirectory(), filename);
        FileOutputStream outputStream = null;
        try {
            outputStream = new FileOutputStream(tabStateFile);
            outputStream.write(Base64.decode(encodedState, 0));
        } catch (FileNotFoundException e) {
            assert false : "Failed to create " + filename;
            return false;
        } catch (IOException e) {
            assert false : "IO exception " + filename;
            return false;
        } finally {
            StreamUtil.closeQuietly(outputStream);
        }

        return true;
    }

    /**
     * Ensures that the state directory and its contents are all wiped from storage.
     */
    public void ensureDirectoryDestroyed() throws Exception {
        if (!mStateDirectory.exists()) return;
        recursivelyDelete(mStateDirectory);
    }

    private void recursivelyDelete(File currentFile) throws Exception {
        if (currentFile.isDirectory()) {
            File[] files = currentFile.listFiles();
            if (files != null) {
                for (File file : files) {
                    recursivelyDelete(file);
                }
            }
        }

        if (!currentFile.delete()) Log.e(TAG, "Failed to delete: " + currentFile);
    }
}
