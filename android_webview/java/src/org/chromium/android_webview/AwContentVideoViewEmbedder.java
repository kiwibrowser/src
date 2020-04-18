// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview;

import android.content.Context;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.content_public.browser.ContentVideoViewEmbedder;

/**
 * Implementation of {@link ContentViewViewEmbedder} for webview.
 */
@JNINamespace("android_webview")
public class AwContentVideoViewEmbedder implements ContentVideoViewEmbedder {
    private final Context mContext;
    private final AwContentsClient mAwContentsClient;

    private FrameLayout mCustomView;
    private View mProgressView;

    public AwContentVideoViewEmbedder(Context context, AwContentsClient contentsClient,
            FrameLayout customView) {
        mContext = context;
        mAwContentsClient = contentsClient;
        mCustomView = customView;
    }

    public void setCustomView(FrameLayout customView) {
        mCustomView = customView;
    }

    @Override
    public void enterFullscreenVideo(View videoView, boolean isVideoLoaded) {
        if (mCustomView == null) {
            // enterFullscreenVideo will only be called after enterFullscreen, but
            // in this case exitFullscreen has been invoked in between them.
            // TODO(igsolla): Fix http://crbug/425926 and replace with assert.
            return;
        }

        mCustomView.addView(videoView, 0);

        if (isVideoLoaded) return;

        mProgressView = mAwContentsClient.getVideoLoadingProgressView();
        if (mProgressView == null) {
            mProgressView = new ProgressView(mContext);
        }
        mCustomView.addView(
                mProgressView, new FrameLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,
                                       ViewGroup.LayoutParams.WRAP_CONTENT, Gravity.CENTER));
    }

    @Override
    public void fullscreenVideoLoaded() {
        if (mCustomView == null) return;

        if (mProgressView != null) {
            mCustomView.removeView(mProgressView);
            mProgressView = null;
        }
    }

    @Override
    public void exitFullscreenVideo() {
        // Intentional no-op
    }

    @Override
    public void setSystemUiVisibility(boolean enterFullscreen) {
    }

    private static class ProgressView extends LinearLayout {
        private final ProgressBar mProgressBar;
        private final TextView mTextView;

        public ProgressView(Context context) {
            super(context);
            setOrientation(LinearLayout.VERTICAL);
            setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT,
                    LinearLayout.LayoutParams.WRAP_CONTENT));
            mProgressBar = new ProgressBar(context, null, android.R.attr.progressBarStyleLarge);
            mTextView = new TextView(context);

            String videoLoadingText = context.getString(
                    org.chromium.android_webview.R.string.media_player_loading_video);

            mTextView.setText(videoLoadingText);
            addView(mProgressBar);
            addView(mTextView);
        }
    }
}
