// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextual_suggestions;

import android.support.annotation.NonNull;
import android.text.TextUtils;

import org.chromium.base.Callback;
import org.chromium.chrome.browser.ntp.cards.ChildNode;
import org.chromium.chrome.browser.ntp.cards.InnerNode;
import org.chromium.chrome.browser.ntp.cards.ItemViewType;
import org.chromium.chrome.browser.ntp.cards.NewTabPageViewHolder;
import org.chromium.chrome.browser.ntp.cards.NodeVisitor;
import org.chromium.chrome.browser.ntp.cards.SuggestionsCategoryInfo;
import org.chromium.chrome.browser.ntp.snippets.ContentSuggestionsCardLayout;
import org.chromium.chrome.browser.ntp.snippets.KnownCategories;
import org.chromium.chrome.browser.ntp.snippets.SectionHeader;
import org.chromium.chrome.browser.ntp.snippets.SnippetArticle;
import org.chromium.chrome.browser.ntp.snippets.SnippetArticleViewHolder;
import org.chromium.chrome.browser.offlinepages.OfflinePageBridge;
import org.chromium.chrome.browser.offlinepages.OfflinePageItem;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.suggestions.ContentSuggestionsAdditionalAction;
import org.chromium.chrome.browser.suggestions.SuggestionsOfflineModelObserver;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

/** A node in a tree that groups contextual suggestions in a cluster of related items. */
class ContextualSuggestionsCluster extends InnerNode {
    private final String mTitle;
    private final boolean mShouldShowTitle;
    private final List<SnippetArticle> mSuggestions = new ArrayList<>();

    private SectionHeader mHeader;
    private SuggestionsList mSuggestionsList;
    private OfflineModelObserver mOfflineModelObserver;

    /** Creates a new contextual suggestions cluster with provided title. */
    ContextualSuggestionsCluster(String title) {
        mTitle = title;
        mShouldShowTitle = !TextUtils.isEmpty(title);
    }

    /** @return A title related to this cluster */
    String getTitle() {
        return mTitle;
    }

    /** @return A list of suggestions in this cluster */
    List<SnippetArticle> getSuggestions() {
        return mSuggestions;
    }

    /**
     * Called to build the tree node's children. Should be called after all suggestions have been
     * added.
     */
    void buildChildren() {
        if (mShouldShowTitle) {
            mHeader = new SectionHeader(mTitle);
            addChild(mHeader);
        }

        mSuggestionsList = new SuggestionsList();
        mSuggestionsList.addAll(mSuggestions);
        addChild(mSuggestionsList);

        // Only add observer after suggestions have been added to the cluster node to avoid
        // OfflineModelObserver requesting a null list.
        mOfflineModelObserver = new OfflineModelObserver(
                OfflinePageBridge.getForProfile(Profile.getLastUsedProfile().getOriginalProfile()));
        mOfflineModelObserver.updateAllSuggestionsOfflineAvailability(false);
    }

    @Override
    public void detach() {
        super.detach();
        if (mOfflineModelObserver != null) mOfflineModelObserver.onDestroy();
    }

    /** A tree node that holds a list of suggestions. */
    private static class SuggestionsList extends ChildNode implements Iterable<SnippetArticle> {
        private final List<SnippetArticle> mSuggestions = new ArrayList<>();

        private final SuggestionsCategoryInfo mCategoryInfo;

        public SuggestionsList() {
            mCategoryInfo = new SuggestionsCategoryInfo(KnownCategories.CONTEXTUAL, "",
                    ContentSuggestionsCardLayout.FULL_CARD, ContentSuggestionsAdditionalAction.NONE,
                    false, "");
        }

        @Override
        protected int getItemCountForDebugging() {
            return mSuggestions.size();
        }

        @Override
        @ItemViewType
        public int getItemViewType(int position) {
            checkIndex(position);
            return ItemViewType.SNIPPET;
        }

        @Override
        public void onBindViewHolder(NewTabPageViewHolder holder, int position) {
            checkIndex(position);
            SnippetArticle suggestion = getSuggestionAt(position);
            ((ContextualSuggestionCardViewHolder) holder)
                    .onBindViewHolder(suggestion, mCategoryInfo);
        }

        private SnippetArticle getSuggestionAt(int position) {
            return mSuggestions.get(position);
        }

        public void addAll(List<SnippetArticle> suggestions) {
            if (suggestions.isEmpty()) return;

            int insertionPointIndex = mSuggestions.size();
            mSuggestions.addAll(suggestions);
            notifyItemRangeInserted(insertionPointIndex, suggestions.size());
        }

        @Override
        public void visitItems(NodeVisitor visitor) {
            for (SnippetArticle suggestion : mSuggestions) {
                visitor.visitSuggestion(suggestion);
            }
        }

        @Override
        public Set<Integer> getItemDismissalGroup(int position) {
            // Contextual suggestions are not dismissible.
            assert false;

            return null;
        }

        @Override
        public void dismissItem(int position, Callback<String> itemRemovedCallback) {
            // Contextual suggestions are not dismissible.
            assert false;
        }

        @NonNull
        @Override
        public Iterator<SnippetArticle> iterator() {
            return mSuggestions.iterator();
        }

        // TODO(huayinz): Look at a way to share this with SuggestionsSection.
        void updateSuggestionOfflineId(SnippetArticle article, Long newId) {
            int index = mSuggestions.indexOf(article);
            // The suggestions could have been removed / replaced in the meantime.
            if (index == -1) return;

            Long oldId = article.getOfflinePageOfflineId();
            article.setOfflinePageOfflineId(newId);

            if ((oldId == null) == (newId == null)) return;
            notifyItemChanged(index, SnippetArticleViewHolder::refreshOfflineBadgeVisibility);
        }
    }

    /** An observer to offline changes on suggestions. */
    private class OfflineModelObserver extends SuggestionsOfflineModelObserver<SnippetArticle> {
        OfflineModelObserver(OfflinePageBridge bridge) {
            super(bridge);
        }

        @Override
        public void onSuggestionOfflineIdChanged(SnippetArticle suggestion, OfflinePageItem item) {
            mSuggestionsList.updateSuggestionOfflineId(
                    suggestion, item == null ? null : item.getOfflineId());
        }

        @Override
        public Iterable<SnippetArticle> getOfflinableSuggestions() {
            return mSuggestionsList;
        }
    }
}
