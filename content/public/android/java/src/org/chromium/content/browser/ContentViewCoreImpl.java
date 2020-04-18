// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.Configuration;
import android.view.ViewGroup;

import org.chromium.base.TraceEvent;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.content.browser.accessibility.WebContentsAccessibilityImpl;
import org.chromium.content.browser.input.ImeAdapterImpl;
import org.chromium.content.browser.input.SelectPopup;
import org.chromium.content.browser.input.TextSuggestionHost;
import org.chromium.content.browser.selection.SelectionPopupControllerImpl;
import org.chromium.content.browser.webcontents.WebContentsImpl;
import org.chromium.content_public.browser.ContentViewCore;
import org.chromium.content_public.browser.ContentViewCore.InternalAccessDelegate;
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

    private Context mContext;

    private InternalAccessDelegate mContainerViewInternals;
    private WebContentsImpl mWebContents;

    // Native pointer to C++ ContentViewCore object which will be set by nativeInit().
    private long mNativeContentViewCore;

    // Cached copy of all positions and scales as reported by the renderer.
    private RenderCoordinatesImpl mRenderCoordinates;

    // Whether the container view has view-level focus.
    private Boolean mHasViewFocus;

    // This is used in place of window focus on the container view, as we can't actually use window
    // focus due to issues where content expects to be focused while a popup steals window focus.
    // See https://crbug.com/686232 for more context.
    private boolean mPaused;

    // Whether we consider this CVC to have input focus. This is computed through mHasViewFocus and
    // mPaused. See the comments on mPaused for how this doesn't exactly match Android's notion of
    // input focus and why we need to do this.
    private Boolean mHasInputFocus;
    private boolean mHideKeyboardOnBlur;

    private boolean mInitialized;

    private static final class UserDataFactoryLazyHolder {
        private static final UserDataFactory<ContentViewCoreImpl> INSTANCE =
                ContentViewCoreImpl::new;
    }

    public Context getContext() {
        return mContext;
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
        mContext = context;
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

        setContainerViewInternals(internalDispatcher);
        mRenderCoordinates = mWebContents.getRenderCoordinates();
        mRenderCoordinates.setDeviceScaleFactor(windowAndroid.getDisplay().getDipScale());

        mInitialized = true;
    }

    public boolean initialized() {
        return mInitialized;
    }

    private SelectionPopupControllerImpl getSelectionPopupController() {
        return SelectionPopupControllerImpl.fromWebContents(mWebContents);
    }

    private GestureListenerManagerImpl getGestureListenerManager() {
        return GestureListenerManagerImpl.fromWebContents(mWebContents);
    }

    private ImeAdapterImpl getImeAdapter() {
        return ImeAdapterImpl.fromWebContents(mWebContents);
    }

    @CalledByNative
    private void onNativeContentViewCoreDestroyed(long nativeContentViewCore) {
        assert nativeContentViewCore == mNativeContentViewCore;
        mNativeContentViewCore = 0;
    }

    @Override
    public void setContainerViewInternals(InternalAccessDelegate internalDispatcher) {
        mContainerViewInternals = internalDispatcher;
        getGestureListenerManager().setScrollDelegate(internalDispatcher);
        ContentUiEventHandler.fromWebContents(mWebContents).setEventDelegate(internalDispatcher);
    }

    @Override
    public void destroy() {
        if (mNativeContentViewCore != 0) {
            nativeOnJavaContentViewCoreDestroyed(mNativeContentViewCore);
        }
        // This is called to fix crash. See https://crbug.com/803244
        // TODO(jinsukkim): Use an observer to let the manager handle it on its own.
        getGestureListenerManager().reset();
        mWebContents.destroyContentsInternal();
        mWebContents = null;
        mNativeContentViewCore = 0;

        // See warning in javadoc before adding more clean up code here.
    }

    private void hidePopupsAndClearSelection() {
        getSelectionPopupController().clearSelection();
        PopupController.hideAll(mWebContents);
    }

    private void hidePopupsAndPreserveSelection() {
        getSelectionPopupController().hidePopupsAndPreserveSelection();
    }

    @SuppressWarnings("javadoc")
    @Override
    public void onAttachedToWindow() {
        WindowEventObserverManager.from(mWebContents).onAttachedToWindow();
    }

    @SuppressWarnings("javadoc")
    @SuppressLint("MissingSuperCall")
    @Override
    public void onDetachedFromWindow() {
        WindowEventObserverManager.from(mWebContents).onDetachedFromWindow();
    }

    @SuppressWarnings("javadoc")
    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        try {
            TraceEvent.begin("ContentViewCore.onConfigurationChanged");
            getImeAdapter().onKeyboardConfigurationChanged(newConfig);
            // To request layout has side effect, but it seems OK as it only happen in
            // onConfigurationChange and layout has to be changed in most case.
            ViewAndroidDelegate viewDelegate = mWebContents.getViewAndroidDelegate();
            if (viewDelegate != null) viewDelegate.getContainerView().requestLayout();
        } finally {
            TraceEvent.end("ContentViewCore.onConfigurationChanged");
        }
    }

    @Override
    public void onPause() {
        if (mPaused) return;
        mPaused = true;
        onFocusChanged();
    }

    @Override
    public void onResume() {
        if (!mPaused) return;
        mPaused = false;
        onFocusChanged();
    }

    @Override
    public void onWindowFocusChanged(boolean hasWindowFocus) {
        WindowEventObserverManager.from(mWebContents).onWindowFocusChanged(hasWindowFocus);
    }

    @Override
    public void onViewFocusChanged(boolean gainFocus) {
        if (mHasViewFocus != null && mHasViewFocus == gainFocus) return;
        mHasViewFocus = gainFocus;
        onFocusChanged();
    }

    private void onFocusChanged() {
        // Wait for view focus to be set before propagating focus changes.
        if (mHasViewFocus == null) return;

        // See the comments on mPaused for why we use it to compute input focus.
        boolean hasInputFocus = mHasViewFocus && !mPaused;
        if (mHasInputFocus != null && mHasInputFocus == hasInputFocus) return;
        mHasInputFocus = hasInputFocus;

        if (mWebContents == null) {
            // CVC is on its way to destruction. The rest needs not running as all the states
            // will be discarded, or WebContentsUserData-based objects are not reachable
            // any more. Simply return here.
            return;
        }

        getImeAdapter().onViewFocusChanged(mHasInputFocus, mHideKeyboardOnBlur);
        getJoystick().setScrollEnabled(
                mHasInputFocus && !getSelectionPopupController().isFocusedNodeEditable());

        SelectionPopupControllerImpl controller = getSelectionPopupController();
        if (mHasInputFocus) {
            controller.restoreSelectionPopupsIfNecessary();
        } else {
            getImeAdapter().cancelRequestToScrollFocusedEditableNodeIntoView();
            if (controller.getPreserveSelectionOnNextLossOfFocus()) {
                controller.setPreserveSelectionOnNextLossOfFocus(false);
                hidePopupsAndPreserveSelection();
            } else {
                hidePopupsAndClearSelection();
            }
        }
        if (mNativeContentViewCore != 0) nativeSetFocus(mNativeContentViewCore, mHasInputFocus);
    }

    @Override
    public void setHideKeyboardOnBlur(boolean hideKeyboardOnBlur) {
        mHideKeyboardOnBlur = hideKeyboardOnBlur;
    }

    private JoystickHandler getJoystick() {
        return JoystickHandler.fromWebContents(mWebContents);
    }

    private native long nativeInit(WebContents webContents);
    private native void nativeOnJavaContentViewCoreDestroyed(long nativeContentViewCore);
    private native void nativeSetFocus(long nativeContentViewCore, boolean focused);
}
