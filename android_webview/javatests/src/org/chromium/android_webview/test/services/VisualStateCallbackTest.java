// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.test.services;

import android.content.Context;
import android.support.test.filters.SmallTest;
import android.view.ViewGroup;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.android_webview.AwBrowserContext;
import org.chromium.android_webview.AwContents;
import org.chromium.android_webview.AwContents.DependencyFactory;
import org.chromium.android_webview.AwContents.InternalAccessDelegate;
import org.chromium.android_webview.AwContents.NativeDrawGLFunctorFactory;
import org.chromium.android_webview.AwContentsClient;
import org.chromium.android_webview.AwRenderProcessGoneDetail;
import org.chromium.android_webview.AwSettings;
import org.chromium.android_webview.AwSwitches;
import org.chromium.android_webview.test.AwActivityTestRule;
import org.chromium.android_webview.test.AwJUnit4ClassRunner;
import org.chromium.android_webview.test.AwTestContainerView;
import org.chromium.android_webview.test.RenderProcessGoneHelper;
import org.chromium.android_webview.test.TestAwContents;
import org.chromium.android_webview.test.TestAwContentsClient;
import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.base.test.util.parameter.SkipCommandLineParameterization;
import org.chromium.content_public.common.ContentUrlConstants;

import java.util.concurrent.TimeUnit;

/**
 * Test VisualStateCallback when render process is gone.
 */
@RunWith(AwJUnit4ClassRunner.class)
public class VisualStateCallbackTest {
    @Rule
    public AwActivityTestRule mActivityTestRule = new AwActivityTestRule();

    private static class VisualStateCallbackHelper extends CallbackHelper {
        // Indicates VisualStateCallback has been received by AwContents, but
        // not forwarded to app's callback class.
        private boolean mVisualStateCallbackArrived;

        public void onVisualStateCallbackArrived() {
            mVisualStateCallbackArrived = true;
            notifyCalled();
        }

        public boolean visualStateCallbackArrived() {
            return mVisualStateCallbackArrived;
        }
    }

    private static class RenderProcessGoneTestAwContentsClient extends TestAwContentsClient {
        @Override
        public boolean onRenderProcessGone(AwRenderProcessGoneDetail detail) {
            return true;
        }
    }

    private static class VisualStateCallbackTestAwContents extends TestAwContents {
        private VisualStateCallbackHelper mVisualStateCallbackHelper;

        private VisualStateCallback mCallback;
        private long mRequestId;

        public VisualStateCallbackTestAwContents(AwBrowserContext browserContext,
                ViewGroup containerView, Context context,
                InternalAccessDelegate internalAccessAdapter,
                NativeDrawGLFunctorFactory nativeDrawGLFunctorFactory,
                AwContentsClient contentsClient, AwSettings settings,
                DependencyFactory dependencyFactory) {
            super(browserContext, containerView, context, internalAccessAdapter,
                    nativeDrawGLFunctorFactory, contentsClient, settings, dependencyFactory);
            mVisualStateCallbackHelper = new VisualStateCallbackHelper();
        }

        public VisualStateCallbackHelper getVisualStateCallbackHelper() {
            return mVisualStateCallbackHelper;
        }

        @Override
        public void invokeVisualStateCallback(
                final VisualStateCallback callback, final long requestId) {
            mCallback = callback;
            mRequestId = requestId;
            mVisualStateCallbackHelper.onVisualStateCallbackArrived();
        }

        public void doInvokeVisualStateCallbackOnUiThread() {
            final VisualStateCallbackTestAwContents awContents = this;
            ThreadUtils.runOnUiThread(() -> awContents.doInvokeVisualStateCallback());
        }

        private void doInvokeVisualStateCallback() {
            super.invokeVisualStateCallback(mCallback, mRequestId);
        }
    }

    private static class CrashTestDependencyFactory
            extends AwActivityTestRule.TestDependencyFactory {
        @Override
        public AwContents createAwContents(AwBrowserContext browserContext, ViewGroup containerView,
                Context context, InternalAccessDelegate internalAccessAdapter,
                NativeDrawGLFunctorFactory nativeDrawGLFunctorFactory,
                AwContentsClient contentsClient, AwSettings settings,
                DependencyFactory dependencyFactory) {
            return new VisualStateCallbackTestAwContents(browserContext, containerView, context,
                    internalAccessAdapter, nativeDrawGLFunctorFactory, contentsClient, settings,
                    dependencyFactory);
        }
    }

    private static class VisualStateCallbackImpl extends AwContents.VisualStateCallback {
        private int mRequestId;
        private boolean mCalled;

        @Override
        public void onComplete(long requestId) {
            mCalled = true;
        }

        public int requestId() {
            return mRequestId;
        }

        public boolean called() {
            return mCalled;
        }
    }

    private VisualStateCallbackTestAwContents mAwContents;
    private RenderProcessGoneHelper mHelper;

    @Before
    public void setUp() throws Exception {
        RenderProcessGoneTestAwContentsClient contentsClient =
                new RenderProcessGoneTestAwContentsClient();
        AwTestContainerView testView = mActivityTestRule.createAwTestContainerViewOnMainSync(
                contentsClient, false, new CrashTestDependencyFactory());
        mAwContents = (VisualStateCallbackTestAwContents) testView.getAwContents();
        mHelper = mAwContents.getRenderProcessGoneHelper();
    }

    // Tests the callback isn't invoked if insertVisualStateCallback() is called after render
    // process gone, but before the AwContentsClient knows about it.
    @Test
    @Feature({"AndroidWebView"})
    @SmallTest
    @CommandLineFlags.Add(AwSwitches.WEBVIEW_SANDBOXED_RENDERER)
    @SkipCommandLineParameterization
    public void testAddVisualStateCallbackAfterRendererGone() throws Throwable {
        final VisualStateCallbackImpl vsImpl = new VisualStateCallbackImpl();
        mHelper.setOnRenderProcessGoneTask(
                () -> mAwContents.insertVisualStateCallback(vsImpl.requestId(), vsImpl));
        mActivityTestRule.loadUrlAsync(mAwContents, "chrome://kill");

        mHelper.waitForRenderProcessGoneNotifiedToAwContentsClient();

        mActivityTestRule.destroyAwContentsOnMainSync(mAwContents);

        mHelper.waitForAwContentsDestroyed();
        Assert.assertFalse(vsImpl.called());
    }

    // Tests the callback isn't invoked when AwContents knows about render process being gone.
    @Test
    @Feature({"AndroidWebView"})
    @SmallTest
    @RetryOnFailure
    @CommandLineFlags.Add(AwSwitches.WEBVIEW_SANDBOXED_RENDERER)
    @SkipCommandLineParameterization
    public void testVisualStateCallbackNotCalledAfterRendererGone() throws Throwable {
        VisualStateCallbackImpl vsImpl = new VisualStateCallbackImpl();
        mActivityTestRule.insertVisualStateCallbackOnUIThread(
                mAwContents, vsImpl.requestId(), vsImpl);
        VisualStateCallbackHelper vsCallbackHelper = mAwContents.getVisualStateCallbackHelper();
        int callCount = vsCallbackHelper.getCallCount();
        mActivityTestRule.loadUrlAsync(mAwContents, ContentUrlConstants.ABOUT_BLANK_DISPLAY_URL);
        vsCallbackHelper.waitForCallback(
                callCount, 1, CallbackHelper.WAIT_TIMEOUT_SECONDS, TimeUnit.SECONDS);
        Assert.assertEquals(callCount + 1, vsCallbackHelper.getCallCount());
        Assert.assertTrue(vsCallbackHelper.visualStateCallbackArrived());
        mActivityTestRule.killRenderProcessOnUiThreadAsync(mAwContents);

        mHelper.waitForRenderProcessGone();
        mAwContents.doInvokeVisualStateCallbackOnUiThread();

        mHelper.waitForRenderProcessGoneNotifiedToAwContentsClient();
        Assert.assertFalse(vsImpl.called());

        mActivityTestRule.destroyAwContentsOnMainSync(mAwContents);

        mHelper.waitForAwContentsDestroyed();
        Assert.assertFalse(vsImpl.called());
    }
}
