// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.webrestrictions.browser;

import static org.hamcrest.CoreMatchers.is;
import static org.hamcrest.CoreMatchers.nullValue;
import static org.junit.Assert.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.isNull;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.ContentProvider;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.database.Cursor;
import android.net.Uri;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.android.controller.ContentProviderController;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;

/**
 * Tests of WebRestrictionsClient.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class WebRestrictionsClientTest {
    private static final String TEST_CONTENT_PROVIDER = "example.com";
    private WebRestrictionsClient mTestClient;
    ContentProvider mProvider;

    @Before
    public void setUp() {
        mTestClient = Mockito.spy(new WebRestrictionsClient());
        Mockito.doNothing().when(mTestClient).nativeOnWebRestrictionsChanged(anyLong());
        mProvider = Mockito.mock(ContentProvider.class);

        mTestClient.init(TEST_CONTENT_PROVIDER, 1234L);
    }

    @Test
    public void testSupportsRequest() {
        assertThat(mTestClient.supportsRequest(), is(false));

        ContentProviderController.of(mProvider).create(TEST_CONTENT_PROVIDER);

        when(mProvider.getType(any(Uri.class))).thenReturn(null);
        assertThat(mTestClient.supportsRequest(), is(false));

        when(mProvider.getType(any(Uri.class))).thenReturn("dummy");
        assertThat(mTestClient.supportsRequest(), is(true));
    }

    @Test
    public void testShouldProceed() {
        ContentProviderController.of(mProvider).create(TEST_CONTENT_PROVIDER);

        when(mProvider.query(any(Uri.class), (String[]) isNull(), anyString(), (String[]) isNull(),
                (String) isNull())).thenReturn(null);
        WebRestrictionsClientResult result = mTestClient.shouldProceed("http://example.com");
        verify(mProvider).query(Uri.parse("content://" + TEST_CONTENT_PROVIDER + "/authorized"),
                null, "url = 'http://example.com'", null, null);
        assertThat(result.shouldProceed(), is(false));
        assertThat(result.getString(2), is(nullValue()));

        Cursor cursor = Mockito.mock(Cursor.class);
        when(cursor.getInt(0)).thenReturn(1);
        when(cursor.getInt(1)).thenReturn(42);
        when(cursor.getString(2)).thenReturn("No error");
        when(cursor.getColumnName(1)).thenReturn("Error Int");
        when(cursor.getColumnName(2)).thenReturn("Error String");
        when(cursor.getColumnCount()).thenReturn(3);
        when(mProvider.query(any(Uri.class), (String[]) isNull(), anyString(),
                (String[]) isNull(), (String) isNull())).thenReturn(cursor);
        result = mTestClient.shouldProceed("http://example.com");
        assertThat(result.shouldProceed(), is(true));
        assertThat(result.getInt(1), is(42));
        assertThat(result.getString(2), is("No error"));

        when(cursor.getInt(0)).thenReturn(0);
        when(mProvider.query(any(Uri.class), (String[]) isNull(), anyString(),
                (String[]) isNull(), (String) isNull())).thenReturn(cursor);
        result = mTestClient.shouldProceed("http://example.com");
        assertThat(result.shouldProceed(), is(false));
    }

    @Test
    public void testRequestPermission() {
        ContentProviderController.of(mProvider).create(TEST_CONTENT_PROVIDER);

        ContentValues expectedValues = new ContentValues();
        expectedValues.put("url", "http://example.com");
        when(mProvider.insert(any(Uri.class), any(ContentValues.class))).thenReturn(null);
        assertThat(mTestClient.requestPermission("http://example.com"), is(false));
        verify(mProvider).insert(
                Uri.parse("content://" + TEST_CONTENT_PROVIDER + "/requested"), expectedValues);

        when(mProvider.insert(any(Uri.class), any(ContentValues.class)))
                .thenReturn(Uri.parse("content://example.com/xxx"));
        assertThat(mTestClient.requestPermission("http://example.com"), is(true));
    }

    @Test
    public void testNotifyChange() {
        ContentProviderController.of(mProvider).create(TEST_CONTENT_PROVIDER);

        ContentResolver resolver = RuntimeEnvironment.application.getContentResolver();
        resolver.notifyChange(Uri.parse("content://" + TEST_CONTENT_PROVIDER), null);
        verify(mTestClient).nativeOnWebRestrictionsChanged(1234L);
    }
}
