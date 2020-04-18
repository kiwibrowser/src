// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.test.util;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.SharedPreferences;
import android.support.v4.content.ContextCompat;

import static org.chromium.base.test.util.ScalableTimeout.scaleTimeout;

import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;

import java.io.File;
import java.util.concurrent.TimeUnit;

/**
 * A utility class to do operations on the application data directory, such as clearing the
 * application data.
 */
public final class ApplicationData {

    private static final long MAX_CLEAR_APP_DATA_TIMEOUT_MS =
            scaleTimeout(TimeUnit.SECONDS.toMillis(3));
    private static final long CLEAR_APP_DATA_POLL_INTERVAL_MS = MAX_CLEAR_APP_DATA_TIMEOUT_MS / 10;

    private ApplicationData() {}

    /**
     * Clear all files in target contexts application directory, except the directory 'lib'.
     * The timeout is specified in |MAX_CLEAR_APP_DATA_TIMEOUT_MS|.
     *
     * When deleting all files is complete, the 'cache' directory is recreated, to ensure that the
     * framework does not try to create it for sandbox processes and fail.
     *
     * When this is invoked from tests, the target context from the instrumentation must be used.
     *
     * @param targetContext the target Context.
     */
    public static void clearAppData(final Context targetContext) {
        CriteriaHelper.pollInstrumentationThread(
                new Criteria() {
                    private boolean mDataRemoved;

                    // The lint check for calling apply() rather than commit() on a shared pref
                    // was recently renamed.
                    @SuppressLint({"ApplySharedPref", "CommitPrefEdits"})
                    @Override
                    public boolean isSatisfied() {
                        SharedPreferences multidexPrefs =
                                targetContext.getSharedPreferences("multidex.version", 0);
                        if (!mDataRemoved && !removeAppData(targetContext)) {
                            return false;
                        }
                        mDataRemoved = true;
                        // We have to make sure the cache directory still exists, as the framework
                        // will try to create it otherwise and will fail for sandbox processes with
                        // a NullPointerException.
                        File cacheDir = new File(ContextCompat.getDataDir(targetContext), "cache");
                        // Removing app data cleared out all shared prefs. Multidex uses shared
                        // prefs to cache hashes of the secondary dexes it has extracted; without
                        // them, it'll attempt to reextract the dexes the next time the tests
                        // start up.
                        multidexPrefs.edit().commit();
                        return cacheDir.exists() || cacheDir.mkdir();
                    }
                },
                MAX_CLEAR_APP_DATA_TIMEOUT_MS, CLEAR_APP_DATA_POLL_INTERVAL_MS);
    }

    /**
     * Remove all files and directories under the given application directory, except 'lib'.
     *
     * @param appDir the application directory to remove.
     *
     * @return whether removal succeeded.
     */
    private static boolean removeAppData(final Context targetContext) {
        File dataDir = ContextCompat.getDataDir(targetContext);
        File codeCacheDir = ContextCompat.getCodeCacheDir(targetContext);
        File[] files = dataDir.listFiles();
        if (files == null) return true;
        for (File file : files) {
            if (!(file.getName().equals("lib") || file.getName().equals("incremental-install-files")
                        || file.getName().equals(codeCacheDir.getName()))
                    && !removeFile(file)) {
                return false;
            }
        }
        return true;
    }

    /**
     * Remove the given file or directory.
     *
     * @param file the file or directory to remove.
     *
     * @return whether removal succeeded.
     */
    private static boolean removeFile(File file) {
        if (file.isDirectory()) {
            File[] files = file.listFiles();
            if (files == null) return true;
            for (File sub_file : files) {
                if (!removeFile(sub_file)) return false;
            }
        }
        return file.delete();
    }
}
