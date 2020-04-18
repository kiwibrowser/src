// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.toolbar;

import android.view.ViewGroup;

import org.chromium.chrome.browser.compositor.scene_layer.ScrollingBottomViewSceneLayer;
import org.chromium.chrome.browser.modelutil.PropertyModelChangeProcessor;
import org.chromium.chrome.browser.toolbar.BottomToolbarModel.PropertyKey;

/**
 * This class is responsible for pushing updates to both the Android view and the compositor
 * component of the bottom toolbar. These updates are pulled from the {@link BottomToolbarModel}
 * when a notification of an update is received.
 */
public class BottomToolbarViewBinder
        implements PropertyModelChangeProcessor.ViewBinder<BottomToolbarModel,
                BottomToolbarViewBinder.ViewHolder, BottomToolbarModel.PropertyKey> {
    /**
     * A wrapper class that holds a {@link ViewGroup} (the toolbar view) and a composited layer to
     * be used with the {@link BottomToolbarViewBinder}.
     */
    public static class ViewHolder {
        /** A handle to the composited bottom toolbar layer. */
        public final ScrollingBottomViewSceneLayer sceneLayer;

        /** A handle to the Android {@link ViewGroup} version of the toolbar. */
        public final ViewGroup toolbarRoot;

        /**
         * @param compositedSceneLayer The composited bottom toolbar view.
         * @param toolbarRootView The Android {@link ViewGroup} toolbar.
         */
        public ViewHolder(
                ScrollingBottomViewSceneLayer compositedSceneLayer, ViewGroup toolbarRootView) {
            sceneLayer = compositedSceneLayer;
            toolbarRoot = toolbarRootView;
        }
    }

    /**
     * Build a binder that handles interaction between the model and the views that make up the
     * bottom toolbar.
     */
    public BottomToolbarViewBinder() {}

    @Override
    public final void bind(BottomToolbarModel model, ViewHolder view, PropertyKey propertyKey) {
        if (PropertyKey.Y_OFFSET == propertyKey) {
            view.sceneLayer.setYOffset(model.getYOffset());
        } else if (PropertyKey.ANDROID_VIEW_VISIBILITY == propertyKey) {
            view.toolbarRoot.setVisibility(model.getAndroidViewVisibility());
        } else {
            assert false : "Unhandled property detected in BottomToolbarViewBinder!";
        }
    }
}
