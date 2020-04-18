/*
 * Copyright (C) 2012 The Android Open Source Project
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

package com.android.ex.chips;

import android.content.res.Resources;
import android.net.Uri;
import android.provider.ContactsContract;
import android.provider.ContactsContract.CommonDataKinds.Email;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.provider.ContactsContract.Contacts;

/**
 * Phone and Email queries for supporting Chips UI.
 */
/* package */ class Queries {

    public static final Query PHONE = new Query(new String[] {
            Contacts.DISPLAY_NAME,                          // 0
            Phone.NUMBER,                                   // 1
            Phone.TYPE,                                     // 2
            Phone.LABEL,                                    // 3
            Phone.CONTACT_ID,                               // 4
            Phone._ID,                                      // 5
            Contacts.PHOTO_THUMBNAIL_URI,                   // 6
            Contacts.DISPLAY_NAME_SOURCE,                   // 7
            Contacts.LOOKUP_KEY,                            // 8
            ContactsContract.CommonDataKinds.Email.MIMETYPE // 9
        }, Phone.CONTENT_FILTER_URI, Phone.CONTENT_URI) {

            @Override
            public CharSequence getTypeLabel(Resources res, int type, CharSequence label) {
                return Phone.getTypeLabel(res, type, label);
            }

    };

    public static final Query EMAIL = new Query(new String[]{
            Contacts.DISPLAY_NAME,                          // 0
            Email.DATA,                                     // 1
            Email.TYPE,                                     // 2
            Email.LABEL,                                    // 3
            Email.CONTACT_ID,                               // 4
            Email._ID,                                      // 5
            Contacts.PHOTO_THUMBNAIL_URI,                   // 6
            Contacts.DISPLAY_NAME_SOURCE,                   // 7
            Contacts.LOOKUP_KEY,                            // 8
            ContactsContract.CommonDataKinds.Email.MIMETYPE // 9
        }, Email.CONTENT_FILTER_URI, Email.CONTENT_URI) {

            @Override
            public CharSequence getTypeLabel(Resources res, int type, CharSequence label) {
                return Email.getTypeLabel(res, type, label);
            }

    };

    static abstract class Query {
        private final String[] mProjection;
        private final Uri mContentFilterUri;
        private final Uri mContentUri;

        public static final int NAME = 0;                // String
        public static final int DESTINATION = 1;         // String
        public static final int DESTINATION_TYPE = 2;    // int
        public static final int DESTINATION_LABEL = 3;   // String
        public static final int CONTACT_ID = 4;          // long
        public static final int DATA_ID = 5;             // long
        public static final int PHOTO_THUMBNAIL_URI = 6; // String
        public static final int DISPLAY_NAME_SOURCE = 7; // int
        public static final int LOOKUP_KEY = 8;          // String
        public static final int MIME_TYPE = 9;           // String

        public Query(String[] projection, Uri contentFilter, Uri content) {
            mProjection = projection;
            mContentFilterUri = contentFilter;
            mContentUri = content;
        }

        public String[] getProjection() {
            return mProjection;
        }

        public Uri getContentFilterUri() {
            return mContentFilterUri;
        }

        public Uri getContentUri() {
            return mContentUri;
        }

        public abstract CharSequence getTypeLabel(Resources res, int type, CharSequence label);
    }
}
