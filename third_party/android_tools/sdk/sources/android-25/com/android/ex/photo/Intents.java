/*
 * Copyright (C) 2011 Google Inc.
 * Licensed to The Android Open Source Project.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.ex.photo;

import android.app.Activity;
import android.content.ContentProvider;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;

import com.android.ex.photo.fragments.PhotoViewFragment;

/**
 * Build intents to start app activities
 */

public class Intents {
    // Intent extras
    public static final String EXTRA_PHOTO_INDEX = "photo_index";
    public static final String EXTRA_INITIAL_PHOTO_URI = "initial_photo_uri";
    public static final String EXTRA_PHOTOS_URI = "photos_uri";
    public static final String EXTRA_RESOLVED_PHOTO_URI = "resolved_photo_uri";
    public static final String EXTRA_PROJECTION = "projection";
    public static final String EXTRA_THUMBNAIL_URI = "thumbnail_uri";
    public static final String EXTRA_CONTENT_DESCRIPTION = "content_description";
    public static final String EXTRA_MAX_INITIAL_SCALE = "max_scale";
    public static final String EXTRA_WATCH_NETWORK = "watch_network";
    public static final String EXTRA_ENABLE_TIMER_LIGHTS_OUT = "enable_timer_lights_out";


    // Parameters affecting the intro/exit animation
    public static final String EXTRA_SCALE_UP_ANIMATION = "scale_up_animation";
    public static final String EXTRA_ANIMATION_START_X = "start_x_extra";
    public static final String EXTRA_ANIMATION_START_Y = "start_y_extra";
    public static final String EXTRA_ANIMATION_START_WIDTH = "start_width_extra";
    public static final String EXTRA_ANIMATION_START_HEIGHT = "start_height_extra";

    // Parameters affecting the display and features
    public static final String EXTRA_ACTION_BAR_HIDDEN_INITIALLY = "action_bar_hidden_initially";
    public static final String EXTRA_DISPLAY_THUMBS_FULLSCREEN = "display_thumbs_fullscreen";

    /**
     * Gets a photo view intent builder to display the photos from phone activity.
     *
     * @param context The context
     * @return The intent builder
     */
    public static PhotoViewIntentBuilder newPhotoViewActivityIntentBuilder(Context context) {
        return new PhotoViewIntentBuilder(context, PhotoViewActivity.class);
    }

    /**
     * Gets a photo view intent builder to display the photo view fragment
     *
     * @param context The context
     * @return The intent builder
     */
    public static PhotoViewIntentBuilder newPhotoViewFragmentIntentBuilder(Context context) {
        return newPhotoViewFragmentIntentBuilder(context, PhotoViewFragment.class);
    }

    /**
     * Gets a photo view intent builder to display the photo view fragment with a custom fragment
     * subclass.
     *
     * @param context The context
     * @param clazz Subclass of PhotoViewFragment to use
     * @return The intent builder
     */
    public static PhotoViewIntentBuilder newPhotoViewFragmentIntentBuilder(Context context,
            Class<? extends PhotoViewFragment> clazz) {
        return new PhotoViewIntentBuilder(context, clazz);
    }

    /** Gets a new photo view intent builder */
    public static PhotoViewIntentBuilder newPhotoViewIntentBuilder(
            Context context, Class<? extends Activity> cls) {
        return new PhotoViewIntentBuilder(context, cls);
    }

    /** Gets a new photo view intent builder */
    public static PhotoViewIntentBuilder newPhotoViewIntentBuilder(
            Context context, String activityName) {
        return new PhotoViewIntentBuilder(context, activityName);
    }

    /** Builder to create a photo view intent */
    public static class PhotoViewIntentBuilder {
        private final Intent mIntent;

        /** The index of the photo to show */
        private Integer mPhotoIndex;
        /** The URI of the initial photo to show */
        private String mInitialPhotoUri;
        /** The URI of the initial thumbnail to show */
        private String mInitialThumbnailUri;
        /** The URI of the group of photos to display */
        private String mPhotosUri;
        /** The URL of the photo to display */
        private String mResolvedPhotoUri;
        /** The projection for the query to use; optional */
        private String[] mProjection;
        /** The URI of a thumbnail of the photo to display */
        private String mThumbnailUri;
        /** The content Description for the photo to show */
        private String mContentDescription;
        /** The maximum scale to display images at before  */
        private Float mMaxInitialScale;
        /** True if lights out should automatically be invoked on a timer basis */
        private boolean mEnableTimerLightsOut;
        /**
         * True if the PhotoViewFragments should watch for network changes to restart their loaders
         */
        private boolean mWatchNetwork;
        /** true we want to run the image scale animation */
        private boolean mScaleAnimation;
        /** The parameters for performing the scale up/scale down animations
         * upon enter and exit. StartX and StartY represent the screen coordinates
         * of the upper left corner of the start rectangle, startWidth and startHeight
         * represent the width and height of the start rectangle.
         */
        private int mStartX;
        private int mStartY;
        private int mStartWidth;
        private int mStartHeight;

        private boolean mActionBarHiddenInitially;
        private boolean mDisplayFullScreenThumbs;

        private PhotoViewIntentBuilder(Context context, Class<?> cls) {
            mIntent = new Intent(context, cls);
            initialize();
        }

        private PhotoViewIntentBuilder(Context context, String activityName) {
            mIntent = new Intent();
            mIntent.setClassName(context, activityName);
            initialize();
        }

        private void initialize() {
            mScaleAnimation = false;
            mActionBarHiddenInitially = false;
            mDisplayFullScreenThumbs = false;
            mEnableTimerLightsOut = true;
        }

        /** Sets auto lights out */
        public PhotoViewIntentBuilder setEnableTimerLightsOut(boolean enable) {
            mEnableTimerLightsOut = enable;
            return this;
        }

        /** Sets the photo index */
        public PhotoViewIntentBuilder setPhotoIndex(Integer photoIndex) {
            mPhotoIndex = photoIndex;
            return this;
        }

        /** Sets the initial photo URI */
        public PhotoViewIntentBuilder setInitialPhotoUri(String initialPhotoUri) {
            mInitialPhotoUri = initialPhotoUri;
            return this;
        }

        /** Sets the photos URI */
        public PhotoViewIntentBuilder setPhotosUri(String photosUri) {
            mPhotosUri = photosUri;
            return this;
        }

        /** Sets the query projection */
        public PhotoViewIntentBuilder setProjection(String[] projection) {
            mProjection = projection;
            return this;
        }

        /** Sets the resolved photo URI. This method is for the case
         *  where the URI given to {@link PhotoViewActivity} points directly
         *  to a single image and does not need to be resolved via a query
         *  to the {@link ContentProvider}. If this value is set, it supersedes
         *  {@link #setPhotosUri(String)}. */
        public PhotoViewIntentBuilder setResolvedPhotoUri(String resolvedPhotoUri) {
            mResolvedPhotoUri = resolvedPhotoUri;
            return this;
        }

        /**
         * Sets the URI for a thumbnail preview of the photo.
         */
        public PhotoViewIntentBuilder setThumbnailUri(String thumbnailUri) {
            mThumbnailUri = thumbnailUri;
            return this;
        }

        /**
         * Sets the content Description for the photo
         */
        public PhotoViewIntentBuilder setContentDescription(String contentDescription) {
            mContentDescription = contentDescription;
            return this;
        }

        /**
         * Sets the maximum scale which an image is initially displayed at
         */
        public PhotoViewIntentBuilder setMaxInitialScale(float maxScale) {
            mMaxInitialScale = maxScale;
            return this;
        }

        /**
         * Enable watching the network for connectivity changes.
         *
         * When a change is detected, bitmap loaders will be restarted if required.
         */
        public PhotoViewIntentBuilder watchNetworkConnectivityChanges() {
            mWatchNetwork = true;
            return this;
        }

        /**
         * Enable a scale animation that animates the initial photo URI passed in using
         * {@link #setInitialPhotoUri}.
         *
         * Note: To avoid janky transitions, particularly when exiting the photoviewer, ensure the
         * following system UI flags are set on the root view of the relying app's activity
         * (via @{link View.setSystemUiVisibility(int)}):
         *     {@code View.SYSTEM_UI_FLAG_VISIBLE | View.SYSTEM_UI_FLAG_LAYOUT_STABLE}
         * As well, client should ensure {@code android:fitsSystemWindows} is set on the root
         * content view.
         */
        public PhotoViewIntentBuilder setScaleAnimation(int startX, int startY,
                int startWidth, int startHeight) {
            mScaleAnimation = true;
            mStartX = startX;
            mStartY = startY;
            mStartWidth = startWidth;
            mStartHeight = startHeight;
            return this;
        }

        // If this option is turned on, then the photoViewer will be initially
        // displayed with the action bar hidden. This is as opposed to the default
        // behavior, where the actionBar is initially shown.
        public PhotoViewIntentBuilder setActionBarHiddenInitially(
                boolean actionBarHiddenInitially) {
            mActionBarHiddenInitially = actionBarHiddenInitially;
            return this;
        }

        // If this option is turned on, then the small, lo-res thumbnail will
        // be scaled up to the maximum size to cover as much of the screen as
        // possible while still maintaining the correct aspect ratio. This means
        // that the image may appear blurry until the the full-res image is
        // loaded.
        // This is as opposed to the default behavior, where only part of the
        // thumbnail is displayed in a small view in the center of the screen,
        // and a loading spinner is displayed until the full-res image is loaded.
        public PhotoViewIntentBuilder setDisplayThumbsFullScreen(
                boolean displayFullScreenThumbs) {
            mDisplayFullScreenThumbs = displayFullScreenThumbs;
            return this;
        }

        /** Build the intent */
        public Intent build() {
            mIntent.setAction(Intent.ACTION_VIEW);

            // In Lollipop, each list of photos should appear as a document in the "Recents"
            // list. In earlier versions, this flag was Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET.
            mIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_DOCUMENT
                    // FLAG_ACTIVITY_CLEAR_TOP is needed for the case where the app tries to
                    // display a different photo while there is an existing activity instance
                    // for that list of photos. Since the initial photo is specified as an
                    // extra, without FLAG_ACTIVITY_CLEAR_TOP, the activity instance would
                    // just get restarted and it would display whatever photo it was last
                    // displaying. FLAG_ACTIVITY_CLEAR_TOP causes a new instance to be created,
                    // and it will display the new initial photo.
                    | Intent.FLAG_ACTIVITY_CLEAR_TOP);

            if (mPhotoIndex != null) {
                mIntent.putExtra(EXTRA_PHOTO_INDEX, (int) mPhotoIndex);
            }

            if (mInitialPhotoUri != null) {
                mIntent.putExtra(EXTRA_INITIAL_PHOTO_URI, mInitialPhotoUri);
            }

            if (mInitialPhotoUri != null && mPhotoIndex != null) {
                throw new IllegalStateException(
                        "specified both photo index and photo uri");
            }

            if (mPhotosUri != null) {
                mIntent.putExtra(EXTRA_PHOTOS_URI, mPhotosUri);
                mIntent.setData(Uri.parse(mPhotosUri));
            }

            if (mResolvedPhotoUri != null) {
                mIntent.putExtra(EXTRA_RESOLVED_PHOTO_URI, mResolvedPhotoUri);
            }

            if (mProjection != null) {
                mIntent.putExtra(EXTRA_PROJECTION, mProjection);
            }

            if (mThumbnailUri != null) {
                mIntent.putExtra(EXTRA_THUMBNAIL_URI, mThumbnailUri);
            }

            if (mContentDescription != null) {
                mIntent.putExtra(EXTRA_CONTENT_DESCRIPTION, mContentDescription);
            }

            if (mMaxInitialScale != null) {
                mIntent.putExtra(EXTRA_MAX_INITIAL_SCALE, mMaxInitialScale);
            }

            mIntent.putExtra(EXTRA_WATCH_NETWORK, mWatchNetwork);

            mIntent.putExtra(EXTRA_SCALE_UP_ANIMATION, mScaleAnimation);
            if (mScaleAnimation) {
                mIntent.putExtra(EXTRA_ANIMATION_START_X, mStartX);
                mIntent.putExtra(EXTRA_ANIMATION_START_Y, mStartY);
                mIntent.putExtra(EXTRA_ANIMATION_START_WIDTH, mStartWidth);
                mIntent.putExtra(EXTRA_ANIMATION_START_HEIGHT, mStartHeight);
            }

            mIntent.putExtra(EXTRA_ACTION_BAR_HIDDEN_INITIALLY, mActionBarHiddenInitially);
            mIntent.putExtra(EXTRA_DISPLAY_THUMBS_FULLSCREEN, mDisplayFullScreenThumbs);
            mIntent.putExtra(EXTRA_ENABLE_TIMER_LIGHTS_OUT, mEnableTimerLightsOut);
            return mIntent;
        }
    }
}
