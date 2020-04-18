// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_public.common;

import android.graphics.Rect;
import android.support.annotation.NonNull;
import android.text.TextUtils;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

import java.util.ArrayList;
import java.util.List;

/**
 * The MediaMetadata class carries information related to a media session. It is
 * the Java counterpart of content::MediaMetadata.
 */
@JNINamespace("content")
public final class MediaMetadata {
    /**
     * The MediaImage class carries the artwork information in MediaMetadata. It is the Java
     * counterpart of content::MediaMetadata::MediaImage.
     */
    public static final class MediaImage {
        @NonNull
        private String mSrc;

        private String mType;

        @NonNull
        private List<Rect> mSizes = new ArrayList<Rect>();

        /**
         * Creates a new MediaImage.
         */
        public MediaImage(@NonNull String src, @NonNull String type, @NonNull List<Rect> sizes) {
            mSrc = src;
            mType = type;
            mSizes = sizes;
        }

        /**
         * @return The URL of this MediaImage.
         */
        @NonNull
        public String getSrc() {
            return mSrc;
        }

        /**
         * @return The MIME type of this MediaImage.
         */
        public String getType() {
            return mType;
        }

        /**
         * @return The hinted sizes of this MediaImage.
         */
        public List<Rect> getSizes() {
            return mSizes;
        }

        /**
         * Sets the URL of this MediaImage.
         */
        public void setSrc(@NonNull String src) {
            mSrc = src;
        }

        /**
         * Sets the MIME type of this MediaImage.
         */
        public void setType(@NonNull String type) {
            mType = type;
        }

        /**
         * Sets the sizes of this MediaImage.
         */
        public void setSizes(@NonNull List<Rect> sizes) {
            mSizes = sizes;
        }

        @Override
        public boolean equals(Object obj) {
            if (obj == this) return true;
            if (!(obj instanceof MediaImage)) return false;

            MediaImage other = (MediaImage) obj;
            return TextUtils.equals(mSrc, other.mSrc)
                    && TextUtils.equals(mType, other.mType)
                    && mSizes.equals(other.mSizes);
        }

        /**
         * @return The hash code of this {@link MediaImage}. The method uses the same algorithm in
         * {@link java.util.List} for combinine hash values.
         */
        @Override
        public int hashCode() {
            int result = mSrc.hashCode();
            result = 31 * result + mType.hashCode();
            result = 31 * result + mSizes.hashCode();
            return result;
        }
    }

    @NonNull
    private String mTitle;

    @NonNull
    private String mArtist;

    @NonNull
    private String mAlbum;

    @NonNull
    private List<MediaImage> mArtwork = new ArrayList<MediaImage>();

    /**
     * Returns the title associated with the media session.
     */
    public String getTitle() {
        return mTitle;
    }

    /**
     * Returns the artist name associated with the media session.
     */
    public String getArtist() {
        return mArtist;
    }

    /**
     * Returns the album name associated with the media session.
     */
    public String getAlbum() {
        return mAlbum;
    }

    public List<MediaImage> getArtwork() {
        return mArtwork;
    }

    /**
     * Sets the title associated with the media session.
     * @param title The title to use for the media session.
     */
    public void setTitle(@NonNull String title) {
        mTitle = title;
    }

    /**
     * Sets the arstist name associated with the media session.
     * @param arstist The artist name to use for the media session.
     */
    public void setArtist(@NonNull String artist) {
        mArtist = artist;
    }

    /**
     * Sets the album name associated with the media session.
     * @param album The album name to use for the media session.
     */
    public void setAlbum(@NonNull String album) {
        mAlbum = album;
    }

    /**
     * Create a new {@link MediaImage} from the C++ code, and add it to the Metadata.
     * @param src The URL of the image.
     * @param type The MIME type of the image.
     * @param flattenedSizes The flattened array of image sizes. In native code, it is of type
     *         `std::vector<gfx::Size>` before flattening.
     */
    @CalledByNative
    private void createAndAddMediaImage(String src, String type, int[] flattenedSizes) {
        assert (flattenedSizes.length % 2) == 0;
        List<Rect> sizes = new ArrayList<Rect>();
        for (int i = 0; (i + 1) < flattenedSizes.length; i += 2) {
            sizes.add(new Rect(0, 0, flattenedSizes[i], flattenedSizes[i + 1]));
        }
        mArtwork.add(new MediaImage(src, type, sizes));
    }

    /**
     * Creates a new MediaMetadata from the C++ code. This is exactly like the
     * constructor below apart that it can be called by native code.
     */
    @CalledByNative
    private static MediaMetadata create(String title, String artist, String album) {
        return new MediaMetadata(title == null ? "" : title, artist == null ? "" : artist,
            album == null ? "" : album);
    }

    /**
     * Creates a new MediaMetadata.
     */
    public MediaMetadata(@NonNull String title, @NonNull String artist, @NonNull String album) {
        mTitle = title;
        mArtist = artist;
        mAlbum = album;
    }

    /**
     * Comparing MediaMetadata is expensive and should be used sparingly
     */
    @Override
    public boolean equals(Object obj) {
        if (obj == this) return true;
        if (!(obj instanceof MediaMetadata)) return false;

        MediaMetadata other = (MediaMetadata) obj;
        return TextUtils.equals(mTitle, other.mTitle)
                && TextUtils.equals(mArtist, other.mArtist)
                && TextUtils.equals(mAlbum, other.mAlbum)
                && mArtwork.equals(other.mArtwork);
    }

    /**
     * @return The hash code of this {@link MediaMetadata}. The method uses the same algorithm in
     * {@link java.util.List} for combinine hash values.
     */
    @Override
    public int hashCode() {
        int result = mTitle.hashCode();
        result = 31 * result + mArtist.hashCode();
        result = 31 * result + mAlbum.hashCode();
        result = 31 * result + mArtwork.hashCode();
        return result;
    }
}
