// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp.cards;

import org.chromium.chrome.browser.ntp.snippets.SnippetArticle;
import org.chromium.chrome.browser.suggestions.ContentSuggestionsAdditionalAction;

/**
 * Allows implementing a visitor pattern to iterate over all items under a (sub-)tree.
 */
public class NodeVisitor {
    /**
     * Visits the above-the-fold item.
     */
    public void visitAboveTheFoldItem() {}

    /**
     * Visits an action item.
     * @param currentAction The action enum value for the item.
     */
    public void visitActionItem(@ContentSuggestionsAdditionalAction int currentAction) {}

    /**
     * Visits the "all dimissed" item.
     */
    public void visitAllDismissedItem() {}

    /**
     * Visits the footer.
     */
    public void visitFooter() {}

    /**
     * Visits a progress item.
     */
    public void visitProgressItem() {}

    /**
     * Visits a sign-in promo.
     */
    public void visitSignInPromo() {}

    /**
     * Visits a "no suggestions" item.
     */
    public void visitNoSuggestionsItem() {}

    /**
     * Visits a suggestion.
     * @param suggestion The {@link SnippetArticle} represented by the item.
     */
    public void visitSuggestion(SnippetArticle suggestion) {}

    /**
     * Visits a header.
     */
    public void visitHeader() {}

    /**
     * Visits the tile grid.
     */
    public void visitTileGrid() {}

    /**
     * Visits a logo.
     */
    public void visitLogo() {}
}
