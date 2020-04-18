// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextual_suggestions;

import android.support.annotation.Nullable;
import android.view.View.OnClickListener;

import org.chromium.chrome.browser.modelutil.ListObservable;
import org.chromium.chrome.browser.modelutil.PropertyObservable;
import org.chromium.chrome.browser.ntp.cards.NewTabPageViewHolder;
import org.chromium.chrome.browser.ntp.cards.NodeParent;
import org.chromium.chrome.browser.ntp.cards.TreeNode;
import org.chromium.chrome.browser.widget.ListMenuButton;

import java.util.Collections;

/** A model for the contextual suggestions UI component. */
class ContextualSuggestionsModel
        extends PropertyObservable<ContextualSuggestionsModel.PropertyKey> {
    /** Keys uniquely identifying model properties. */
    static class PropertyKey {
        static final PropertyKey CLOSE_BUTTON_ON_CLICK_LISTENER = new PropertyKey();
        static final PropertyKey MENU_BUTTON_VISIBILITY = new PropertyKey();
        static final PropertyKey MENU_BUTTON_ALPHA = new PropertyKey();
        static final PropertyKey MENU_BUTTON_DELEGATE = new PropertyKey();
        static final PropertyKey TITLE = new PropertyKey();
        static final PropertyKey TOOLBAR_SHADOW_VISIBILITY = new PropertyKey();
        static final PropertyKey DEFAULT_TOOLBAR_ON_CLICK_LISTENER = new PropertyKey();
        static final PropertyKey SLIM_PEEK_ENABLED = new PropertyKey();
        static final PropertyKey TOOLBAR_TRANSLATION_PERCENT = new PropertyKey();
        static final PropertyKey TOOLBAR_ARROW_TINT_RESOURCE_ID = new PropertyKey();

        private PropertyKey() {}
    }

    /** A {@link ListObservable} containing the current cluster list. */
    class ClusterListObservable extends ListObservable implements NodeParent {
        ClusterList mClusterList = new ClusterList(Collections.emptyList());

        /** Constructor to initialize parent of cluster list. */
        ClusterListObservable() {
            mClusterList.setParent(this);
        }

        private void setClusterList(ClusterList clusterList) {
            assert clusterList != null;

            // Destroy the old cluster list.
            mClusterList.destroy();

            mClusterList = clusterList;
            mClusterList.setParent(this);

            if (getItemCount() != 0) notifyItemRangeInserted(0, getItemCount());
        }

        @Override
        public int getItemCount() {
            return mClusterList.getItemCount();
        }

        // NodeParent implementations.
        @Override
        public void onItemRangeChanged(TreeNode child, int index, int count,
                @Nullable NewTabPageViewHolder.PartialBindCallback callback) {
            assert child == mClusterList;
            notifyItemRangeChanged(index, count, callback);
        }

        @Override
        public void onItemRangeInserted(TreeNode child, int index, int count) {
            assert child == mClusterList;
            notifyItemRangeInserted(index, count);
        }

        @Override
        public void onItemRangeRemoved(TreeNode child, int index, int count) {
            assert child == mClusterList;
            notifyItemRangeRemoved(index, count);
        }
    }

    ClusterListObservable mClusterListObservable = new ClusterListObservable();
    private OnClickListener mCloseButtonOnClickListener;
    private boolean mMenuButtonVisibility;
    private float mMenuButtonAlpha = 1.f;
    private ListMenuButton.Delegate mMenuButtonDelegate;
    private OnClickListener mDefaultToolbarOnClickListener;
    private String mTitle;
    private boolean mToolbarShadowVisibility;
    private boolean mIsSlimPeekEnabled;
    private float mToolbarTranslationPercent;
    private int mToolbarArrowTintResourceId;

    /** @param clusterList The current list of clusters. */
    void setClusterList(ClusterList clusterList) {
        mClusterListObservable.setClusterList(clusterList);
    }

    /** @return The current list of clusters. */
    ClusterList getClusterList() {
        return mClusterListObservable.mClusterList;
    }

    /** @param listener The {@link OnClickListener} for the close button. */
    void setCloseButtonOnClickListener(OnClickListener listener) {
        mCloseButtonOnClickListener = listener;
        notifyPropertyChanged(PropertyKey.CLOSE_BUTTON_ON_CLICK_LISTENER);
    }

    /** @return The {@link OnClickListener} for the close button. */
    OnClickListener getCloseButtonOnClickListener() {
        return mCloseButtonOnClickListener;
    }

    /** @param visible Whether the menu button is visible. */
    void setMenuButtonVisibility(boolean visible) {
        mMenuButtonVisibility = visible;
        notifyPropertyChanged(PropertyKey.MENU_BUTTON_VISIBILITY);
    }

    /** @return Whether the menu button is visible. */
    boolean getMenuButtonVisibility() {
        return mMenuButtonVisibility;
    }

    /** @param alpha The opacity of the menu button. */
    void setMenuButtonAlpha(float alpha) {
        mMenuButtonAlpha = alpha;
        notifyPropertyChanged(PropertyKey.MENU_BUTTON_ALPHA);
    }

    /** @return The opacity of the menu button. */
    float getMenuButtonAlpha() {
        return mMenuButtonAlpha;
    }

    /** @param delegate The delegate for handles actions for the menu. */
    void setMenuButtonDelegate(ListMenuButton.Delegate delegate) {
        mMenuButtonDelegate = delegate;
        notifyPropertyChanged(PropertyKey.MENU_BUTTON_DELEGATE);
    }

    /** @return The delegate that handles actions for the menu. */
    ListMenuButton.Delegate getMenuButtonDelegate() {
        return mMenuButtonDelegate;
    }

    /** @param title The title to display in the toolbar. */
    void setTitle(String title) {
        mTitle = title;
        notifyPropertyChanged(PropertyKey.TITLE);
    }

    /** @return title The title to display in the toolbar. */
    String getTitle() {
        return mTitle;
    }

    /**
     * @return Whether there are any suggestions to be shown.
     */
    boolean hasSuggestions() {
        return getClusterList().getItemCount() > 0;
    }

    /** @param visible Whether the toolbar shadow should be visible. */
    void setToolbarShadowVisibility(boolean visible) {
        mToolbarShadowVisibility = visible;
        notifyPropertyChanged(PropertyKey.TOOLBAR_SHADOW_VISIBILITY);
    }

    /** @return Whether the toolbar shadow should be visible. */
    boolean getToolbarShadowVisibility() {
        return mToolbarShadowVisibility;
    }

    /**
     * @param listener The default toolbar {@link OnClickListener}.
     */
    void setDefaultToolbarClickListener(OnClickListener listener) {
        mDefaultToolbarOnClickListener = listener;
        notifyPropertyChanged(PropertyKey.DEFAULT_TOOLBAR_ON_CLICK_LISTENER);
    }

    /**
     * @return The default toolbar {@link OnClickListener}.
     */
    OnClickListener getDefaultToolbarClickListener() {
        return mDefaultToolbarOnClickListener;
    }

    /** @param enabled Whether the slim peek UI is enabled. */
    void setSlimPeekEnabled(boolean enabled) {
        mIsSlimPeekEnabled = enabled;
        notifyPropertyChanged(PropertyKey.SLIM_PEEK_ENABLED);
    }

    /** @return Whether the slim peek UI is enabled. */
    boolean isSlimPeekEnabled() {
        return mIsSlimPeekEnabled;
    }

    /**
     * @param translation The toolbar translation percent where 1.f means the main toolbar content
     *                    is fully translated and 0.f means it is not translated at all. This is
     *                    used by the slim peek UI to animate from fully translated when the sheet
     *                    is closed to not at all translated when the sheet is opened.
     */
    void setToolbarTranslationPercent(float translation) {
        mToolbarTranslationPercent = translation;
        notifyPropertyChanged(PropertyKey.TOOLBAR_TRANSLATION_PERCENT);
    }

    /**
     * @param return The toolbar translation percent where 1.f means the main toolbar content is
     *               fully translated and 0.f means it is not translated at all. This is used by
     *               the slim peek UI to animate from fully translated when the sheet is closed
     *               to not at all translated when the sheet is opened.
     */
    float getToolbarTranslationPercent() {
        return mToolbarTranslationPercent;
    }

    /**
     * @param resourceId The resource id of the tint for the toolbar arrow.
     */
    void setToolbarArrowTintResourceId(int resourceId) {
        mToolbarArrowTintResourceId = resourceId;
        notifyPropertyChanged(PropertyKey.TOOLBAR_ARROW_TINT_RESOURCE_ID);
    }

    /**
     * @return The resource id of the tint for the toolbar arrow.
     */
    int getToolbarArrowTintResourceId() {
        return mToolbarArrowTintResourceId;
    }
}
