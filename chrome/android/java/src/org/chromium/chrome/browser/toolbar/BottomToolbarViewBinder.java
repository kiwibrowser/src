// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.toolbar;

import android.graphics.Color;
import android.view.ViewGroup;
import android.content.res.ColorStateList;

import org.chromium.base.ContextUtils;
import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.browser.widget.TintedImageButton;
import org.chromium.chrome.R;
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
    private static ColorStateList mLightModeTint;
    private static ColorStateList mDarkModeTint;

    /**
     * A wrapper class that holds a {@link ViewGroup} (the toolbar view) and a composited layer to
     * be used with the {@link BottomToolbarViewBinder}.
     */
    public static class ViewHolder {
        /** A handle to the composited bottom toolbar layer. */
        public final ScrollingBottomViewSceneLayer sceneLayer;

        /** A handle to the Android {@link ViewGroup} version of the toolbar. */
        public final ScrollingBottomViewResourceFrameLayout toolbarRoot;

        /**
         * @param compositedSceneLayer The composited bottom toolbar view.
         * @param toolbarRootView The Android {@link ViewGroup} toolbar.
         */
        public ViewHolder(
                ScrollingBottomViewSceneLayer compositedSceneLayer, ScrollingBottomViewResourceFrameLayout toolbarRootView) {
            sceneLayer = compositedSceneLayer;
            toolbarRoot = toolbarRootView;
            mLightModeTint =
                    ApiCompatibilityUtils.getColorStateList(toolbarRoot.getResources(), R.color.light_mode_tint);
            mDarkModeTint =
                    ApiCompatibilityUtils.getColorStateList(toolbarRoot.getResources(), R.color.dark_mode_tint);

            if (ContextUtils.getAppSharedPreferences().getBoolean("user_night_mode_enabled", false) || ContextUtils.getAppSharedPreferences().getString("active_theme", "").equals("Diamond Black")) {
              toolbarRoot.setBackgroundColor(Color.BLACK);
              toolbarRoot.findViewById(R.id.bottom_sheet_toolbar).setBackgroundColor(Color.BLACK);
            }
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
            if (view != null && view.toolbarRoot != null)
              view.toolbarRoot.setVisibility(model.getAndroidViewVisibility());
        } else if (PropertyKey.SEARCH_ACCELERATOR_LISTENER == propertyKey) {
            if (view != null && view.toolbarRoot != null) {
              view.toolbarRoot.findViewById(R.id.search_button)
                      .setOnClickListener(model.getSearchAcceleratorListener());
              if (ContextUtils.getAppSharedPreferences().getBoolean("user_night_mode_enabled", false) || ContextUtils.getAppSharedPreferences().getString("active_theme", "").equals("Diamond Black"))
                  ((TintedImageButton)view.toolbarRoot.findViewById(R.id.search_button)).setTint(mLightModeTint);
              else
                  ((TintedImageButton)view.toolbarRoot.findViewById(R.id.search_button)).setTint(mDarkModeTint);
            }
        } else if (PropertyKey.HOME_BUTTON_LISTENER == propertyKey) {
            if (view != null && view.toolbarRoot != null) {
              view.toolbarRoot.findViewById(R.id.home_button)
                      .setOnClickListener(model.getHomeButtonListener());
              if (ContextUtils.getAppSharedPreferences().getBoolean("user_night_mode_enabled", false) || ContextUtils.getAppSharedPreferences().getString("active_theme", "").equals("Diamond Black"))
                  ((TintedImageButton)view.toolbarRoot.findViewById(R.id.home_button)).setTint(mLightModeTint);
              else
                  ((TintedImageButton)view.toolbarRoot.findViewById(R.id.home_button)).setTint(mDarkModeTint);
            }
        } else if (PropertyKey.MENU_BUTTON_LISTENER == propertyKey) {
            if (view != null && view.toolbarRoot != null) {
              view.toolbarRoot.findViewById(R.id.menu_button)
                      .setOnTouchListener(model.getMenuButtonListener());
              if (ContextUtils.getAppSharedPreferences().getBoolean("user_night_mode_enabled", false) || ContextUtils.getAppSharedPreferences().getString("active_theme", "").equals("Diamond Black"))
                  ((TintedImageButton)view.toolbarRoot.findViewById(R.id.menu_button)).setTint(mLightModeTint);
              else
                  ((TintedImageButton)view.toolbarRoot.findViewById(R.id.menu_button)).setTint(mDarkModeTint);
            }
        } else if (PropertyKey.TOOLBAR_SWIPE_HANDLER == propertyKey) {
            if (view != null && view.toolbarRoot != null)
              view.toolbarRoot.setSwipeDetector(
                      model.getValue(PropertyKey.TOOLBAR_SWIPE_HANDLER));
        } else {
            assert false : "Unhandled property detected in BottomToolbarViewBinder!";
        }
    }
}
