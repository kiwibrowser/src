// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.photo_picker;

import android.support.annotation.NonNull;
import android.webkit.MimeTypeMap;

import java.io.File;
import java.io.FileFilter;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;

/**
 * A file filter for handling extensions mapping to MIME types (such as images/jpeg and images/*).
 */
class MimeTypeFileFilter implements FileFilter {
    private HashSet<String> mExtensions = new HashSet<>();
    private HashSet<String> mMimeTypes = new HashSet<>();
    private HashSet<String> mMimeSupertypes = new HashSet<>();
    private MimeTypeMap mMimeTypeMap;

    /**
     * Contructs a MimeTypeFileFilter object.
     * @param mimeTypes A list of MIME types this filter accepts.
     *                  For example: images/gif, video/*.
     */
    public MimeTypeFileFilter(@NonNull List<String> mimeTypes) {
        for (String field : mimeTypes) {
            field = field.trim().toLowerCase(Locale.US);
            if (field.startsWith(".")) {
                mExtensions.add(field.substring(1));
            } else if (field.endsWith("/*")) {
                mMimeSupertypes.add(field.substring(0, field.length() - 2));
            } else if (field.contains("/")) {
                mMimeTypes.add(field);
            } else {
                // Throw exception?
            }
        }

        mMimeTypeMap = MimeTypeMap.getSingleton();
    }

    @Override
    public boolean accept(@NonNull File file) {
        if (file.isDirectory()) {
            return true;
        }

        String uri = file.toURI().toString();
        String ext = MimeTypeMap.getFileExtensionFromUrl(uri).toLowerCase(Locale.US);
        if (mExtensions.contains(ext)) {
            return true;
        }

        String mimeType = getMimeTypeFromExtension(ext);
        if (mimeType != null) {
            if (mMimeTypes.contains(mimeType)
                    || mMimeSupertypes.contains(getMimeSupertype(mimeType))) {
                return true;
            }
        }

        return false;
    }

    private HashSet<String> getAcceptedSupertypes() {
        HashSet<String> supertypes = new HashSet<>();
        supertypes.addAll(mMimeSupertypes);
        for (String mimeType : mMimeTypes) {
            supertypes.add(getMimeSupertype(mimeType));
        }
        for (String ext : mExtensions) {
            String mimeType = getMimeTypeFromExtension(ext);
            if (mimeType != null) {
                supertypes.add(getMimeSupertype(mimeType));
            }
        }
        return supertypes;
    }

    private String getMimeTypeFromExtension(@NonNull String ext) {
        String mimeType = mMimeTypeMap.getMimeTypeFromExtension(ext);
        return (mimeType != null) ? mimeType.toLowerCase(Locale.US) : null;
    }

    @NonNull
    private String getMimeSupertype(@NonNull String mimeType) {
        return mimeType.split("/", 2)[0];
    }
}
