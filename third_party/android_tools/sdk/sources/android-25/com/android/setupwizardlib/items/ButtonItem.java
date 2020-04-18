/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.setupwizardlib.items;

import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.view.ContextThemeWrapper;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;

import com.android.setupwizardlib.R;

/**
 * Description of a button inside {@link com.android.setupwizardlib.items.ButtonBarItem}. This item
 * will not be bound by the adapter, and must be a child of {@code ButtonBarItem}.
 */
public class ButtonItem extends AbstractItem implements View.OnClickListener {

    public interface OnClickListener {
        void onClick(ButtonItem item);
    }

    private boolean mEnabled = true;
    private CharSequence mText;
    private int mTheme = R.style.SuwButtonItem;
    private OnClickListener mListener;

    private Button mButton;

    public ButtonItem() {
        super();
    }

    public ButtonItem(Context context, AttributeSet attrs) {
        super(context, attrs);
        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.SuwButtonItem);
        mEnabled = a.getBoolean(R.styleable.SuwButtonItem_android_enabled, true);
        mText = a.getText(R.styleable.SuwButtonItem_android_text);
        mTheme = a.getResourceId(R.styleable.SuwButtonItem_android_theme, R.style.SuwButtonItem);
        a.recycle();
    }

    public void setOnClickListener(OnClickListener listener) {
        mListener = listener;
    }

    public void setText(CharSequence text) {
        mText = text;
    }

    public CharSequence getText() {
        return mText;
    }

    /**
     * The theme to use for this button. This can be used to create button of a particular style
     * (e.g. a colored or borderless button). Typically {@code android:buttonStyle} will be set in
     * the theme to change the style applied by the button.
     *
     * @param theme Resource ID of the theme
     */
    public void setTheme(int theme) {
        mTheme = theme;
        mButton = null;
    }

    /**
     * @return Resource ID of the theme used by this button.
     */
    public int getTheme() {
        return mTheme;
    }

    public void setEnabled(boolean enabled) {
        mEnabled = enabled;
    }

    @Override
    public int getCount() {
        return 0;
    }

    @Override
    public boolean isEnabled() {
        return mEnabled;
    }

    @Override
    public int getLayoutResource() {
        return 0;
    }

    /**
     * Do not use this since ButtonItem is not directly part of a list.
     */
    @Override
    public final void onBindView(View view) {
        throw new UnsupportedOperationException("Cannot bind to ButtonItem's view");
    }

    /**
     * Create a button according to this button item.
     *
     * @param parent The parent of the button, used to retrieve the theme and context for this
     *               button.
     * @return A button that can be added to the parent.
     */
    protected Button createButton(ViewGroup parent) {
        if (mButton == null) {
            Context context = parent.getContext();
            if (mTheme != 0) {
                context = new ContextThemeWrapper(context, mTheme);
            }
            mButton = new Button(context);
            mButton.setOnClickListener(this);
        } else {
            if (mButton.getParent() instanceof ViewGroup) {
                // A view cannot be added to a different parent if one already exists. Remove this
                // button from its parent before returning.
                ((ViewGroup) mButton.getParent()).removeView(mButton);
            }
        }
        mButton.setEnabled(mEnabled);
        mButton.setText(mText);
        return mButton;
    }

    @Override
    public void onClick(View v) {
        if (mListener != null) {
            mListener.onClick(this);
        }
    }
}
