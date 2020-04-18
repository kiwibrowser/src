// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.compositor.scene_layer;

import android.graphics.RectF;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.chrome.browser.compositor.LayerTitleCache;
import org.chromium.chrome.browser.compositor.layouts.components.VirtualView;
import org.chromium.chrome.browser.compositor.layouts.eventfilter.EventFilter;
import org.chromium.chrome.browser.compositor.overlays.SceneOverlay;
import org.chromium.chrome.browser.widget.ViewResourceFrameLayout;
import org.chromium.ui.resources.ResourceManager;

import java.util.List;

/**
 * A composited view that sits at the bottom of the screen and listens to changes in the browser
 * controls. When visible, the view will mimic the behavior of the top browser controls when
 * scrolling.
 */
@JNINamespace("android")
public class ScrollingBottomViewSceneLayer extends SceneOverlayLayer implements SceneOverlay {
    /** Handle to the native side of this class. */
    private long mNativePtr;

    /** The resource ID used to reference the view bitmap in native. */
    private int mResourceId;

    /** The height of the view's top shadow. */
    private int mTopShadowHeightPx;

    /** The current offset of the bottom view in px. */
    private int mCurrentOffsetPx;

    /** The {@link ViewResourceFrameLayout} that this scene layer represents. */
    private ViewResourceFrameLayout mBottomView;

    /**
     * Build a composited bottom view layer.
     * @param resourceManager A resource manager for dynamic resource creation.
     * @param bottomView The view used to generate the composited version.
     * @param topShadowHeightPx The height of the shadow on the top of the view in px if it exists.
     */
    public ScrollingBottomViewSceneLayer(ResourceManager resourceManager,
            ViewResourceFrameLayout bottomView, int topShadowHeightPx) {
        mBottomView = bottomView;
        mResourceId = mBottomView.getId();
        mTopShadowHeightPx = topShadowHeightPx;
        resourceManager.getDynamicResourceLoader().registerResource(
                mResourceId, mBottomView.getResourceAdapter());
    }

    /**
     * Set the view's offset from the bottom of the screen in px. An offset of 0 means the view is
     * completely visible. An increasing offset will move the view down.
     * @param offsetPx The view's offset in px.
     */
    public void setYOffset(int offsetPx) {
        mCurrentOffsetPx = offsetPx;
    }

    @Override
    protected void initializeNative() {
        if (mNativePtr == 0) {
            mNativePtr = nativeInit();
        }
        assert mNativePtr != 0;
    }

    @Override
    public void setContentTree(SceneLayer contentTree) {
        nativeSetContentTree(mNativePtr, contentTree);
    }

    @Override
    public SceneOverlayLayer getUpdatedSceneOverlayTree(RectF viewport, RectF visibleViewport,
            LayerTitleCache layerTitleCache, ResourceManager resourceManager, float yOffset) {
        nativeUpdateScrollingBottomViewLayer(mNativePtr, resourceManager, mResourceId,
                mTopShadowHeightPx, viewport.height() + mCurrentOffsetPx, mCurrentOffsetPx > 0);

        return this;
    }

    @Override
    public boolean isSceneOverlayTreeShowing() {
        // If the offset is greater than the toolbar's height, don't draw the layer.
        return mCurrentOffsetPx < mBottomView.getHeight() - mTopShadowHeightPx;
    }

    @Override
    public EventFilter getEventFilter() {
        return null;
    }

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
    public boolean handlesTabCreating() {
        return false;
    }

    @Override
    public void onSizeChanged(
            float width, float height, float visibleViewportOffsetY, int orientation) {}

    @Override
    public void onHideLayout() {}

    @Override
    public void getVirtualViews(List<VirtualView> views) {}

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
            long nativeScrollingBottomViewSceneLayer, SceneLayer contentTree);
    private native void nativeUpdateScrollingBottomViewLayer(
            long nativeScrollingBottomViewSceneLayer, ResourceManager resourceManager,
            int viewResourceId, int shadowHeightPx, float yOffset, boolean showShadow);
}
