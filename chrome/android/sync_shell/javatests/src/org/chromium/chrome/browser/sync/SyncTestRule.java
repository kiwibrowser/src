// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.sync;

import android.accounts.Account;
import android.content.Context;
import android.support.test.InstrumentationRegistry;

import org.junit.Assert;
import org.junit.runner.Description;
import org.junit.runners.model.Statement;

import org.chromium.base.ThreadUtils;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.identity.UniqueIdentificationGenerator;
import org.chromium.chrome.browser.identity.UniqueIdentificationGeneratorFactory;
import org.chromium.chrome.browser.identity.UuidBasedUniqueIdentificationGenerator;
import org.chromium.chrome.browser.signin.SigninManager;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.util.ApplicationTestUtils;
import org.chromium.chrome.test.util.browser.signin.SigninTestUtil;
import org.chromium.chrome.test.util.browser.sync.SyncTestUtil;
import org.chromium.components.sync.AndroidSyncSettings;
import org.chromium.components.sync.ModelType;
import org.chromium.components.sync.test.util.MockSyncContentResolverDelegate;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;

import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

/**
 * TestRule for common functionality between sync tests.
 */
public class SyncTestRule extends ChromeActivityTestRule<ChromeActivity> {
    private static final String TAG = "SyncTestBase";

    private static final String CLIENT_ID = "Client_ID";

    private static final Set<Integer> USER_SELECTABLE_TYPES =
            new HashSet<Integer>(Arrays.asList(new Integer[] {
                    ModelType.AUTOFILL, ModelType.BOOKMARKS, ModelType.PASSWORDS,
                    ModelType.PREFERENCES, ModelType.PROXY_TABS, ModelType.TYPED_URLS,
            }));

    public abstract static class DataCriteria<T> extends Criteria {
        public DataCriteria() {
            super("Sync data criteria not met.");
        }

        public abstract boolean isSatisfied(List<T> data);

        public abstract List<T> getData() throws Exception;

        @Override
        public boolean isSatisfied() {
            try {
                return isSatisfied(getData());
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
        }
    }

    private Context mContext;
    private FakeServerHelper mFakeServerHelper;
    private ProfileSyncService mProfileSyncService;
    private MockSyncContentResolverDelegate mSyncContentResolver;

    public SyncTestRule() {
        super(ChromeActivity.class);
    }

    /**Getters for Test variables */
    public Context getTargetContext() {
        return mContext;
    }

    public FakeServerHelper getFakeServerHelper() {
        return mFakeServerHelper;
    }

    public ProfileSyncService getProfileSyncService() {
        return mProfileSyncService;
    }

    public MockSyncContentResolverDelegate getSyncContentResolver() {
        return mSyncContentResolver;
    }

    public void startMainActivityForSyncTest() throws Exception {
        // Start the activity by opening about:blank. This URL is ideal because it is not synced as
        // a typed URL. If another URL is used, it could interfere with test data.
        startMainActivityOnBlankPage();
    }

    private void setUpMockAndroidSyncSettings() {
        mSyncContentResolver = new MockSyncContentResolverDelegate();
        mSyncContentResolver.setMasterSyncAutomatically(true);
        AndroidSyncSettings.overrideForTests(mContext, mSyncContentResolver, null);
    }

    public Account setUpTestAccount() {
        Account account = SigninTestUtil.addTestAccount();
        Assert.assertFalse(SyncTestUtil.isSyncRequested());
        return account;
    }

    public Account setUpTestAccountAndSignIn() {
        Account account = setUpTestAccount();
        signIn(account);
        return account;
    }

    public void startSync() {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mProfileSyncService.requestStart();
            }
        });
    }

    public void startSyncAndWait() {
        startSync();
        SyncTestUtil.waitForSyncActive();
    }

    public void stopSync() {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mProfileSyncService.requestStop();
            }
        });
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
    }

    public void signIn(final Account account) {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                SigninManager.get().signIn(account, null, null);
            }
        });
        SyncTestUtil.waitForSyncActive();
        SyncTestUtil.triggerSyncAndWaitForCompletion();
        Assert.assertEquals(account, SigninTestUtil.getCurrentAccount());
    }

    public void signOut() throws InterruptedException {
        final Semaphore s = new Semaphore(0);
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                SigninManager.get().signOut(new Runnable() {
                    @Override
                    public void run() {
                        s.release();
                    }
                });
            }
        });
        Assert.assertTrue(s.tryAcquire(SyncTestUtil.TIMEOUT_MS, TimeUnit.MILLISECONDS));
        Assert.assertNull(SigninTestUtil.getCurrentAccount());
        Assert.assertFalse(SyncTestUtil.isSyncRequested());
    }

    public void clearServerData() {
        mFakeServerHelper.clearServerData();
        SyncTestUtil.triggerSync();
        CriteriaHelper.pollUiThread(new Criteria("Timed out waiting for sync to stop.") {
            @Override
            public boolean isSatisfied() {
                return !ProfileSyncService.get().isSyncRequested();
            }
        }, SyncTestUtil.TIMEOUT_MS, SyncTestUtil.INTERVAL_MS);
    }

    /*
     * Enable the |modelType| Sync data type. This also forces all types to be
     * USER_SELECTABLE_TYPES.
     */
    public void enableDataType(final int modelType) {
        ThreadUtils.runOnUiThreadBlocking(() -> {
            Set<Integer> preferredTypes = mProfileSyncService.getPreferredDataTypes();
            preferredTypes.retainAll(USER_SELECTABLE_TYPES);
            preferredTypes.add(modelType);
            mProfileSyncService.setPreferredDataTypes(false, preferredTypes);
        });
    }

    public void disableDataType(final int modelType) {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                Set<Integer> preferredTypes = mProfileSyncService.getPreferredDataTypes();
                preferredTypes.retainAll(USER_SELECTABLE_TYPES);
                preferredTypes.remove(modelType);
                mProfileSyncService.setPreferredDataTypes(false, preferredTypes);
            }
        });
    }

    public void pollInstrumentationThread(Criteria criteria) {
        CriteriaHelper.pollInstrumentationThread(
                criteria, SyncTestUtil.TIMEOUT_MS, SyncTestUtil.INTERVAL_MS);
    }

    @Override
    public Statement apply(final Statement statement, final Description desc) {
        final Statement base = super.apply(new Statement() {
            @Override
            public void evaluate() throws Throwable {
                startMainActivityForSyncTest();
                mContext = InstrumentationRegistry.getTargetContext();

                setUpMockAndroidSyncSettings();

                ThreadUtils.runOnUiThreadBlocking(new Runnable() {
                    @Override
                    public void run() {
                        // Ensure SyncController is registered with the new AndroidSyncSettings.
                        AndroidSyncSettings.registerObserver(
                                mContext, SyncController.get(mContext));
                        mFakeServerHelper = FakeServerHelper.get();
                    }
                });
                FakeServerHelper.useFakeServer(mContext);
                ThreadUtils.runOnUiThreadBlocking(new Runnable() {
                    @Override
                    public void run() {
                        mProfileSyncService = ProfileSyncService.get();
                    }
                });

                UniqueIdentificationGeneratorFactory.registerGenerator(
                        UuidBasedUniqueIdentificationGenerator.GENERATOR_ID,
                        new UniqueIdentificationGenerator() {
                            @Override
                            public String getUniqueId(String salt) {
                                return CLIENT_ID;
                            }
                        }, true);
                statement.evaluate();
            }
        }, desc);
        return new Statement() {
            @Override
            public void evaluate() throws Throwable {
                ruleSetUp();
                base.evaluate();
                ruleTearDown();
            }
        };
    }

    private void ruleSetUp() throws Throwable {
        // This must be called before doing anything with the SharedPreferences because
        // ChromeActivityTestRule's setUp normally wipes them clean between runs.
        // Setting a SharedPreference here via the SigninTestUtil.setUpAuthForTest() call below
        // causes Android to cache them before the files get cleared, meaning that a data clear
        // is useless and test runs influence each other.
        ApplicationTestUtils.clearAppData(InstrumentationRegistry.getTargetContext());

        // This must be called before super.setUp() in order for test authentication to work.
        SigninTestUtil.setUpAuthForTest(InstrumentationRegistry.getInstrumentation());
    }

    private void ruleTearDown() throws Exception {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mProfileSyncService.requestStop();
                FakeServerHelper.deleteFakeServer();
            }
        });
        SigninTestUtil.resetSigninState();
        SigninTestUtil.tearDownAuthForTest();
    }

}
