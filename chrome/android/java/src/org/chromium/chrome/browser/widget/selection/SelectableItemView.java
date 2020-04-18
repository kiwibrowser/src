// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget.selection;

import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.drawable.Drawable;
import android.support.annotation.Nullable;
import android.support.annotation.VisibleForTesting;
import android.support.graphics.drawable.AnimatedVectorDrawableCompat;
import android.util.AttributeSet;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnLongClickListener;
import android.widget.Checkable;
import android.widget.FrameLayout;
import android.widget.TextView;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.util.FeatureUtilities;
import org.chromium.chrome.browser.widget.TintedDrawable;
import org.chromium.chrome.browser.widget.TintedImageView;
import org.chromium.chrome.browser.widget.selection.SelectionDelegate.SelectionObserver;

import java.util.List;

/**
 * An item that can be selected. When selected, the item will be highlighted. A selection is
 * initially established via long-press. If a selection is already established, clicking on the item
 * will toggle its selection.
 *
 * @param <E> The type of the item associated with this SelectableItemView.
 */
public abstract class SelectableItemView<E> extends FrameLayout implements Checkable,
        OnClickListener, OnLongClickListener, SelectionObserver<E> {
    protected final int mDefaultLevel;
    protected final int mSelectedLevel;
    protected final AnimatedVectorDrawableCompat mCheckDrawable;

    protected TintedImageView mIconView;
    protected TextView mTitleView;
    protected TextView mDescriptionView;
    protected ColorStateList mIconColorList;

    private SelectionDelegate<E> mSelectionDelegate;
    private E mItem;
    private boolean mIsChecked;
    private Drawable mIconDrawable;

    /**
     * Constructor for inflating from XML.
     */
    public SelectableItemView(Context context, AttributeSet attrs) {
        super(context, attrs);
        mIconColorList =
                ApiCompatibilityUtils.getColorStateList(getResources(), R.color.white_mode_tint);
        mDefaultLevel = getResources().getInteger(R.integer.list_item_level_default);
        mSelectedLevel = getResources().getInteger(R.integer.list_item_level_selected);
        mCheckDrawable = AnimatedVectorDrawableCompat.create(
                getContext(), R.drawable.ic_check_googblue_24dp_animated);
    }

    /**
     * Destroys and cleans up itself.
     */
    public void destroy() {
        if (mSelectionDelegate != null) {
            mSelectionDelegate.removeObserver(this);
        }
    }

    /**
     * Sets the SelectionDelegate and registers this object as an observer. The SelectionDelegate
     * must be set before the item can respond to click events.
     * @param delegate The SelectionDelegate that will inform this item of selection changes.
     */
    public void setSelectionDelegate(SelectionDelegate<E> delegate) {
        if (mSelectionDelegate != delegate) {
            if (mSelectionDelegate != null) mSelectionDelegate.removeObserver(this);
            mSelectionDelegate = delegate;
            mSelectionDelegate.addObserver(this);
        }
    }

    /**
     * @param item The item associated with this SelectableItemView.
     */
    public void setItem(E item) {
        mItem = item;
        setChecked(mSelectionDelegate.isItemSelected(item));
    }

    /**
     * @return The item associated with this SelectableItemView.
     */
    public E getItem() {
        return mItem;
    }

    // FrameLayout implementations.
    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        mIconView = findViewById(R.id.icon_view);
        mTitleView = findViewById(R.id.title);
        mDescriptionView = findViewById(R.id.description);

        if (mIconView != null) {
            mIconView.setBackgroundResource(R.drawable.list_item_icon_modern_bg);
            mIconView.setTint(getDefaultIconTint());
            if (!FeatureUtilities.isChromeModernDesignEnabled()) {
                mIconView.getBackground().setAlpha(0);
            }
        }

        setOnClickListener(this);
        setOnLongClickListener(this);
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        if (mSelectionDelegate != null) {
            setChecked(mSelectionDelegate.isItemSelected(mItem));
        }
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        setChecked(false);
    }

    // OnClickListener implementation.
    @Override
    public final void onClick(View view) {
        assert view == this;

        if (isSelectionModeActive()) {
            onLongClick(view);
        }  else {
            onClick();
        }
    }

    // OnLongClickListener implementation.
    @Override
    public boolean onLongClick(View view) {
        assert view == this;
        boolean checked = toggleSelectionForItem(mItem);
        setChecked(checked);
        return true;
    }

    /**
     * @return Whether we are currently in selection mode.
     */
    protected boolean isSelectionModeActive() {
        return mSelectionDelegate.isSelectionEnabled();
    }

    /**
     * Toggles the selection state for a given item.
     * @param item The given item.
     * @return Whether the item was in selected state after the toggle.
     */
    protected boolean toggleSelectionForItem(E item) {
        return mSelectionDelegate.toggleSelectionForItem(item);
    }

    // Checkable implementations.
    @Override
    public boolean isChecked() {
        return mIsChecked;
    }

    @Override
    public void toggle() {
        setChecked(!isChecked());
    }

    @Override
    public void setChecked(boolean checked) {
        if (checked == mIsChecked) return;
        mIsChecked = checked;
        updateIconView();
    }

    // SelectionObserver implementation.
    @Override
    public void onSelectionStateChange(List<E> selectedItems) {
        setChecked(mSelectionDelegate.isItemSelected(mItem));
    }

    /**
     * Set drawable for the icon view. Note that you may need to use this method instead of
     * mIconView#setImageDrawable to ensure icon view is correctly set in selection mode.
     */
    protected void setIconDrawable(Drawable iconDrawable) {
        mIconDrawable = iconDrawable;
        updateIconView();
    }

    /**
     * Update icon image and background based on whether this item is selected.
     */
    protected void updateIconView() {
        // TODO(huayinz): Refactor this method so that mIconView is not exposed to subclass.
        if (mIconView == null) return;

        if (isChecked()) {
            mIconView.getBackground().setLevel(mSelectedLevel);
            mIconView.setImageDrawable(mCheckDrawable);
            mIconView.setTint(mIconColorList);
            mCheckDrawable.start();
        } else {
            mIconView.getBackground().setLevel(mDefaultLevel);
            mIconView.setImageDrawable(mIconDrawable);
            mIconView.setTint(getDefaultIconTint());
        }

        if (!FeatureUtilities.isChromeModernDesignEnabled()) {
            mIconView.getBackground().setAlpha(isChecked() ? 255 : 0);
        }
    }

    /**
     * @return The {@link ColorStateList} used to tint the icon drawable set via
     *         {@link #setIconDrawable(Drawable)} when the item is not selected.
     */
    protected @Nullable ColorStateList getDefaultIconTint() {
        return null;
    }

    /**
     * Same as {@link OnClickListener#onClick(View)} on this.
     * Subclasses should override this instead of setting their own OnClickListener because this
     * class handles onClick events in selection mode, and won't forward events to subclasses in
     * that case.
     */
    protected abstract void onClick();

    @VisibleForTesting
    public void endAnimationsForTests() {
        mCheckDrawable.stop();
    }

    /**
     * Sets the icon for the image view: the default icon if unselected, the check mark if selected.
     *
     * @param imageView     The image view in which the icon will be presented.
     * @param defaultIcon   The default icon that will be displayed if not selected.
     * @param isSelected    Whether the item is selected or not.
     */
    public static void applyModernIconStyle(
            TintedImageView imageView, Drawable defaultIcon, boolean isSelected) {
        imageView.setBackgroundResource(R.drawable.list_item_icon_modern_bg);
        imageView.setImageDrawable(isSelected
                        ? TintedDrawable.constructTintedDrawable(imageView.getResources(),
                                  R.drawable.ic_check_googblue_24dp, R.color.white_mode_tint)
                        : defaultIcon);
        imageView.getBackground().setLevel(isSelected
                        ? imageView.getResources().getInteger(R.integer.list_item_level_selected)
                        : imageView.getResources().getInteger(R.integer.list_item_level_default));
    }
}
