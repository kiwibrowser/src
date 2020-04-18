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

package com.android.ex.photo.adapters;

import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.support.v4.app.Fragment;
import android.support.v4.util.SimpleArrayMap;

import com.android.ex.photo.Intents;
import com.android.ex.photo.Intents.PhotoViewIntentBuilder;
import com.android.ex.photo.fragments.PhotoViewFragment;
import com.android.ex.photo.provider.PhotoContract.PhotoViewColumns;
import com.android.ex.photo.provider.PhotoContract.PhotoQuery;

/**
 * Pager adapter for the photo view
 */
public class PhotoPagerAdapter extends BaseCursorPagerAdapter {
    protected SimpleArrayMap<String, Integer> mColumnIndices =
            new SimpleArrayMap<String, Integer>(PhotoQuery.PROJECTION.length);
    protected final float mMaxScale;
    protected boolean mDisplayThumbsFullScreen;

    public PhotoPagerAdapter(
            Context context, android.support.v4.app.FragmentManager fm, Cursor c,
            float maxScale, boolean thumbsFullScreen) {
        super(context, fm, c);
        mMaxScale = maxScale;
        mDisplayThumbsFullScreen = thumbsFullScreen;
    }

    @Override
    public Fragment getItem(Context context, Cursor cursor, int position) {
        final String photoUri = getPhotoUri(cursor);
        final String thumbnailUri = getThumbnailUri(cursor);
        final String contentDescription = getPhotoName(cursor);
        boolean loading = shouldShowLoadingIndicator(cursor);
        boolean onlyShowSpinner = false;
        if(photoUri == null && loading) {
            onlyShowSpinner = true;
        }

        // create new PhotoViewFragment
        final PhotoViewIntentBuilder builder =
                Intents.newPhotoViewFragmentIntentBuilder(mContext, getPhotoViewFragmentClass());
        builder
            .setResolvedPhotoUri(photoUri)
            .setThumbnailUri(thumbnailUri)
            .setContentDescription(contentDescription)
            .setDisplayThumbsFullScreen(mDisplayThumbsFullScreen)
            .setMaxInitialScale(mMaxScale);

        return createPhotoViewFragment(builder.build(), position, onlyShowSpinner);
    }

    protected Class<? extends PhotoViewFragment> getPhotoViewFragmentClass() {
        return PhotoViewFragment.class;
    }

    protected PhotoViewFragment createPhotoViewFragment(
            Intent intent, int position, boolean onlyShowSpinner) {
        return PhotoViewFragment.newInstance(intent, position, onlyShowSpinner);
    }

    @Override
    public Cursor swapCursor(Cursor newCursor) {
        mColumnIndices.clear();

        if (newCursor != null) {
            for(String column : PhotoQuery.PROJECTION) {
                mColumnIndices.put(column, newCursor.getColumnIndexOrThrow(column));
            }

            for(String column : PhotoQuery.OPTIONAL_COLUMNS) {
                int index = newCursor.getColumnIndex(column);
                if (index != -1) {
                    mColumnIndices.put(column, index);
                }
            }
        }

        return super.swapCursor(newCursor);
    }

    public String getPhotoUri(Cursor cursor) {
        return getString(cursor, PhotoViewColumns.CONTENT_URI);
    }

    public String getThumbnailUri(Cursor cursor) {
        return getString(cursor, PhotoViewColumns.THUMBNAIL_URI);
    }

    public String getContentType(Cursor cursor) {
        return getString(cursor, PhotoViewColumns.CONTENT_TYPE);
    }

    public String getPhotoName(Cursor cursor) {
        return getString(cursor, PhotoViewColumns.NAME);
    }

    public boolean shouldShowLoadingIndicator(Cursor cursor) {
        String value = getString(cursor, PhotoViewColumns.LOADING_INDICATOR);
        if (value == null) {
            return false;
        } else {
            return Boolean.valueOf(value);
        }
    }

    private String getString(Cursor cursor, String column) {
        if (mColumnIndices.containsKey(column)) {
            return cursor.getString(mColumnIndices.get(column));
        } else {
            return null;
        }
    }
}
