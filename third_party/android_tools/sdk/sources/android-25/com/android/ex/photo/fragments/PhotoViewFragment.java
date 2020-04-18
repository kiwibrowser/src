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

package com.android.ex.photo.fragments;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.Cursor;
import android.graphics.drawable.Drawable;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.LoaderManager;
import android.support.v4.content.Loader;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.android.ex.photo.Intents;
import com.android.ex.photo.PhotoViewCallbacks;
import com.android.ex.photo.PhotoViewCallbacks.CursorChangedListener;
import com.android.ex.photo.PhotoViewCallbacks.OnScreenListener;
import com.android.ex.photo.PhotoViewController.ActivityInterface;
import com.android.ex.photo.R;
import com.android.ex.photo.adapters.PhotoPagerAdapter;
import com.android.ex.photo.loaders.PhotoBitmapLoaderInterface;
import com.android.ex.photo.loaders.PhotoBitmapLoaderInterface.BitmapResult;
import com.android.ex.photo.views.PhotoView;
import com.android.ex.photo.views.ProgressBarWrapper;

/**
 * Displays a photo.
 */
public class PhotoViewFragment extends Fragment implements
        LoaderManager.LoaderCallbacks<BitmapResult>,
        OnClickListener,
        OnScreenListener,
        CursorChangedListener {

    /**
     * Interface for components that are internally scrollable left-to-right.
     */
    public static interface HorizontallyScrollable {
        /**
         * Return {@code true} if the component needs to receive right-to-left
         * touch movements.
         *
         * @param origX the raw x coordinate of the initial touch
         * @param origY the raw y coordinate of the initial touch
         */

        public boolean interceptMoveLeft(float origX, float origY);

        /**
         * Return {@code true} if the component needs to receive left-to-right
         * touch movements.
         *
         * @param origX the raw x coordinate of the initial touch
         * @param origY the raw y coordinate of the initial touch
         */
        public boolean interceptMoveRight(float origX, float origY);
    }

    protected final static String STATE_INTENT_KEY =
            "com.android.mail.photo.fragments.PhotoViewFragment.INTENT";

    protected final static String ARG_INTENT = "arg-intent";
    protected final static String ARG_POSITION = "arg-position";
    protected final static String ARG_SHOW_SPINNER = "arg-show-spinner";

    /** The URL of a photo to display */
    protected String mResolvedPhotoUri;
    protected String mThumbnailUri;
    protected String mContentDescription;
    /** The intent we were launched with */
    protected Intent mIntent;
    protected PhotoViewCallbacks mCallback;
    protected PhotoPagerAdapter mAdapter;

    protected BroadcastReceiver mInternetStateReceiver;

    protected PhotoView mPhotoView;
    protected ImageView mPhotoPreviewImage;
    protected TextView mEmptyText;
    protected ImageView mRetryButton;
    protected ProgressBarWrapper mPhotoProgressBar;

    protected int mPosition;

    /** Whether or not the fragment should make the photo full-screen */
    protected boolean mFullScreen;

    /**
     * True if the PhotoViewFragment should watch the network state in order to restart loaders.
     */
    protected boolean mWatchNetworkState;

    /** Whether or not this fragment will only show the loading spinner */
    protected boolean mOnlyShowSpinner;

    /** Whether or not the progress bar is showing valid information about the progress stated */
    protected boolean mProgressBarNeeded = true;

    protected View mPhotoPreviewAndProgress;
    protected boolean mThumbnailShown;

    /** Whether or not there is currently a connection to the internet */
    protected boolean mConnected;

    /** Whether or not we can display the thumbnail at fullscreen size */
    protected boolean mDisplayThumbsFullScreen;

    /** Public no-arg constructor for allowing the framework to handle orientation changes */
    public PhotoViewFragment() {
        // Do nothing.
    }

    /**
     * Create a {@link PhotoViewFragment}.
     * @param intent
     * @param position
     * @param onlyShowSpinner
     */
    public static PhotoViewFragment newInstance(
            Intent intent, int position, boolean onlyShowSpinner) {
        final PhotoViewFragment f = new PhotoViewFragment();
        initializeArguments(intent, position, onlyShowSpinner, f);
        return f;
    }

    public static void initializeArguments(
            Intent intent, int position, boolean onlyShowSpinner, PhotoViewFragment f) {
        final Bundle b = new Bundle();
        b.putParcelable(ARG_INTENT, intent);
        b.putInt(ARG_POSITION, position);
        b.putBoolean(ARG_SHOW_SPINNER, onlyShowSpinner);
        f.setArguments(b);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        mCallback = getCallbacks();
        if (mCallback == null) {
            throw new IllegalArgumentException(
                    "Activity must be a derived class of PhotoViewActivity");
        }
        mAdapter = mCallback.getAdapter();
        if (mAdapter == null) {
            throw new IllegalStateException("Callback reported null adapter");
        }
        // Don't call until we've setup the entire view
        setViewVisibility();
    }

    protected PhotoViewCallbacks getCallbacks() {
        return ((ActivityInterface) getActivity()).getController();
    }

    @Override
    public void onDetach() {
        mCallback = null;
        super.onDetach();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        final Bundle bundle = getArguments();
        if (bundle == null) {
            return;
        }
        mIntent = bundle.getParcelable(ARG_INTENT);
        mDisplayThumbsFullScreen = mIntent.getBooleanExtra(
                Intents.EXTRA_DISPLAY_THUMBS_FULLSCREEN, false);

        mPosition = bundle.getInt(ARG_POSITION);
        mOnlyShowSpinner = bundle.getBoolean(ARG_SHOW_SPINNER);
        mProgressBarNeeded = true;

        if (savedInstanceState != null) {
            final Bundle state = savedInstanceState.getBundle(STATE_INTENT_KEY);
            if (state != null) {
                mIntent = new Intent().putExtras(state);
            }
        }

        if (mIntent != null) {
            mResolvedPhotoUri = mIntent.getStringExtra(Intents.EXTRA_RESOLVED_PHOTO_URI);
            mThumbnailUri = mIntent.getStringExtra(Intents.EXTRA_THUMBNAIL_URI);
            mContentDescription = mIntent.getStringExtra(Intents.EXTRA_CONTENT_DESCRIPTION);
            mWatchNetworkState = mIntent.getBooleanExtra(Intents.EXTRA_WATCH_NETWORK, false);
        }
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        final View view = inflater.inflate(R.layout.photo_fragment_view, container, false);
        initializeView(view);
        return view;
    }

    protected void initializeView(View view) {
        mPhotoView = (PhotoView) view.findViewById(R.id.photo_view);
        mPhotoView.setMaxInitialScale(mIntent.getFloatExtra(Intents.EXTRA_MAX_INITIAL_SCALE, 1));
        mPhotoView.setOnClickListener(this);
        mPhotoView.setFullScreen(mFullScreen, false);
        mPhotoView.enableImageTransforms(false);
        mPhotoView.setContentDescription(mContentDescription);

        mPhotoPreviewAndProgress = view.findViewById(R.id.photo_preview);
        mPhotoPreviewImage = (ImageView) view.findViewById(R.id.photo_preview_image);
        mThumbnailShown = false;
        final ProgressBar indeterminate =
                (ProgressBar) view.findViewById(R.id.indeterminate_progress);
        final ProgressBar determinate =
                (ProgressBar) view.findViewById(R.id.determinate_progress);
        mPhotoProgressBar = new ProgressBarWrapper(determinate, indeterminate, true);
        mEmptyText = (TextView) view.findViewById(R.id.empty_text);
        mRetryButton = (ImageView) view.findViewById(R.id.retry_button);

        // Don't call until we've setup the entire view
        setViewVisibility();
    }

    @Override
    public void onResume() {
        super.onResume();
        mCallback.addScreenListener(mPosition, this);
        mCallback.addCursorListener(this);

        if (mWatchNetworkState) {
            if (mInternetStateReceiver == null) {
                mInternetStateReceiver = new InternetStateBroadcastReceiver();
            }
            getActivity().registerReceiver(mInternetStateReceiver,
                    new IntentFilter(ConnectivityManager.CONNECTIVITY_ACTION));
            ConnectivityManager connectivityManager = (ConnectivityManager)
                    getActivity().getSystemService(Context.CONNECTIVITY_SERVICE);
            NetworkInfo activeNetInfo = connectivityManager.getActiveNetworkInfo();
            if (activeNetInfo != null) {
                mConnected = activeNetInfo.isConnected();
            } else {
                // Best to set this to false, since it won't stop us from trying to download,
                // only allow us to try re-download if we get notified that we do have a connection.
                mConnected = false;
            }
        }

        if (!isPhotoBound()) {
            mProgressBarNeeded = true;
            mPhotoPreviewAndProgress.setVisibility(View.VISIBLE);

            getLoaderManager().initLoader(PhotoViewCallbacks.BITMAP_LOADER_THUMBNAIL,
                    null, this);

            // FLAG: If we are displaying thumbnails at fullscreen size, then we
            // could defer the loading of the fullscreen image until the thumbnail
            // has finished loading, or even until the user attempts to zoom in.
            getLoaderManager().initLoader(PhotoViewCallbacks.BITMAP_LOADER_PHOTO,
                    null, this);
        }
    }

    @Override
    public void onPause() {
        // Remove listeners
        if (mWatchNetworkState) {
            getActivity().unregisterReceiver(mInternetStateReceiver);
        }
        mCallback.removeCursorListener(this);
        mCallback.removeScreenListener(mPosition);
        super.onPause();
    }

    @Override
    public void onDestroyView() {
        // Clean up views and other components
        if (mPhotoView != null) {
            mPhotoView.clear();
            mPhotoView = null;
        }
        super.onDestroyView();
    }

    public String getPhotoUri() {
        return mResolvedPhotoUri;
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);

        if (mIntent != null) {
            outState.putParcelable(STATE_INTENT_KEY, mIntent.getExtras());
        }
    }

    @Override
    public Loader<BitmapResult> onCreateLoader(int id, Bundle args) {
        if(mOnlyShowSpinner) {
            return null;
        }
        String uri = null;
        switch (id) {
            case PhotoViewCallbacks.BITMAP_LOADER_THUMBNAIL:
                uri = mThumbnailUri;
                break;
            case PhotoViewCallbacks.BITMAP_LOADER_PHOTO:
                uri = mResolvedPhotoUri;
                break;
        }
        return mCallback.onCreateBitmapLoader(id, args, uri);
    }

    @Override
    public void onLoadFinished(Loader<BitmapResult> loader, BitmapResult result) {
        // If we don't have a view, the fragment has been paused. We'll get the cursor again later.
        // If we're not added, the fragment has detached during the loading process. We no longer
        // need the result.
        if (getView() == null || !isAdded()) {
            return;
        }

        final Drawable data = result.getDrawable(getResources());

        final int id = loader.getId();
        switch (id) {
            case PhotoViewCallbacks.BITMAP_LOADER_THUMBNAIL:
                if (mDisplayThumbsFullScreen) {
                    displayPhoto(result);
                } else {
                    if (isPhotoBound()) {
                        // There is need to do anything with the thumbnail
                        // image, as the full size image is being shown.
                        return;
                    }

                    if (data == null) {
                        // no preview, show default
                        mPhotoPreviewImage.setImageResource(R.drawable.default_image);
                        mThumbnailShown = false;
                    } else {
                        // show preview
                        mPhotoPreviewImage.setImageDrawable(data);
                        mThumbnailShown = true;
                    }
                    mPhotoPreviewImage.setVisibility(View.VISIBLE);
                    if (getResources().getBoolean(R.bool.force_thumbnail_no_scaling)) {
                        mPhotoPreviewImage.setScaleType(ImageView.ScaleType.CENTER);
                    }
                    enableImageTransforms(false);
                }
                break;

            case PhotoViewCallbacks.BITMAP_LOADER_PHOTO:
                displayPhoto(result);
                break;
            default:
                break;
        }

        if (mProgressBarNeeded == false) {
            // Hide the progress bar as it isn't needed anymore.
            mPhotoProgressBar.setVisibility(View.GONE);
        }

        if (data != null) {
            mCallback.onNewPhotoLoaded(mPosition);
        }
        setViewVisibility();
    }

    private void displayPhoto(BitmapResult result) {
        if (result.status == BitmapResult.STATUS_EXCEPTION) {
            mProgressBarNeeded = false;
            mEmptyText.setText(R.string.failed);
            mEmptyText.setVisibility(View.VISIBLE);
            mCallback.onFragmentPhotoLoadComplete(this, false /* success */);
        } else {
            mEmptyText.setVisibility(View.GONE);
            final Drawable data = result.getDrawable(getResources());
            bindPhoto(data);
            mCallback.onFragmentPhotoLoadComplete(this, true /* success */);
        }
    }

    /**
     * Binds an image to the photo view.
     */
    private void bindPhoto(Drawable drawable) {
        if (drawable != null) {
            if (mPhotoView != null) {
                mPhotoView.bindDrawable(drawable);
            }
            enableImageTransforms(true);
            mPhotoPreviewAndProgress.setVisibility(View.GONE);
            mProgressBarNeeded = false;
        }
    }

    public Drawable getDrawable() {
        return (mPhotoView != null ? mPhotoView.getDrawable() : null);
    }

    /**
     * Enable or disable image transformations. When transformations are enabled, this view
     * consumes all touch events.
     */
    public void enableImageTransforms(boolean enable) {
        mPhotoView.enableImageTransforms(enable);
    }

    @Override
    public void onLoaderReset(Loader<BitmapResult> loader) {
        // Do nothing
    }

    @Override
    public void onClick(View v) {
        mCallback.toggleFullScreen();
    }

    @Override
    public void onFullScreenChanged(boolean fullScreen) {
        setViewVisibility();
    }

    @Override
    public void onViewUpNext() {
        resetViews();
    }

    @Override
    public void onViewActivated() {
        if (!mCallback.isFragmentActive(this)) {
            // we're not in the foreground; reset our view
            resetViews();
        } else {
            if (!isPhotoBound()) {
                // Restart the loader
                getLoaderManager().restartLoader(PhotoViewCallbacks.BITMAP_LOADER_THUMBNAIL,
                        null, this);
            }
            mCallback.onFragmentVisible(this);
        }
    }

    /**
     * Reset the views to their default states
     */
    public void resetViews() {
        if (mPhotoView != null) {
            mPhotoView.resetTransformations();
        }
    }

    @Override
    public boolean onInterceptMoveLeft(float origX, float origY) {
        if (!mCallback.isFragmentActive(this)) {
            // we're not in the foreground; don't intercept any touches
            return false;
        }

        return (mPhotoView != null && mPhotoView.interceptMoveLeft(origX, origY));
    }

    @Override
    public boolean onInterceptMoveRight(float origX, float origY) {
        if (!mCallback.isFragmentActive(this)) {
            // we're not in the foreground; don't intercept any touches
            return false;
        }

        return (mPhotoView != null && mPhotoView.interceptMoveRight(origX, origY));
    }

    /**
     * Returns {@code true} if a photo has been bound. Otherwise, returns {@code false}.
     */
    public boolean isPhotoBound() {
        return (mPhotoView != null && mPhotoView.isPhotoBound());
    }

    /**
     * Sets view visibility depending upon whether or not we're in "full screen" mode.
     */
    private void setViewVisibility() {
        final boolean fullScreen = mCallback == null ? false : mCallback.isFragmentFullScreen(this);
        setFullScreen(fullScreen);
    }

    /**
     * Sets full-screen mode for the views.
     */
    public void setFullScreen(boolean fullScreen) {
        mFullScreen = fullScreen;
    }

    @Override
    public void onCursorChanged(Cursor cursor) {
        if (mAdapter == null) {
            // The adapter is set in onAttach(), and is guaranteed to be non-null. We have magically
            // received an onCursorChanged without attaching to an activity. Ignore this cursor
            // change.
            return;
        }
        // FLAG: There is a problem here:
        // If the cursor changes, and new items are added at an earlier position than
        // the current item, we will switch photos here. Really we should probably
        // try to find a photo with the same url and move the cursor to that position.
        if (cursor.moveToPosition(mPosition) && !isPhotoBound()) {
            mCallback.onCursorChanged(this, cursor);

            final LoaderManager manager = getLoaderManager();

            final Loader<BitmapResult> fakePhotoLoader = manager.getLoader(
                    PhotoViewCallbacks.BITMAP_LOADER_PHOTO);
            if (fakePhotoLoader != null) {
                final PhotoBitmapLoaderInterface loader = (PhotoBitmapLoaderInterface) fakePhotoLoader;
                mResolvedPhotoUri = mAdapter.getPhotoUri(cursor);
                loader.setPhotoUri(mResolvedPhotoUri);
                loader.forceLoad();
            }

            if (!mThumbnailShown) {
                final Loader<BitmapResult> fakeThumbnailLoader = manager.getLoader(
                        PhotoViewCallbacks.BITMAP_LOADER_THUMBNAIL);
                if (fakeThumbnailLoader != null) {
                    final PhotoBitmapLoaderInterface loader = (PhotoBitmapLoaderInterface) fakeThumbnailLoader;
                    mThumbnailUri = mAdapter.getThumbnailUri(cursor);
                    loader.setPhotoUri(mThumbnailUri);
                    loader.forceLoad();
                }
            }
        }
    }

    public int getPosition() {
        return mPosition;
    }

    public ProgressBarWrapper getPhotoProgressBar() {
        return mPhotoProgressBar;
    }

    public TextView getEmptyText() {
        return mEmptyText;
    }

    public ImageView getRetryButton() {
        return mRetryButton;
    }

    public boolean isProgressBarNeeded() {
        return mProgressBarNeeded;
    }

    private class InternetStateBroadcastReceiver extends BroadcastReceiver {

        @Override
        public void onReceive(Context context, Intent intent) {
            // This is only created if we have the correct permissions, so
            ConnectivityManager connectivityManager = (ConnectivityManager)
                    context.getSystemService(Context.CONNECTIVITY_SERVICE);
            NetworkInfo activeNetInfo = connectivityManager.getActiveNetworkInfo();
            if (activeNetInfo == null || !activeNetInfo.isConnected()) {
                mConnected = false;
                return;
            }
            if (mConnected == false && !isPhotoBound()) {
                if (mThumbnailShown == false) {
                    getLoaderManager().restartLoader(PhotoViewCallbacks.BITMAP_LOADER_THUMBNAIL,
                            null, PhotoViewFragment.this);
                }
                getLoaderManager().restartLoader(PhotoViewCallbacks.BITMAP_LOADER_PHOTO,
                        null, PhotoViewFragment.this);
                mConnected = true;
                mPhotoProgressBar.setVisibility(View.VISIBLE);
            }
        }
    }
}
