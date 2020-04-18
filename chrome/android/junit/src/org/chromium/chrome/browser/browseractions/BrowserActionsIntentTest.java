// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.browseractions;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.when;

import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.support.annotation.DrawableRes;
import android.support.customtabs.browseractions.BrowserActionItem;
import android.support.customtabs.browseractions.BrowserActionsIntent;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Feature;

import java.util.ArrayList;
import java.util.List;

/**
 * Unit tests for BrowserActionsIntent.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BrowserActionsIntentTest {
    private static final String HTTP_SCHEME_TEST_URL = "http://www.example.com";
    private static final String HTTPS_SCHEME_TEST_URL = "https://www.example.com";
    private static final String CHROME_SCHEME_TEST_URL = "chrome://example";
    private static final String CONTENT_SCHEME_TEST_URL = "content://example";
    private static final String SENDER_PACKAGE_NAME = "some.other.app.package.sender_name";
    private static final String RECEIVER_PACKAGE_NAME = "some.other.app.package.receiver_name";
    private static final String CUSTOM_ITEM_WITHOUT_ICON_TITLE = "Custom item without icon";
    private static final String CUSTOM_ITEM_WITH_ICON_TITLE = "Custom item with icon";
    @DrawableRes
    private static final int CUSTOM_ITEM_WITH_ICON_ICONID = 1;

    private Context mContext;
    @Mock
    private BrowserActionActivity mActivity;
    @Mock
    private PendingIntent mPendingIntent;

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        mContext = RuntimeEnvironment.application;
        mActivity = Mockito.mock(BrowserActionActivity.class);
        when(mActivity.getPackageName()).thenReturn(RECEIVER_PACKAGE_NAME);
        when(mActivity.isStartedUpCorrectly(any(Intent.class))).thenCallRealMethod();
        when(mPendingIntent.getCreatorPackage()).thenReturn(SENDER_PACKAGE_NAME);
    }

    @Test
    @Feature({"BrowserActions"})
    public void testStartedUpCorrectly() {
        assertFalse(mActivity.isStartedUpCorrectly(null));
        assertFalse(mActivity.isStartedUpCorrectly(new Intent()));

        Intent mIntent = createBaseBrowserActionsIntent(HTTP_SCHEME_TEST_URL);
        assertTrue(mActivity.isStartedUpCorrectly(mIntent));

        mIntent = createBaseBrowserActionsIntent(HTTP_SCHEME_TEST_URL);
        mIntent.removeExtra(BrowserActionsIntent.EXTRA_APP_ID);
        assertFalse(mActivity.isStartedUpCorrectly(mIntent));

        mIntent = createBaseBrowserActionsIntent(HTTPS_SCHEME_TEST_URL);
        assertTrue(mActivity.isStartedUpCorrectly(mIntent));

        mIntent = createBaseBrowserActionsIntent(CHROME_SCHEME_TEST_URL);
        assertFalse(mActivity.isStartedUpCorrectly(mIntent));

        mIntent = createBaseBrowserActionsIntent(CONTENT_SCHEME_TEST_URL);
        assertFalse(mActivity.isStartedUpCorrectly(mIntent));

        mIntent = createBaseBrowserActionsIntent(HTTP_SCHEME_TEST_URL);
        mIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        assertFalse(mActivity.isStartedUpCorrectly(mIntent));

        mIntent = createBaseBrowserActionsIntent(HTTP_SCHEME_TEST_URL);
        mIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_DOCUMENT);
        assertFalse(mActivity.isStartedUpCorrectly(mIntent));

        mIntent = createBrowserActionsIntentWithCustomItems(
                HTTP_SCHEME_TEST_URL, createCustomItems());
        assertTrue(mActivity.isStartedUpCorrectly(mIntent));
        testParseCustomItems(mActivity.mActions);
    }

    /**
     * A convenient method to create a simple Intent for Browser Actions without custom item.
     * @param url The url for the data of the Intent.
     * @return The simple Intent for Browser Actions.
     */
    private Intent createBaseBrowserActionsIntent(String url) {
        return createBrowserActionsIntentWithCustomItems(url, new ArrayList<>());
    }

    /**
     * Creates an Intent for Browser Actions which contains a url and a list of custom items.
     * @param url The url for the data of the Intent.
     * @param items A List of custom items for Browser Actions menu.
     * @return The Intent for Browser Actions.
     */
    private Intent createBrowserActionsIntentWithCustomItems(
            String url, ArrayList<BrowserActionItem> items) {
        return new BrowserActionsIntent.Builder(mContext, Uri.parse(url))
                .setCustomItems(items)
                .build()
                .getIntent()
                .putExtra(BrowserActionsIntent.EXTRA_APP_ID, mPendingIntent);
    }

    private ArrayList<BrowserActionItem> createCustomItems() {
        BrowserActionItem item1 =
                new BrowserActionItem(CUSTOM_ITEM_WITHOUT_ICON_TITLE, mPendingIntent);
        BrowserActionItem item2 = new BrowserActionItem(
                CUSTOM_ITEM_WITH_ICON_TITLE, mPendingIntent, CUSTOM_ITEM_WITH_ICON_ICONID);
        ArrayList<BrowserActionItem> items = new ArrayList<>();
        items.add(item1);
        items.add(item2);
        return items;
    }

    private void testParseCustomItems(List<BrowserActionItem> items) {
        assertEquals(2, mActivity.mActions.size());

        assertEquals(CUSTOM_ITEM_WITHOUT_ICON_TITLE, mActivity.mActions.get(0).getTitle());
        assertEquals(0, mActivity.mActions.get(0).getIconId());
        assertEquals(mPendingIntent, mActivity.mActions.get(0).getAction());

        assertEquals(CUSTOM_ITEM_WITH_ICON_TITLE, mActivity.mActions.get(1).getTitle());
        assertEquals(CUSTOM_ITEM_WITH_ICON_ICONID, mActivity.mActions.get(1).getIconId());
        assertEquals(mPendingIntent, mActivity.mActions.get(1).getAction());
    }
}
