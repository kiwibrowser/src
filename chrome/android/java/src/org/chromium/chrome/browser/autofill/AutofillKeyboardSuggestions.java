// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.autofill;

import android.graphics.PorterDuff;
import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.support.annotation.NonNull;
import android.support.v7.content.res.AppCompatResources;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.components.autofill.AutofillDelegate;
import org.chromium.components.autofill.AutofillSuggestion;
import org.chromium.ui.base.WindowAndroid;

/**
 * The lists that shows autofill suggestions in the keyboard accessory.
 */
public class AutofillKeyboardSuggestions
        extends LinearLayout implements View.OnClickListener, View.OnLongClickListener {
    private final WindowAndroid mWindowAndroid;
    private final AutofillDelegate mAutofillDelegate;
    // If |mMaximumLabelWidthPx| is 0, we do not call |setMaxWidth| on the |TextView| for a
    // fillable suggestion label.
    private final int mMaximumLabelWidthPx;
    private final int mMaximumSublabelWidthPx;

    /**
     * Creates an AutofillKeyboardAccessory with specified parameters.
     * @param windowAndroid The owning WindowAndroid.
     * @param autofillDelegate A object that handles the calls to the native
     *                         AutofillKeyboardAccessoryView.
     * @param shouldLimitLabelWidth If true, limit suggestion label width to 1/2 device's width.
     */
    public AutofillKeyboardSuggestions(WindowAndroid windowAndroid,
            AutofillDelegate autofillDelegate, boolean shouldLimitLabelWidth) {
        super(windowAndroid.getActivity().get());
        assert windowAndroid.getActivity().get() != null;
        assert autofillDelegate != null;
        mAutofillDelegate = autofillDelegate;
        mWindowAndroid = windowAndroid;

        int deviceWidthPx = windowAndroid.getDisplay().getDisplayWidth();
        mMaximumLabelWidthPx = shouldLimitLabelWidth ? deviceWidthPx / 2 : 0;
        mMaximumSublabelWidthPx = deviceWidthPx / 4;

        int horizontalPaddingPx =
                getResources().getDimensionPixelSize(R.dimen.keyboard_accessory_half_padding);
        setPadding(horizontalPaddingPx, 0, horizontalPaddingPx, 0);
    }

    /**
     * @param isRtl Gives the layout direction for the <input> field.
     */
    public void setSuggestions(AutofillSuggestion[] suggestions, boolean isRtl) {
        assert suggestions.length > 0;
        removeAllViews();
        // The first suggestion may be a hint to call attention to the keyboard accessory. See
        // |IsHintEnabledInKeyboardAccessory|. A 'hint' suggestion does not have a label and is
        // not fillable, but has an icon.
        final boolean isFirstSuggestionAHint = TextUtils.isEmpty(suggestions[0].getLabel());
        if (isFirstSuggestionAHint) {
            assert suggestions[0].getIconId() != 0 && !suggestions[0].isFillable();
        }
        int separatorPosition = -1;
        int startIndex = isRtl ? suggestions.length - 1 : 0;
        int endIndex = isRtl ? -1 : suggestions.length; // The index after the last element.
        int i = startIndex;
        while (i != endIndex) {
            AutofillSuggestion suggestion = suggestions[i];
            boolean isKeyboardAccessoryHint = i == 0 && isFirstSuggestionAHint;
            if (!isKeyboardAccessoryHint) {
                assert !TextUtils.isEmpty(suggestion.getLabel());
            }

            View touchTarget;
            if (suggestion.isFillable() || suggestion.getIconId() == 0) {
                touchTarget = createAccessoryItem(suggestion);
            } else {
                if (separatorPosition == -1 && !isKeyboardAccessoryHint) separatorPosition = i;
                touchTarget = createAccessoryIcon(suggestion, isKeyboardAccessoryHint);
            }

            if (!isKeyboardAccessoryHint) {
                touchTarget.setTag(i);
                touchTarget.setOnClickListener(this);
                if (suggestion.isDeletable()) {
                    touchTarget.setOnLongClickListener(this);
                }
            }
            addView(touchTarget);
            i = isRtl ? i - 1 : i + 1;
        }

        if (separatorPosition != -1) {
            addView(createSeparatorView(), separatorPosition);
        }
    }

    @NonNull
    private View createAccessoryItem(AutofillSuggestion suggestion) {
        View touchTarget;
        touchTarget = LayoutInflater.from(getContext())
                              .inflate(R.layout.autofill_keyboard_accessory_item, this, false);

        TextView label =
                (TextView) touchTarget.findViewById(R.id.autofill_keyboard_accessory_item_label);

        if (mMaximumLabelWidthPx > 0 && suggestion.isFillable()) {
            label.setMaxWidth(mMaximumLabelWidthPx);
        }

        label.setText(suggestion.getLabel());
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) {
            label.setTypeface(Typeface.DEFAULT_BOLD);
        }

        if (suggestion.getIconId() != 0) {
            ApiCompatibilityUtils.setCompoundDrawablesRelativeWithIntrinsicBounds(label,
                    AppCompatResources.getDrawable(getContext(), suggestion.getIconId()),
                    null /* top */, null /* end */, null /* bottom */);
        }

        if (!TextUtils.isEmpty(suggestion.getSublabel())) {
            assert suggestion.isFillable();
            TextView sublabel = (TextView) touchTarget.findViewById(
                    R.id.autofill_keyboard_accessory_item_sublabel);
            sublabel.setText(suggestion.getSublabel());
            sublabel.setVisibility(View.VISIBLE);
            sublabel.setMaxWidth(mMaximumSublabelWidthPx);
        }
        return touchTarget;
    }

    @NonNull
    private View createAccessoryIcon(
            AutofillSuggestion suggestion, boolean isKeyboardAccessoryHint) {
        View touchTarget;
        touchTarget = LayoutInflater.from(getContext())
                              .inflate(R.layout.autofill_keyboard_accessory_icon, this, false);

        ImageView icon = (ImageView) touchTarget;
        Drawable drawable = AppCompatResources.getDrawable(getContext(), suggestion.getIconId());
        if (isKeyboardAccessoryHint) {
            drawable.setColorFilter(
                    ApiCompatibilityUtils.getColor(getResources(), R.color.google_blue_500),
                    PorterDuff.Mode.SRC_IN);
        } else {
            icon.setContentDescription(suggestion.getLabel());
        }
        icon.setImageDrawable(drawable);
        return touchTarget;
    }

    public void dismiss() {
        removeAllViews();
        mAutofillDelegate.dismissed();
    }

    @Override
    public void onClick(View v) {
        mAutofillDelegate.suggestionSelected((int) v.getTag());
    }

    @Override
    public boolean onLongClick(View v) {
        mAutofillDelegate.deleteSuggestion((int) v.getTag());
        return true;
    }

    // Helper to create separator view so that the settings icon is aligned to the right of the
    // screen.
    private View createSeparatorView() {
        View separator = new View(getContext());
        // Specify a layout weight so that the settings icon, which is displayed after the
        // separator, is aligned with the edge of the viewport.
        separator.setLayoutParams(new LinearLayout.LayoutParams(0, 0, 1));
        return separator;
    }
}
