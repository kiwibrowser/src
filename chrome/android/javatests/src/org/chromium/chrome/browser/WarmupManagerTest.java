// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import static org.chromium.base.test.util.Restriction.RESTRICTION_TYPE_LOW_END_DEVICE;
import static org.chromium.base.test.util.Restriction.RESTRICTION_TYPE_NON_LOW_END_DEVICE;

import android.content.Context;
import android.support.test.InstrumentationRegistry;
import android.support.test.annotation.UiThreadTest;
import android.support.test.filters.SmallTest;
import android.support.test.rule.UiThreadTestRule;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.RuleChain;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.MetricsUtils;
import org.chromium.base.test.util.Restriction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.init.ChromeBrowserInitializer;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.test.ChromeBrowserTestRule;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.WebContentsObserver;
import org.chromium.net.test.EmbeddedTestServer;

import java.util.concurrent.Callable;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;

/** Tests for {@link WarmupManager} */
@RunWith(BaseJUnit4ClassRunner.class)
public class WarmupManagerTest {
    @Rule
    public final RuleChain mChain =
            RuleChain.outerRule(new ChromeBrowserTestRule()).around(new UiThreadTestRule());

    private WarmupManager mWarmupManager;
    private Context mContext;

    @Before
    public void setUp() throws Exception {
        mContext = InstrumentationRegistry.getInstrumentation()
                           .getTargetContext()
                           .getApplicationContext();
        ThreadUtils.runOnUiThreadBlocking(new Callable<Void>() {
            @Override
            public Void call() throws Exception {
                ChromeBrowserInitializer.getInstance(mContext).handleSynchronousStartup();
                mWarmupManager = WarmupManager.getInstance();
                return null;
            }
        });
    }

    @After
    public void tearDown() throws Exception {
        ThreadUtils.runOnUiThreadBlocking(() -> mWarmupManager.destroySpareWebContents());
    }

    @Test
    @SmallTest
    @UiThreadTest
    @Restriction(RESTRICTION_TYPE_LOW_END_DEVICE)
    public void testNoSpareRendererOnLowEndDevices() throws Throwable {
        mWarmupManager.createSpareWebContents();
        Assert.assertFalse(mWarmupManager.hasSpareWebContents());
    }

    @Test
    @SmallTest
    @Restriction(RESTRICTION_TYPE_NON_LOW_END_DEVICE)
    public void testCreateAndTakeSpareRenderer() {
        final AtomicBoolean isRenderViewReady = new AtomicBoolean();
        final AtomicReference<WebContents> webContentsReference = new AtomicReference<>();

        ThreadUtils.runOnUiThread(() -> {
            mWarmupManager.createSpareWebContents();
            Assert.assertTrue(mWarmupManager.hasSpareWebContents());
            WebContents webContents = mWarmupManager.takeSpareWebContents(false, false);
            Assert.assertNotNull(webContents);
            Assert.assertFalse(mWarmupManager.hasSpareWebContents());
            WebContentsObserver observer = new WebContentsObserver(webContents) {
                @Override
                public void renderViewReady() {
                    isRenderViewReady.set(true);
                }
            };

            // This is not racy because {@link WebContentsObserver} methods are called on the UI
            // thread by posting a task. See {@link RenderViewHostImpl::PostRenderViewReady}.
            webContents.addObserver(observer);
            webContentsReference.set(webContents);
        });
        CriteriaHelper.pollUiThread(new Criteria("Spare renderer is not initialized") {
            @Override
            public boolean isSatisfied() {
                return isRenderViewReady.get();
            }
        });
        ThreadUtils.runOnUiThread(() -> webContentsReference.get().destroy());
    }

    /** Tests that taking a spare WebContents makes it unavailable to subsequent callers. */
    @Test
    @SmallTest
    @UiThreadTest
    @Restriction(RESTRICTION_TYPE_NON_LOW_END_DEVICE)
    public void testTakeSpareWebContents() throws Throwable {
        mWarmupManager.createSpareWebContents();
        WebContents webContents = mWarmupManager.takeSpareWebContents(false, false);
        Assert.assertNotNull(webContents);
        Assert.assertFalse(mWarmupManager.hasSpareWebContents());
        webContents.destroy();
    }

    @Test
    @SmallTest
    @UiThreadTest
    @Restriction(RESTRICTION_TYPE_NON_LOW_END_DEVICE)
    public void testTakeSpareWebContentsChecksArguments() throws Throwable {
        mWarmupManager.createSpareWebContents();
        Assert.assertNull(mWarmupManager.takeSpareWebContents(true, false));
        Assert.assertNull(mWarmupManager.takeSpareWebContents(true, true));
        Assert.assertTrue(mWarmupManager.hasSpareWebContents());
        Assert.assertNotNull(mWarmupManager.takeSpareWebContents(false, true));
        Assert.assertFalse(mWarmupManager.hasSpareWebContents());
    }

    @Test
    @SmallTest
    @UiThreadTest
    @Restriction(RESTRICTION_TYPE_NON_LOW_END_DEVICE)
    public void testClearsDeadWebContents() throws Throwable {
        mWarmupManager.createSpareWebContents();
        mWarmupManager.mSpareWebContents.simulateRendererKilledForTesting(false);
        Assert.assertNull(mWarmupManager.takeSpareWebContents(false, false));
    }

    @Test
    @SmallTest
    @UiThreadTest
    @Restriction(RESTRICTION_TYPE_NON_LOW_END_DEVICE)
    public void testRecordSpareWebContentsStatus() throws Throwable {
        String name = WarmupManager.WEBCONTENTS_STATUS_HISTOGRAM;
        MetricsUtils.HistogramDelta createdDelta =
                new MetricsUtils.HistogramDelta(name, WarmupManager.WEBCONTENTS_STATUS_CREATED);
        MetricsUtils.HistogramDelta usedDelta =
                new MetricsUtils.HistogramDelta(name, WarmupManager.WEBCONTENTS_STATUS_USED);
        MetricsUtils.HistogramDelta killedDelta =
                new MetricsUtils.HistogramDelta(name, WarmupManager.WEBCONTENTS_STATUS_KILLED);
        MetricsUtils.HistogramDelta destroyedDelta =
                new MetricsUtils.HistogramDelta(name, WarmupManager.WEBCONTENTS_STATUS_DESTROYED);

        // Created, used.
        mWarmupManager.createSpareWebContents();
        Assert.assertEquals(1, createdDelta.getDelta());
        Assert.assertNotNull(mWarmupManager.takeSpareWebContents(false, false));
        Assert.assertEquals(1, usedDelta.getDelta());

        // Created, killed.
        mWarmupManager.createSpareWebContents();
        Assert.assertEquals(2, createdDelta.getDelta());
        Assert.assertNotNull(mWarmupManager.mSpareWebContents);
        mWarmupManager.mSpareWebContents.simulateRendererKilledForTesting(false);
        Assert.assertEquals(1, killedDelta.getDelta());
        Assert.assertNull(mWarmupManager.takeSpareWebContents(false, false));

        // Created, destroyed.
        mWarmupManager.createSpareWebContents();
        Assert.assertEquals(3, createdDelta.getDelta());
        Assert.assertNotNull(mWarmupManager.mSpareWebContents);
        mWarmupManager.destroySpareWebContents();
        Assert.assertEquals(1, destroyedDelta.getDelta());
    }

    /** Checks that the View inflation works. */
    @Test
    @SmallTest
    @UiThreadTest
    public void testInflateLayout() throws Throwable {
        int layoutId = R.layout.custom_tabs_control_container;
        int toolbarId = R.layout.custom_tabs_toolbar;
        mWarmupManager.initializeViewHierarchy(mContext, layoutId, toolbarId);
        Assert.assertTrue(mWarmupManager.hasViewHierarchyWithToolbar(layoutId));
    }

    /** Tests that preconnects can be initiated from the Java side. */
    @Test
    @SmallTest
    public void testPreconnect() throws Exception {
        EmbeddedTestServer server = new EmbeddedTestServer();
        try {
            // The predictor prepares 2 connections when asked to preconnect. Initializes the
            // semaphore to be unlocked after 2 connections.
            final Semaphore connectionsSemaphore = new Semaphore(1 - 2);

            // Cannot use EmbeddedTestServer#createAndStartServer(), as we need to add the
            // connection listener.
            server.initializeNative(mContext, EmbeddedTestServer.ServerHTTPSSetting.USE_HTTP);
            server.addDefaultHandlers("");
            server.setConnectionListener(new EmbeddedTestServer.ConnectionListener() {
                @Override
                public void acceptedSocket(long socketId) {
                    connectionsSemaphore.release();
                }
            });
            server.start();

            final String url = server.getURL("/hello_world.html");
            ThreadUtils.runOnUiThread(() -> mWarmupManager.maybePreconnectUrlAndSubResources(
                    Profile.getLastUsedProfile(), url));
            if (!connectionsSemaphore.tryAcquire(5, TimeUnit.SECONDS)) {
                // Starts at -1.
                int actualConnections = connectionsSemaphore.availablePermits() + 1;
                Assert.fail("Expected 2 connections, got " + actualConnections);
            }
        } finally {
            server.stopAndDestroyServer();
        }
    }
}
