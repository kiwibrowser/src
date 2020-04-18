// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.autofill;

import org.chromium.ui.DropdownItemBase;

/**
 * Autofill suggestion container used to store information needed for each Autofill popup entry.
 */
public class AutofillSuggestion extends DropdownItemBase {
    /**
     * The constant used to specify warning messages in a list of Autofill suggestions.
     * Has to be kept in sync with {@code POPUP_ITEM_ID_SEPARATOR} enum in
     * components/autofill/core/browser/popup_item_ids.h
     */
    private static final int ITEM_ID_INSECURE_CONTEXT_PAYMENT_DISABLED_MESSAGE = -1;
    private static final int ITEM_ID_HTTP_NOT_SECURE_WARNING_MESSAGE = -10;

    private final String mLabel;
    private final String mSublabel;
    private final int mIconId;
    private final boolean mIsIconAtStart;
    private final int mSuggestionId;
    private final boolean mIsDeletable;
    private final boolean mIsMultilineLabel;
    private final boolean mIsBoldLabel;

    /**
     * Constructs a Autofill suggestion container.
     *
     * @param label The main label of the Autofill suggestion.
     * @param sublabel The describing sublabel of the Autofill suggestion.
     * @param iconId The resource ID for the icon associated with the suggestion, or
     *               {@code DropdownItem.NO_ICON} for no icon.
     * @param isIconAtStart {@code true} if {@code iconId} is displayed before {@code label}.
     * @param suggestionId The type of suggestion.
     * @param isDeletable Whether the item can be deleted by the user.
     * @param isMultilineLabel Whether the label is displayed over multiple lines.
     * @param isBoldLabel Whether the label is displayed in {@code Typeface.BOLD}.
     */
    public AutofillSuggestion(String label, String sublabel, int iconId, boolean isIconAtStart,
            int suggestionId, boolean isDeletable, boolean isMultilineLabel, boolean isBoldLabel) {
        mLabel = label;
        mSublabel = sublabel;
        mIconId = iconId;
        mIsIconAtStart = isIconAtStart;
        mSuggestionId = suggestionId;
        mIsDeletable = isDeletable;
        mIsMultilineLabel = isMultilineLabel;
        mIsBoldLabel = isBoldLabel;
    }

    @Override
    public String getLabel() {
        return mLabel;
    }

    @Override
    public String getSublabel() {
        return mSublabel;
    }

    @Override
    public int getIconId() {
        return mIconId;
    }

    @Override
    public boolean isMultilineLabel() {
        return mIsMultilineLabel;
    }

    @Override
    public boolean isBoldLabel() {
        return mIsBoldLabel;
    }

    @Override
    public int getLabelFontColorResId() {
        if (mSuggestionId == ITEM_ID_HTTP_NOT_SECURE_WARNING_MESSAGE) {
            return R.color.http_bad_warning_message_text;
        } else if (mSuggestionId == ITEM_ID_INSECURE_CONTEXT_PAYMENT_DISABLED_MESSAGE) {
            return R.color.insecure_context_payment_disabled_message_text;
        }
        return super.getLabelFontColorResId();
    }

    @Override
    public int getSublabelFontSizeResId() {
        if (mSuggestionId == ITEM_ID_HTTP_NOT_SECURE_WARNING_MESSAGE) {
            return R.dimen.dropdown_item_larger_sublabel_font_size;
        }
        return super.getSublabelFontSizeResId();
    }

    @Override
    public boolean isLabelAndSublabelOnSameLine() {
        if (mSuggestionId == ITEM_ID_HTTP_NOT_SECURE_WARNING_MESSAGE) {
            return true;
        }
        return super.isLabelAndSublabelOnSameLine();
    }

    @Override
    public boolean isIconAtStart() {
        if (mIsIconAtStart) {
            return true;
        }
        return super.isIconAtStart();
    }

    @Override
    public int getIconSizeResId() {
        if (mSuggestionId == ITEM_ID_HTTP_NOT_SECURE_WARNING_MESSAGE) {
            return R.dimen.dropdown_large_icon_size;
        }
        return super.getIconSizeResId();
    }

    @Override
    public int getIconMarginResId() {
        if (mSuggestionId == ITEM_ID_HTTP_NOT_SECURE_WARNING_MESSAGE) {
            return R.dimen.dropdown_large_icon_margin;
        }
        return super.getIconMarginResId();
    }

    public int getSuggestionId() {
        return mSuggestionId;
    }

    public boolean isDeletable() {
        return mIsDeletable;
    }

    public boolean isFillable() {
        // Negative suggestion ID indiciates a tool like "settings" or "scan credit card."
        // Non-negative suggestion ID indicates suggestions that can be filled into the form.
        return mSuggestionId >= 0;
    }
}
