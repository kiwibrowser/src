// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.base.library_loader.LoaderErrors;
import org.chromium.base.library_loader.ProcessInitException;
import org.chromium.base.test.BaseJUnit4ClassRunner;

/**
 * Test of BrowserStartupController
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class BrowserStartupControllerTest {
    private TestBrowserStartupController mController;

    private static class TestBrowserStartupController extends BrowserStartupController {

        private int mStartupResult;
        private boolean mLibraryLoadSucceeds;
        private int mInitializedCounter = 0;
        private boolean mStartupCompleteCalled;

        @Override
        void prepareToStartBrowserProcess(boolean singleProcess, Runnable completionCallback)
                throws ProcessInitException {
            if (!mLibraryLoadSucceeds) {
                throw new ProcessInitException(
                        LoaderErrors.LOADER_ERROR_NATIVE_LIBRARY_LOAD_FAILED);
            } else if (completionCallback != null) {
                completionCallback.run();
            }
        }

        private TestBrowserStartupController() {
            super(LibraryProcessType.PROCESS_BROWSER);
        }

        @Override
        int contentStart() {
            mInitializedCounter++;
            // Post to the UI thread to emulate what would happen in a real scenario.
            ThreadUtils.postOnUiThread(new Runnable() {
                @Override
                public void run() {
                    if (mStartupCompleteCalled) return;
                    BrowserStartupController.browserStartupComplete(mStartupResult);
                }
            });
            return mStartupResult;
        }

        @Override
        void flushStartupTasks() {
            assert mInitializedCounter > 0;
            if (mStartupCompleteCalled) return;
            BrowserStartupController.browserStartupComplete(mStartupResult);
        }

        private int initializedCounter() {
            return mInitializedCounter;
        }
    }

    private static class TestStartupCallback implements BrowserStartupController.StartupCallback {
        private boolean mWasSuccess;
        private boolean mWasFailure;
        private boolean mHasStartupResult;

        @Override
        public void onSuccess() {
            assert !mHasStartupResult;
            mWasSuccess = true;
            mHasStartupResult = true;
        }

        @Override
        public void onFailure() {
            assert !mHasStartupResult;
            mWasFailure = true;
            mHasStartupResult = true;
        }
    }

    @Before
    public void setUp() throws Exception {
        mController = new TestBrowserStartupController();
        // Setting the static singleton instance field enables more correct testing, since it is
        // is possible to call {@link BrowserStartupController#browserStartupComplete(int)} instead
        // of {@link BrowserStartupController#executeEnqueuedCallbacks(int, boolean)} directly.
        BrowserStartupController.overrideInstanceForTest(mController);
    }

    @Test
    @SmallTest
    public void testSingleAsynchronousStartupRequest() {
        mController.mStartupResult = BrowserStartupController.STARTUP_SUCCESS;
        mController.mLibraryLoadSucceeds = true;
        final TestStartupCallback callback = new TestStartupCallback();

        // Kick off the asynchronous startup request.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                try {
                    mController.startBrowserProcessesAsync(true, callback);
                } catch (Exception e) {
                    Assert.fail("Browser should have started successfully");
                }
            }
        });

        Assert.assertEquals("The browser process should have been initialized one time.", 1,
                mController.initializedCounter());

        // Wait for callbacks to complete.
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();

        Assert.assertTrue("Callback should have been executed.", callback.mHasStartupResult);
        Assert.assertTrue("Callback should have been a success.", callback.mWasSuccess);
    }

    @Test
    @SmallTest
    public void testMultipleAsynchronousStartupRequests() {
        mController.mStartupResult = BrowserStartupController.STARTUP_SUCCESS;
        mController.mLibraryLoadSucceeds = true;
        final TestStartupCallback callback1 = new TestStartupCallback();
        final TestStartupCallback callback2 = new TestStartupCallback();
        final TestStartupCallback callback3 = new TestStartupCallback();

        // Kick off the asynchronous startup requests.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                try {
                    mController.startBrowserProcessesAsync(true, callback1);
                } catch (Exception e) {
                    Assert.fail("Browser should have started successfully");
                }
            }
        });
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                try {
                    mController.startBrowserProcessesAsync(true, callback2);
                } catch (Exception e) {
                    Assert.fail("Browser should have started successfully");
                }
            }
        });
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mController.addStartupCompletedObserver(callback3);
            }
        });

        Assert.assertEquals("The browser process should have been initialized one time.", 1,
                mController.initializedCounter());

        // Wait for callbacks to complete.
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();

        Assert.assertTrue("Callback 1 should have been executed.", callback1.mHasStartupResult);
        Assert.assertTrue("Callback 1 should have been a success.", callback1.mWasSuccess);
        Assert.assertTrue("Callback 2 should have been executed.", callback2.mHasStartupResult);
        Assert.assertTrue("Callback 2 should have been a success.", callback2.mWasSuccess);
        Assert.assertTrue("Callback 3 should have been executed.", callback3.mHasStartupResult);
        Assert.assertTrue("Callback 3 should have been a success.", callback3.mWasSuccess);
    }

    @Test
    @SmallTest
    public void testConsecutiveAsynchronousStartupRequests() {
        mController.mStartupResult = BrowserStartupController.STARTUP_SUCCESS;
        mController.mLibraryLoadSucceeds = true;
        final TestStartupCallback callback1 = new TestStartupCallback();
        final TestStartupCallback callback2 = new TestStartupCallback();

        // Kick off the asynchronous startup requests.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                try {
                    mController.startBrowserProcessesAsync(true, callback1);
                } catch (Exception e) {
                    Assert.fail("Browser should have started successfully");
                }
            }
        });
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mController.addStartupCompletedObserver(callback2);
            }
        });

        Assert.assertEquals("The browser process should have been initialized one time.", 1,
                mController.initializedCounter());

        // Wait for callbacks to complete.
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();

        Assert.assertTrue("Callback 1 should have been executed.", callback1.mHasStartupResult);
        Assert.assertTrue("Callback 1 should have been a success.", callback1.mWasSuccess);
        Assert.assertTrue("Callback 2 should have been executed.", callback2.mHasStartupResult);
        Assert.assertTrue("Callback 2 should have been a success.", callback2.mWasSuccess);

        final TestStartupCallback callback3 = new TestStartupCallback();
        final TestStartupCallback callback4 = new TestStartupCallback();

        // Kick off more asynchronous startup requests.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                try {
                    mController.startBrowserProcessesAsync(true, callback3);
                } catch (Exception e) {
                    Assert.fail("Browser should have started successfully");
                }
            }
        });
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mController.addStartupCompletedObserver(callback4);
            }
        });

        // Wait for callbacks to complete.
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();

        Assert.assertTrue("Callback 3 should have been executed.", callback3.mHasStartupResult);
        Assert.assertTrue("Callback 3 should have been a success.", callback3.mWasSuccess);
        Assert.assertTrue("Callback 4 should have been executed.", callback4.mHasStartupResult);
        Assert.assertTrue("Callback 4 should have been a success.", callback4.mWasSuccess);
    }

    @Test
    @SmallTest
    public void testSingleFailedAsynchronousStartupRequest() {
        mController.mStartupResult = BrowserStartupController.STARTUP_FAILURE;
        mController.mLibraryLoadSucceeds = true;
        final TestStartupCallback callback = new TestStartupCallback();

        // Kick off the asynchronous startup request.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                try {
                    mController.startBrowserProcessesAsync(true, callback);
                } catch (Exception e) {
                    Assert.fail("Browser should have started successfully");
                }
            }
        });

        Assert.assertEquals("The browser process should have been initialized one time.", 1,
                mController.initializedCounter());

        // Wait for callbacks to complete.
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();

        Assert.assertTrue("Callback should have been executed.", callback.mHasStartupResult);
        Assert.assertTrue("Callback should have been a failure.", callback.mWasFailure);
    }

    @Test
    @SmallTest
    public void testConsecutiveFailedAsynchronousStartupRequests() {
        mController.mStartupResult = BrowserStartupController.STARTUP_FAILURE;
        mController.mLibraryLoadSucceeds = true;
        final TestStartupCallback callback1 = new TestStartupCallback();
        final TestStartupCallback callback2 = new TestStartupCallback();

        // Kick off the asynchronous startup requests.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                try {
                    mController.startBrowserProcessesAsync(true, callback1);
                } catch (Exception e) {
                    Assert.fail("Browser should have started successfully");
                }
            }
        });
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mController.addStartupCompletedObserver(callback2);
            }
        });

        Assert.assertEquals("The browser process should have been initialized one time.", 1,
                mController.initializedCounter());

        // Wait for callbacks to complete.
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();

        Assert.assertTrue("Callback 1 should have been executed.", callback1.mHasStartupResult);
        Assert.assertTrue("Callback 1 should have been a failure.", callback1.mWasFailure);
        Assert.assertTrue("Callback 2 should have been executed.", callback2.mHasStartupResult);
        Assert.assertTrue("Callback 2 should have been a failure.", callback2.mWasFailure);

        final TestStartupCallback callback3 = new TestStartupCallback();
        final TestStartupCallback callback4 = new TestStartupCallback();

        // Kick off more asynchronous startup requests.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                try {
                    mController.startBrowserProcessesAsync(true, callback3);
                } catch (Exception e) {
                    Assert.fail("Browser should have started successfully");
                }
            }
        });
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mController.addStartupCompletedObserver(callback4);
            }
        });

        // Wait for callbacks to complete.
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();

        Assert.assertTrue("Callback 3 should have been executed.", callback3.mHasStartupResult);
        Assert.assertTrue("Callback 3 should have been a failure.", callback3.mWasFailure);
        Assert.assertTrue("Callback 4 should have been executed.", callback4.mHasStartupResult);
        Assert.assertTrue("Callback 4 should have been a failure.", callback4.mWasFailure);
    }

    @Test
    @SmallTest
    public void testSingleSynchronousRequest() {
        mController.mStartupResult = BrowserStartupController.STARTUP_SUCCESS;
        mController.mLibraryLoadSucceeds = true;
        // Kick off the synchronous startup.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                try {
                    mController.startBrowserProcessesSync(false);
                } catch (Exception e) {
                    Assert.fail("Browser should have started successfully");
                }
            }
        });

        Assert.assertEquals("The browser process should have been initialized one time.", 1,
                mController.initializedCounter());
    }

    @Test
    @SmallTest
    public void testAsyncThenSyncRequests() {
        mController.mStartupResult = BrowserStartupController.STARTUP_SUCCESS;
        mController.mLibraryLoadSucceeds = true;
        final TestStartupCallback callback = new TestStartupCallback();

        // Kick off the startups.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                try {
                    mController.startBrowserProcessesAsync(true, callback);
                } catch (Exception e) {
                    Assert.fail("Browser should have started successfully");
                }
                // To ensure that the async startup doesn't complete too soon we have
                // to do both these in a since Runnable instance. This avoids the
                // unpredictable race that happens in real situations.
                try {
                    mController.startBrowserProcessesSync(false);
                } catch (Exception e) {
                    Assert.fail("Browser should have started successfully");
                }
            }
        });

        Assert.assertEquals("The browser process should have been initialized twice.", 2,
                mController.initializedCounter());

        Assert.assertTrue("Callback should have been executed.", callback.mHasStartupResult);
        Assert.assertTrue("Callback should have been a success.", callback.mWasSuccess);
    }

    @Test
    @SmallTest
    public void testSyncThenAsyncRequests() {
        mController.mStartupResult = BrowserStartupController.STARTUP_SUCCESS;
        mController.mLibraryLoadSucceeds = true;
        final TestStartupCallback callback = new TestStartupCallback();

        // Do a synchronous startup first.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                try {
                    mController.startBrowserProcessesSync(false);
                } catch (Exception e) {
                    Assert.fail("Browser should have started successfully");
                }
            }
        });

        Assert.assertEquals("The browser process should have been initialized once.", 1,
                mController.initializedCounter());

        // Kick off the asynchronous startup request. This should just queue the callback.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                try {
                    mController.startBrowserProcessesAsync(true, callback);
                } catch (Exception e) {
                    Assert.fail("Browser should have started successfully");
                }
            }
        });

        Assert.assertEquals("The browser process should not have been initialized a second time.",
                1, mController.initializedCounter());

        // Wait for callbacks to complete.
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();

        Assert.assertTrue("Callback should have been executed.", callback.mHasStartupResult);
        Assert.assertTrue("Callback should have been a success.", callback.mWasSuccess);
    }

    @Test
    @SmallTest
    public void testLibraryLoadFails() {
        mController.mLibraryLoadSucceeds = false;
        final TestStartupCallback callback = new TestStartupCallback();

        // Kick off the asynchronous startup request.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                try {
                    mController.startBrowserProcessesAsync(true, callback);
                    Assert.fail("Browser should not have started successfully");
                } catch (Exception e) {
                    // Exception expected, ignore.
                }
            }
        });

        Assert.assertEquals("The browser process should not have been initialized.", 0,
                mController.initializedCounter());

        // Wait for callbacks to complete.
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
    }

}
