// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import static org.junit.Assert.assertEquals;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;

import static org.chromium.chrome.test.util.browser.suggestions.ContentSuggestionsTestUtils.createDummySuggestion;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.annotation.Config;

import org.chromium.base.Callback;
import org.chromium.base.DiscardableReferencePool;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.DisableHistogramsRule;
import org.chromium.chrome.browser.NativePageHost;
import org.chromium.chrome.browser.favicon.FaviconHelper;
import org.chromium.chrome.browser.favicon.FaviconHelper.FaviconImageCallback;
import org.chromium.chrome.browser.favicon.LargeIconBridge;
import org.chromium.chrome.browser.favicon.LargeIconBridge.LargeIconCallback;
import org.chromium.chrome.browser.ntp.cards.CardsVariationParameters;
import org.chromium.chrome.browser.ntp.snippets.KnownCategories;
import org.chromium.chrome.browser.ntp.snippets.SnippetArticle;
import org.chromium.chrome.browser.ntp.snippets.SuggestionsSource;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.suggestions.ImageFetcher.DownloadThumbnailRequest;
import org.chromium.chrome.browser.widget.ThumbnailProvider;
import org.chromium.chrome.test.util.browser.suggestions.SuggestionsDependenciesRule;

import java.net.URI;
import java.util.HashMap;
/**
 * Unit tests for {@link ImageFetcher}.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class ImageFetcherTest {
    public static final int IMAGE_SIZE_PX = 100;
    public static final String URL_STRING = "http://www.test.com";
    @Rule
    public DisableHistogramsRule disableHistogramsRule = new DisableHistogramsRule();

    @Rule
    public SuggestionsDependenciesRule mSuggestionsDeps = new SuggestionsDependenciesRule();

    private DiscardableReferencePool mReferencePool = new DiscardableReferencePool();

    @Mock
    private FaviconHelper mFaviconHelper;
    @Mock
    private ThumbnailProvider mThumbnailProvider;
    @Mock
    private SuggestionsSource mSuggestionsSource;
    @Mock
    private LargeIconBridge mLargeIconBridge;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        CardsVariationParameters.setTestVariationParams(new HashMap<>());

        mSuggestionsDeps.getFactory().largeIconBridge = mLargeIconBridge;
        mSuggestionsDeps.getFactory().thumbnailProvider = mThumbnailProvider;
        mSuggestionsDeps.getFactory().faviconHelper = mFaviconHelper;
        mSuggestionsDeps.getFactory().suggestionsSource = mSuggestionsSource;
    }

    @SuppressWarnings("unchecked")
    @Test
    public void testFaviconFetch() {
        ImageFetcher imageFetcher = new ImageFetcher(mSuggestionsSource, mock(Profile.class),
                mReferencePool, mock(NativePageHost.class));

        SnippetArticle suggestion = createDummySuggestion(KnownCategories.BOOKMARKS);
        imageFetcher.makeFaviconRequest(suggestion, IMAGE_SIZE_PX, mock(Callback.class));

        String expectedFaviconDomain =
                ImageFetcher.getSnippetDomain(URI.create(suggestion.getUrl()));
        verify(mFaviconHelper)
                .getLocalFaviconImageForURL(any(Profile.class), eq(expectedFaviconDomain),
                        eq(IMAGE_SIZE_PX), any(FaviconImageCallback.class));
    }

    @Test
    public void testDownloadThumbnailFetch() {
        ImageFetcher imageFetcher = new ImageFetcher(mSuggestionsSource, mock(Profile.class),
                mReferencePool, mock(NativePageHost.class));

        SnippetArticle suggestion = createDummySuggestion(KnownCategories.DOWNLOADS);

        DownloadThumbnailRequest request =
                imageFetcher.makeDownloadThumbnailRequest(suggestion, IMAGE_SIZE_PX);
        verify(mThumbnailProvider).getThumbnail(eq(request));

        request.cancel();
        verify(mThumbnailProvider).cancelRetrieval(eq(request));
    }

    @SuppressWarnings("unchecked")
    @Test
    public void testArticleThumbnailFetch() {
        ImageFetcher imageFetcher = new ImageFetcher(mSuggestionsSource, mock(Profile.class),
                mReferencePool, mock(NativePageHost.class));

        SnippetArticle suggestion = createDummySuggestion(KnownCategories.ARTICLES);
        imageFetcher.makeArticleThumbnailRequest(suggestion, mock(Callback.class));

        verify(mSuggestionsSource).fetchSuggestionImage(eq(suggestion), any(Callback.class));
    }

    @Test
    public void testLargeIconFetch() {
        ImageFetcher imageFetcher = new ImageFetcher(mSuggestionsSource, mock(Profile.class),
                mReferencePool, mock(NativePageHost.class));

        imageFetcher.makeLargeIconRequest(URL_STRING, IMAGE_SIZE_PX, mock(LargeIconCallback.class));

        String expectedIconDomain = ImageFetcher.getSnippetDomain(URI.create(URL_STRING));
        verify(mLargeIconBridge)
                .getLargeIconForUrl(
                        eq(expectedIconDomain), eq(IMAGE_SIZE_PX), any(LargeIconCallback.class));
    }

    @Test
    public void testSnippetDomainExtraction() {
        URI uri = URI.create(URL_STRING + "/test");

        String expected = URL_STRING;
        String actual = ImageFetcher.getSnippetDomain(uri);

        assertEquals(expected, actual);
    }
}
