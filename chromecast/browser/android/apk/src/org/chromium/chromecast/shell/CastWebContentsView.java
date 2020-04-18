// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.shell;

import android.content.Context;
import android.widget.FrameLayout;

import org.chromium.chromecast.base.Observable;
import org.chromium.chromecast.base.ScopeFactory;
import org.chromium.components.content_view.ContentView;
import org.chromium.content.browser.ContentViewRenderView;
import org.chromium.content_public.browser.ContentViewCore;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.ViewAndroidDelegate;
import org.chromium.ui.base.WindowAndroid;

class CastWebContentsView {
    public static ScopeFactory<WebContents> onLayout(
            Context context, FrameLayout layout, int backgroundColor, Observable<?> resumedState) {
        layout.setBackgroundColor(backgroundColor);
        return (WebContents webContents) -> {
            WindowAndroid window = new WindowAndroid(context);
            ContentViewRenderView contentViewRenderView = new ContentViewRenderView(context) {
                @Override
                protected void onReadyToRender() {
                    setOverlayVideoMode(true);
                }
            };
            contentViewRenderView.onNativeLibraryLoaded(window);
            contentViewRenderView.setSurfaceViewBackgroundColor(backgroundColor);
            FrameLayout.LayoutParams matchParent = new FrameLayout.LayoutParams(
                    FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT);
            layout.addView(contentViewRenderView, matchParent);

            ContentView contentView = ContentView.createContentView(context, webContents);
            // TODO(derekjchow): productVersion
            ContentViewCore contentViewCore = ContentViewCore.create(context, "", webContents,
                    ViewAndroidDelegate.createBasicDelegate(contentView), contentView, window);

            // Enable display of current webContents.
            webContents.onShow();
            layout.addView(contentView, matchParent);
            contentView.setFocusable(true);
            contentView.requestFocus();
            contentViewRenderView.setCurrentWebContents(webContents);
            resumedState.watch(() -> {
                contentViewCore.onResume();
                return contentViewCore::onPause;
            });
            return () -> {
                layout.removeView(contentView);
                layout.removeView(contentViewRenderView);
                contentViewCore.destroy();
                contentViewRenderView.destroy();
                window.destroy();
            };
        };
    }

    public static ScopeFactory<WebContents> withoutLayout(Context context) {
        return (WebContents webContents) -> {
            WindowAndroid window = new WindowAndroid(context);
            ContentView contentView = ContentView.createContentView(context, webContents);
            // TODO(derekjchow): productVersion
            ContentViewCore contentViewCore = ContentViewCore.create(context, "", webContents,
                    ViewAndroidDelegate.createBasicDelegate(contentView), contentView, window);
            // Enable display of current webContents.
            webContents.onShow();
            return webContents::onHide;
        };
    }
}
