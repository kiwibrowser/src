// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.compositor.scene_layer;

import android.content.Context;
import android.graphics.Color;
import android.graphics.RectF;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.compositor.LayerTitleCache;
import org.chromium.chrome.browser.compositor.layouts.Layout.ViewportMode;
import org.chromium.chrome.browser.compositor.layouts.LayoutProvider;
import org.chromium.chrome.browser.compositor.layouts.LayoutRenderHost;
import org.chromium.chrome.browser.compositor.layouts.components.VirtualView;
import org.chromium.chrome.browser.compositor.layouts.eventfilter.EventFilter;
import org.chromium.chrome.browser.compositor.overlays.SceneOverlay;
import org.chromium.chrome.browser.device.DeviceClassManager;
import org.chromium.chrome.browser.fullscreen.ChromeFullscreenManager;
import org.chromium.chrome.browser.ntp.NewTabPage;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.util.ColorUtils;
import org.chromium.chrome.browser.util.FeatureUtilities;
import org.chromium.chrome.browser.widget.ClipDrawableProgressBar.DrawingInfo;
import org.chromium.chrome.browser.widget.ControlContainer;
import org.chromium.ui.base.DeviceFormFactor;
import org.chromium.ui.resources.ResourceManager;

import java.util.List;

/**
 * A SceneLayer to render layers for the toolbar.
 */
@JNINamespace("android")
public class ToolbarSceneLayer extends SceneOverlayLayer implements SceneOverlay {
    /** Pointer to native ToolbarSceneLayer. */
    private long mNativePtr;

    /** Information used to draw the progress bar. */
    private DrawingInfo mProgressBarDrawingInfo;

    /** An Android Context. */
    private Context mContext;

    /** A LayoutProvider for accessing the current layout. */
    private LayoutProvider mLayoutProvider;

    /** A LayoutRenderHost for accessing drawing information about the toolbar. */
    private LayoutRenderHost mRenderHost;

    /**
     * @param context An Android context to use.
     * @param provider A LayoutProvider for accessing the current layout.
     * @param renderHost A LayoutRenderHost for accessing drawing information about the toolbar.
     */
    public ToolbarSceneLayer(Context context, LayoutProvider provider,
            LayoutRenderHost renderHost) {
        mContext = context;
        mLayoutProvider = provider;
        mRenderHost = renderHost;
    }

    /**
     * Update the toolbar and progress bar layers.
     *
     * @param browserControlsBackgroundColor The background color of the browser controls.
     * @param browserControlsUrlBarAlpha The alpha of the URL bar.
     * @param fullscreenManager A ChromeFullscreenManager instance.
     * @param resourceManager A ResourceManager for loading static resources.
     * @param forceHideAndroidBrowserControls True if the Android browser controls are being hidden.
     * @param viewportMode The sizing mode of the viewport being drawn in.
     * @param isTablet If the device is a tablet.
     * @param windowHeight The height of the window.
     */
    private void update(int browserControlsBackgroundColor, float browserControlsUrlBarAlpha,
            ChromeFullscreenManager fullscreenManager, ResourceManager resourceManager,
            boolean forceHideAndroidBrowserControls, ViewportMode viewportMode, boolean isTablet,
            float windowHeight) {
        if (!DeviceClassManager.enableFullscreen()) return;

        if (fullscreenManager == null) return;
        ControlContainer toolbarContainer = fullscreenManager.getControlContainer();
        if (!isTablet && toolbarContainer != null) {
            if (mProgressBarDrawingInfo == null) mProgressBarDrawingInfo = new DrawingInfo();
            toolbarContainer.getProgressBarDrawingInfo(mProgressBarDrawingInfo);
        } else {
            assert mProgressBarDrawingInfo == null;
        }

        // Texture is always used unless it is completely off-screen.
        boolean useTexture = !fullscreenManager.areBrowserControlsOffScreen()
                && viewportMode != ViewportMode.ALWAYS_FULLSCREEN;
        boolean showShadow = fullscreenManager.drawControlsAsTexture()
                || forceHideAndroidBrowserControls;

        int textBoxColor = Color.WHITE;
        int textBoxResourceId = R.drawable.card_single;

        boolean isLocationBarShownInNtp = false;
        boolean useModern = false;
        Tab currentTab = fullscreenManager.getTab();
        if (currentTab != null) {
            boolean isNtp =
                    currentTab != null ? currentTab.getNativePage() instanceof NewTabPage : false;
            if (isNtp) {
                isLocationBarShownInNtp =
                        ((NewTabPage) currentTab.getNativePage()).isLocationBarShownInNTP();
            }
            useModern = currentTab.getActivity().supportsModernDesign()
                    && FeatureUtilities.isChromeModernDesignEnabled();
        }

        if (useModern) {
            textBoxColor = ColorUtils.getTextBoxColorForToolbarBackground(mContext.getResources(),
                    isLocationBarShownInNtp, browserControlsBackgroundColor, true);
            textBoxResourceId = R.drawable.modern_location_bar;
        }

        nativeUpdateToolbarLayer(mNativePtr, resourceManager, R.id.control_container,
                browserControlsBackgroundColor, textBoxResourceId, browserControlsUrlBarAlpha,
                textBoxColor, fullscreenManager.getTopControlOffset(), windowHeight, useTexture,
                showShadow, useModern);

        if (mProgressBarDrawingInfo == null) return;
        nativeUpdateProgressBar(mNativePtr, mProgressBarDrawingInfo.progressBarRect.left,
                mProgressBarDrawingInfo.progressBarRect.top,
                mProgressBarDrawingInfo.progressBarRect.width(),
                mProgressBarDrawingInfo.progressBarRect.height(),
                mProgressBarDrawingInfo.progressBarColor,
                mProgressBarDrawingInfo.progressBarBackgroundRect.left,
                mProgressBarDrawingInfo.progressBarBackgroundRect.top,
                mProgressBarDrawingInfo.progressBarBackgroundRect.width(),
                mProgressBarDrawingInfo.progressBarBackgroundRect.height(),
                mProgressBarDrawingInfo.progressBarBackgroundColor);
    }

    @Override
    public void setContentTree(SceneLayer contentTree) {
        nativeSetContentTree(mNativePtr, contentTree);
    }

    @Override
    protected void initializeNative() {
        if (mNativePtr == 0) {
            mNativePtr = nativeInit();
        }
        assert mNativePtr != 0;
    }

    /**
     * Destroys this object and the corresponding native component.
     */
    @Override
    public void destroy() {
        super.destroy();
        mNativePtr = 0;
    }

    // SceneOverlay implementation.

    @Override
    public SceneOverlayLayer getUpdatedSceneOverlayTree(RectF viewport, RectF visibleViewport,
            LayerTitleCache layerTitleCache, ResourceManager resourceManager, float yOffset) {
        boolean forceHideBrowserControlsAndroidView =
                mLayoutProvider.getActiveLayout().forceHideBrowserControlsAndroidView();
        ViewportMode viewportMode = mLayoutProvider.getActiveLayout().getViewportMode();

        // In Chrome modern design, the url bar is always opaque since it is drawn in the
        // compositor.
        float alpha = mRenderHost.getBrowserControlsUrlBarAlpha();
        if (FeatureUtilities.isChromeModernDesignEnabled()) alpha = 1;

        update(mRenderHost.getBrowserControlsBackgroundColor(), alpha,
                mLayoutProvider.getFullscreenManager(), resourceManager,
                forceHideBrowserControlsAndroidView, viewportMode,
                DeviceFormFactor.isNonMultiDisplayContextOnTablet(mContext), viewport.height());

        return this;
    }

    @Override
    public boolean isSceneOverlayTreeShowing() {
        return true;
    }

    @Override
    public EventFilter getEventFilter() {
        return null;
    }

    @Override
    public void onSizeChanged(
            float width, float height, float visibleViewportOffsetY, int orientation) {}

    @Override
    public void getVirtualViews(List<VirtualView> views) {}

    @Override
    public boolean shouldHideAndroidBrowserControls() {
        return false;
    }

    @Override
    public boolean updateOverlay(long time, long dt) {
        return false;
    }

    @Override
    public boolean onBackPressed() {
        return false;
    }

    @Override
    public void onHideLayout() {}

    @Override
    public boolean handlesTabCreating() {
        return false;
    }

    @Override
    public void tabTitleChanged(int tabId, String title) {}

    @Override
    public void tabStateInitialized() {}

    @Override
    public void tabModelSwitched(boolean incognito) {}

    @Override
    public void tabSelected(long time, boolean incognito, int id, int prevId) {}

    @Override
    public void tabMoved(long time, boolean incognito, int id, int oldIndex, int newIndex) {}

    @Override
    public void tabClosed(long time, boolean incognito, int id) {}

    @Override
    public void tabClosureCancelled(long time, boolean incognito, int id) {}

    @Override
    public void tabCreated(long time, boolean incognito, int id, int prevId, boolean selected) {}

    @Override
    public void tabPageLoadStarted(int id, boolean incognito) {}

    @Override
    public void tabPageLoadFinished(int id, boolean incognito) {}

    @Override
    public void tabLoadStarted(int id, boolean incognito) {}

    @Override
    public void tabLoadFinished(int id, boolean incognito) {}

    private native long nativeInit();
    private native void nativeSetContentTree(
            long nativeToolbarSceneLayer,
            SceneLayer contentTree);
    private native void nativeUpdateToolbarLayer(
            long nativeToolbarSceneLayer,
            ResourceManager resourceManager,
            int resourceId,
            int toolbarBackgroundColor,
            int urlBarResourceId,
            float urlBarAlpha,
            int urlBarColor,
            float topOffset,
            float viewHeight,
            boolean visible,
            boolean showShadow,
            boolean browserControlsAtBottom);
    private native void nativeUpdateProgressBar(
            long nativeToolbarSceneLayer,
            int progressBarX,
            int progressBarY,
            int progressBarWidth,
            int progressBarHeight,
            int progressBarColor,
            int progressBarBackgroundX,
            int progressBarBackgroundY,
            int progressBarBackgroundWidth,
            int progressBarBackgroundHeight,
            int progressBarBackgroundColor);
}
