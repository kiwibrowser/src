// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.feed;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.support.v7.content.res.AppCompatResources;

import com.google.android.libraries.feed.common.functional.Consumer;
import com.google.android.libraries.feed.host.imageloader.ImageLoaderApi;

import org.chromium.base.Callback;
import org.chromium.chrome.browser.profiles.Profile;

import java.util.ArrayList;
import java.util.List;

/**
 * Provides image loading and other host-specific asset fetches for Feed.
 */
public class FeedImageLoader implements ImageLoaderApi {
    private static final String ASSET_PREFIX = "asset://";
    private static final String DRAWABLE_RESOURCE_TYPE = "drawable";

    private FeedImageLoaderBridge mFeedImageLoaderBridge;
    private Context mActivityContext;

    /**
     * Creates a FeedImageLoader for fetching image for the current user.
     *
     * @param profile Profile of the user we are rendering the Feed for.
     * @param activityContext Context of the user we are rendering the Feed for.
     */
    public FeedImageLoader(Profile profile, Context activityContext) {
        this(profile, activityContext, new FeedImageLoaderBridge());
    }

    /**
     * Creates a FeedImageLoader for fetching image for the current user.
     *
     * @param profile Profile of the user we are rendering the Feed for.
     * @param activityContext Context of the user we are rendering the Feed for.
     * @param bridge The FeedImageLoaderBridge implementation can handle fetching image request.
     */
    public FeedImageLoader(Profile profile, Context activityContext, FeedImageLoaderBridge bridge) {
        mFeedImageLoaderBridge = bridge;
        mFeedImageLoaderBridge.init(profile);
        mActivityContext = activityContext;
    }

    @Override
    public void loadDrawable(List<String> urls, Consumer<Drawable> consumer) {
        assert mFeedImageLoaderBridge != null;
        List<String> assetUrls = new ArrayList<>();
        List<String> networkUrls = new ArrayList<>();

        // Since loading APK resource("asset://"") only can be done in Java side, we filter out
        // asset urls, and pass the other URLs to C++ side. This will change the order of |urls|,
        // because we will process asset:// URLs after network URLs, but once
        // https://crbug.com/840578 resolved, we can process URLs ordering same as |urls|.
        for (String url : urls) {
            if (url.startsWith(ASSET_PREFIX)) {
                assetUrls.add(url);
            } else {
                // Assume this is a web image.
                networkUrls.add(url);
            }
        }

        if (networkUrls.size() == 0) {
            Drawable drawable = getAssetDrawable(assetUrls);
            consumer.accept(drawable);
            return;
        }

        mFeedImageLoaderBridge.fetchImage(networkUrls, new Callback<Bitmap>() {
            @Override
            public void onResult(Bitmap bitmap) {
                if (bitmap != null) {
                    Drawable drawable = new BitmapDrawable(mActivityContext.getResources(), bitmap);
                    consumer.accept(drawable);
                    return;
                }

                // Since no image was available for downloading over the network, attempt to load a
                // drawable locally.
                Drawable drawable = getAssetDrawable(assetUrls);
                consumer.accept(drawable);
            }
        });
    }

    /** Cleans up FeedImageLoaderBridge. */
    public void destroy() {
        assert mFeedImageLoaderBridge != null;

        mFeedImageLoaderBridge.destroy();
        mFeedImageLoaderBridge = null;
    }

    private Drawable getAssetDrawable(List<String> assetUrls) {
        for (String url : assetUrls) {
            String resourceName = url.substring(ASSET_PREFIX.length());
            int resourceId = mActivityContext.getResources().getIdentifier(
                    resourceName, DRAWABLE_RESOURCE_TYPE, mActivityContext.getPackageName());
            if (resourceId != 0) {
                Drawable drawable = AppCompatResources.getDrawable(mActivityContext, resourceId);
                if (drawable != null) {
                    return drawable;
                }
            }
        }
        return null;
    }
}
