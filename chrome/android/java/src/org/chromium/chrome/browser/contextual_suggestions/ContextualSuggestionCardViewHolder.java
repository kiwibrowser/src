// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextual_suggestions;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.ntp.ContextMenuManager;
import org.chromium.chrome.browser.ntp.snippets.SnippetArticleViewHolder;
import org.chromium.chrome.browser.offlinepages.OfflinePageBridge;
import org.chromium.chrome.browser.suggestions.SuggestionsBinder;
import org.chromium.chrome.browser.suggestions.SuggestionsRecyclerView;
import org.chromium.chrome.browser.suggestions.SuggestionsUiDelegate;
import org.chromium.chrome.browser.widget.displaystyle.UiConfig;

/** Holder for a contextual suggestions card. **/
public class ContextualSuggestionCardViewHolder extends SnippetArticleViewHolder {
    ContextualSuggestionCardViewHolder(SuggestionsRecyclerView parent,
            ContextMenuManager contextMenuManager, SuggestionsUiDelegate uiDelegate,
            UiConfig uiConfig, OfflinePageBridge offlinePageBridge) {
        super(parent, contextMenuManager, uiDelegate, uiConfig, offlinePageBridge,
                R.layout.contextual_suggestions_card_modern);
    }

    @Override
    public boolean isItemSupported(@ContextMenuManager.ContextMenuItemId int menuItemId) {
        return menuItemId != ContextMenuManager.ID_LEARN_MORE && super.isItemSupported(menuItemId);
    }

    @Override
    public boolean isDismissable() {
        return false;
    }

    @Override
    protected SuggestionsBinder createBinder(SuggestionsUiDelegate uiDelegate) {
        return new SuggestionsBinder(itemView, uiDelegate, true);
    }
}
