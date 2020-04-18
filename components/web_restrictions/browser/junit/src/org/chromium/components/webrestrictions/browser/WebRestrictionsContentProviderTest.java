// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.webrestrictions.browser;

import static org.hamcrest.CoreMatchers.is;
import static org.hamcrest.CoreMatchers.not;
import static org.hamcrest.CoreMatchers.nullValue;
import static org.junit.Assert.assertThat;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.pm.ProviderInfo;
import android.database.Cursor;
import android.net.Uri;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentMatchers;
import org.mockito.Mockito;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowContentResolver;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.components.webrestrictions.browser.WebRestrictionsContentProvider.WebRestrictionsResult;

/**
 * Tests of WebRestrictionsContentProvider.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class WebRestrictionsContentProviderTest {
    private static final String AUTHORITY = "org.chromium.browser.DummyProvider";

    private WebRestrictionsContentProvider mContentProvider;
    private ContentResolver mContentResolver;
    private Uri mUri;

    @Before
    public void setUp() {
        // The test needs a concrete version of the test class, but mocks the additional members as
        // necessary.
        mContentProvider = Mockito.spy(new WebRestrictionsContentProvider() {
            @Override
            protected WebRestrictionsResult shouldProceed(String callingPackage, String url) {
                return null;
            }

            @Override
            protected boolean canInsert() {
                return false;
            }

            @Override
            protected boolean requestInsert(String url) {
                return false;
            }

            @Override
            protected boolean contentProviderEnabled() {
                return false;
            }

            @Override
            protected String[] getErrorColumnNames() {
                return null;
            }
        });
        mContentProvider.onCreate();
        ShadowContentResolver.registerProviderInternal(AUTHORITY, mContentProvider);
        ProviderInfo info = new ProviderInfo();
        info.authority = AUTHORITY;
        mContentProvider.attachInfo(null, info);
        mContentResolver = RuntimeEnvironment.application.getContentResolver();
        mUri = new Uri.Builder()
                       .scheme(ContentResolver.SCHEME_CONTENT)
                       .authority(AUTHORITY)
                       .build();
    }

    @Test
    public void testQuery() {
        when(mContentProvider.contentProviderEnabled()).thenReturn(false);
        assertThat(mContentResolver.query(mUri.buildUpon().appendPath("authorized").build(), null,
                           "url = 'dummy'", null, null),
                is(nullValue()));
        when(mContentProvider.contentProviderEnabled()).thenReturn(true);
        int errorInt[] = {42};
        String errorString[] = {"Error Message"};
        WebRestrictionsResult result = new WebRestrictionsResult(false, errorInt, errorString);
        when(mContentProvider.shouldProceed(ArgumentMatchers.<String>isNull(), anyString()))
                .thenReturn(result);
        Cursor cursor = mContentResolver.query(mUri.buildUpon().appendPath("authorized").build(),
                null, "url = 'dummy'", null, null);
        verify(mContentProvider).shouldProceed(null, "dummy");
        assertThat(cursor, is(not(nullValue())));
        assertThat(cursor.getInt(0), is(WebRestrictionsContentProvider.BLOCKED));
        assertThat(cursor.getInt(1), is(42));
        assertThat(cursor.getString(2), is("Error Message"));
        result = new WebRestrictionsResult(true, null, null);
        when(mContentProvider.shouldProceed(ArgumentMatchers.<String>isNull(), anyString()))
                .thenReturn(result);
        cursor = mContentResolver.query(mUri.buildUpon().appendPath("authorized").build(), null,
                "url = 'dummy'", null, null);
        assertThat(cursor, is(not(nullValue())));
        assertThat(cursor.getInt(0), is(WebRestrictionsContentProvider.PROCEED));
    }

    @Test
    public void testInsert() {
        ContentValues values = new ContentValues();
        values.put("url", "dummy2");
        when(mContentProvider.contentProviderEnabled()).thenReturn(true);
        when(mContentProvider.requestInsert(anyString())).thenReturn(false);
        assertThat(
                mContentResolver.insert(mUri.buildUpon().appendPath("requested").build(), values),
                is(nullValue()));
        verify(mContentProvider).requestInsert("dummy2");
        values.put("url", "dummy3");
        when(mContentProvider.requestInsert(anyString())).thenReturn(true);
        assertThat(
                mContentResolver.insert(mUri.buildUpon().appendPath("requested").build(), values),
                is(not(nullValue())));
        verify(mContentProvider).requestInsert("dummy3");
        when(mContentProvider.contentProviderEnabled()).thenReturn(false);
        assertThat(
                mContentResolver.insert(mUri.buildUpon().appendPath("requested").build(), values),
                is(nullValue()));
    }

    @Test
    public void testGetType() {
        when(mContentProvider.contentProviderEnabled()).thenReturn(true);
        assertThat(mContentResolver.getType(mUri.buildUpon().appendPath("junk").build()),
                is(nullValue()));
        when(mContentProvider.canInsert()).thenReturn(false);
        assertThat(mContentResolver.getType(mUri.buildUpon().appendPath("requested").build()),
                is(nullValue()));
        when(mContentProvider.canInsert()).thenReturn(true);
        assertThat(mContentResolver.getType(mUri.buildUpon().appendPath("requested").build()),
                is(not(nullValue())));
        when(mContentProvider.contentProviderEnabled()).thenReturn(false);
        assertThat(mContentResolver.getType(mUri.buildUpon().appendPath("junk").build()),
                is(nullValue()));
    }
}
