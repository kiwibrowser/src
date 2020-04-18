// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.invalidation;

import android.accounts.Account;
import android.app.Activity;
import android.app.Application;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.SyncResult;
import android.os.Bundle;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.CommandLine;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.base.test.util.ScalableTimeout;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.invalidation.PendingInvalidation;
import org.chromium.components.signin.AccountManagerFacade;
import org.chromium.components.sync.AndroidSyncSettings;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;

/**
 * Tests for ChromeBrowserSyncAdapter.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class ChromeBrowserSyncAdapterTest {
    @Rule
    public ChromeActivityTestRule<ChromeActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeActivity.class);

    private static final Account TEST_ACCOUNT =
            AccountManagerFacade.createAccountFromName("test@gmail.com");
    private static final long WAIT_FOR_LAUNCHER_MS = ScalableTimeout.scaleTimeout(10 * 1000);
    private static final long POLL_INTERVAL_MS = 100;

    private TestSyncAdapter mSyncAdapter;

    private static class TestSyncAdapter extends ChromeBrowserSyncAdapter {
        private boolean mInvalidated;
        private boolean mInvalidatedAllTypes;
        private int mObjectSource;
        private String mObjectId;
        private long mVersion;
        private String mPayload;

        public TestSyncAdapter(Context context, Application application) {
            super(context, application);
        }

        @Override
        public void notifyInvalidation(
                int objectSource, String objectId, long version, String payload) {
            mObjectSource = objectSource;
            mObjectId = objectId;
            mVersion = version;
            mPayload = payload;
            if (objectId == null) {
                mInvalidatedAllTypes = true;
            } else {
                mInvalidated = true;
            }
        }
    }

    @Before
    public void setUp() throws Exception {
        mActivityTestRule.startMainActivityOnBlankPage();
        mSyncAdapter = new TestSyncAdapter(InstrumentationRegistry.getTargetContext(),
                mActivityTestRule.getActivity().getApplication());
    }

    private void performSyncWithBundle(Bundle bundle) {
        mSyncAdapter.onPerformSync(TEST_ACCOUNT, bundle,
                AndroidSyncSettings.getContractAuthority(mActivityTestRule.getActivity()), null,
                new SyncResult());
    }

    private void sendChromeToBackground(Activity activity) {
        Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.addCategory(Intent.CATEGORY_HOME);
        activity.startActivity(intent);

        CriteriaHelper.pollInstrumentationThread(new Criteria("Activity should have been resumed") {
            @Override
            public boolean isSatisfied() {
                return !isActivityResumed();
            }
        }, WAIT_FOR_LAUNCHER_MS, POLL_INTERVAL_MS);
    }

    private boolean isActivityResumed() {
        return ApplicationStatus.hasVisibleActivities();
    }

    @Test
    @MediumTest
    @Feature({"Sync"})
    @RetryOnFailure
    public void testRequestSyncNoInvalidationData() {
        performSyncWithBundle(new Bundle());
        Assert.assertTrue(mSyncAdapter.mInvalidatedAllTypes);
        Assert.assertFalse(mSyncAdapter.mInvalidated);
        Assert.assertTrue(CommandLine.isInitialized());
    }

    @Test
    @MediumTest
    @Feature({"Sync"})
    public void testRequestSyncSpecificDataType() {
        String objectId = "objectid_value";
        int objectSource = 61;
        long version = 42L;
        String payload = "payload_value";

        performSyncWithBundle(
                PendingInvalidation.createBundle(objectId, objectSource, version, payload));

        Assert.assertFalse(mSyncAdapter.mInvalidatedAllTypes);
        Assert.assertTrue(mSyncAdapter.mInvalidated);
        Assert.assertEquals(objectSource, mSyncAdapter.mObjectSource);
        Assert.assertEquals(objectId, mSyncAdapter.mObjectId);
        Assert.assertEquals(version, mSyncAdapter.mVersion);
        Assert.assertEquals(payload, mSyncAdapter.mPayload);
        Assert.assertTrue(CommandLine.isInitialized());
    }

    @Test
    @MediumTest
    @Feature({"Sync"})
    @RetryOnFailure
    public void testRequestSyncWhenChromeInBackground() {
        sendChromeToBackground(mActivityTestRule.getActivity());
        performSyncWithBundle(new Bundle());
        Assert.assertFalse(mSyncAdapter.mInvalidatedAllTypes);
        Assert.assertFalse(mSyncAdapter.mInvalidated);
        Assert.assertTrue(CommandLine.isInitialized());
    }

    @Test
    @MediumTest
    @Feature({"Sync"})
    @RetryOnFailure
    public void testRequestInitializeSync() {
        Bundle extras = new Bundle();
        extras.putBoolean(ContentResolver.SYNC_EXTRAS_INITIALIZE, true);
        performSyncWithBundle(extras);
        Assert.assertFalse(mSyncAdapter.mInvalidatedAllTypes);
        Assert.assertFalse(mSyncAdapter.mInvalidated);
    }
}
