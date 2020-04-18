// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.invalidation;

import android.accounts.Account;
import android.app.Activity;
import android.content.ContentResolver;
import android.content.Context;
import android.os.Bundle;
import android.support.test.InstrumentationRegistry;
import android.support.test.annotation.UiThreadTest;
import android.support.test.filters.SmallTest;
import android.support.test.rule.UiThreadTestRule;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ActivityState;
import org.chromium.base.ApplicationStatus;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.invalidation.PendingInvalidation;

import java.util.List;

/**
 * Tests for DelayedInvalidationsController.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
public class DelayedInvalidationsControllerTest {
    private static final String TEST_ACCOUNT = "something@gmail.com";

    private static final String OBJECT_ID = "object_id";
    private static final int OBJECT_SRC = 4;
    private static final long VERSION = 1L;
    private static final String PAYLOAD = "payload";

    private static final String OBJECT_ID_2 = "object_id_2";
    private static final int OBJECT_SRC_2 = 5;
    private static final long VERSION_2 = 2L;
    private static final String PAYLOAD_2 = "payload_2";

    private MockDelayedInvalidationsController mController;
    private Context mContext;
    private Activity mPlaceholderActivity;

    @Rule
    public UiThreadTestRule mRule = new UiThreadTestRule();

    /**
     * Mocks {@link DelayedInvalidationsController} for testing.
     * It intercepts access to the Android Sync Adapter.
     */
    private static class MockDelayedInvalidationsController extends DelayedInvalidationsController {
        private boolean mInvalidated = false;
        private List<Bundle> mBundles = null;

        private MockDelayedInvalidationsController() {}

        @Override
        void notifyInvalidationsOnBackgroundThread(
                Context context, Account account, List<Bundle> bundles) {
            mInvalidated = true;
            mBundles = bundles;
        }
    }

    @Before
    public void setUp() throws Exception {
        mController = new MockDelayedInvalidationsController();
        mContext = InstrumentationRegistry.getTargetContext();

        mPlaceholderActivity = new Activity();
        setApplicationState(ActivityState.CREATED);
        setApplicationState(ActivityState.RESUMED);
    }

    private void setApplicationState(int newState) {
        ApplicationStatus.onStateChangeForTesting(mPlaceholderActivity, newState);
    }

    @Test
    @SmallTest
    @Feature({"Sync"})
    @UiThreadTest
    public void testManualSyncRequestsShouldAlwaysTriggerSync() throws InterruptedException {
        // Sync should trigger for manual requests when Chrome is in the foreground.
        Bundle extras = new Bundle();
        extras.putBoolean(ContentResolver.SYNC_EXTRAS_MANUAL, true);
        Assert.assertTrue(mController.shouldNotifyInvalidation(extras));

        // Sync should trigger for manual requests when Chrome is in the background.
        setApplicationState(ActivityState.STOPPED);
        extras.putBoolean(ContentResolver.SYNC_EXTRAS_MANUAL, true);
        Assert.assertTrue(mController.shouldNotifyInvalidation(extras));
    }

    @Test
    @SmallTest
    @Feature({"Sync", "Invalidation"})
    @UiThreadTest
    public void testInvalidationsTriggeredWhenChromeIsInForeground() {
        Assert.assertTrue(mController.shouldNotifyInvalidation(new Bundle()));
    }

    @Test
    @SmallTest
    @Feature({"Sync", "Invalidation"})
    @UiThreadTest
    public void testInvalidationsReceivedWhenChromeIsInBackgroundIsDelayed()
            throws InterruptedException {
        setApplicationState(ActivityState.STOPPED);
        Assert.assertFalse(mController.shouldNotifyInvalidation(new Bundle()));
    }

    @Test
    @SmallTest
    @Feature({"Sync", "Invalidation"})
    @UiThreadTest
    public void testOnlySpecificInvalidationsTriggeredOnResume() throws InterruptedException {
        // First make sure there are no pending invalidations.
        mController.clearPendingInvalidations(mContext);
        Assert.assertFalse(mController.notifyPendingInvalidations(mContext));
        Assert.assertFalse(mController.mInvalidated);

        // Create some invalidations.
        PendingInvalidation firstInv =
                new PendingInvalidation(OBJECT_ID, OBJECT_SRC, VERSION, PAYLOAD);
        PendingInvalidation secondInv =
                new PendingInvalidation(OBJECT_ID_2, OBJECT_SRC_2, VERSION_2, PAYLOAD_2);

        // Can't invalidate while Chrome is in the background.
        setApplicationState(ActivityState.STOPPED);
        Assert.assertFalse(mController.shouldNotifyInvalidation(new Bundle()));

        // Add multiple pending invalidations.
        mController.addPendingInvalidation(mContext, TEST_ACCOUNT, firstInv);
        mController.addPendingInvalidation(mContext, TEST_ACCOUNT, secondInv);

        // Make sure there are pending invalidations.
        Assert.assertTrue(mController.notifyPendingInvalidations(mContext));
        Assert.assertTrue(mController.mInvalidated);

        // Ensure only specific invalidations are being notified.
        Assert.assertEquals(2, mController.mBundles.size());
        PendingInvalidation parsedInv1 = new PendingInvalidation(mController.mBundles.get(0));
        PendingInvalidation parsedInv2 = new PendingInvalidation(mController.mBundles.get(1));
        Assert.assertTrue(firstInv.equals(parsedInv1) ^ firstInv.equals(parsedInv2));
        Assert.assertTrue(secondInv.equals(parsedInv1) ^ secondInv.equals(parsedInv2));
    }

    @Test
    @SmallTest
    @Feature({"Sync", "Invalidation"})
    @UiThreadTest
    public void testAllInvalidationsTriggeredOnResume() throws InterruptedException {
        // First make sure there are no pending invalidations.
        mController.clearPendingInvalidations(mContext);
        Assert.assertFalse(mController.notifyPendingInvalidations(mContext));
        Assert.assertFalse(mController.mInvalidated);

        // Create some invalidations.
        PendingInvalidation firstInv =
                new PendingInvalidation(OBJECT_ID, OBJECT_SRC, VERSION, PAYLOAD);
        PendingInvalidation secondInv =
                new PendingInvalidation(OBJECT_ID_2, OBJECT_SRC_2, VERSION_2, PAYLOAD_2);
        PendingInvalidation allInvalidations = new PendingInvalidation(new Bundle());
        Assert.assertEquals(allInvalidations.mObjectSource, 0);

        // Can't invalidate while Chrome is in the background.
        setApplicationState(ActivityState.STOPPED);
        Assert.assertFalse(mController.shouldNotifyInvalidation(new Bundle()));

        // Add multiple pending invalidations.
        mController.addPendingInvalidation(mContext, TEST_ACCOUNT, firstInv);
        mController.addPendingInvalidation(mContext, TEST_ACCOUNT, allInvalidations);
        mController.addPendingInvalidation(mContext, TEST_ACCOUNT, secondInv);

        // Make sure there are pending invalidations.
        Assert.assertTrue(mController.notifyPendingInvalidations(mContext));
        Assert.assertTrue(mController.mInvalidated);

        // As Invalidation for all ids has been received, it will supersede all other invalidations.
        Assert.assertEquals(1, mController.mBundles.size());
        Assert.assertEquals(allInvalidations, new PendingInvalidation(mController.mBundles.get(0)));
    }

    @Test
    @SmallTest
    @Feature({"Sync", "Invalidation"})
    @UiThreadTest
    public void testSameObjectInvalidationsGetCombined() throws InterruptedException {
        // First make sure there are no pending invalidations.
        mController.clearPendingInvalidations(mContext);
        Assert.assertFalse(mController.notifyPendingInvalidations(mContext));
        Assert.assertFalse(mController.mInvalidated);

        // Create invalidations with the same id/src, but different versions and payloads.
        PendingInvalidation lowerVersionInv =
                new PendingInvalidation(OBJECT_ID, OBJECT_SRC, VERSION, PAYLOAD);
        PendingInvalidation higherVersionInv =
                new PendingInvalidation(OBJECT_ID, OBJECT_SRC, VERSION_2, PAYLOAD_2);

        // Can't invalidate while Chrome is in the background.
        setApplicationState(ActivityState.STOPPED);
        Assert.assertFalse(mController.shouldNotifyInvalidation(new Bundle()));

        // Add multiple pending invalidations.
        mController.addPendingInvalidation(mContext, TEST_ACCOUNT, lowerVersionInv);
        mController.addPendingInvalidation(mContext, TEST_ACCOUNT, higherVersionInv);

        // Make sure there are pending invalidations.
        Assert.assertTrue(mController.notifyPendingInvalidations(mContext));
        Assert.assertTrue(mController.mInvalidated);

        // As Invalidation for all ids has been received, it will supersede all other invalidations.
        Assert.assertEquals(1, mController.mBundles.size());
        Assert.assertEquals(higherVersionInv, new PendingInvalidation(mController.mBundles.get(0)));
    }

    @Test
    @SmallTest
    @Feature({"Sync", "Invalidation"})
    @UiThreadTest
    public void testSameObjectLowerVersionInvalidationGetsDiscarded() throws InterruptedException {
        // First make sure there are no pending invalidations.
        mController.clearPendingInvalidations(mContext);
        Assert.assertFalse(mController.notifyPendingInvalidations(mContext));
        Assert.assertFalse(mController.mInvalidated);

        // Create invalidations with the same id/src, but different versions and payloads.
        PendingInvalidation lowerVersionInv =
                new PendingInvalidation(OBJECT_ID, OBJECT_SRC, VERSION, PAYLOAD);
        PendingInvalidation higherVersionInv =
                new PendingInvalidation(OBJECT_ID, OBJECT_SRC, VERSION_2, PAYLOAD_2);

        // Can't invalidate while Chrome is in the background.
        setApplicationState(ActivityState.STOPPED);
        Assert.assertFalse(mController.shouldNotifyInvalidation(new Bundle()));

        // Add multiple pending invalidations.
        mController.addPendingInvalidation(mContext, TEST_ACCOUNT, higherVersionInv);
        mController.addPendingInvalidation(mContext, TEST_ACCOUNT, lowerVersionInv);

        // Make sure there are pending invalidations.
        Assert.assertTrue(mController.notifyPendingInvalidations(mContext));
        Assert.assertTrue(mController.mInvalidated);

        // As Invalidation for all ids has been received, it will supersede all other invalidations.
        Assert.assertEquals(1, mController.mBundles.size());
        Assert.assertEquals(higherVersionInv, new PendingInvalidation(mController.mBundles.get(0)));
    }
}
