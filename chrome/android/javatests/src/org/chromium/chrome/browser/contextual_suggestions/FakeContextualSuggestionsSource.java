// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextual_suggestions;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.util.Pair;

import org.chromium.base.Callback;
import org.chromium.base.test.util.UrlUtils;
import org.chromium.chrome.browser.contextual_suggestions.ContextualSuggestionsBridge.ContextualSuggestionsResult;
import org.chromium.chrome.browser.ntp.snippets.KnownCategories;
import org.chromium.chrome.browser.ntp.snippets.SnippetArticle;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.content_public.browser.WebContents;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * A fake {@link ContextualSuggestionsSource} for use in testing.
 */
public class FakeContextualSuggestionsSource extends ContextualSuggestionsSource {
    final static String TEST_TOOLBAR_TITLE = "More about capybaras";
    // There should be 6 items in the cluster list - 5 articles and one cluster title.
    final static Integer TOTAL_ITEM_COUNT = 6;
    final static float TEST_PEEK_TARGET_PERCENTAGE = .5f;
    final static int TEST_PEEK_COUNT = 3;
    final static int TEST_PEEK_DELAY_SECONDS = 2;

    private final Map<String, Bitmap> mSuggestionBitmaps = new HashMap<>();
    private final List<Pair<SnippetArticle, Callback<Bitmap>>> mPendingImageRequests =
            new ArrayList<>();

    FakeContextualSuggestionsSource() {
        super(null);

        Bitmap capybaraBitmap = BitmapFactory.decodeFile(
                UrlUtils.getIsolatedTestFilePath("chrome/test/data/android/capybara.jpg"));
        Bitmap watchBitmap = BitmapFactory.decodeFile(
                UrlUtils.getIsolatedTestFilePath("chrome/test/data/android/watch.jpg"));
        mSuggestionBitmaps.put("id1", capybaraBitmap);
        mSuggestionBitmaps.put("id3", capybaraBitmap);
        mSuggestionBitmaps.put("id4", watchBitmap);
    }

    @Override
    protected void init(Profile profile) {
        // Intentionally do nothing.
    }

    @Override
    public void destroy() {
        // Intentionally do nothing.
    }

    @Override
    public void fetchSuggestionImage(SnippetArticle suggestion, Callback<Bitmap> callback) {}

    @Override
    public void fetchContextualSuggestionImage(
            SnippetArticle suggestion, Callback<Bitmap> callback) {
        mPendingImageRequests.add(new Pair<>(suggestion, callback));
    }

    @Override
    public void fetchSuggestionFavicon(SnippetArticle suggestion, int minimumSizePx,
            int desiredSizePx, Callback<Bitmap> callback) {}

    @Override
    void fetchSuggestions(String url, Callback<ContextualSuggestionsResult> callback) {
        callback.onResult(createDummyResults());
    }

    @Override
    void reportEvent(WebContents webContents, @ContextualSuggestionsEvent int eventId) {}

    @Override
    void clearState() {}

    void runImageFetchCallbacks() {
        for (Pair<SnippetArticle, Callback<Bitmap>> pair : mPendingImageRequests) {
            String id = pair.first.mIdWithinCategory;
            if (mSuggestionBitmaps.containsKey(id)) {
                pair.second.onResult(mSuggestionBitmaps.get(id));
            }
        }
        mPendingImageRequests.clear();
    }

    private static ContextualSuggestionsResult createDummyResults() {
        SnippetArticle suggestion1 = new SnippetArticle(KnownCategories.CONTEXTUAL, "id1",
                "Capybaras also love hats",
                "Lorem ipsum dolor sit amet, consectetur adipiscing "
                        + "elit. Nulla lacus tortor, aliquam sed tempor at, consectetur sit amet "
                        + "mauris. Nunc ornare vulputate erat, eget tempus magna ultricies a.",
                "Capybara Central", "https://site.com/url1", 0, 0, 0, false, null, true);
        SnippetArticle suggestion2 = new SnippetArticle(KnownCategories.CONTEXTUAL, "id2",
                "All you ever wanted to know about the capybara and its impeccable taste in "
                        + "high fashion watches, hats, and other various accessories",
                "Sed tincidunt ex et quam mollis vestibulum. In lobortis eget massa sed tincidunt. "
                        + "Aliquam augue erat, tempus at consectetur ac, sagittis pellentesque "
                        + "purus. Morbi aliquet nisi sed felis auctor, at bibendum leo sodales. "
                        + "Cras in felis a dui ultricies efficitur sed vitae magna.",
                "All About Capybaras and Fashion", "https://site.com/url2", 0, 0, 0, false, null,
                false);
        SnippetArticle suggestion3 =
                new SnippetArticle(KnownCategories.CONTEXTUAL, "id3", "Capybaras don't like ties",
                        "Pellentesque nec lorem nec velit convallis suscipit "
                                + "non eget nunc.",
                        "Breaking Capybara News Updates Delivered Daily", "https://site.com/url3",
                        0, 0, 0, false, null, true);
        SnippetArticle article4 =
                new SnippetArticle(KnownCategories.CONTEXTUAL, "id4", "Fancy watches",
                        "Duis egestas est vitae eros consectetur vulputate. Integer "
                                + "tincidunt condimentum sapien, vel volutpat lorem.",
                        "Watch Shop", "https://site.com/url4", 0, 0, 0, false, null, true);
        SnippetArticle article5 =
                new SnippetArticle(KnownCategories.CONTEXTUAL, "id5", "Less fancy watches",
                        "Donec in luctus purus. Cras scelerisque urna ac sem congue "
                                + "dictum.",
                        "Watch Wholesalers", "https://site.com/url5", 0, 0, 0, false, null, false);

        ContextualSuggestionsCluster cluster1 = new ContextualSuggestionsCluster("");
        cluster1.getSuggestions().add(suggestion1);
        cluster1.getSuggestions().add(suggestion2);
        cluster1.getSuggestions().add(suggestion3);

        ContextualSuggestionsCluster cluster2 =
                new ContextualSuggestionsCluster("More about watches");
        cluster2.getSuggestions().add(article4);
        cluster2.getSuggestions().add(article5);

        ContextualSuggestionsResult result = new ContextualSuggestionsResult(TEST_TOOLBAR_TITLE);
        result.setPeekConditions(new PeekConditions(
                TEST_PEEK_TARGET_PERCENTAGE, TEST_PEEK_DELAY_SECONDS, TEST_PEEK_COUNT));
        result.getClusters().add(cluster1);
        result.getClusters().add(cluster2);

        return result;
    }
}
