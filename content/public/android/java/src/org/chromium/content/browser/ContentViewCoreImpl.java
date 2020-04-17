// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.content.Context;
import android.view.ViewGroup;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.content.browser.accessibility.WebContentsAccessibilityImpl;
import org.chromium.content.browser.input.ImeAdapterImpl;
import org.chromium.content.browser.input.SelectPopup;
import org.chromium.content.browser.input.TextSuggestionHost;
import org.chromium.content.browser.selection.SelectionPopupControllerImpl;
import org.chromium.content.browser.webcontents.WebContentsImpl;
import org.chromium.content_public.browser.ContentViewCore;
import org.chromium.content_public.browser.ViewEventSink.InternalAccessDelegate;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.WebContents.UserDataFactory;
import org.chromium.ui.base.ViewAndroidDelegate;
import org.chromium.ui.base.WindowAndroid;

/**
 * Implementation of the interface {@ContentViewCore}.
 */
@JNINamespace("content")
public class ContentViewCoreImpl implements ContentViewCore {
    private static final String TAG = "cr_ContentViewCore";

    private WebContentsImpl mWebContents;

    // Native pointer to C++ ContentViewCore object which will be set by nativeInit().
    private long mNativeContentViewCore;

    private boolean mInitialized;

    private static final class UserDataFactoryLazyHolder {
        private static final UserDataFactory<ContentViewCoreImpl> INSTANCE =
                ContentViewCoreImpl::new;
    }

    /**
     * @param webContents The {@link WebContents} to find a {@link ContentViewCore} of.
     * @return            A {@link ContentViewCore} that is connected to {@code webContents} or
     *                    {@code null} if none exists.
     */
    public static ContentViewCoreImpl fromWebContents(WebContents webContents) {
        return webContents.getOrSetUserData(ContentViewCoreImpl.class, null);
    }

    /**
     * Constructs a new ContentViewCore.
     *
     * @param webContents {@link WebContents} to be associated with this object.
     */
    public ContentViewCoreImpl(WebContents webContents) {
        mWebContents = (WebContentsImpl) webContents;
    }

    public static ContentViewCoreImpl create(Context context, String productVersion,
            WebContents webContents, ViewAndroidDelegate viewDelegate,
            InternalAccessDelegate internalDispatcher, WindowAndroid windowAndroid) {
        ContentViewCoreImpl core = webContents.getOrSetUserData(
                ContentViewCoreImpl.class, UserDataFactoryLazyHolder.INSTANCE);
        assert core != null;
        assert !core.initialized();
        core.initialize(context, productVersion, viewDelegate, internalDispatcher, windowAndroid);
        return core;
    }

    private void initialize(Context context, String productVersion,
            ViewAndroidDelegate viewDelegate, InternalAccessDelegate internalDispatcher,
            WindowAndroid windowAndroid) {
        // Ensure ViewEventSinkImpl is initialized first before being accessed by
        // WindowEventObserverManagerImpl via WebContentsImpl.
        ViewEventSinkImpl.create(context, mWebContents);

        mWebContents.setViewAndroidDelegate(viewDelegate);
        mWebContents.setTopLevelNativeWindow(windowAndroid);
        mNativeContentViewCore = nativeInit(mWebContents);

        ViewGroup containerView = viewDelegate.getContainerView();
        ImeAdapterImpl.create(
                mWebContents, ImeAdapterImpl.createDefaultInputMethodManagerWrapper(context));
        SelectionPopupControllerImpl.create(context, windowAndroid, mWebContents);
        WebContentsAccessibilityImpl.create(context, containerView, mWebContents, productVersion);
        TapDisambiguator.create(context, mWebContents, containerView);
        TextSuggestionHost.create(context, mWebContents, windowAndroid);
        SelectPopup.create(context, mWebContents);
        Gamepad.create(context, mWebContents);

        ViewEventSinkImpl.from(mWebContents).setAccessDelegate(internalDispatcher);
        mWebContents.getRenderCoordinates().setDeviceScaleFactor(
                windowAndroid.getDisplay().getDipScale());

        mInitialized = true;
    }

    public boolean initialized() {
        return mInitialized;
    }

    @CalledByNative
    private void onNativeContentViewCoreDestroyed(long nativeContentViewCore) {
        assert nativeContentViewCore == mNativeContentViewCore;
        mNativeContentViewCore = 0;
    }

    @Override
    public void destroy() {
        if (mNativeContentViewCore != 0) {
            nativeOnJavaContentViewCoreDestroyed(mNativeContentViewCore);
        }
        // This is called to fix crash. See https://crbug.com/803244
        // TODO(jinsukkim): Use an observer to let the manager handle it on its own.
        GestureListenerManagerImpl.fromWebContents(mWebContents).reset();
        mWebContents.destroyContentsInternal();
        mWebContents = null;
        mNativeContentViewCore = 0;

        // See warning in javadoc before adding more clean up code here.
    }

    private native long nativeInit(WebContents webContents);
    private native void nativeOnJavaContentViewCoreDestroyed(long nativeContentViewCore);
}
