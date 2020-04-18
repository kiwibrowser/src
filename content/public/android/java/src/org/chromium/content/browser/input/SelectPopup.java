// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.input;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;

import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.content.browser.PopupController;
import org.chromium.content.browser.PopupController.HideablePopup;
import org.chromium.content.browser.WindowEventObserver;
import org.chromium.content.browser.WindowEventObserverManager;
import org.chromium.content.browser.accessibility.WebContentsAccessibilityImpl;
import org.chromium.content.browser.webcontents.WebContentsImpl;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.WebContents.UserDataFactory;
import org.chromium.ui.base.DeviceFormFactor;
import org.chromium.ui.base.ViewAndroidDelegate;
import org.chromium.ui.base.WindowAndroid;

import java.util.ArrayList;
import java.util.List;

/**
 * Handles the popup UI for the lt&;select&gt; HTML tag support.
 */
@JNINamespace("content")
public class SelectPopup
        implements HideablePopup, ViewAndroidDelegate.ContainerViewObserver, WindowEventObserver {
    /** UI for Select popup. */
    public interface Ui {
        /**
         * Shows the popup.
         */
        public void show();
        /**
         * Hides the popup.
         * @param sendsCancelMessage Sends cancel message before hiding if true.
         */
        public void hide(boolean sendsCancelMessage);
    }

    private final WebContentsImpl mWebContents;
    private Context mContext;
    private View mContainerView;
    private Ui mPopupView;
    private long mNativeSelectPopup;
    private long mNativeSelectPopupSourceFrame;
    private boolean mInitialized;

    private static final class UserDataFactoryLazyHolder {
        private static final UserDataFactory<SelectPopup> INSTANCE = SelectPopup::new;
    }

    /**
     * Create {@link SelectPopup} instance.
     * @param context Context instance.
     * @param webContents WebContents instance.
     */
    public static SelectPopup create(Context context, WebContents webContents) {
        SelectPopup selectPopup =
                webContents.getOrSetUserData(SelectPopup.class, UserDataFactoryLazyHolder.INSTANCE);
        assert selectPopup != null && !selectPopup.initialized();
        selectPopup.init(context);
        return selectPopup;
    }

    /**
     * Get {@link SelectPopup} object used for the give WebContents. {@link #create()} should
     * precede any calls to this.
     * @param webContents {@link WebContents} object.
     * @return {@link SelectPopup} object. {@code null} if not available because
     *         {@link #create()} is not called yet.
     */
    public static SelectPopup fromWebContents(WebContents webContents) {
        return webContents.getOrSetUserData(SelectPopup.class, null);
    }

    /**
     * Create {@link SelectPopup} instance.
     * @param webContents WebContents instance.
     */
    public SelectPopup(WebContents webContents) {
        mWebContents = (WebContentsImpl) webContents;
    }

    private void init(Context context) {
        mContext = context;
        ViewAndroidDelegate viewDelegate = mWebContents.getViewAndroidDelegate();
        assert viewDelegate != null;
        mContainerView = viewDelegate.getContainerView();
        viewDelegate.addObserver(this);
        mNativeSelectPopup = nativeInit(mWebContents);
        PopupController.register(mWebContents, this);
        WindowEventObserverManager.from(mWebContents).addObserver(this);
        mInitialized = true;
    }

    private boolean initialized() {
        return mInitialized;
    }

    /**
     * Close popup. Called when {@link WindowAndroid} is updated.
     */
    public void close() {
        mPopupView = null;
    }

    // HideablePopup

    @Override
    public void hide() {
        // Cancels the selection by calling nativeSelectMenuItems() with null indices.
        if (mPopupView != null) mPopupView.hide(true);
    }

    // ViewAndroidDelegate.ContainerViewObserver

    @Override
    public void onUpdateContainerView(ViewGroup view) {
        mContainerView = view;
        hide();
    }

    // WindowEventObserver

    @Override
    public void onWindowAndroidChanged(WindowAndroid windowAndroid) {
        close();
    }

    /**
     * Called (from native) when the lt&;select&gt; popup needs to be shown.
     * @param anchorView View anchored for popup.
     * @param nativeSelectPopupSourceFrame The native RenderFrameHost that owns the popup.
     * @param items           Items to show.
     * @param enabled         POPUP_ITEM_TYPEs for items.
     * @param multiple        Whether the popup menu should support multi-select.
     * @param selectedIndices Indices of selected items.
     */
    @SuppressWarnings("unused")
    @CalledByNative
    private void show(View anchorView, long nativeSelectPopupSourceFrame, String[] items,
            int[] enabled, boolean multiple, int[] selectedIndices, boolean rightAligned) {
        if (mContainerView.getParent() == null || mContainerView.getVisibility() != View.VISIBLE) {
            mNativeSelectPopupSourceFrame = nativeSelectPopupSourceFrame;
            selectMenuItems(null);
            return;
        }

        PopupController.hidePopupsAndClearSelection(mWebContents);
        assert mNativeSelectPopupSourceFrame == 0 : "Zombie popup did not clear the frame source";

        assert items.length == enabled.length;
        List<SelectPopupItem> popupItems = new ArrayList<SelectPopupItem>();
        for (int i = 0; i < items.length; i++) {
            popupItems.add(new SelectPopupItem(items[i], enabled[i]));
        }
        WebContentsAccessibilityImpl wcax =
                WebContentsAccessibilityImpl.fromWebContents(mWebContents);
        if (DeviceFormFactor.isTablet() && !multiple && !wcax.isTouchExplorationEnabled()) {
            mPopupView = new SelectPopupDropdown(
                    this, mContext, anchorView, popupItems, selectedIndices, rightAligned);
        } else {
            WindowAndroid window = getWindowAndroid();
            if (window == null) return;
            Context windowContext = window.getContext().get();
            if (windowContext == null) return;
            mPopupView = new SelectPopupDialog(
                    this, windowContext, popupItems, multiple, selectedIndices);
        }
        mNativeSelectPopupSourceFrame = nativeSelectPopupSourceFrame;
        mPopupView.show();
    }

    /**
     * Called when the &lt;select&gt; popup needs to be hidden.
     */
    @CalledByNative
    public void hideWithoutCancel() {
        if (mPopupView == null) return;
        mPopupView.hide(false);
        mPopupView = null;
        mNativeSelectPopupSourceFrame = 0;
    }

    @CalledByNative
    private void destroy() {
        mNativeSelectPopup = 0;
    }

    /**
     * @return {@code true} if select popup is being shown.
     */
    @VisibleForTesting
    public boolean isVisibleForTesting() {
        return mPopupView != null;
    }

    private WindowAndroid getWindowAndroid() {
        return (mNativeSelectPopup != 0) ? nativeGetWindowAndroid(mNativeSelectPopup) : null;
    }

    /**
     * Notifies that items were selected in the currently showing select popup.
     * @param indices Array of indices of the selected items.
     */
    public void selectMenuItems(int[] indices) {
        if (mNativeSelectPopup != 0) {
            nativeSelectMenuItems(mNativeSelectPopup, mNativeSelectPopupSourceFrame, indices);
        }
        mNativeSelectPopupSourceFrame = 0;
        mPopupView = null;
    }

    private native long nativeInit(WebContents webContents);
    private native void nativeSelectMenuItems(
            long nativeSelectPopup, long nativeSelectPopupSourceFrame, int[] indices);
    private native WindowAndroid nativeGetWindowAndroid(long nativeSelectPopup);
}
