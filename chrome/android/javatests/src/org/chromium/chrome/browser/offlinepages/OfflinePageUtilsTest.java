// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.offlinepages;

import android.app.Activity;
import android.net.Uri;
import android.os.Environment;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.support.test.filters.SmallTest;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.Callback;
import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.DisabledTest;
import org.chromium.base.test.util.UrlUtils;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.offlinepages.OfflinePageBridge.OfflinePageModelObserver;
import org.chromium.chrome.browser.offlinepages.OfflinePageBridge.SavePageCallback;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.share.ShareParams;
import org.chromium.chrome.browser.snackbar.SnackbarManager.SnackbarController;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.offlinepages.SavePageResult;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.net.ConnectionType;
import org.chromium.net.NetworkChangeNotifier;
import org.chromium.net.test.EmbeddedTestServer;

import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;

/** Instrumentation tests for {@link OfflinePageUtils}. */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.
Add({"enable-features=OfflinePagesSharing", ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class OfflinePageUtilsTest {
    @Rule
    public ChromeActivityTestRule<ChromeActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeActivity.class);

    private static final String TAG = "OfflinePageUtilsTest";
    private static final String TEST_PAGE = "/chrome/test/data/android/about.html";
    private static final int TIMEOUT_MS = 5000;
    private static final ClientId BOOKMARK_ID =
            new ClientId(OfflinePageBridge.BOOKMARK_NAMESPACE, "1234");
    private static final ClientId ASYNC_ID =
            new ClientId(OfflinePageBridge.ASYNC_NAMESPACE, "5678");
    private static final String SHARED_URI = "http://127.0.0.1/chrome/test/data/android/about.html";
    private static final String CONTENT_URI = "content://chromium/some-content-id";
    private static final String FILE_URI = "file://some-dir/some-file.mhtml";
    private static final String INVALID_URI = "This is not a uri.";
    private static final String EMPTY_URI = "";
    private static final String EMPTY_PATH = "";
    private static final String CACHE_SUBDIR = "/Offline Pages/archives";
    private static final String NEW_FILE = "/newfile.mhtml";
    private static final String TITLE = "My web page";
    private static final String PAGE_ID = "42";
    private static final long OFFLINE_ID = 42;
    private static final long FILE_SIZE = 65535;
    private static final String REQUEST_ORIGIN = "";

    private OfflinePageBridge mOfflinePageBridge;
    private EmbeddedTestServer mTestServer;
    private String mTestPage;
    private boolean mServerTurnedOn = false;

    @Before
    public void setUp() throws Exception {
        mActivityTestRule.startMainActivityOnBlankPage();
        final Semaphore semaphore = new Semaphore(0);
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                // Ensure we start in an online state.
                NetworkChangeNotifier.forceConnectivityState(true);

                Profile profile = Profile.getLastUsedProfile();
                mOfflinePageBridge = OfflinePageBridge.getForProfile(profile);
                if (!NetworkChangeNotifier.isInitialized()) {
                    NetworkChangeNotifier.init();
                }
                if (mOfflinePageBridge.isOfflinePageModelLoaded()) {
                    semaphore.release();
                } else {
                    mOfflinePageBridge.addObserver(new OfflinePageModelObserver() {
                        @Override
                        public void offlinePageModelLoaded() {
                            semaphore.release();
                            mOfflinePageBridge.removeObserver(this);
                        }
                    });
                }
            }
        });
        Assert.assertTrue(semaphore.tryAcquire(TIMEOUT_MS, TimeUnit.MILLISECONDS));

        mTestServer = EmbeddedTestServer.createAndStartServer(InstrumentationRegistry.getContext());
        mServerTurnedOn = true;
    }

    @After
    public void tearDown() throws Exception {
        turnOffServer();
    }

    Activity activity() {
        return mActivityTestRule.getActivity();
    }

    /** We must turn off the server only once, since stopAndDestroyServer() assumes the server is
     * on, and will wait indefinitely for it, timing out the unit test otherwise.
     */
    public void turnOffServer() throws Exception {
        if (mServerTurnedOn) {
            mTestServer.stopAndDestroyServer();
            mServerTurnedOn = false;
        }
    }

    /**
     * Mock implementation of the SnackbarController.
     */
    static class MockSnackbarController implements SnackbarController {
        private int mTabId;
        private boolean mDismissed;
        private static final long SNACKBAR_TIMEOUT = 7 * 1000;
        private static final long POLLING_INTERVAL = 100;

        public MockSnackbarController() {
            super();
            mTabId = Tab.INVALID_TAB_ID;
            mDismissed = false;
        }

        public void waitForSnackbarControllerToFinish() {
            CriteriaHelper.pollUiThread(
                    new Criteria("Failed while waiting for snackbar calls to complete.") {
                        @Override
                        public boolean isSatisfied() {
                            return mDismissed;
                        }
                    },
                    SNACKBAR_TIMEOUT, POLLING_INTERVAL);
        }

        @Override
        public void onAction(Object actionData) {
            mTabId = (int) actionData;
        }

        @Override
        public void onDismissNoAction(Object actionData) {
            if (actionData == null) return;
            mTabId = (int) actionData;
            mDismissed = true;
        }

        public int getLastTabId() {
            return mTabId;
        }

        public boolean getDismissed() {
            return mDismissed;
        }
    }

    /**
     * Share callback to be used by tests.  So that we can wait for the callback, it
     * takes a param of a semaphore to clear when the callback is finally called.
     */
    class TestShareCallback implements Callback<ShareParams> {
        private Semaphore mSemaphore;
        private String mUri;

        public TestShareCallback(Semaphore semaphore) {
            mSemaphore = semaphore;
        }

        @Override
        public void onResult(ShareParams shareParams) {
            mUri = shareParams.getUrl();
            mSemaphore.release();
        }

        public String getSharedUri() {
            return mUri;
        }
    }

    @Test
    @SmallTest
    @DisabledTest(message = "crbug.com/786237")
    public void testShowOfflineSnackbarIfNecessary() throws Exception {
        // Arrange - build a mock controller for sensing.
        OfflinePageUtils.setSnackbarDurationForTesting(1000);
        final MockSnackbarController mockSnackbarController = new MockSnackbarController();

        // Save an offline page.
        loadPageAndSave(BOOKMARK_ID);

        // With network disconnected, loading an online URL will result in loading an offline page.
        // Note that this will create a SnackbarController when the page loads, but we use our own
        // for the test. The one created here will also get the notification, but that won't
        // interfere with our test.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                NetworkChangeNotifier.forceConnectivityState(false);
            }
        });
        String testUrl = mTestServer.getURL(TEST_PAGE);
        mActivityTestRule.loadUrl(testUrl);

        int tabId = mActivityTestRule.getActivity().getActivityTab().getId();

        // Act.  This needs to be called from the UI thread.
        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                OfflinePageTabObserver offlineObserver = new OfflinePageTabObserver(
                        mActivityTestRule.getActivity().getTabModelSelector(),
                        mActivityTestRule.getActivity().getSnackbarManager(),
                        mockSnackbarController);
                OfflinePageTabObserver.setObserverForTesting(
                        mActivityTestRule.getActivity(), offlineObserver);
                OfflinePageUtils.showOfflineSnackbarIfNecessary(
                        mActivityTestRule.getActivity().getActivityTab());

                // Pretend that we went online, this should cause the snackbar to show.
                // This call will set the isConnected call to return true.
                NetworkChangeNotifier.forceConnectivityState(true);
                // This call will make an event get sent with connection type CONNECTION_WIFI.
                NetworkChangeNotifier.fakeNetworkConnected(0, ConnectionType.CONNECTION_WIFI);
            }
        });

        // Wait for the snackbar to be dismissed before we check its values.  The snackbar is on a
        // three second timer, and will dismiss itself in about 3 seconds.
        mockSnackbarController.waitForSnackbarControllerToFinish();

        // Assert snackbar was shown.
        Assert.assertEquals(tabId, mockSnackbarController.getLastTabId());
        Assert.assertTrue(mockSnackbarController.getDismissed());
    }

    @Test
    @MediumTest
    @CommandLineFlags.Remove({"enable-features=OfflinePagesSharing"})
    @CommandLineFlags.Add({"disable-features=OfflinePagesSharing"})
    // This tests that the offline page can't be shared if the sharing flag turned off.
    public void testDoNotShareOfflinePageWhenFeatureDisabled() throws Exception {
        loadOfflinePage(ASYNC_ID);

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                boolean shared =
                        OfflinePageUtils.maybeShareOfflinePage(mActivityTestRule.getActivity(),
                                mActivityTestRule.getActivity().getActivityTab(), null);
                // Since the sharing flag is off, we do not share the page.  This allows
                // normal URL-as-text sharing mechanisms to proceed.
                Assert.assertFalse(shared);
            }
        });
    }

    @Test
    @MediumTest
    @CommandLineFlags.Add({"enable-features=OfflinePagesSharing"})
    public void testSharePublicOfflinePage() throws Exception {
        loadOfflinePage(ASYNC_ID);
        final Semaphore semaphore = new Semaphore(0);
        final TestShareCallback shareCallback = new TestShareCallback(semaphore);

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                boolean shared =
                        OfflinePageUtils.maybeShareOfflinePage(mActivityTestRule.getActivity(),
                                mActivityTestRule.getActivity().getActivityTab(), shareCallback);
                // Attempt to share a public page should pass the initial checks and return true,
                // which means the callback will be called.
                Assert.assertTrue(shared);
            }
        });

        // Wait for share callback to get called.
        Assert.assertTrue(semaphore.tryAcquire(TIMEOUT_MS, TimeUnit.MILLISECONDS));
        // Assert that URI is what we expected.
        String foundUri = shareCallback.getSharedUri();
        Uri uri = Uri.parse(foundUri);
        String uriPath = uri.getPath();
        Assert.assertEquals(TEST_PAGE, uriPath);
    }

    @Test
    @MediumTest
    @CommandLineFlags.Add({"enable-features=OfflinePagesSharing"})
    public void testShareTemporaryOfflinePage() throws Exception {
        loadOfflinePage(BOOKMARK_ID);
        final Semaphore semaphore = new Semaphore(0);
        final TestShareCallback shareCallback = new TestShareCallback(semaphore);

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                boolean shared =
                        OfflinePageUtils.maybeShareOfflinePage(mActivityTestRule.getActivity(),
                                mActivityTestRule.getActivity().getActivityTab(), shareCallback);
                // The attempt to share a temporary page should share a content URL.
                Assert.assertTrue(shared);
            }
        });
        // Wait for share callback to get called.
        Assert.assertTrue(semaphore.tryAcquire(TIMEOUT_MS, TimeUnit.MILLISECONDS));
        // Assert that URI is what we expected.
        String foundUri = shareCallback.getSharedUri();
        Uri uri = Uri.parse(foundUri);
        String uriPath = uri.getPath();
        Assert.assertEquals(TEST_PAGE, uriPath);
    }

    // Checks on the UI thread if an offline path corresponds to a sharable file.
    private void checkIfOfflinePageIsSharable(
            final String filePath, final String uriPath, final String namespace, boolean sharable) {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                OfflinePageItem privateOfflinePageItem = new OfflinePageItem(uriPath, OFFLINE_ID,
                        namespace, PAGE_ID, TITLE, filePath, FILE_SIZE, 0, 0, 0, REQUEST_ORIGIN);
                OfflinePageBridge offlinePageBridge = OfflinePageBridge.getForProfile(
                        mActivityTestRule.getActivity().getActivityTab().getProfile());

                boolean isSharable = OfflinePageUtils.isOfflinePageShareable(
                        offlinePageBridge, privateOfflinePageItem, Uri.parse(uriPath));
                Assert.assertEquals(sharable, isSharable);
            }
        });
    }

    @Test
    @MediumTest
    public void testIsOfflinePageSharable() throws Exception {
        // This test needs the sharing command line flag turned on. so we do not override the
        // default.
        final String privatePath = activity().getApplicationContext().getCacheDir().getPath();
        final String publicPath = Environment.getExternalStorageDirectory().getPath();
        final String async = OfflinePageBridge.ASYNC_NAMESPACE;

        // Check that an offline page item in the private directory is sharable, since we can
        // upgrade it.
        final String fullPrivatePath = privatePath + CACHE_SUBDIR + NEW_FILE;
        checkIfOfflinePageIsSharable(fullPrivatePath, SHARED_URI, async, true);

        // Check that an offline page item with no file path is not sharable.
        checkIfOfflinePageIsSharable(EMPTY_PATH, SHARED_URI, async, false);

        // Check that a public offline page item with a file path is sharable.
        final String fullPublicPath = publicPath + NEW_FILE;
        checkIfOfflinePageIsSharable(fullPublicPath, SHARED_URI, async, true);

        // Check that a page with a content URI and no file path is sharable.
        checkIfOfflinePageIsSharable(EMPTY_PATH, CONTENT_URI, async, true);

        // Check that a page with a file URI and no file path is sharable.
        checkIfOfflinePageIsSharable(EMPTY_PATH, FILE_URI, async, true);

        // Check that a malformed URI is not sharable.
        checkIfOfflinePageIsSharable(EMPTY_PATH, INVALID_URI, async, false);

        // Check that an empty URL is not sharable.
        checkIfOfflinePageIsSharable(fullPublicPath, EMPTY_URI, async, false);

        // Check that pages with temporary namespaces are not sharable.
        checkIfOfflinePageIsSharable(
                fullPrivatePath, SHARED_URI, OfflinePageBridge.BOOKMARK_NAMESPACE, true);
        checkIfOfflinePageIsSharable(
                fullPrivatePath, SHARED_URI, OfflinePageBridge.LAST_N_NAMESPACE, true);
        checkIfOfflinePageIsSharable(
                fullPrivatePath, SHARED_URI, OfflinePageBridge.CCT_NAMESPACE, true);
        checkIfOfflinePageIsSharable(
                fullPrivatePath, SHARED_URI, OfflinePageBridge.SUGGESTED_ARTICLES_NAMESPACE, true);
    }

    @Test
    @SmallTest
    public void testMhtmlPropertiesFromRenderer() throws Exception {
        // This gets a file:// URL which should result in an untrusted offline page.
        String testUrl = UrlUtils.getTestFileUrl("offline_pages/hello.mhtml");
        mActivityTestRule.loadUrl(testUrl);

        final AtomicReference<OfflinePageItem> offlinePageItem = new AtomicReference<>();
        ThreadUtils.runOnUiThreadBlocking(() -> {
            offlinePageItem.set(OfflinePageUtils.getOfflinePage(
                    mActivityTestRule.getActivity().getActivityTab()));
        });

        Assert.assertEquals("http://www.example.com/", offlinePageItem.get().getUrl());
        Assert.assertEquals(1321901946000L, offlinePageItem.get().getCreationTimeMs());
    }

    private void loadPageAndSave(ClientId clientId) throws Exception {
        mTestPage = mTestServer.getURL(TEST_PAGE);
        mActivityTestRule.loadUrl(mTestPage);
        savePage(SavePageResult.SUCCESS, mTestPage, clientId);
    }

    // Utility to load an offline page into the current tab.
    private void loadOfflinePage(ClientId clientId) throws Exception {
        // Start by loading a normal page, and saving an offline copy.
        loadPageAndSave(clientId);

        // Change the state to offline by shutting down the server and simulating the network being
        // turned off.
        turnOffServer();
        // Turning off the network must be done on the UI thread.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                NetworkChangeNotifier.forceConnectivityState(false);
            }
        });

        // Reload the page, which will cause the offline version to be loaded, since we are
        // now "offline".
        mActivityTestRule.loadUrl(mTestPage);
    }

    // Save an offline copy of the current page in the tab.
    private void savePage(final int expectedResult, final String expectedUrl, ClientId clientId)
            throws InterruptedException {
        final Semaphore semaphore = new Semaphore(0);
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mOfflinePageBridge.savePage(
                        mActivityTestRule.getWebContents(), clientId, new SavePageCallback() {
                            @Override
                            public void onSavePageDone(
                                    int savePageResult, String url, long offlineId) {
                                Assert.assertEquals(
                                        "Requested and returned URLs differ.", expectedUrl, url);
                                Assert.assertEquals(
                                        "Save result incorrect.", expectedResult, savePageResult);
                                semaphore.release();
                            }
                        });
            }
        });
        Assert.assertTrue(semaphore.tryAcquire(TIMEOUT_MS, TimeUnit.MILLISECONDS));
    }
}
