// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.autofill.keyboard_accessory;

import static org.chromium.ui.base.LocalizationUtils.isLayoutRtl;

import android.content.Context;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.support.design.widget.TabLayout;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;
import android.view.accessibility.AccessibilityEvent;
import android.widget.HorizontalScrollView;
import android.widget.LinearLayout;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.autofill.AutofillKeyboardSuggestions;

import javax.annotation.Nullable;

/**
 * The Accessory sitting above the keyboard and below the content area. It is used for autofill
 * suggestions and manual entry points assisting the user in filling forms.
 */
class KeyboardAccessoryView extends LinearLayout {
    private HorizontalScrollView mSuggestionsView;
    private RecyclerView mActionsView;
    private TabLayout mTabLayout;

    private static class HorizontalDividerItemDecoration extends RecyclerView.ItemDecoration {
        private final int mHorizontalMargin;
        HorizontalDividerItemDecoration(int horizontalMargin) {
            this.mHorizontalMargin = horizontalMargin;
        }
        @Override
        public void getItemOffsets(
                Rect outRect, View view, RecyclerView parent, RecyclerView.State state) {
            outRect.right = mHorizontalMargin;
        }
    }

    /**
     * Constructor for inflating from XML.
     */
    public KeyboardAccessoryView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        sendAccessibilityEvent(AccessibilityEvent.TYPE_WINDOW_STATE_CHANGED);

        mActionsView = findViewById(R.id.actions_view);
        initializeHorizontalRecyclerView(mActionsView);

        mTabLayout = findViewById(R.id.tabs);

        mSuggestionsView = findViewById(R.id.suggestions_view);

        // Apply RTL layout changes to the views childen:
        ApiCompatibilityUtils.setLayoutDirection(mSuggestionsView,
                isLayoutRtl() ? View.LAYOUT_DIRECTION_RTL : View.LAYOUT_DIRECTION_LTR);
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        super.onLayout(changed, left, top, right, bottom);
        // When the size changes, the scrolling should be reset.
        mSuggestionsView.fullScroll(isLayoutRtl() ? FOCUS_RIGHT : FOCUS_LEFT);
    }

    void setVisible(boolean visible) {
        if (visible) {
            show();
        } else {
            hide();
        }
    }

    void setActionsAdapter(RecyclerView.Adapter adapter) {
        mActionsView.setAdapter(adapter);
    }

    /**
     * Creates a new tab and appends it to the end of the tab layout at the start of the bar.
     * @param icon The icon to be displayed in the tab bar.
     * @param contentDescription The contentDescription to be used for the tab icon.
     */
    void addTabAt(int position, Drawable icon, CharSequence contentDescription) {
        TabLayout.Tab tab = mTabLayout.newTab();
        tab.setIcon(icon); // TODO(fhorschig): Call .mutate() when changing the active tint.
        tab.setContentDescription(contentDescription);
        mTabLayout.addTab(tab, position);
    }

    void removeTabAt(int position) {
        mTabLayout.removeTabAt(position);
    }

    /**
     * Removes all tabs.
     */
    void clearTabs() {
        mTabLayout.removeAllTabs();
    }

    // TODO(crbug/722897): Check to handle RTL.
    // TODO(fhorschig): This should use a RecyclerView. The model should contain single suggestions.
    /**
     * Shows the given suggestions. If set to null, it only removes existing suggestions.
     * @param suggestions Autofill suggestion data.
     */
    void updateSuggestions(@Nullable AutofillKeyboardSuggestions suggestions) {
        mSuggestionsView.removeAllViews();
        if (suggestions == null) return;
        mSuggestionsView.addView(suggestions);
    }

    private void show() {
        setVisibility(View.VISIBLE);
        announceForAccessibility(((ViewGroup) getParent()).getContentDescription());
    }

    private void hide() {
        setVisibility(View.GONE);
    }

    private void initializeHorizontalRecyclerView(RecyclerView recyclerView) {
        // Set horizontal layout.
        recyclerView.setLayoutManager(
                new LinearLayoutManager(getContext(), LinearLayoutManager.HORIZONTAL, false));

        // Create margins between every element.
        recyclerView.addItemDecoration(new HorizontalDividerItemDecoration(
                getResources().getDimensionPixelSize(R.dimen.keyboard_accessory_padding)));

        // Remove all animations - the accessory shouldn't be visibly built anyway.
        recyclerView.setItemAnimator(null);

        int pad = getResources().getDimensionPixelSize(R.dimen.keyboard_accessory_padding);
        int halfPad = getResources().getDimensionPixelSize(R.dimen.keyboard_accessory_half_padding);
        recyclerView.setPadding(pad, halfPad, pad, halfPad);
    }
}
