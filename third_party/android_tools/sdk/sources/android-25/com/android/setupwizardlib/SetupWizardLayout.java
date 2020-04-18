/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.setupwizardlib;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.content.Context;
import android.content.res.ColorStateList;
import android.content.res.TypedArray;
import android.graphics.Shader.TileMode;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.LayerDrawable;
import android.os.Build.VERSION;
import android.os.Build.VERSION_CODES;
import android.os.Parcel;
import android.os.Parcelable;
import android.util.AttributeSet;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.InflateException;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewStub;
import android.widget.ProgressBar;
import android.widget.ScrollView;
import android.widget.TextView;

import com.android.setupwizardlib.util.RequireScrollHelper;
import com.android.setupwizardlib.view.BottomScrollView;
import com.android.setupwizardlib.view.Illustration;
import com.android.setupwizardlib.view.NavigationBar;

public class SetupWizardLayout extends TemplateLayout {

    private static final String TAG = "SetupWizardLayout";

    private ColorStateList mProgressBarColor;

    public SetupWizardLayout(Context context) {
        super(context, 0, 0);
        init(null, R.attr.suwLayoutTheme);
    }

    public SetupWizardLayout(Context context, int template) {
        this(context, template, 0);
    }

    public SetupWizardLayout(Context context, int template, int containerId) {
        super(context, template, containerId);
        init(null, R.attr.suwLayoutTheme);
    }

    public SetupWizardLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(attrs, R.attr.suwLayoutTheme);
    }

    @TargetApi(VERSION_CODES.HONEYCOMB)
    public SetupWizardLayout(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init(attrs, defStyleAttr);
    }

    // All the constructors delegate to this init method. The 3-argument constructor is not
    // available in LinearLayout before v11, so call super with the exact same arguments.
    private void init(AttributeSet attrs, int defStyleAttr) {
        final TypedArray a = getContext().obtainStyledAttributes(attrs,
                R.styleable.SuwSetupWizardLayout, defStyleAttr, 0);

        // Set the background from XML, either directly or built from a bitmap tile
        final Drawable background =
                a.getDrawable(R.styleable.SuwSetupWizardLayout_suwBackground);
        if (background != null) {
            setLayoutBackground(background);
        } else {
            final Drawable backgroundTile =
                    a.getDrawable(R.styleable.SuwSetupWizardLayout_suwBackgroundTile);
            if (backgroundTile != null) {
                setBackgroundTile(backgroundTile);
            }
        }

        // Set the illustration from XML, either directly or built from image + horizontal tile
        final Drawable illustration =
                a.getDrawable(R.styleable.SuwSetupWizardLayout_suwIllustration);
        if (illustration != null) {
            setIllustration(illustration);
        } else {
            final Drawable illustrationImage =
                    a.getDrawable(R.styleable.SuwSetupWizardLayout_suwIllustrationImage);
            final Drawable horizontalTile = a.getDrawable(
                    R.styleable.SuwSetupWizardLayout_suwIllustrationHorizontalTile);
            if (illustrationImage != null && horizontalTile != null) {
                setIllustration(illustrationImage, horizontalTile);
            }
        }

        // Set the top padding of the illustration
        int decorPaddingTop = a.getDimensionPixelSize(
                R.styleable.SuwSetupWizardLayout_suwDecorPaddingTop, -1);
        if (decorPaddingTop == -1) {
            decorPaddingTop = getResources().getDimensionPixelSize(R.dimen.suw_decor_padding_top);
        }
        setDecorPaddingTop(decorPaddingTop);


        // Set the illustration aspect ratio. See Illustration.setAspectRatio(float). This will
        // override suwIllustrationPaddingTop if its value is not 0.
        float illustrationAspectRatio = a.getFloat(
                R.styleable.SuwSetupWizardLayout_suwIllustrationAspectRatio, -1f);
        if (illustrationAspectRatio == -1f) {
            final TypedValue out = new TypedValue();
            getResources().getValue(R.dimen.suw_illustration_aspect_ratio, out, true);
            illustrationAspectRatio = out.getFloat();
        }
        setIllustrationAspectRatio(illustrationAspectRatio);

        // Set the header text
        final CharSequence headerText =
                a.getText(R.styleable.SuwSetupWizardLayout_suwHeaderText);
        if (headerText != null) {
            setHeaderText(headerText);
        }

        a.recycle();
    }

    @Override
    protected Parcelable onSaveInstanceState() {
        final Parcelable parcelable = super.onSaveInstanceState();
        final SavedState ss = new SavedState(parcelable);
        ss.mIsProgressBarShown = isProgressBarShown();
        return ss;
    }

    @Override
    protected void onRestoreInstanceState(Parcelable state) {
        if (!(state instanceof SavedState)) {
            Log.w(TAG, "Ignoring restore instance state " + state);
            super.onRestoreInstanceState(state);
            return;
        }

        final SavedState ss = (SavedState) state;
        super.onRestoreInstanceState(ss.getSuperState());
        final boolean isProgressBarShown = ss.mIsProgressBarShown;
        if (isProgressBarShown) {
            showProgressBar();
        } else {
            hideProgressBar();
        }
    }

    @Override
    protected View onInflateTemplate(LayoutInflater inflater, int template) {
        if (template == 0) {
            template = R.layout.suw_template;
        }
        try {
            return super.onInflateTemplate(inflater, template);
        } catch (RuntimeException e) {
            // Versions before M throws RuntimeException for unsuccessful attribute resolution
            // Versions M+ will throw an InflateException (which extends from RuntimeException)
            throw new InflateException("Unable to inflate layout. Are you using "
                    + "@style/SuwThemeMaterial (or its descendant) as your theme?", e);
        }
    }

    @Override
    protected ViewGroup findContainer(int containerId) {
        if (containerId == 0) {
            containerId = R.id.suw_layout_content;
        }
        return super.findContainer(containerId);
    }

    public NavigationBar getNavigationBar() {
        final View view = findManagedViewById(R.id.suw_layout_navigation_bar);
        return view instanceof NavigationBar ? (NavigationBar) view : null;
    }

    public ScrollView getScrollView() {
        final View view = findManagedViewById(R.id.suw_bottom_scroll_view);
        return view instanceof ScrollView ? (ScrollView) view : null;
    }

    public void requireScrollToBottom() {
        final NavigationBar navigationBar = getNavigationBar();
        final ScrollView scrollView = getScrollView();
        if (navigationBar != null && (scrollView instanceof BottomScrollView)) {
            RequireScrollHelper.requireScroll(navigationBar, (BottomScrollView) scrollView);
        } else {
            Log.e(TAG, "Both suw_layout_navigation_bar and suw_bottom_scroll_view must exist in"
                    + " the template to require scrolling.");
        }
    }

    public void setHeaderText(int title) {
        final TextView titleView = getHeaderTextView();
        if (titleView != null) {
            titleView.setText(title);
        }
    }

    public void setHeaderText(CharSequence title) {
        final TextView titleView = getHeaderTextView();
        if (titleView != null) {
            titleView.setText(title);
        }
    }

    public CharSequence getHeaderText() {
        final TextView titleView = getHeaderTextView();
        return titleView != null ? titleView.getText() : null;
    }

    public TextView getHeaderTextView() {
        return (TextView) findManagedViewById(R.id.suw_layout_title);
    }

    /**
     * Set the illustration of the layout. The drawable will be applied as is, and the bounds will
     * be set as implemented in {@link com.android.setupwizardlib.view.Illustration}. To create
     * a suitable drawable from an asset and a horizontal repeating tile, use
     * {@link #setIllustration(int, int)} instead.
     *
     * @param drawable The drawable specifying the illustration.
     */
    public void setIllustration(Drawable drawable) {
        final View view = findManagedViewById(R.id.suw_layout_decor);
        if (view instanceof Illustration) {
            final Illustration illustration = (Illustration) view;
            illustration.setIllustration(drawable);
        }
    }

    /**
     * Set the illustration of the layout, which will be created asset and the horizontal tile as
     * suitable. On phone layouts (not sw600dp), the asset will be scaled, maintaining aspect ratio.
     * On tablets (sw600dp), the assets will always have 256dp height and the rest of the
     * illustration area that the asset doesn't fill will be covered by the horizontalTile.
     *
     * @param asset Resource ID of the illustration asset.
     * @param horizontalTile Resource ID of the horizontally repeating tile for tablet layout.
     */
    public void setIllustration(int asset, int horizontalTile) {
        final View view = findManagedViewById(R.id.suw_layout_decor);
        if (view instanceof Illustration) {
            final Illustration illustration = (Illustration) view;
            final Drawable illustrationDrawable = getIllustration(asset, horizontalTile);
            illustration.setIllustration(illustrationDrawable);
        }
    }

    private void setIllustration(Drawable asset, Drawable horizontalTile) {
        final View view = findManagedViewById(R.id.suw_layout_decor);
        if (view instanceof Illustration) {
            final Illustration illustration = (Illustration) view;
            final Drawable illustrationDrawable = getIllustration(asset, horizontalTile);
            illustration.setIllustration(illustrationDrawable);
        }
    }

    /**
     * Sets the aspect ratio of the illustration. This will be the space (padding top) reserved
     * above the header text. This will override the padding top of the illustration.
     *
     * @param aspectRatio The aspect ratio
     * @see com.android.setupwizardlib.view.Illustration#setAspectRatio(float)
     */
    public void setIllustrationAspectRatio(float aspectRatio) {
        final View view = findManagedViewById(R.id.suw_layout_decor);
        if (view instanceof Illustration) {
            final Illustration illustration = (Illustration) view;
            illustration.setAspectRatio(aspectRatio);
        }
    }

    /**
     * Set the top padding of the decor view. If the decor is an Illustration and the aspect ratio
     * is set, this value will be overridden.
     *
     * <p>Note: Currently the default top padding for tablet landscape is 128dp, which is the offset
     * of the card from the top. This is likely to change in future versions so this value aligns
     * with the height of the illustration instead.
     *
     * @param paddingTop The top padding in pixels.
     */
    public void setDecorPaddingTop(int paddingTop) {
        final View view = findManagedViewById(R.id.suw_layout_decor);
        if (view != null) {
            view.setPadding(view.getPaddingLeft(), paddingTop, view.getPaddingRight(),
                    view.getPaddingBottom());
        }
    }

    /**
     * Set the background of the layout, which is expected to be able to extend infinitely. If it is
     * a bitmap tile and you want it to repeat, use {@link #setBackgroundTile(int)} instead.
     */
    public void setLayoutBackground(Drawable background) {
        final View view = findManagedViewById(R.id.suw_layout_decor);
        if (view != null) {
            //noinspection deprecation
            view.setBackgroundDrawable(background);
        }
    }

    /**
     * Set the background of the layout to a repeating bitmap tile. To use a different kind of
     * drawable, use {@link #setLayoutBackground(android.graphics.drawable.Drawable)} instead.
     */
    public void setBackgroundTile(int backgroundTile) {
        final Drawable backgroundTileDrawable =
                getContext().getResources().getDrawable(backgroundTile);
        setBackgroundTile(backgroundTileDrawable);
    }

    private void setBackgroundTile(Drawable backgroundTile) {
        if (backgroundTile instanceof BitmapDrawable) {
            ((BitmapDrawable) backgroundTile).setTileModeXY(TileMode.REPEAT, TileMode.REPEAT);
        }
        setLayoutBackground(backgroundTile);
    }

    private Drawable getIllustration(int asset, int horizontalTile) {
        final Context context = getContext();
        final Drawable assetDrawable = context.getResources().getDrawable(asset);
        final Drawable tile = context.getResources().getDrawable(horizontalTile);
        return getIllustration(assetDrawable, tile);
    }

    @SuppressLint("RtlHardcoded")
    private Drawable getIllustration(Drawable asset, Drawable horizontalTile) {
        final Context context = getContext();
        if (context.getResources().getBoolean(R.bool.suwUseTabletLayout)) {
            // If it is a "tablet" (sw600dp), create a LayerDrawable with the horizontal tile.
            if (horizontalTile instanceof BitmapDrawable) {
                ((BitmapDrawable) horizontalTile).setTileModeX(TileMode.REPEAT);
                ((BitmapDrawable) horizontalTile).setGravity(Gravity.TOP);
            }
            if (asset instanceof BitmapDrawable) {
                // Always specify TOP | LEFT, Illustration will flip the entire LayerDrawable.
                ((BitmapDrawable) asset).setGravity(Gravity.TOP | Gravity.LEFT);
            }
            final LayerDrawable layers =
                    new LayerDrawable(new Drawable[] { horizontalTile, asset });
            if (VERSION.SDK_INT >= VERSION_CODES.KITKAT) {
                layers.setAutoMirrored(true);
            }
            return layers;
        } else {
            // If it is a "phone" (not sw600dp), simply return the illustration
            if (VERSION.SDK_INT >= VERSION_CODES.KITKAT) {
                asset.setAutoMirrored(true);
            }
            return asset;
        }
    }

    /**
     * Same as {@link android.view.View#findViewById(int)}, but may include views that are managed
     * by this view but not currently added to the view hierarchy. e.g. recycler view or list view
     * headers that are not currently shown.
     */
    protected View findManagedViewById(int id) {
        return findViewById(id);
    }

    public boolean isProgressBarShown() {
        final View progressBar = findManagedViewById(R.id.suw_layout_progress);
        return progressBar != null && progressBar.getVisibility() == View.VISIBLE;
    }

    /**
     * Sets whether the progress bar below the header text is shown or not. The progress bar is
     * a lazily inflated ViewStub, which means the progress bar will not actually be part of the
     * view hierarchy until the first time this is set to {@code true}.
     */
    public void setProgressBarShown(boolean shown) {
        final View progressBar = findManagedViewById(R.id.suw_layout_progress);
        if (progressBar != null) {
            progressBar.setVisibility(shown ? View.VISIBLE : View.GONE);
        } else if (shown) {
            final ViewStub progressBarStub =
                    (ViewStub) findManagedViewById(R.id.suw_layout_progress_stub);
            if (progressBarStub != null) {
                progressBarStub.inflate();
            }
            if (mProgressBarColor != null) {
                setProgressBarColor(mProgressBarColor);
            }
        }
    }

    /**
     * @deprecated Use {@link #setProgressBarShown(boolean)}
     */
    @Deprecated
    public void showProgressBar() {
        setProgressBarShown(true);
    }

    /**
     * @deprecated Use {@link #setProgressBarShown(boolean)}
     */
    @Deprecated
    public void hideProgressBar() {
        setProgressBarShown(false);
    }

    public void setProgressBarColor(ColorStateList color) {
        mProgressBarColor = color;
        if (VERSION.SDK_INT >= VERSION_CODES.LOLLIPOP) {
            // Suppress lint error caused by
            // https://code.google.com/p/android/issues/detail?id=183136
            // noinspection AndroidLintWrongViewCast
            final ProgressBar bar = (ProgressBar) findViewById(R.id.suw_layout_progress);
            if (bar != null) {
                bar.setIndeterminateTintList(color);
            }
        }
    }

    public ColorStateList getProgressBarColor() {
        return mProgressBarColor;
    }

    /* Misc */

    protected static class SavedState extends BaseSavedState {

        boolean mIsProgressBarShown = false;

        public SavedState(Parcelable parcelable) {
            super(parcelable);
        }

        public SavedState(Parcel source) {
            super(source);
            mIsProgressBarShown = source.readInt() != 0;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            super.writeToParcel(dest, flags);
            dest.writeInt(mIsProgressBarShown ? 1 : 0);
        }

        public static final Parcelable.Creator<SavedState> CREATOR =
                new Parcelable.Creator<SavedState>() {

                    @Override
                    public SavedState createFromParcel(Parcel parcel) {
                        return new SavedState(parcel);
                    }

                    @Override
                    public SavedState[] newArray(int size) {
                        return new SavedState[size];
                    }
                };
    }
}
