// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.chrome.browser.media.router.cast.remoting;

import android.support.v7.media.MediaRouteSelector;
import android.util.Base64;

import com.google.android.gms.cast.CastMediaControlIntent;

import org.chromium.base.Log;
import org.chromium.chrome.browser.media.router.MediaSource;

import java.io.UnsupportedEncodingException;

import javax.annotation.Nullable;

/**
 * Abstracts parsing the Cast application id and other parameters from the source id.
 */
public class RemotingMediaSource implements MediaSource {
    private static final String TAG = "MediaRemoting";

    // Need to be in sync with third_party/WebKit/Source/modules/remoteplayback/RemotePlayback.cpp.
    // TODO(avayvod): Find a way to share the constants somehow.
    private static final String SOURCE_PREFIX = "remote-playback://";

    /**
     * The original source URL that the {@link MediaSource} object was created from.
     */
    private final String mSourceId;

    /**
     * The Cast application id.
     */
    private final String mApplicationId;

    /**
     * The URL to fling to the Cast device.
     */
    private final String mMediaUrl;

    /**
     * Initializes the media source from the source id.
     * @param sourceId a URL containing encoded info about the media element's source.
     * @return an initialized media source if the id is valid, null otherwise.
     */
    @Nullable
    public static RemotingMediaSource from(String sourceId) {
        assert sourceId != null;

        if (!sourceId.startsWith(SOURCE_PREFIX)) return null;

        String encodedContentUrl = sourceId.substring(SOURCE_PREFIX.length());

        String mediaUrl;
        try {
            mediaUrl = new String(Base64.decode(encodedContentUrl, Base64.URL_SAFE), "UTF-8");
        } catch (IllegalArgumentException | UnsupportedEncodingException e) {
            Log.e(TAG, "Couldn't parse the source id.", e);
            return null;
        }

        // TODO(avayvod): check the content URL and override the app id if needed.
        String applicationId = CastMediaControlIntent.DEFAULT_MEDIA_RECEIVER_APPLICATION_ID;
        return new RemotingMediaSource(sourceId, applicationId, mediaUrl);
    }

    /**
     * Returns a new {@link MediaRouteSelector} to use for Cast device filtering for this
     * particular media source or null if the application id is invalid.
     *
     * @return an initialized route selector or null.
     */
    @Override
    public MediaRouteSelector buildRouteSelector() {
        return new MediaRouteSelector.Builder()
                .addControlCategory(CastMediaControlIntent.categoryForCast(mApplicationId))
                .build();
    }

    /**
     * @return the Cast application id corresponding to the source. Can be overridden downstream.
     */
    @Override
    public String getApplicationId() {
        return mApplicationId;
    }

    /**
     * @return the id identifying the media source
     */
    @Override
    public String getSourceId() {
        return mSourceId;
    }

    /**
     * @return the media URL to fling to the Cast device.
     */
    public String getMediaUrl() {
        return mMediaUrl;
    }

    private RemotingMediaSource(String sourceId, String applicationId, String mediaUrl) {
        mSourceId = sourceId;
        mApplicationId = applicationId;
        mMediaUrl = mediaUrl;
    }
}
