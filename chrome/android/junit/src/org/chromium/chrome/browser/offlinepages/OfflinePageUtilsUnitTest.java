// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.offlinepages;

import static org.junit.Assert.assertEquals;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.ArgumentMatchers.isNull;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.os.Environment;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.shadows.multidex.ShadowMultiDex;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.components.bookmarks.BookmarkId;
import org.chromium.components.bookmarks.BookmarkType;
import org.chromium.content_public.browser.WebContents;

import java.io.File;

/**
 * Unit tests for OfflinePageUtils.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE,
        shadows = {OfflinePageUtilsUnitTest.WrappedEnvironment.class, ShadowMultiDex.class})
public class OfflinePageUtilsUnitTest {
    @Mock
    private File mMockDataDirectory;
    @Mock
    private Tab mTab;
    @Mock
    private WebContents mWebContents;
    @Mock
    private OfflinePageBridge mOfflinePageBridge;
    @Mock
    private OfflinePageUtils.Internal mOfflinePageUtils;

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        WrappedEnvironment.setDataDirectoryForTest(mMockDataDirectory);

        // Setting up a mock tab. These are the values common to most tests, but individual
        // tests might easily overwrite them.
        doReturn(false).when(mTab).isShowingErrorPage();
        doReturn(false).when(mTab).isShowingSadTab();
        doReturn(mWebContents).when(mTab).getWebContents();
        doReturn(false).when(mWebContents).isDestroyed();
        doReturn(false).when(mWebContents).isIncognito();

        doNothing()
                .when(mOfflinePageBridge)
                .savePage(eq(mWebContents), any(ClientId.class),
                        any(OfflinePageBridge.SavePageCallback.class));

        doReturn(mOfflinePageBridge)
                .when(mOfflinePageUtils)
                .getOfflinePageBridge((Profile) isNull());
        OfflinePageUtils.setInstanceForTesting(mOfflinePageUtils);

        // Setting the default value is required because unit tests don't load native code needed
        // to normally call the respective getter.
        OfflinePageBridge.setOfflineBookmarksEnabledForTesting(true);
    }

    @Test
    @Feature({"OfflinePages"})
    public void testGetFreeSpaceInBytes() {
        when(mMockDataDirectory.getUsableSpace()).thenReturn(1234L);
        assertEquals(1234L, OfflinePageUtils.getFreeSpaceInBytes());
    }

    @Test
    @Feature({"OfflinePages"})
    public void testGetTotalSpaceInBytes() {
        when(mMockDataDirectory.getTotalSpace()).thenReturn(56789L);
        assertEquals(56789L, OfflinePageUtils.getTotalSpaceInBytes());
    }

    @Test
    @Feature({"OfflinePages"})
    public void testStripSchemeFromOnlineUrl() {
        // Only scheme gets stripped.
        assertEquals("cs.chromium.org",
                OfflinePageUtils.stripSchemeFromOnlineUrl("https://cs.chromium.org"));
        assertEquals("cs.chromium.org",
                OfflinePageUtils.stripSchemeFromOnlineUrl("http://cs.chromium.org"));
        // If there is no scheme, nothing changes.
        assertEquals(
                "cs.chromium.org", OfflinePageUtils.stripSchemeFromOnlineUrl("cs.chromium.org"));
        // Path is not touched/changed.
        String urlWithPath = "code.google.com/p/chromium/codesearch#search"
                + "/&q=offlinepageutils&sq=package:chromium&type=cs";
        assertEquals(
                urlWithPath, OfflinePageUtils.stripSchemeFromOnlineUrl("https://" + urlWithPath));
        // Beginning and ending spaces get trimmed.
        assertEquals("cs.chromium.org",
                OfflinePageUtils.stripSchemeFromOnlineUrl("  https://cs.chromium.org  "));
    }

    @Test
    @Feature({"OfflinePages"})
    public void testSaveBookmarkOffline() {
        OfflinePageUtils.saveBookmarkOffline(new BookmarkId(42, BookmarkType.NORMAL), mTab);
        verify(mOfflinePageBridge, times(1))
                .savePage(eq(mWebContents), any(ClientId.class),
                        any(OfflinePageBridge.SavePageCallback.class));
    }

    @Test
    @Feature({"OfflinePages"})
    public void testSaveBookmarkOffline_inputValidation() {
        OfflinePageUtils.saveBookmarkOffline(null, mTab);
        // Save page not called because bookmarkId is null.
        verify(mOfflinePageBridge, times(0))
                .savePage(eq(mWebContents), any(ClientId.class),
                        any(OfflinePageBridge.SavePageCallback.class));

        BookmarkId bookmarkId = new BookmarkId(42, BookmarkType.NORMAL);
        OfflinePageBridge.setOfflineBookmarksEnabledForTesting(false);
        OfflinePageUtils.saveBookmarkOffline(bookmarkId, mTab);
        // Save page not called because offline bookmarks are disabled.
        verify(mOfflinePageBridge, times(0))
                .savePage(eq(mWebContents), any(ClientId.class),
                        any(OfflinePageBridge.SavePageCallback.class));

        OfflinePageBridge.setOfflineBookmarksEnabledForTesting(true);
        doReturn(true).when(mTab).isShowingErrorPage();
        OfflinePageUtils.saveBookmarkOffline(bookmarkId, mTab);
        // Save page not called because tab is showing an error page.
        verify(mOfflinePageBridge, times(0))
                .savePage(eq(mWebContents), any(ClientId.class),
                        any(OfflinePageBridge.SavePageCallback.class));

        doReturn(false).when(mTab).isShowingErrorPage();
        doReturn(true).when(mTab).isShowingSadTab();
        OfflinePageUtils.saveBookmarkOffline(bookmarkId, mTab);
        // Save page not called because tab is showing a sad tab.
        verify(mOfflinePageBridge, times(0))
                .savePage(eq(mWebContents), any(ClientId.class),
                        any(OfflinePageBridge.SavePageCallback.class));

        doReturn(false).when(mTab).isShowingSadTab();
        doReturn(null).when(mTab).getWebContents();
        OfflinePageUtils.saveBookmarkOffline(bookmarkId, mTab);
        // Save page not called because tab returns null web contents.
        verify(mOfflinePageBridge, times(0))
                .savePage(eq(mWebContents), any(ClientId.class),
                        any(OfflinePageBridge.SavePageCallback.class));

        doReturn(mWebContents).when(mTab).getWebContents();
        doReturn(true).when(mWebContents).isDestroyed();
        OfflinePageUtils.saveBookmarkOffline(bookmarkId, mTab);
        // Save page not called because web contents is destroyed.
        verify(mOfflinePageBridge, times(0))
                .savePage(eq(mWebContents), any(ClientId.class),
                        any(OfflinePageBridge.SavePageCallback.class));

        doReturn(false).when(mWebContents).isDestroyed();
        doReturn(true).when(mWebContents).isIncognito();
        OfflinePageUtils.saveBookmarkOffline(bookmarkId, mTab);
        // Save page not called because web contents is incognito.
        verify(mOfflinePageBridge, times(0))
                .savePage(eq(mWebContents), any(ClientId.class),
                        any(OfflinePageBridge.SavePageCallback.class));
    }

    /** A shadow/wrapper of android.os.Environment that allows injecting a test directory. */
    @Implements(Environment.class)
    public static class WrappedEnvironment {
        private static File sDataDirectory = null;

        public static void setDataDirectoryForTest(File testDirectory) {
            sDataDirectory = testDirectory;
        }

        @Implementation
        public static File getDataDirectory() {
            return sDataDirectory;
        }
    }
}
