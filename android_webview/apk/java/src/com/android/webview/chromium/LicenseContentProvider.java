// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.android.webview.chromium;

import android.annotation.TargetApi;
import android.content.ContentProvider;
import android.content.ContentValues;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import android.util.Log;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * Content provider for the OSS licenses file.
 * This is compiled into the stub WebView and so should not depend on any classes from Chromium.
 */
@TargetApi(Build.VERSION_CODES.KITKAT)
public class LicenseContentProvider
        extends ContentProvider implements ContentProvider.PipeDataWriter<String> {
    public static final String LICENSES_URI_SUFFIX = "LicenseContentProvider/webview_licenses";
    public static final String LICENSES_CONTENT_TYPE = "text/html";
    private static final String TAG = "LicenseCP";

    @Override
    public boolean onCreate() {
        return true;
    }

    @Override
    public ParcelFileDescriptor openFile(Uri uri, String mode) throws FileNotFoundException {
        if (uri != null && uri.toString().endsWith(LICENSES_URI_SUFFIX)) {
            return openPipeHelper(null, null, null, "webview_licenses.notice", this);
        }
        return null;
    }

    @Override
    public void writeDataToPipe(
            ParcelFileDescriptor output, Uri uri, String mimeType, Bundle opts, String filename) {
        try (InputStream in = getContext().getAssets().open(filename);
                OutputStream out = new FileOutputStream(output.getFileDescriptor());) {
            byte[] buf = new byte[8192];
            int size = -1;
            while ((size = in.read(buf)) != -1) {
                out.write(buf, 0, size);
            }
        } catch (IOException e) {
            Log.e(TAG, "Failed to read the license file", e);
        }
    }

    @Override
    public String getType(Uri uri) {
        if (uri != null && uri.toString().endsWith(LICENSES_URI_SUFFIX)) {
            return LICENSES_CONTENT_TYPE;
        }
        return null;
    }

    @Override
    public int update(Uri uri, ContentValues values, String where, String[] whereArgs) {
        throw new UnsupportedOperationException();
    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        throw new UnsupportedOperationException();
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) {
        throw new UnsupportedOperationException();
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs,
            String sortOrder) {
        throw new UnsupportedOperationException();
    }
}