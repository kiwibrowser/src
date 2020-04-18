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

package com.android.ex.photo.loaders;

import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.support.v4.content.CursorLoader;

import com.android.ex.photo.provider.PhotoContract;

/**
 * Loader for a set of photo IDs.
 */
public class PhotoPagerLoader extends CursorLoader {
    private final Uri mPhotosUri;
    private final String[] mProjection;

    public PhotoPagerLoader(
            Context context, Uri photosUri, String[] projection) {
        super(context);
        mPhotosUri = photosUri;
        mProjection = projection != null ? projection : PhotoContract.PhotoQuery.PROJECTION;
    }

    @Override
    public Cursor loadInBackground() {
        Cursor returnCursor = null;

        final Uri loaderUri = mPhotosUri.buildUpon().appendQueryParameter(
                PhotoContract.ContentTypeParameters.CONTENT_TYPE, "image/").build();
        setUri(loaderUri);
        setProjection(mProjection);
        returnCursor = super.loadInBackground();

        return returnCursor;
    }
}
