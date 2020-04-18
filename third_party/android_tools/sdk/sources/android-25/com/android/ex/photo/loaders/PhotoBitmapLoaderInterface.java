package com.android.ex.photo.loaders;

import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;

public interface PhotoBitmapLoaderInterface {

    public void setPhotoUri(String photoUri);

    public void forceLoad();

    public static class BitmapResult {
        public static final int STATUS_SUCCESS = 0;
        public static final int STATUS_EXCEPTION = 1;

        public Drawable drawable;
        public Bitmap bitmap;
        public int status;

        /**
         * Returns a drawable to be used in the {@link com.android.ex.photo.views.PhotoView}.
         * Should return null if the drawable is not ready to be shown (for instance, if
         * the underlying bitmap is null).
         */
        public Drawable getDrawable(Resources resources) {
            if (resources == null) {
                throw new IllegalArgumentException("resources can not be null!");
            }

            if (drawable != null) {
                return drawable;
            }

            // Don't create a new drawable if there's no bitmap. PhotoViewFragment regards
            // a null drawable as a signal to keep showing the loading stuff.
            // b/12348405.
            if (bitmap == null) {
                return null;
            }

            return new BitmapDrawable(resources, bitmap);
        }
    }
}
