package com.android.ex.photo;

import android.database.Cursor;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.content.Loader;

import com.android.ex.photo.adapters.PhotoPagerAdapter;
import com.android.ex.photo.fragments.PhotoViewFragment;
import com.android.ex.photo.loaders.PhotoBitmapLoaderInterface.BitmapResult;

public interface PhotoViewCallbacks {

    public static final int BITMAP_LOADER_AVATAR = 1;
    public static final int BITMAP_LOADER_THUMBNAIL = 2;
    public static final int BITMAP_LOADER_PHOTO = 3;

    /**
     * Listener to be invoked for screen events.
     */
    public static interface OnScreenListener {

        /**
         * The full screen state has changed.
         */
        public void onFullScreenChanged(boolean fullScreen);

        /**
         * A new view has been activated and the previous view de-activated.
         */
        public void onViewActivated();

        /**
         * This view is a candidate for being the next view.
         *
         * This will be called when the view is focused completely on the view immediately before
         * or after this one, so that this view can reset itself if nessecary.
         */
        public void onViewUpNext();

        /**
         * Called when a right-to-left touch move intercept is about to occur.
         *
         * @param origX the raw x coordinate of the initial touch
         * @param origY the raw y coordinate of the initial touch
         * @return {@code true} if the touch should be intercepted.
         */
        public boolean onInterceptMoveLeft(float origX, float origY);

        /**
         * Called when a left-to-right touch move intercept is about to occur.
         *
         * @param origX the raw x coordinate of the initial touch
         * @param origY the raw y coordinate of the initial touch
         * @return {@code true} if the touch should be intercepted.
         */
        public boolean onInterceptMoveRight(float origX, float origY);
    }

    public static interface CursorChangedListener {
        /**
         * Called when the cursor that contains the photo list data
         * is updated. Note that there is no guarantee that the cursor
         * will be at the proper position.
         * @param cursor the cursor containing the photo list data
         */
        public void onCursorChanged(Cursor cursor);
    }

    public void addScreenListener(int position, OnScreenListener listener);

    public void removeScreenListener(int position);

    public void addCursorListener(CursorChangedListener listener);

    public void removeCursorListener(CursorChangedListener listener);

    public void setViewActivated(int position);

    public void onNewPhotoLoaded(int position);

    public void onFragmentPhotoLoadComplete(PhotoViewFragment fragment,
            boolean success);

    public void toggleFullScreen();

    public boolean isFragmentActive(Fragment fragment);

    public void onFragmentVisible(PhotoViewFragment fragment);

    public boolean isFragmentFullScreen(Fragment fragment);

    public void onCursorChanged(PhotoViewFragment fragment, Cursor cursor);

    public Loader<BitmapResult> onCreateBitmapLoader(int id, Bundle args, String uri);

    /**
     * Returns the adapter associated with this activity.
     */
    public PhotoPagerAdapter getAdapter();
}
