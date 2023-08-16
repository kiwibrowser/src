// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.omnibox;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.support.annotation.IntDef;
import android.support.v4.view.ViewCompat;
import android.support.v7.app.AlertDialog;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.TextPaint;
import android.text.TextUtils;
import android.text.style.StyleSpan;
import android.util.TypedValue;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.TextView.BufferType;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.metrics.RecordUserAction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.toolbar.ToolbarModel;
import org.chromium.chrome.browser.omnibox.OmniboxResultsAdapter.OmniboxResultItem;
import org.chromium.chrome.browser.omnibox.OmniboxResultsAdapter.OmniboxSuggestionDelegate;
import org.chromium.chrome.browser.omnibox.OmniboxSuggestion.MatchClassification;
import org.chromium.chrome.browser.util.ViewUtils;
import org.chromium.chrome.browser.widget.TintedDrawable;
import org.chromium.ui.base.DeviceFormFactor;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.List;

/**
 * Container view for omnibox suggestions made very specific for omnibox suggestions to minimize
 * any unnecessary measures and layouts.
 */
class SuggestionView extends ViewGroup {
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({
            SUGGESTION_ICON_UNDEFINED,
            SUGGESTION_ICON_BOOKMARK,
            SUGGESTION_ICON_HISTORY,
            SUGGESTION_ICON_GLOBE,
            SUGGESTION_ICON_MAGNIFIER,
            SUGGESTION_ICON_VOICE
    })
    private @interface SuggestionIcon {}

    private static final int SUGGESTION_ICON_UNDEFINED = -1;
    private static final int SUGGESTION_ICON_BOOKMARK = 0;
    private static final int SUGGESTION_ICON_HISTORY = 1;
    private static final int SUGGESTION_ICON_GLOBE = 2;
    private static final int SUGGESTION_ICON_MAGNIFIER = 3;
    private static final int SUGGESTION_ICON_VOICE = 4;

    private static final long RELAYOUT_DELAY_MS = 20;

    private static final float ANSWER_IMAGE_SCALING_FACTOR = 1.15f;

    private final LocationBar mLocationBar;
    private UrlBar mUrlBar;
    private ImageView mNavigationButton;

    private final int mSuggestionHeight;
    private final int mSuggestionAnswerHeight;
    private int mNumAnswerLines = 1;

    private final int mDarkTitleColorStandardFont;
    private final int mLightTitleColorStandardFont;
    private final int mDarkUrlStandardModernColor;
    private final int mLightUrlStandardModernColor;
    private final int mLightUrlStandardColor;

    private OmniboxResultItem mSuggestionItem;
    private OmniboxSuggestion mSuggestion;
    private OmniboxSuggestionDelegate mSuggestionDelegate;
    private Boolean mUseDarkColors;
    private int mPosition;
    private int mRefineViewOffsetPx;
    private int mSuggestionViewStartOffset;

    private final SuggestionContentsContainer mContentsView;

    private final int mRefineWidth;
    private final View mRefineView;
    private TintedDrawable mRefineIcon;
    private final int mRefineViewModernEndPadding;
    private final int mSuggestionListModernOffset;

    private final int[] mViewPositionHolder = new int[2];

    // Pre-computed offsets in px.
    private final int mPhoneUrlBarLeftOffsetPx;
    private final int mPhoneUrlBarLeftOffsetRtlPx;

    /**
     * Constructs a new omnibox suggestion view.
     *
     * @param context The context used to construct the suggestion view.
     * @param locationBar The location bar showing these suggestions.
     */
    public SuggestionView(Context context, LocationBar locationBar) {
        super(context);
        mLocationBar = locationBar;

        mSuggestionHeight =
                context.getResources().getDimensionPixelOffset(R.dimen.omnibox_suggestion_height);
        mSuggestionAnswerHeight =
                context.getResources().getDimensionPixelOffset(
                        R.dimen.omnibox_suggestion_answer_height);

        Resources resources = getResources();
        mDarkTitleColorStandardFont =
                ApiCompatibilityUtils.getColor(resources, R.color.url_emphasis_default_text);
        mLightTitleColorStandardFont =
                ApiCompatibilityUtils.getColor(resources, R.color.url_emphasis_light_default_text);
        mDarkUrlStandardModernColor =
                ApiCompatibilityUtils.getColor(resources, R.color.suggestion_url_dark_modern);
        mLightUrlStandardModernColor =
                ApiCompatibilityUtils.getColor(resources, R.color.suggestion_url_light_modern);
        mLightUrlStandardColor =
                ApiCompatibilityUtils.getColor(resources, R.color.suggestion_url_light);

        TypedArray a = getContext().obtainStyledAttributes(
                new int [] {R.attr.selectableItemBackground});
        Drawable itemBackground = a.getDrawable(0);
        a.recycle();

        mContentsView = new SuggestionContentsContainer(context, itemBackground);
        addView(mContentsView);

        mRefineView = new View(context) {
            @Override
            protected void onDraw(Canvas canvas) {
                super.onDraw(canvas);

                if (mRefineIcon == null) return;
                canvas.save();
                canvas.translate(
                        (getMeasuredWidth() - mRefineIcon.getIntrinsicWidth()) / 2f,
                        (getMeasuredHeight() - mRefineIcon.getIntrinsicHeight()) / 2f);
                mRefineIcon.draw(canvas);
                canvas.restore();
            }

            @Override
            public void setVisibility(int visibility) {
                super.setVisibility(visibility);

                if (visibility == VISIBLE) {
                    setClickable(true);
                    setFocusable(true);
                } else {
                    setClickable(false);
                    setFocusable(false);
                }
            }

            @Override
            protected void drawableStateChanged() {
                super.drawableStateChanged();

                if (mRefineIcon != null && mRefineIcon.isStateful()) {
                    mRefineIcon.setState(getDrawableState());
                }
            }
        };
        mRefineView.setContentDescription(getContext().getString(
                R.string.accessibility_omnibox_btn_refine));

        // Although this has the same background as the suggestion view, it can not be shared as
        // it will result in the state of the drawable being shared and always showing up in the
        // refine view.
        mRefineView.setBackground(itemBackground.getConstantState().newDrawable());
        mRefineView.setId(R.id.refine_view_id);
        mRefineView.setClickable(true);
        mRefineView.setFocusable(true);
        mRefineView.setLayoutParams(new LayoutParams(0, 0));
        addView(mRefineView);

        mRefineWidth = getResources()
                .getDimensionPixelSize(R.dimen.omnibox_suggestion_refine_width);

        mRefineViewModernEndPadding = getResources().getDimensionPixelSize(
                R.dimen.omnibox_suggestion_refine_view_modern_end_padding);

        mSuggestionListModernOffset =
                getResources().getDimensionPixelSize(R.dimen.omnibox_suggestion_list_modern_offset);

        mUrlBar = (UrlBar) locationBar.getContainerView().findViewById(R.id.url_bar);

        mPhoneUrlBarLeftOffsetPx = getResources().getDimensionPixelOffset(
                R.dimen.omnibox_suggestion_phone_url_bar_left_offset);
        mPhoneUrlBarLeftOffsetRtlPx = getResources().getDimensionPixelOffset(
                R.dimen.omnibox_suggestion_phone_url_bar_left_offset_rtl);
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        if (getMeasuredWidth() == 0) return;

        if (mSuggestion.getType() != OmniboxSuggestionType.SEARCH_SUGGEST_TAIL) {
            mContentsView.resetTextWidths();
        }

        boolean refineVisible = mRefineView.getVisibility() == VISIBLE;
        boolean isRtl = ApiCompatibilityUtils.isLayoutRtl(this);
        int contentsViewOffsetX = isRtl && refineVisible ? mRefineWidth : 0;
        mContentsView.layout(contentsViewOffsetX, 0,
                contentsViewOffsetX + mContentsView.getMeasuredWidth(),
                mContentsView.getMeasuredHeight());

        int refineViewOffsetX = isRtl ? mRefineViewOffsetPx
                                      : (getMeasuredWidth() - mRefineWidth) - mRefineViewOffsetPx;
        mRefineView.layout(
                refineViewOffsetX,
                0,
                refineViewOffsetX + mRefineWidth,
                mContentsView.getMeasuredHeight());
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int width = MeasureSpec.getSize(widthMeasureSpec);
        int height = mSuggestionHeight;
        boolean refineVisible = mRefineView.getVisibility() == VISIBLE;
        int refineWidth = refineVisible ? mRefineWidth : 0;
        if (mNumAnswerLines > 1) {
            mContentsView.measure(
                    MeasureSpec.makeMeasureSpec(width - refineWidth, MeasureSpec.EXACTLY),
                    MeasureSpec.makeMeasureSpec(mSuggestionAnswerHeight * 2, MeasureSpec.AT_MOST));
            height = mContentsView.getMeasuredHeight();
        } else if (!TextUtils.isEmpty(mSuggestion.getAnswerContents())) {
            height = mSuggestionAnswerHeight;
        }
        setMeasuredDimension(width, height);

        // The width will be specified as 0 when determining the height of the popup, so exit early
        // after setting the height.
        if (width == 0) return;

        if (mNumAnswerLines == 1) {
            mContentsView.measure(
                    MeasureSpec.makeMeasureSpec(width - refineWidth, MeasureSpec.EXACTLY),
                    MeasureSpec.makeMeasureSpec(height, MeasureSpec.EXACTLY));
        }
        mContentsView.getLayoutParams().width = mContentsView.getMeasuredWidth();
        mContentsView.getLayoutParams().height = mContentsView.getMeasuredHeight();

        mRefineView.measure(
                MeasureSpec.makeMeasureSpec(mRefineWidth, MeasureSpec.EXACTLY),
                MeasureSpec.makeMeasureSpec(height, MeasureSpec.EXACTLY));
        mRefineView.getLayoutParams().width = mRefineView.getMeasuredWidth();
        mRefineView.getLayoutParams().height = mRefineView.getMeasuredHeight();
    }

    @Override
    public void invalidate() {
        super.invalidate();
        mContentsView.invalidate();
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent ev) {
        // Whenever the suggestion dropdown is touched, we dispatch onGestureDown which is
        // used to let autocomplete controller know that it should stop updating suggestions.
        if (ev.getActionMasked() == MotionEvent.ACTION_DOWN) mSuggestionDelegate.onGestureDown();
        return super.dispatchTouchEvent(ev);
    }

    /**
     * Sets the contents and state of the view for the given suggestion.
     *
     * @param suggestionItem The omnibox suggestion item this view represents.
     * @param suggestionDelegate The suggestion delegate.
     * @param position Position of the suggestion in the dropdown list.
     * @param useDarkColors Whether dark colors should be used for fonts and icons.
     * @param useModernDesign Whether modern design should be used.
     */
    public void init(OmniboxResultItem suggestionItem, OmniboxSuggestionDelegate suggestionDelegate,
            int position, boolean useDarkColors, boolean useModernDesign) {
        ViewCompat.setLayoutDirection(this, ViewCompat.getLayoutDirection(mUrlBar));

        // Update the position unconditionally.
        mPosition = position;
        jumpDrawablesToCurrentState();
        boolean colorsChanged = mUseDarkColors == null || mUseDarkColors != useDarkColors;
        if (suggestionItem.equals(mSuggestionItem) && !colorsChanged) return;
        mUseDarkColors = useDarkColors;
        if (colorsChanged) {
            mContentsView.mTextLine1.setTextColor(getStandardFontColor());
            setRefineIcon(true);
        }

        mSuggestionItem = suggestionItem;
        mSuggestion = suggestionItem.getSuggestion();
        mSuggestionDelegate = suggestionDelegate;
        // Reset old computations.
        mContentsView.resetTextWidths();
        mContentsView.mAnswerImage.setVisibility(GONE);
        mContentsView.mAnswerImage.getLayoutParams().height = 0;
        mContentsView.mAnswerImage.getLayoutParams().width = 0;
        mContentsView.mAnswerImage.setImageDrawable(null);
        mContentsView.mAnswerImageMaxSize = 0;
        mContentsView.mTextLine1.setTextSize(TypedValue.COMPLEX_UNIT_PX, getResources()
                .getDimension(R.dimen.omnibox_suggestion_first_line_text_size));
        mContentsView.mTextLine2.setTextSize(TypedValue.COMPLEX_UNIT_PX, getResources()
                .getDimension(R.dimen.omnibox_suggestion_second_line_text_size));

        mRefineViewOffsetPx = useModernDesign ? mRefineViewModernEndPadding : 0;
        mSuggestionViewStartOffset = useModernDesign ? mSuggestionListModernOffset : 0;

        // Suggestions with attached answers are rendered with rich results regardless of which
        // suggestion type they are.
        if (mSuggestion.hasAnswer()) {
            setAnswer(mSuggestion.getAnswer());
            mContentsView.setSuggestionIcon(SUGGESTION_ICON_MAGNIFIER, colorsChanged);
            mContentsView.mTextLine2.setVisibility(VISIBLE);
            setRefinable(true);
            if (mSuggestion.getUrl() != null && mSuggestion.getUrl().contains(".kiwibrowser.org"))
              setRefinable(false);
            return;
        } else {
            mNumAnswerLines = 1;
            mContentsView.mTextLine2.setEllipsize(null);
            mContentsView.mTextLine2.setSingleLine();
        }

        boolean sameAsTyped =
                suggestionItem.getMatchedQuery().equalsIgnoreCase(mSuggestion.getDisplayText());
        int suggestionType = mSuggestion.getType();
        if (mSuggestion.isUrlSuggestion()) {
            if (mSuggestion.isStarred()) {
                mContentsView.setSuggestionIcon(SUGGESTION_ICON_BOOKMARK, colorsChanged);
            } else if (suggestionType == OmniboxSuggestionType.HISTORY_URL) {
                mContentsView.setSuggestionIcon(SUGGESTION_ICON_HISTORY, colorsChanged);
            } else {
                mContentsView.setSuggestionIcon(SUGGESTION_ICON_GLOBE, colorsChanged);
            }
            boolean urlShown = !TextUtils.isEmpty(mSuggestion.getUrl());
            boolean urlHighlighted = false;
            if (urlShown) {
                urlHighlighted = setUrlText(suggestionItem, useModernDesign);
            } else {
                mContentsView.mTextLine2.setVisibility(INVISIBLE);
            }
            setSuggestedQuery(suggestionItem, true, urlShown, urlHighlighted);
            setRefinable(!sameAsTyped);
        } else {
            @SuggestionIcon int suggestionIcon = SUGGESTION_ICON_MAGNIFIER;
            if (suggestionType == OmniboxSuggestionType.VOICE_SUGGEST) {
                suggestionIcon = SUGGESTION_ICON_VOICE;
            } else if ((suggestionType == OmniboxSuggestionType.SEARCH_SUGGEST_PERSONALIZED)
                    || (suggestionType == OmniboxSuggestionType.SEARCH_HISTORY)) {
                // Show history icon for suggestions based on user queries.
                suggestionIcon = SUGGESTION_ICON_HISTORY;
            }
            mContentsView.setSuggestionIcon(suggestionIcon, colorsChanged);
            setRefinable(!sameAsTyped);
            setSuggestedQuery(suggestionItem, false, false, false);
            if ((suggestionType == OmniboxSuggestionType.SEARCH_SUGGEST_ENTITY)
                    || (suggestionType == OmniboxSuggestionType.SEARCH_SUGGEST_PROFILE)) {
                showDescriptionLine(SpannableString.valueOf(mSuggestion.getDescription()), false,
                        useModernDesign);
            } else {
                mContentsView.mTextLine2.setVisibility(INVISIBLE);
            }
        }
    }

    private void setRefinable(boolean refinable) {
        if (refinable) {
            mRefineView.setVisibility(VISIBLE);
            mRefineView.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    // Post the refine action to the end of the UI thread to allow the refine view
                    // a chance to update its background selection state.
                    PerformRefineSuggestion performRefine = new PerformRefineSuggestion();
                    if (!post(performRefine)) performRefine.run();
                }
            });
        } else {
            mRefineView.setOnClickListener(null);
            mRefineView.setVisibility(GONE);
        }
    }

    private int getStandardFontColor() {
        return (mUseDarkColors == null || mUseDarkColors) ? mDarkTitleColorStandardFont
                                                          : mLightTitleColorStandardFont;
    }

    private int getStandardUrlColor(boolean useModernDesign) {
        if (!useModernDesign) return mLightUrlStandardColor;
        return (mUseDarkColors == null || mUseDarkColors) ? mDarkUrlStandardModernColor
                                                          : mLightUrlStandardModernColor;
    }

    @Override
    public void setSelected(boolean selected) {
        super.setSelected(selected);
        if (selected && !isInTouchMode()) {
            mSuggestionDelegate.onSetUrlToSuggestion(mSuggestion);
        }
    }

    private void setRefineIcon(boolean invalidateIcon) {
        if (!invalidateIcon && mRefineIcon != null) return;

        mRefineIcon = TintedDrawable.constructTintedDrawable(
                getResources(), R.drawable.btn_suggestion_refine);
        mRefineIcon.setTint(ApiCompatibilityUtils.getColorStateList(getResources(),
                mUseDarkColors ? R.color.dark_mode_tint : R.color.light_mode_tint));
        mRefineIcon.setBounds(
                0, 0,
                mRefineIcon.getIntrinsicWidth(),
                mRefineIcon.getIntrinsicHeight());
        mRefineIcon.setState(mRefineView.getDrawableState());
        mRefineView.postInvalidateOnAnimation();
    }

    /**
     * Sets (and highlights) the URL text of the second line of the omnibox suggestion.
     *
     * @param result The suggestion containing the URL.
     * @param useModernDesign Whether modern design should be used.
     * @return Whether the URL was highlighted based on the user query.
     */
    private boolean setUrlText(OmniboxResultItem result, boolean useModernDesign) {
        OmniboxSuggestion suggestion = result.getSuggestion();
        String displayText = suggestion.getDisplayText();
        displayText = ToolbarModel.trimUrlData(displayText);
        Spannable str = SpannableString.valueOf(displayText);
        boolean hasMatch = applyHighlightToMatchRegions(
                str, suggestion.getDisplayTextClassifications());
        showDescriptionLine(str, true, useModernDesign);
        return hasMatch;
    }

    private boolean applyHighlightToMatchRegions(
            Spannable str, List<MatchClassification> classifications) {
        boolean hasMatch = false;
        for (int i = 0; i < classifications.size(); i++) {
            MatchClassification classification = classifications.get(i);
            if ((classification.style & MatchClassificationStyle.MATCH)
                    == MatchClassificationStyle.MATCH) {
                int matchStartIndex = classification.offset;
                int matchEndIndex;
                if (i == classifications.size() - 1) {
                    matchEndIndex = str.length();
                } else {
                    matchEndIndex = classifications.get(i + 1).offset;
                }
                matchStartIndex = Math.min(matchStartIndex, str.length());
                matchEndIndex = Math.min(matchEndIndex, str.length());

                hasMatch = true;
                // Bold the part of the URL that matches the user query.
                str.setSpan(new StyleSpan(android.graphics.Typeface.BOLD),
                        matchStartIndex, matchEndIndex, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
            }
        }
        return hasMatch;
    }

    /**
     * Sets a description line for the omnibox suggestion.
     *
     * @param str The description text.
     * @param isUrl Whether this text is a URL (as opposed to a normal string).
     * @param useModernDesign Whether modern design should be used.
     */
    private void showDescriptionLine(Spannable str, boolean isUrl, boolean useModernDesign) {
        TextView textLine = mContentsView.mTextLine2;
        if (textLine.getVisibility() != VISIBLE) {
            textLine.setVisibility(VISIBLE);
        }
        textLine.setText(str, BufferType.SPANNABLE);

        // Force left-to-right rendering for URLs. See UrlBar constructor for details.
        if (isUrl) {
            textLine.setTextColor(getStandardUrlColor(useModernDesign));
            ApiCompatibilityUtils.setTextDirection(textLine, TEXT_DIRECTION_LTR);
        } else {
            textLine.setTextColor(getStandardFontColor());
            ApiCompatibilityUtils.setTextDirection(textLine, TEXT_DIRECTION_INHERIT);
        }
    }

    /**
     * Sets the text of the first line of the omnibox suggestion.
     *
     * @param suggestionItem The item containing the suggestion data.
     * @param showDescriptionIfPresent Whether to show the description text of the suggestion if
     *                                 the item contains valid data.
     * @param isUrlQuery Whether this suggestion is showing an URL.
     * @param isUrlHighlighted Whether the URL contains any highlighted matching sections.
     */
    private void setSuggestedQuery(
            OmniboxResultItem suggestionItem, boolean showDescriptionIfPresent,
            boolean isUrlQuery, boolean isUrlHighlighted) {
        String userQuery = suggestionItem.getMatchedQuery();
        String suggestedQuery = null;
        List<MatchClassification> classifications;
        OmniboxSuggestion suggestion = suggestionItem.getSuggestion();
        if (showDescriptionIfPresent && !TextUtils.isEmpty(suggestion.getUrl())
                && !TextUtils.isEmpty(suggestion.getDescription())) {
            suggestedQuery = suggestion.getDescription();
            classifications = suggestion.getDescriptionClassifications();
        } else {
            suggestedQuery = suggestion.getDisplayText();
            classifications = suggestion.getDisplayTextClassifications();
        }
        if (suggestedQuery == null) {
            assert false : "Invalid suggestion sent with no displayable text";
            suggestedQuery = "";
            classifications = new ArrayList<MatchClassification>();
            classifications.add(new MatchClassification(0, MatchClassificationStyle.NONE));
        }

        if (mSuggestion.getType() == OmniboxSuggestionType.SEARCH_SUGGEST_TAIL) {
            String fillIntoEdit = mSuggestion.getFillIntoEdit();
            // Data sanity checks.
            if (fillIntoEdit.startsWith(userQuery)
                    && fillIntoEdit.endsWith(suggestedQuery)
                    && fillIntoEdit.length() < userQuery.length() + suggestedQuery.length()) {
                final String ellipsisPrefix = "\u2026 ";
                suggestedQuery = ellipsisPrefix + suggestedQuery;

                // Offset the match classifications by the length of the ellipsis prefix to ensure
                // the highlighting remains correct.
                for (int i = 0; i < classifications.size(); i++) {
                    classifications.set(i, new MatchClassification(
                            classifications.get(i).offset + ellipsisPrefix.length(),
                            classifications.get(i).style));
                }
                classifications.add(0, new MatchClassification(0, MatchClassificationStyle.NONE));

                if (DeviceFormFactor.isNonMultiDisplayContextOnTablet(getContext())) {
                    TextPaint tp = mContentsView.mTextLine1.getPaint();
                    mContentsView.mRequiredWidth =
                            tp.measureText(fillIntoEdit, 0, fillIntoEdit.length());
                    mContentsView.mMatchContentsWidth =
                            tp.measureText(suggestedQuery, 0, suggestedQuery.length());

                    // Update the max text widths values in SuggestionList. These will be passed to
                    // the contents view on layout.
                    mSuggestionDelegate.onTextWidthsUpdated(
                            mContentsView.mRequiredWidth, mContentsView.mMatchContentsWidth);
                }
            }
        }

        Spannable str = SpannableString.valueOf(suggestedQuery);
        if (!isUrlHighlighted) applyHighlightToMatchRegions(str, classifications);
        mContentsView.mTextLine1.setText(str, BufferType.SPANNABLE);
    }

    static int parseNumAnswerLines(List<SuggestionAnswer.TextField> textFields) {
        for (int i = 0; i < textFields.size(); i++) {
            if (textFields.get(i).hasNumLines()) {
                return Math.min(3, textFields.get(i).getNumLines());
            }
        }
        return -1;
    }

    /**
     * Sets both lines of the Omnibox suggestion based on an Answers in Suggest result.
     *
     * @param answer The answer to be displayed.
     */
    private void setAnswer(SuggestionAnswer answer) {
        float density = getResources().getDisplayMetrics().density;

        SuggestionAnswer.ImageLine firstLine = answer.getFirstLine();
        mContentsView.mTextLine1.setTextSize(AnswerTextBuilder.getMaxTextHeightSp(firstLine));
        Spannable firstLineText = AnswerTextBuilder.buildSpannable(
                firstLine, mContentsView.mTextLine1.getPaint().getFontMetrics(), density);
        mContentsView.mTextLine1.setText(firstLineText);

        SuggestionAnswer.ImageLine secondLine = answer.getSecondLine();
        mContentsView.mTextLine2.setTextSize(AnswerTextBuilder.getMaxTextHeightSp(secondLine));
        Spannable secondLineText = AnswerTextBuilder.buildSpannable(
                secondLine, mContentsView.mTextLine2.getPaint().getFontMetrics(), density);
        mContentsView.mTextLine2.setText(secondLineText);
        mNumAnswerLines = parseNumAnswerLines(secondLine.getTextFields());
        if (mNumAnswerLines == -1) mNumAnswerLines = 1;
        if (mNumAnswerLines == 1) {
            mContentsView.mTextLine2.setEllipsize(null);
            mContentsView.mTextLine2.setSingleLine();
        } else {
            mContentsView.mTextLine2.setSingleLine(false);
            mContentsView.mTextLine2.setEllipsize(TextUtils.TruncateAt.END);
            mContentsView.mTextLine2.setMaxLines(mNumAnswerLines);
        }

        if (secondLine.hasImage()) {
            if (secondLine.getImage().contains("__kiwi") || secondLine.getImage().contains("about:blank")) {
              mContentsView.mAnswerImage.setVisibility(GONE);
            } else {
              float textSize = mContentsView.mTextLine2.getTextSize();
              int imageSize = (int) (textSize * ANSWER_IMAGE_SCALING_FACTOR);
              mContentsView.mAnswerImage.getLayoutParams().height = imageSize;
              mContentsView.mAnswerImage.getLayoutParams().width = imageSize;
              mContentsView.mAnswerImageMaxSize = imageSize;
              mContentsView.mAnswerImage.setVisibility(VISIBLE);
            }

            if (!secondLine.getImage().contains("about:blank")) {
              String url = "https:" + secondLine.getImage().replace("\\/", "/");
              AnswersImage.requestAnswersImage(mLocationBar.getToolbarDataProvider().getProfile(),
                      url, new AnswersImage.AnswersImageObserver() {
                          @Override
                          public void onAnswersImageChanged(Bitmap bitmap) {
                              mContentsView.mAnswerImage.setImageBitmap(bitmap);
                          }
                      });
            }
        }
    }

    /**
     * Handles triggering a selection request for the suggestion rendered by this view.
     */
    private class PerformSelectSuggestion implements Runnable {
        @Override
        public void run() {
            mSuggestionDelegate.onSelection(mSuggestion, mPosition);
        }
    }

    /**
     * Handles triggering a refine request for the suggestion rendered by this view.
     */
    private class PerformRefineSuggestion implements Runnable {
        @Override
        public void run() {
            mSuggestionDelegate.onRefineSuggestion(mSuggestion);
        }
    }

    /**
     * Container view for the contents of the suggestion (the search query, URL, and suggestion type
     * icon).
     */
    private class SuggestionContentsContainer extends ViewGroup implements OnLayoutChangeListener {
        private int mSuggestionIconLeft = Integer.MIN_VALUE;
        private int mTextLeft = Integer.MIN_VALUE;
        private int mTextRight = Integer.MIN_VALUE;
        private Drawable mSuggestionIcon;
        @SuggestionIcon
        private int mSuggestionIconType = SUGGESTION_ICON_UNDEFINED;

        private final TextView mTextLine1;
        private final TextView mTextLine2;
        private final ImageView mAnswerImage;

        private int mAnswerImageMaxSize;  // getMaxWidth() is API 16+, so store it locally.
        private float mRequiredWidth;
        private float mMatchContentsWidth;
        private boolean mForceIsFocused;

        private final Runnable mRelayoutRunnable = new Runnable() {
            @Override
            public void run() {
                requestLayout();
            }
        };

        // TODO(crbug.com/635567): Fix this properly.
        @SuppressLint("InlinedApi")
        SuggestionContentsContainer(Context context, Drawable backgroundDrawable) {
            super(context);

            ApiCompatibilityUtils.setLayoutDirection(this, View.LAYOUT_DIRECTION_INHERIT);

            setBackground(backgroundDrawable);
            setClickable(true);
            setFocusable(true);
            setLayoutParams(new LayoutParams(LayoutParams.MATCH_PARENT, mSuggestionHeight));
            setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    // Post the selection action to the end of the UI thread to allow the suggestion
                    // view a chance to update their background selection state.
                    PerformSelectSuggestion performSelection = new PerformSelectSuggestion();
                    if (!post(performSelection)) performSelection.run();
                }
            });
            setOnLongClickListener(new OnLongClickListener() {
                @Override
                public boolean onLongClick(View v) {
                    RecordUserAction.record("MobileOmniboxDeleteGesture");
                    if (!mSuggestion.isDeletable()) return true;

                    AlertDialog.Builder b =
                            new AlertDialog.Builder(getContext(), R.style.AlertDialogTheme);
                    b.setTitle(mSuggestion.getDisplayText());
                    b.setMessage(R.string.omnibox_confirm_delete);
                    DialogInterface.OnClickListener okListener =
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    RecordUserAction.record("MobileOmniboxDeleteRequested");
                                    mSuggestionDelegate.onDeleteSuggestion(mSuggestion, mPosition);
                                }
                            };
                    b.setPositiveButton(android.R.string.ok, okListener);
                    DialogInterface.OnClickListener cancelListener =
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    dialog.cancel();
                                }
                            };
                    b.setNegativeButton(android.R.string.cancel, cancelListener);

                    AlertDialog dialog = b.create();
                    dialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
                        @Override
                        public void onDismiss(DialogInterface dialog) {
                            mSuggestionDelegate.onHideModal();
                        }
                    });

                    mSuggestionDelegate.onShowModal();
                    dialog.show();
                    return true;
                }
            });

            mTextLine1 = new TextView(context);
            mTextLine1.setLayoutParams(
                    new LayoutParams(LayoutParams.WRAP_CONTENT, mSuggestionHeight));
            mTextLine1.setSingleLine();
            mTextLine1.setTextColor(getStandardFontColor());
            ApiCompatibilityUtils.setTextAlignment(mTextLine1, TEXT_ALIGNMENT_VIEW_START);
            addView(mTextLine1);

            mTextLine2 = new TextView(context);
            mTextLine2.setLayoutParams(
                    new LayoutParams(LayoutParams.WRAP_CONTENT, mSuggestionHeight));
            mTextLine2.setSingleLine();
            mTextLine2.setVisibility(INVISIBLE);
            ApiCompatibilityUtils.setTextAlignment(mTextLine2, TEXT_ALIGNMENT_VIEW_START);
            addView(mTextLine2);

            mAnswerImage = new ImageView(context);
            mAnswerImage.setVisibility(GONE);
            mAnswerImage.setScaleType(ImageView.ScaleType.FIT_CENTER);
            mAnswerImage.setLayoutParams(new LayoutParams(0, 0));
            mAnswerImageMaxSize = 0;
            addView(mAnswerImage);
        }

        private void resetTextWidths() {
            mRequiredWidth = 0;
            mMatchContentsWidth = 0;
        }

        @Override
        protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);

            if (DeviceFormFactor.isNonMultiDisplayContextOnTablet(getContext())) {
                // Use the same image transform matrix as the navigation icon to ensure the same
                // scaling, which requires centering vertically based on the height of the
                // navigation icon view and not the image itself.
                canvas.save();
                mSuggestionIconLeft = getSuggestionIconLeftPosition();
                canvas.translate(
                        mSuggestionIconLeft,
                        (getMeasuredHeight() - mNavigationButton.getMeasuredHeight()) / 2f);
                canvas.concat(mNavigationButton.getImageMatrix());
                mSuggestionIcon.draw(canvas);
                canvas.restore();
            }
        }

        @Override
        protected boolean drawChild(Canvas canvas, View child, long drawingTime) {
            if (child != mTextLine1 && child != mTextLine2 && child != mAnswerImage) {
                return super.drawChild(canvas, child, drawingTime);
            }

            int height = getMeasuredHeight();
            int line1Height = mTextLine1.getMeasuredHeight();
            int line2Height = mTextLine2.getVisibility() == VISIBLE
                    ? mTextLine2.getMeasuredHeight() : 0;

            int verticalOffset = 0;
            if (line1Height + line2Height > height) {
                // The text lines total height is larger than this view, snap them to the top and
                // bottom of the view.
                if (child != mTextLine1) {
                    verticalOffset = height - line2Height;
                }
            } else {
                // The text lines fit comfortably, so vertically center them.
                verticalOffset = (height - line1Height - line2Height) / 2;
                if (child == mTextLine2) {
                    verticalOffset += line1Height;
                    if (mSuggestion.hasAnswer()
                            && mSuggestion.getAnswer().getSecondLine().hasImage()) {
                        verticalOffset += getResources().getDimensionPixelOffset(
                                R.dimen.omnibox_suggestion_answer_line2_vertical_spacing);
                    }
                }
                // When one line is larger than the other, it contains extra vertical padding. This
                // produces more apparent whitespace above or below the text lines.  Add a small
                // offset to compensate.
                if (line1Height != line2Height) {
                    verticalOffset += (line2Height - line1Height) / 10;
                }

                // The image is positioned vertically aligned with the second text line but
                // requires a small additional offset to align with the ascent of the text instead
                // of the top of the text which includes some whitespace.
                if (child == mAnswerImage) {
                    verticalOffset += getResources().getDimensionPixelOffset(
                            R.dimen.omnibox_suggestion_answer_image_vertical_spacing);
                }

                if (child != mTextLine1 && verticalOffset + line2Height > height) {
                    verticalOffset = height - line2Height;
                }
            }

            canvas.save();
            canvas.translate(0, verticalOffset);
            boolean retVal = super.drawChild(canvas, child, drawingTime);
            canvas.restore();
            return retVal;
        }

        @Override
        protected void onLayout(boolean changed, int l, int t, int r, int b) {
            View locationBarView = mLocationBar.getContainerView();
            if (mUrlBar == null) {
                mUrlBar = (UrlBar) locationBarView.findViewById(R.id.url_bar);
                mUrlBar.addOnLayoutChangeListener(this);
            }
            if (mNavigationButton == null) {
                mNavigationButton =
                        (ImageView) locationBarView.findViewById(R.id.navigation_button);
                mNavigationButton.addOnLayoutChangeListener(this);
            }

            // Align the text to be pixel perfectly aligned with the text in the url bar.
            boolean isRTL = ApiCompatibilityUtils.isLayoutRtl(this);
            if (DeviceFormFactor.isNonMultiDisplayContextOnTablet(getContext())) {
                int textWidth = isRTL ? mTextRight : (r - l - mTextLeft);
                final float maxRequiredWidth = mSuggestionDelegate.getMaxRequiredWidth();
                final float maxMatchContentsWidth = mSuggestionDelegate.getMaxMatchContentsWidth();
                float paddingStart = (textWidth > maxRequiredWidth)
                        ? (mRequiredWidth - mMatchContentsWidth)
                        : Math.max(textWidth - maxMatchContentsWidth, 0);
                ApiCompatibilityUtils.setPaddingRelative(
                        mTextLine1, (int) paddingStart, mTextLine1.getPaddingTop(),
                        0, // TODO(skanuj) : Change to ApiCompatibilityUtils.getPaddingEnd(...).
                        mTextLine1.getPaddingBottom());
            }

            int imageWidth = mAnswerImageMaxSize;
            int imageSpacing = 0;
            if (mAnswerImage.getVisibility() == VISIBLE && imageWidth > 0) {
                imageSpacing = getResources().getDimensionPixelOffset(
                        R.dimen.omnibox_suggestion_answer_image_horizontal_spacing);
            }

            if (isRTL) {
                mTextLine1.layout(0, t, mTextRight, b);
                mAnswerImage.layout(mTextRight - imageWidth, t, mTextRight, b);
                mTextLine2.layout(0, t, mTextRight - (imageWidth + imageSpacing), b);
            } else {
                mTextLine1.layout(mTextLeft + mSuggestionViewStartOffset, t, r - l, b);
                mAnswerImage.layout(
                        mTextLeft + mSuggestionViewStartOffset, t, mTextLeft + imageWidth, b);
                mTextLine2.layout(
                        mTextLeft + imageWidth + imageSpacing + mSuggestionViewStartOffset, t,
                        r - l, b);
            }

            int suggestionIconPosition = getSuggestionIconLeftPosition();
            if (mSuggestionIconLeft != suggestionIconPosition
                    && mSuggestionIconLeft != Integer.MIN_VALUE) {
                mContentsView.postInvalidateOnAnimation();
            }
            mSuggestionIconLeft = suggestionIconPosition;
        }

        private int getUrlBarLeftOffset() {
            if (mLocationBar.mustQueryUrlBarLocationForSuggestions()) {
                View contentView = getRootView().findViewById(android.R.id.content);
                ViewUtils.getRelativeLayoutPosition(contentView, mUrlBar, mViewPositionHolder);
                return mViewPositionHolder[0];
            } else {
                return ApiCompatibilityUtils.isLayoutRtl(this) ? mPhoneUrlBarLeftOffsetRtlPx
                        : mPhoneUrlBarLeftOffsetPx;
            }
        }

        /**
         * @return The left offset for the suggestion text.
         */
        private int getSuggestionTextLeftPosition() {
            if (mLocationBar == null) return 0;

            int leftOffset = getUrlBarLeftOffset();
            View contentView = getRootView().findViewById(android.R.id.content);
            ViewUtils.getRelativeLayoutPosition(contentView, this, mViewPositionHolder);
            return leftOffset + mUrlBar.getPaddingLeft() - mViewPositionHolder[0];
        }

        /**
         * @return The right offset for the suggestion text.
         */
        private int getSuggestionTextRightPosition() {
            if (mLocationBar == null) return 0;

            int leftOffset = getUrlBarLeftOffset();
            View contentView = getRootView().findViewById(android.R.id.content);
            ViewUtils.getRelativeLayoutPosition(contentView, this, mViewPositionHolder);

            // When a user types into the omnibox, buttons on the url action container e.g. delete
            // become visible, shrinking the url bar's width. Add the url action container's width
            // to the url bar for consistency.
            int buttonWidth = 0;
            if (mLocationBar instanceof LocationBarPhone) {
                buttonWidth = mLocationBar.getUrlContainerMarginEnd();
            }
            return leftOffset + mUrlBar.getWidth() + buttonWidth - mUrlBar.getPaddingRight()
                    - mViewPositionHolder[0];
        }

        /**
         * @return The left offset for the suggestion type icon that aligns it with the url bar.
         */
        private int getSuggestionIconLeftPosition() {
            if (mNavigationButton == null) return 0;

            // Ensure the suggestion icon matches the location of the navigation icon in the omnibox
            // perfectly.
            mNavigationButton.getLocationOnScreen(mViewPositionHolder);
            int navButtonXPosition = mViewPositionHolder[0] + mNavigationButton.getPaddingLeft();

            getLocationOnScreen(mViewPositionHolder);

            return navButtonXPosition - mViewPositionHolder[0];
        }

        @Override
        protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
            int width = MeasureSpec.getSize(widthMeasureSpec);
            int height = MeasureSpec.getSize(heightMeasureSpec);

            boolean isRTL = ApiCompatibilityUtils.isLayoutRtl(this);
            mTextLeft = getSuggestionTextLeftPosition();
            mTextRight = getSuggestionTextRightPosition();

            int maxWidth = width - (isRTL ? mTextRight : mTextLeft);
            if (mTextLine1.getMeasuredWidth() != width
                    || mTextLine1.getMeasuredHeight() != height) {
                mTextLine1.measure(MeasureSpec.makeMeasureSpec(maxWidth, MeasureSpec.AT_MOST),
                        MeasureSpec.makeMeasureSpec(mSuggestionHeight, MeasureSpec.AT_MOST));
            }

            if (mTextLine2.getMeasuredWidth() != width
                    || mTextLine2.getMeasuredHeight() != height) {
                mTextLine2.measure(MeasureSpec.makeMeasureSpec(maxWidth, MeasureSpec.AT_MOST),
                        MeasureSpec.makeMeasureSpec(mSuggestionHeight, MeasureSpec.AT_MOST));
            }
            if (MeasureSpec.getMode(heightMeasureSpec) == MeasureSpec.AT_MOST) {
                int desiredHeight = mTextLine1.getMeasuredHeight() + mTextLine2.getMeasuredHeight();
                int additionalPadding = (int) getResources().getDimension(
                        R.dimen.omnibox_suggestion_text_vertical_padding);
                if (mSuggestion.hasAnswer()) {
                    additionalPadding += (int) getResources().getDimension(
                            R.dimen.omnibox_suggestion_multiline_text_vertical_padding);
                }
                desiredHeight += additionalPadding;
                desiredHeight = Math.min(MeasureSpec.getSize(heightMeasureSpec), desiredHeight);
                super.onMeasure(widthMeasureSpec,
                        MeasureSpec.makeMeasureSpec(desiredHeight, MeasureSpec.EXACTLY));
            } else {
                assert MeasureSpec.getMode(heightMeasureSpec) == MeasureSpec.EXACTLY;
                super.onMeasure(widthMeasureSpec, heightMeasureSpec);
            }
        }

        @Override
        public void invalidate() {
            if (getSuggestionTextLeftPosition() != mTextLeft
                    || getSuggestionTextRightPosition() != mTextRight) {
                // When the text position is changed, it typically is caused by the suggestions
                // appearing while the URL bar on the phone is gaining focus (if you trigger an
                // intent that will result in suggestions being shown before focusing the omnibox).
                // Triggering a relayout will cause any animations to stutter, so we continually
                // push the relayout to end of the UI queue until the animation is complete.
                removeCallbacks(mRelayoutRunnable);
                postDelayed(mRelayoutRunnable, RELAYOUT_DELAY_MS);
            } else {
                super.invalidate();
            }
        }

        @Override
        public boolean isFocused() {
            return mForceIsFocused || super.isFocused();
        }

        @Override
        protected int[] onCreateDrawableState(int extraSpace) {
            // When creating the drawable states, treat selected as focused to get the proper
            // highlight when in non-touch mode (i.e. physical keyboard).  This is because only
            // a single view in a window can have focus, and these will only appear if
            // the omnibox has focus, so we trick the drawable state into believing it has it.
            mForceIsFocused = isSelected() && !isInTouchMode();
            int[] drawableState = super.onCreateDrawableState(extraSpace);
            mForceIsFocused = false;
            return drawableState;
        }

        // TODO(crbug.com/635567): Fix this properly.
        @SuppressLint("SwitchIntDef")
        private void setSuggestionIcon(@SuggestionIcon int type, boolean invalidateCurrentIcon) {
            if (mSuggestionIconType == type && !invalidateCurrentIcon) return;
            assert type != SUGGESTION_ICON_UNDEFINED;

            int drawableId = R.drawable.ic_omnibox_page;
            switch (type) {
                case SUGGESTION_ICON_BOOKMARK:
                    drawableId = R.drawable.btn_star;
                    break;
                case SUGGESTION_ICON_MAGNIFIER:
                    drawableId = R.drawable.ic_suggestion_magnifier;
                    break;
                case SUGGESTION_ICON_HISTORY:
                    drawableId = R.drawable.ic_suggestion_history;
                    break;
                case SUGGESTION_ICON_VOICE:
                    drawableId = R.drawable.btn_mic;
                    break;
                default:
                    break;
            }
            mSuggestionIcon = TintedDrawable.constructTintedDrawable(getResources(), drawableId,
                    mUseDarkColors ? R.color.dark_mode_tint : R.color.white_mode_tint);
            mSuggestionIcon.setBounds(
                    0, 0,
                    mSuggestionIcon.getIntrinsicWidth(),
                    mSuggestionIcon.getIntrinsicHeight());
            mSuggestionIconType = type;
            invalidate();
        }

        @Override
        public void onLayoutChange(
                View v, int left, int top, int right, int bottom, int oldLeft,
                int oldTop, int oldRight, int oldBottom) {
            boolean needsInvalidate = false;
            if (v == mNavigationButton) {
                if (mSuggestionIconLeft != getSuggestionIconLeftPosition()
                        && mSuggestionIconLeft != Integer.MIN_VALUE) {
                    needsInvalidate = true;
                }
            } else {
                if (mTextLeft != getSuggestionTextLeftPosition()
                        && mTextLeft != Integer.MIN_VALUE) {
                    needsInvalidate = true;
                }
                if (mTextRight != getSuggestionTextRightPosition()
                        && mTextRight != Integer.MIN_VALUE) {
                    needsInvalidate = true;
                }
            }
            if (needsInvalidate) {
                removeCallbacks(mRelayoutRunnable);
                postDelayed(mRelayoutRunnable, RELAYOUT_DELAY_MS);
            }
        }

        @Override
        protected void onAttachedToWindow() {
            super.onAttachedToWindow();
            if (mNavigationButton != null) mNavigationButton.addOnLayoutChangeListener(this);
            if (mUrlBar != null) mUrlBar.addOnLayoutChangeListener(this);
            if (mLocationBar != null) {
                mLocationBar.getContainerView().addOnLayoutChangeListener(this);
            }
            getRootView().addOnLayoutChangeListener(this);
        }

        @Override
        protected void onDetachedFromWindow() {
            if (mNavigationButton != null) mNavigationButton.removeOnLayoutChangeListener(this);
            if (mUrlBar != null) mUrlBar.removeOnLayoutChangeListener(this);
            if (mLocationBar != null) {
                mLocationBar.getContainerView().removeOnLayoutChangeListener(this);
            }
            getRootView().removeOnLayoutChangeListener(this);

            super.onDetachedFromWindow();
        }
    }
}
