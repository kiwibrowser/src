// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget.prefeditor;

import android.annotation.SuppressLint;
import android.content.Context;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.accessibility.AccessibilityEvent;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Spinner;
import android.widget.TextView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.autofill.AutofillProfileBridge.DropdownKeyValue;
import org.chromium.ui.UiUtils;

import java.util.ArrayList;
import java.util.List;

import javax.annotation.Nullable;

/**
 * Helper class for creating a dropdown view with a label.
 */
class EditorDropdownField implements EditorFieldView {
    private final EditorFieldModel mFieldModel;
    private final View mLayout;
    private final TextView mLabel;
    private final Spinner mDropdown;
    private int mSelectedIndex;
    private ArrayAdapter<CharSequence> mAdapter;
    @Nullable
    private EditorObserverForTest mObserverForTest;

    /**
     * Builds a dropdown view.
     *
     * @param context         The application context to use when creating widgets.
     * @param root            The object that provides a set of LayoutParams values for the view.
     * @param fieldModel      The data model of the dropdown.
     * @param changedCallback The callback to invoke after user's dropdwn item selection has been
     *                        processed.
     */
    public EditorDropdownField(Context context, ViewGroup root, final EditorFieldModel fieldModel,
            final Runnable changedCallback, @Nullable EditorObserverForTest observer) {
        assert fieldModel.getInputTypeHint() == EditorFieldModel.INPUT_TYPE_HINT_DROPDOWN;
        mFieldModel = fieldModel;
        mObserverForTest = observer;

        mLayout = LayoutInflater.from(context).inflate(
                R.layout.payment_request_editor_dropdown, root, false);

        mLabel = (TextView) mLayout.findViewById(R.id.spinner_label);
        mLabel.setText(mFieldModel.isRequired()
                        ? mFieldModel.getLabel() + EditorDialog.REQUIRED_FIELD_INDICATOR
                        : mFieldModel.getLabel());

        final List<DropdownKeyValue> dropdownKeyValues = mFieldModel.getDropdownKeyValues();
        final List<CharSequence> dropdownValues = getDropdownValues(dropdownKeyValues);
        if (mFieldModel.getHint() != null) {
            // Pass the hint to be displayed as default. If there is a '+' icon needed, use the
            // appropriate class to display it.
            if (mFieldModel.isDisplayPlusIcon()) {
                mAdapter = new HintedDropDownAdapterWithPlusIcon<CharSequence>(context,
                        R.layout.multiline_spinner_item, R.id.spinner_item, dropdownValues,
                        mFieldModel.getHint().toString());
            } else {
                mAdapter = new HintedDropDownAdapter<CharSequence>(context,
                        R.layout.multiline_spinner_item, R.id.spinner_item, dropdownValues,
                        mFieldModel.getHint().toString());
            }
            // Wrap the TextView in the dropdown popup around with a FrameLayout to display the text
            // in multiple lines.
            // Note that the TextView in the dropdown popup is displayed in a DropDownListView for
            // the dropdown style Spinner and the DropDownListView sets to display TextView instance
            // in a single line.
            mAdapter.setDropDownViewResource(R.layout.payment_request_dropdown_item);
        } else {
            mAdapter = new DropdownFieldAdapter<CharSequence>(
                    context, R.layout.multiline_spinner_item, dropdownValues);
            mAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        }

        // If no value is selected, we'll  select the hint, which is the first item on the dropdown
        // adapter.
        mSelectedIndex = TextUtils.isEmpty(mFieldModel.getValue())
                ? 0
                : mAdapter.getPosition(
                          mFieldModel.getDropdownValueByKey((mFieldModel.getValue().toString())));

        assert mSelectedIndex >= 0;

        mDropdown = (Spinner) mLayout.findViewById(R.id.spinner);
        mDropdown.setTag(this);
        mDropdown.setContentDescription(mFieldModel.getLabel());
        mDropdown.setAdapter(mAdapter);
        mDropdown.setSelection(mSelectedIndex);
        mDropdown.setOnItemSelectedListener(new OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                if (mSelectedIndex != position) {
                    String key = mFieldModel.getDropdownKeyByValue(mAdapter.getItem(position));
                    // If the hint is selected, it means that no value is entered by the user.
                    if (mFieldModel.getHint() != null && position == 0) {
                        key = null;
                    }
                    mSelectedIndex = position;
                    mFieldModel.setDropdownKey(key, changedCallback);
                }
                if (mObserverForTest != null) {
                    mObserverForTest.onEditorTextUpdate();
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {}
        });
        mDropdown.setOnTouchListener(new View.OnTouchListener() {
            @SuppressLint("ClickableViewAccessibility")
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if (event.getAction() == MotionEvent.ACTION_DOWN) requestFocusAndHideKeyboard();

                return false;
            }
        });
    }

    /** @return The View containing everything. */
    public View getLayout() {
        return mLayout;
    }

    /** @return The EditorFieldModel that the EditorDropdownField represents. */
    public EditorFieldModel getFieldModel() {
        return mFieldModel;
    }

    /** @return The label view for the spinner. */
    public View getLabel() {
        return mLabel;
    }

    /** @return The dropdown view itself. */
    public Spinner getDropdown() {
        return mDropdown;
    }

    @Override
    public boolean isValid() {
        return mFieldModel.isValid();
    }

    @Override
    public void updateDisplayedError(boolean showError) {
        View view = mDropdown.getSelectedView();
        if (view != null && view instanceof TextView) {
            ((TextView) view).setError(showError ? mFieldModel.getErrorMessage() : null);
        }
    }

    @Override
    public void scrollToAndFocus() {
        updateDisplayedError(!isValid());
        requestFocusAndHideKeyboard();
    }

    private void requestFocusAndHideKeyboard() {
        UiUtils.hideKeyboard(mDropdown);
        ViewGroup parent = (ViewGroup) mDropdown.getParent();
        if (parent != null) parent.requestChildFocus(mDropdown, mDropdown);
        mDropdown.sendAccessibilityEvent(AccessibilityEvent.TYPE_VIEW_FOCUSED);
    }

    @Override
    public void update() {
        // If no value is selected, we'll  select the hint, which is the first item on the dropdown
        // adapter.
        mSelectedIndex = TextUtils.isEmpty(mFieldModel.getValue())
                ? 0
                : mAdapter.getPosition(
                          mFieldModel.getDropdownValueByKey((mFieldModel.getValue().toString())));
        assert mSelectedIndex >= 0;
        mDropdown.setSelection(mSelectedIndex);
    }

    private static List<CharSequence> getDropdownValues(List<DropdownKeyValue> dropdownKeyValues) {
        List<CharSequence> dropdownValues = new ArrayList<CharSequence>();
        for (int i = 0; i < dropdownKeyValues.size(); i++) {
            dropdownValues.add(dropdownKeyValues.get(i).getValue());
        }
        return dropdownValues;
    }
}
