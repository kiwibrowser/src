// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.omnibox;

import static org.chromium.chrome.browser.toolbar.ToolbarPhone.URL_FOCUS_CHANGE_ANIMATION_DURATION_MS;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.app.Activity;
import android.content.Context;
import android.content.res.ColorStateList;
import android.content.res.Configuration;
import android.graphics.drawable.Drawable;
import android.os.Parcelable;
import android.os.SystemClock;
import android.provider.Settings;
import android.support.annotation.DrawableRes;
import android.support.annotation.IntDef;
import android.text.Editable;
import android.text.InputType;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.SparseArray;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.ViewStub;
import android.view.inputmethod.InputMethodManager;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.CommandLine;
import org.chromium.base.Log;
import org.chromium.base.ObserverList;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.metrics.RecordUserAction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.NativePage;
import org.chromium.chrome.browser.WindowDelegate;
import org.chromium.chrome.browser.locale.LocaleManager;
import org.chromium.chrome.browser.ntp.NativePageFactory;
import org.chromium.chrome.browser.ntp.NewTabPage;
import org.chromium.chrome.browser.ntp.NewTabPage.FakeboxDelegate;
import org.chromium.chrome.browser.ntp.NewTabPageUma;
import org.chromium.chrome.browser.omnibox.AutocompleteController.OnSuggestionsReceivedListener;
import org.chromium.chrome.browser.omnibox.OmniboxResultsAdapter.OmniboxResultItem;
import org.chromium.chrome.browser.omnibox.OmniboxResultsAdapter.OmniboxSuggestionDelegate;
import org.chromium.chrome.browser.omnibox.geo.GeolocationHeader;
import org.chromium.chrome.browser.page_info.PageInfoController;
import org.chromium.chrome.browser.preferences.privacy.PrivacyPreferencesManager;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.search_engines.TemplateUrlService;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.toolbar.ToolbarActionModeCallback;
import org.chromium.chrome.browser.toolbar.ToolbarDataProvider;
import org.chromium.chrome.browser.toolbar.ToolbarPhone;
import org.chromium.chrome.browser.util.ColorUtils;
import org.chromium.chrome.browser.util.FeatureUtilities;
import org.chromium.chrome.browser.util.KeyNavigationUtil;
import org.chromium.chrome.browser.widget.FadingBackgroundView;
import org.chromium.chrome.browser.widget.TintedDrawable;
import org.chromium.chrome.browser.widget.TintedImageButton;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheet;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.UiUtils;
import org.chromium.ui.base.DeviceFormFactor;
import org.chromium.ui.base.PageTransition;
import org.chromium.ui.base.WindowAndroid;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.List;

/**
 * This class represents the location bar where the user types in URLs and
 * search terms.
 */
public class LocationBarLayout
        extends FrameLayout implements OnClickListener, OnSuggestionsReceivedListener, LocationBar,
                                       FakeboxDelegate, FadingBackgroundView.FadingViewObserver,
                                       LocationBarVoiceRecognitionHandler.Delegate {
    private static final String TAG = "cr_LocationBar";

    // Delay triggering the omnibox results upon key press to allow the location bar to repaint
    // with the new characters.
    private static final long OMNIBOX_SUGGESTION_START_DELAY_MS = 30;

    protected ImageView mNavigationButton;
    protected TintedImageButton mSecurityButton;
    protected TextView mVerboseStatusTextView;
    protected TintedImageButton mDeleteButton;
    protected TintedImageButton mMicButton;
    protected UrlBar mUrlBar;
    private final boolean mIsTablet;

    /** A handle to the bottom sheet for chrome home. */
    protected BottomSheet mBottomSheet;

    private AutocompleteController mAutocomplete;

    protected ToolbarDataProvider mToolbarDataProvider;
    private ObserverList<UrlFocusChangeListener> mUrlFocusChangeListeners = new ObserverList<>();

    protected boolean mNativeInitialized;

    private final List<Runnable> mDeferredNativeRunnables = new ArrayList<Runnable>();

    // The type of the navigation button currently showing.
    private NavigationButtonType mNavigationButtonType;

    // The type of the security icon currently active.
    @DrawableRes
    private int mSecurityIconResource;

    private final OmniboxResultsAdapter mSuggestionListAdapter;
    private OmniboxSuggestionsList mSuggestionList;

    private final List<OmniboxResultItem> mSuggestionItems;

    /**
     * The text shown in the URL bar (user text + inline autocomplete) after the most recent set of
     * omnibox suggestions was received. When the user presses enter in the omnibox, this value is
     * compared to the URL bar text to determine whether the first suggestion is still valid.
     */
    private String mUrlTextAfterSuggestionsReceived;

    private boolean mIgnoreOmniboxItemSelection = true;

    private String mOriginalUrl = "";

    private WindowAndroid mWindowAndroid;
    private WindowDelegate mWindowDelegate;

    private Runnable mRequestSuggestions;

    private ViewGroup mOmniboxResultsContainer;
    protected FadingBackgroundView mFadingView;

    private boolean mSuggestionsShown;
    private boolean mUrlHasFocus;
    protected boolean mUrlFocusChangeInProgress;
    private boolean mUrlFocusedFromFakebox;
    private boolean mUrlFocusedWithoutAnimations;

    private boolean mVoiceSearchEnabled;

    // Set to true when the user has started typing new input in the omnibox, set to false
    // when the omnibox loses focus or becomes empty.
    private boolean mHasStartedNewOmniboxEditSession;
    // The timestamp (using SystemClock.elapsedRealtime()) at the point when the user started
    // modifying the omnibox with new input.
    private long mNewOmniboxEditSessionTimestamp = -1;

    @LocationBarButtonType private int mLocationBarButtonType;

    private AnimatorSet mLocationBarIconActiveAnimator;
    private AnimatorSet mSecurityButtonShowAnimator;
    private AnimatorSet mNavigationIconShowAnimator;

    private OmniboxPrerender mOmniboxPrerender;

    private boolean mSuggestionModalShown;
    private boolean mUseDarkColors;
    private boolean mIsEmphasizingHttpsScheme;

    private Runnable mShowSuggestions;

    private boolean mShowCachedZeroSuggestResults;

    private DeferredOnSelectionRunnable mDeferredOnSelection;

    private boolean mOmniboxVoiceSearchAlwaysVisible;
    protected float mUrlFocusChangePercent;
    protected LinearLayout mUrlActionContainer;

    protected LocationBarVoiceRecognitionHandler mVoiceRecognitionHandler;

    private static abstract class DeferredOnSelectionRunnable implements Runnable {
        protected final OmniboxSuggestion mSuggestion;
        protected final int mPosition;
        protected boolean mShouldLog;

        public DeferredOnSelectionRunnable(OmniboxSuggestion suggestion, int position) {
            this.mSuggestion = suggestion;
            this.mPosition = position;
        }

        /**
         * Set whether the selection matches with native results for logging to make sense.
         * @param log Whether the selection should be logged in native code.
         */
        public void setShouldLog(boolean log) {
            mShouldLog = log;
        }

        /**
         * @return Whether the selection should be logged in native code.
         */
        public boolean shouldLog() {
            return mShouldLog;
        }
    }

    /**
     * Class to handle input from a hardware keyboard when the focus is on the URL bar. In
     * particular, handle navigating the suggestions list from the keyboard.
     */
    private final class UrlBarKeyListener implements OnKeyListener {
        @Override
        public boolean onKey(View v, int keyCode, KeyEvent event) {
            if (KeyNavigationUtil.isGoDown(event)
                    && mSuggestionList != null
                    && mSuggestionList.isShown()) {
                int suggestionCount = mSuggestionListAdapter.getCount();
                if (mSuggestionList.getSelectedItemPosition() < suggestionCount - 1) {
                    if (suggestionCount > 0) mIgnoreOmniboxItemSelection = false;
                } else {
                    // Do not pass down events when the last item is already selected as it will
                    // dismiss the suggestion list.
                    return true;
                }

                if (mSuggestionList.getSelectedItemPosition()
                        == ListView.INVALID_POSITION) {
                    // When clearing the selection after a text change, state is not reset
                    // correctly so hitting down again will cause it to start from the previous
                    // selection point. We still have to send the key down event to let the list
                    // view items take focus, but then we select the first item explicitly.
                    boolean result = mSuggestionList.onKeyDown(keyCode, event);
                    mSuggestionList.setSelection(0);
                    return result;
                } else {
                    return mSuggestionList.onKeyDown(keyCode, event);
                }
            } else if (KeyNavigationUtil.isGoUp(event)
                    && mSuggestionList != null
                    && mSuggestionList.isShown()) {
                if (mSuggestionList.getSelectedItemPosition() != 0
                        && mSuggestionListAdapter.getCount() > 0) {
                    mIgnoreOmniboxItemSelection = false;
                }
                return mSuggestionList.onKeyDown(keyCode, event);
            } else if (KeyNavigationUtil.isGoRight(event)
                    && mSuggestionList != null
                    && mSuggestionList.isShown()
                    && mSuggestionList.getSelectedItemPosition()
                            != ListView.INVALID_POSITION) {
                OmniboxResultItem selectedItem =
                        (OmniboxResultItem) mSuggestionListAdapter.getItem(
                                mSuggestionList.getSelectedItemPosition());
                // Set the UrlBar text to empty, so that it will trigger a text change when we
                // set the text to the suggestion again.
                setUrlBarText(UrlBarData.EMPTY);
                mUrlBar.setText(selectedItem.getSuggestion().getFillIntoEdit());
                mSuggestionList.setSelection(0);
                mUrlBar.setSelection(mUrlBar.getText().length());
                return true;
            } else if (KeyNavigationUtil.isEnter(event)
                    && LocationBarLayout.this.getVisibility() == VISIBLE) {
                UiUtils.hideKeyboard(mUrlBar);
                final String urlText = mUrlBar.getTextWithAutocomplete();
                if (mNativeInitialized) {
                    findMatchAndLoadUrl(urlText);
                } else {
                    mDeferredNativeRunnables.add(new Runnable() {
                        @Override
                        public void run() {
                            findMatchAndLoadUrl(urlText);
                        }
                    });
                }
                return true;
            } else if (keyCode == KeyEvent.KEYCODE_BACK) {
                if (event.getAction() == KeyEvent.ACTION_DOWN && event.getRepeatCount() == 0) {
                    // Tell the framework to start tracking this event.
                    getKeyDispatcherState().startTracking(event, this);
                    return true;
                } else if (event.getAction() == KeyEvent.ACTION_UP) {
                    getKeyDispatcherState().handleUpEvent(event);
                    if (event.isTracking() && !event.isCanceled()) {
                        backKeyPressed();
                        return true;
                    }
                }
            } else if (keyCode == KeyEvent.KEYCODE_ESCAPE) {
                if (event.getAction() == KeyEvent.ACTION_DOWN && event.getRepeatCount() == 0) {
                    revertChanges();
                    return true;
                }
            }
            return false;
        }

        private void findMatchAndLoadUrl(String urlText) {
            int suggestionMatchPosition;
            OmniboxSuggestion suggestionMatch;
            boolean skipOutOfBoundsCheck = false;

            if (mSuggestionList != null
                    && mSuggestionList.isShown()
                    && mSuggestionList.getSelectedItemPosition()
                    != ListView.INVALID_POSITION) {
                // Bluetooth keyboard case: the user highlighted a suggestion with the arrow
                // keys, then pressed enter.
                suggestionMatchPosition = mSuggestionList.getSelectedItemPosition();
                OmniboxResultItem selectedItem =
                        (OmniboxResultItem) mSuggestionListAdapter.getItem(suggestionMatchPosition);
                suggestionMatch = selectedItem.getSuggestion();
            } else if (!mSuggestionItems.isEmpty()
                    && urlText.equals(mUrlTextAfterSuggestionsReceived)) {
                // Common case: the user typed something, received suggestions, then pressed enter.
                suggestionMatch = mSuggestionItems.get(0).getSuggestion();
                suggestionMatchPosition = 0;
            } else {
                // Less common case: there are no valid omnibox suggestions. This can happen if the
                // user tapped the URL bar to dismiss the suggestions, then pressed enter. This can
                // also happen if the user presses enter before any suggestions have been received
                // from the autocomplete controller.
                suggestionMatch = mAutocomplete.classify(urlText, mUrlFocusedFromFakebox);
                suggestionMatchPosition = 0;
                // Classify matches don't propagate to java, so skip the OOB check.
                skipOutOfBoundsCheck = true;

                // If urlText couldn't be classified, bail.
                if (suggestionMatch == null) return;
            }

            String suggestionMatchUrl = updateSuggestionUrlIfNeeded(suggestionMatch,
                        suggestionMatchPosition, skipOutOfBoundsCheck);
            loadUrlFromOmniboxMatch(suggestionMatchUrl, suggestionMatchPosition, suggestionMatch);
        }
    }

    /**
     * Specifies the types of buttons shown to signify different types of navigation elements.
     */
    protected enum NavigationButtonType {
        PAGE,
        MAGNIFIER,
        EMPTY,
    }

    /** Specifies which button should be shown in location bar, if any. */
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({BUTTON_TYPE_NONE, BUTTON_TYPE_SECURITY_ICON, BUTTON_TYPE_NAVIGATION_ICON})
    public @interface LocationBarButtonType {}
    /** No button should be shown. */
    public static final int BUTTON_TYPE_NONE = 0;
    /** Security button should be shown (includes offline icon). */
    public static final int BUTTON_TYPE_SECURITY_ICON = 1;
    /** Navigation button should be shown. */
    public static final int BUTTON_TYPE_NAVIGATION_ICON = 2;

    public LocationBarLayout(Context context, AttributeSet attrs) {
        this(context, attrs, R.layout.location_bar);
    }

    public LocationBarLayout(Context context, AttributeSet attrs, int layoutId) {
        super(context, attrs);

        LayoutInflater.from(context).inflate(layoutId, this, true);

        mNavigationButton = (ImageView) findViewById(R.id.navigation_button);
        assert mNavigationButton != null : "Missing navigation type view.";
        mIsTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(context);
        mNavigationButtonType = mIsTablet ? NavigationButtonType.PAGE : NavigationButtonType.EMPTY;

        mSecurityButton = (TintedImageButton) findViewById(R.id.security_button);
        mSecurityIconResource = 0;

        mVerboseStatusTextView = (TextView) findViewById(R.id.location_bar_verbose_status);

        mDeleteButton = (TintedImageButton) findViewById(R.id.delete_button);

        mUrlBar = (UrlBar) findViewById(R.id.url_bar);
        // The HTC Sense IME will attempt to autocomplete words in the Omnibox when Prediction is
        // enabled.  We want to disable this feature and rely on the Omnibox's implementation.
        // Their IME does not respect ~TYPE_TEXT_FLAG_AUTO_COMPLETE nor any of the other InputType
        // options I tried, but setting the filter variation prevents it.  Sadly, it also removes
        // the .com button, but the prediction was buggy as it would autocomplete words even when
        // typing at the beginning of the omnibox text when other content was present (messing up
        // what was previously there).  See bug: http://b/issue?id=6200071
        String defaultIme = Settings.Secure.getString(getContext().getContentResolver(),
                Settings.Secure.DEFAULT_INPUT_METHOD);
        if (defaultIme != null && defaultIme.contains("com.htc.android.htcime")) {
            mUrlBar.setInputType(mUrlBar.getInputType() | InputType.TYPE_TEXT_VARIATION_FILTER);
        }
        mUrlBar.setDelegate(this);

        mSuggestionItems = new ArrayList<OmniboxResultItem>();
        mSuggestionListAdapter = new OmniboxResultsAdapter(getContext(), this, mSuggestionItems);

        mMicButton = (TintedImageButton) findViewById(R.id.mic_button);

        mUrlActionContainer = (LinearLayout) findViewById(R.id.url_action_container);

        mVoiceRecognitionHandler = new LocationBarVoiceRecognitionHandler(this);
    }

    @Override
    public boolean isSuggestionsListShown() {
        return mSuggestionsShown;
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        mUrlBar.setCursorVisible(false);

        mLocationBarButtonType = BUTTON_TYPE_NONE;
        mNavigationButton.setVisibility(INVISIBLE);
        mSecurityButton.setVisibility(INVISIBLE);

        setLayoutTransition(null);

        AnimatorListenerAdapter iconChangeAnimatorListener = new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                if (animation == mSecurityButtonShowAnimator) {
                    mNavigationButton.setVisibility(INVISIBLE);
                } else if (animation == mNavigationIconShowAnimator) {
                    mSecurityButton.setVisibility(INVISIBLE);
                }
            }

            @Override
            public void onAnimationStart(Animator animation) {
                if (animation == mSecurityButtonShowAnimator) {
                    mSecurityButton.setVisibility(VISIBLE);
                } else if (animation == mNavigationIconShowAnimator) {
                    mNavigationButton.setVisibility(VISIBLE);
                }
            }
        };

        mSecurityButtonShowAnimator = new AnimatorSet();
        mSecurityButtonShowAnimator.playTogether(
                ObjectAnimator.ofFloat(mNavigationButton, ALPHA, 0),
                ObjectAnimator.ofFloat(mSecurityButton, ALPHA, 1));
        mSecurityButtonShowAnimator.setDuration(URL_FOCUS_CHANGE_ANIMATION_DURATION_MS);
        mSecurityButtonShowAnimator.addListener(iconChangeAnimatorListener);

        mNavigationIconShowAnimator = new AnimatorSet();
        mNavigationIconShowAnimator.playTogether(
                ObjectAnimator.ofFloat(mNavigationButton, ALPHA, 1),
                ObjectAnimator.ofFloat(mSecurityButton, ALPHA, 0));
        mNavigationIconShowAnimator.setDuration(URL_FOCUS_CHANGE_ANIMATION_DURATION_MS);
        mNavigationIconShowAnimator.addListener(iconChangeAnimatorListener);

        mUrlBar.setOnKeyListener(new UrlBarKeyListener());

        // mLocationBar's direction is tied to this UrlBar's text direction. Icons inside the
        // location bar, e.g. lock, refresh, X, should be reversed if UrlBar's text is RTL.
        mUrlBar.setUrlDirectionListener(new UrlBar.UrlDirectionListener() {
            @Override
            public void onUrlDirectionChanged(int layoutDirection) {
                ApiCompatibilityUtils.setLayoutDirection(LocationBarLayout.this, layoutDirection);

                if (mSuggestionList != null) {
                    mSuggestionList.updateSuggestionsLayoutDirection(layoutDirection);
                }
            }
        });
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        updateLayoutParams();
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        boolean retVal = super.dispatchKeyEvent(event);
        if (retVal && mUrlHasFocus && mUrlFocusedWithoutAnimations
                && event.getAction() == KeyEvent.ACTION_DOWN && event.isPrintingKey()
                && event.hasNoModifiers()) {
            handleUrlFocusAnimation(mUrlHasFocus);
        }
        return retVal;
    }

    @Override
    protected void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        if (mUrlHasFocus && mUrlFocusedWithoutAnimations
                && newConfig.keyboard != Configuration.KEYBOARD_QWERTY) {
            // If we lose the hardware keyboard and the focus animations were not run, then the
            // user has not typed any text, so we will just clear the focus instead.
            setUrlBarFocus(false);
        }
    }

    @Override
    public void initializeControls(WindowDelegate windowDelegate,
            WindowAndroid windowAndroid) {
        mWindowDelegate = windowDelegate;
        mWindowAndroid = windowAndroid;

        mUrlBar.setWindowDelegate(windowDelegate);
        // If the user focused the omnibox prior to the native libraries being initialized,
        // autocomplete will not always be enabled, so we force it enabled in that case.
        mUrlBar.setIgnoreTextChangesForAutocomplete(false);
        mAutocomplete = new AutocompleteController(this);
    }

    /**
     * Sets to show cached zero suggest results. This will start both caching zero suggest results
     * in shared preferences and also attempt to show them when appropriate without needing native
     * initialization. See {@link LocationBarLayout#showCachedZeroSuggestResultsIfAvailable()} for
     * showing the loaded results before native initialization.
     * @param showCachedZeroSuggestResults Whether cached zero suggest should be shown.
     */
    public void setShowCachedZeroSuggestResults(boolean showCachedZeroSuggestResults) {
        mShowCachedZeroSuggestResults = showCachedZeroSuggestResults;
        if (mShowCachedZeroSuggestResults) mAutocomplete.startCachedZeroSuggest();
    }

    /**
     * Signals the omnibox to shows the cached zero suggest results if they have been loaded from
     * cache successfully.
     */
    public void showCachedZeroSuggestResultsIfAvailable() {
        if (!mShowCachedZeroSuggestResults || mSuggestionList == null) return;
        setSuggestionsListVisibility(true);
        mSuggestionList.updateLayoutParams();
    }

    /**
     * @param focusable Whether the url bar should be focusable.
     */
    public void setUrlBarFocusable(boolean focusable) {
        if (mUrlBar == null) return;
        mUrlBar.setFocusable(focusable);
        mUrlBar.setFocusableInTouchMode(focusable);
    }

    /**
     * @return The WindowDelegate for the LocationBar. This should be used for all Window related
     * state queries.
     */
    protected WindowDelegate getWindowDelegate() {
        return mWindowDelegate;
    }

    /**
     * Handles native dependent initialization for this class.
     */
    @Override
    public void onNativeLibraryReady() {
        mNativeInitialized = true;

        mSecurityButton.setOnClickListener(this);
        mNavigationButton.setOnClickListener(this);
        mVerboseStatusTextView.setOnClickListener(this);
        updateMicButtonState();
        mDeleteButton.setOnClickListener(this);
        mMicButton.setOnClickListener(this);

        mOmniboxPrerender = new OmniboxPrerender();

        for (Runnable deferredRunnable : mDeferredNativeRunnables) {
            post(deferredRunnable);
        }
        mDeferredNativeRunnables.clear();

        updateVisualsForState();

        mOmniboxVoiceSearchAlwaysVisible =
                ChromeFeatureList.isEnabled(ChromeFeatureList.OMNIBOX_VOICE_SEARCH_ALWAYS_VISIBLE);
        updateMicButtonVisibility(mUrlFocusChangePercent);
    }

    /**
     * @return Whether or not to animate icon changes.
     */
    protected boolean shouldAnimateIconChanges() {
        return mUrlHasFocus;
    }

    /**
     * Sets the autocomplete controller for the location bar.
     *
     * @param controller The controller that will handle autocomplete/omnibox suggestions.
     * @note Only used for testing.
     */
    @VisibleForTesting
    public void setAutocompleteController(AutocompleteController controller) {
        if (mAutocomplete != null) stopAutocomplete(true);
        mAutocomplete = controller;
    }

    /**
     * Updates the profile used for generating autocomplete suggestions.
     * @param profile The profile to be used.
     */
    @Override
    public void setAutocompleteProfile(Profile profile) {
        // This will only be called once at least one tab exists, and the tab model is told to
        // update its state. During Chrome initialization the tab model update happens after the
        // call to onNativeLibraryReady, so this assert will not fire.
        assert mNativeInitialized
                : "Setting Autocomplete Profile before native side initialized";
        mAutocomplete.setProfile(profile);
        mOmniboxPrerender.initializeForProfile(profile);
    }

    @LocationBarButtonType private int getLocationBarButtonToShow() {
        // The navigation icon type is only applicable on tablets.  While smaller form factors do
        // not have an icon visible to the user when the URL is focused, BUTTON_TYPE_NONE is not
        // returned as it will trigger an undesired jump during the animation as it attempts to
        // hide the icon.
        if (mUrlHasFocus && mIsTablet) return BUTTON_TYPE_NAVIGATION_ICON;

        return mToolbarDataProvider.getSecurityIconResource(mIsTablet) != 0
                ? BUTTON_TYPE_SECURITY_ICON
                : BUTTON_TYPE_NONE;
    }

    private void changeLocationBarIcon() {
        if (mLocationBarIconActiveAnimator != null && mLocationBarIconActiveAnimator.isRunning()) {
            mLocationBarIconActiveAnimator.cancel();
        }

        mLocationBarButtonType = getLocationBarButtonToShow();

        View viewToBeShown = null;
        switch (mLocationBarButtonType) {
            case BUTTON_TYPE_SECURITY_ICON:
                viewToBeShown = mSecurityButton;
                mLocationBarIconActiveAnimator = mSecurityButtonShowAnimator;
                break;
            case BUTTON_TYPE_NAVIGATION_ICON:
                viewToBeShown = mNavigationButton;
                mLocationBarIconActiveAnimator = mNavigationIconShowAnimator;
                break;
            case BUTTON_TYPE_NONE:
            default:
                mLocationBarIconActiveAnimator = null;
                return;
        }

        if (viewToBeShown.getVisibility() == VISIBLE && viewToBeShown.getAlpha() == 1) {
            return;
        }
        if (shouldAnimateIconChanges()) {
            mLocationBarIconActiveAnimator.setDuration(URL_FOCUS_CHANGE_ANIMATION_DURATION_MS);
        } else {
            mLocationBarIconActiveAnimator.setDuration(0);
        }
        mLocationBarIconActiveAnimator.start();
    }

    /** Focuses the current page. */
    private void focusCurrentTab() {
        if (mToolbarDataProvider.hasTab()) getCurrentTab().requestFocus();
    }

    @Override
    public void setUrlBarFocus(boolean shouldBeFocused) {
        if (shouldBeFocused) {
            mUrlBar.requestFocus();
        } else {
            mUrlBar.clearFocus();
        }
    }

    @Override
    public boolean isUrlBarFocused() {
        return mUrlHasFocus;
    }

    @Override
    public void selectAll() {
        mUrlBar.selectAll();
    }

    @Override
    public void revertChanges() {
        if (!mUrlHasFocus) {
            setUrlToPageUrl();
        } else {
            String currentUrl = mToolbarDataProvider.getCurrentUrl();
            if (NativePageFactory.isNativePageUrl(currentUrl, mToolbarDataProvider.isIncognito())) {
                setUrlBarText(UrlBarData.EMPTY);
            } else {
                setUrlBarText(mToolbarDataProvider.getUrlBarData());
                selectAll();
            }
            hideSuggestions();
            UiUtils.hideKeyboard(mUrlBar);
        }
    }

    @Override
    public long getFirstUrlBarFocusTime() {
        return mUrlBar.getFirstFocusTime();
    }

    /**
     * @return Whether the URL focus change is taking place, e.g. a focus animation is running on
     *         a phone device.
     */
    protected boolean isUrlFocusChangeInProgress() {
        return mUrlFocusChangeInProgress;
    }

    /**
     * @param inProgress Whether a URL focus change is taking place.
     */
    protected void setUrlFocusChangeInProgress(boolean inProgress) {
        mUrlFocusChangeInProgress = inProgress;
        if (!inProgress) {
            updateButtonVisibility();
        }
    }

    /**
     * Updates the omnibox text selection when focused. When displaying a query in the omnibox,
     * we want to move the cursor to the end of the search terms to more easily refine the search,
     * and when displaying a URL we select all.
     */
    private void updateFocusedUrlBarSelection() {
        if (!mUrlBar.hasFocus()) return;

        if (mToolbarDataProvider.shouldDisplaySearchTerms()) {
            mUrlBar.setSelection(mUrlBar.getText().length());
        } else {
            selectAll();
        }
    }

    /**
     * Triggered when the URL input field has gained or lost focus.
     * @param hasFocus Whether the URL field has gained focus.
     */
    public void onUrlFocusChange(boolean hasFocus) {
        mUrlHasFocus = hasFocus;
        updateButtonVisibility();
        updateNavigationButton();

        if (hasFocus) {
            if (mNativeInitialized) RecordUserAction.record("FocusLocation");
            UrlBarData urlBarData = mToolbarDataProvider.getUrlBarData();
            if (urlBarData.editingText == null || !setUrlBarText(urlBarData)) {
                mUrlBar.deEmphasizeUrl();
            }

            // Explicitly tell InputMethodManager that the url bar is focused before any callbacks
            // so that it updates the active view accordingly. Otherwise, it may fail to update
            // the correct active view if ViewGroup.addView() or ViewGroup.removeView() is called
            // to update a view that accepts text input.
            InputMethodManager imm = (InputMethodManager) mUrlBar.getContext().getSystemService(
                    Context.INPUT_METHOD_SERVICE);
            imm.viewClicked(mUrlBar);
        } else {
            mUrlFocusedFromFakebox = false;
            mUrlFocusedWithoutAnimations = false;
            hideSuggestions();

            // Focus change caused by a close-tab may result in an invalid current tab.
            if (mToolbarDataProvider.hasTab()) {
                setUrlToPageUrl();
                emphasizeUrl();
            }

            // Moving focus away from UrlBar(EditText) to a non-editable focus holder, such as
            // ToolbarPhone, won't automatically hide keyboard app, but restart it with TYPE_NULL,
            // which will result in a visual glitch. Also, currently, we do not allow moving focus
            // directly from omnibox to web content's form field. Therefore, we hide keyboard on
            // focus blur indiscriminately here. Note that hiding keyboard may lower FPS of other
            // animation effects, but we found it tolerable in an experiment.
            InputMethodManager imm = (InputMethodManager) getContext().getSystemService(
                    Context.INPUT_METHOD_SERVICE);
            if (imm.isActive(mUrlBar)) imm.hideSoftInputFromWindow(getWindowToken(), 0, null);
        }

        if (mToolbarDataProvider.isUsingBrandColor()) updateVisualsForState();

        changeLocationBarIcon();
        updateVerboseStatusVisibility();
        updateLocationBarIconContainerVisibility();
        updateFocusedUrlBarSelection();
        mUrlBar.setCursorVisible(hasFocus);

        if (!mUrlFocusedWithoutAnimations) handleUrlFocusAnimation(hasFocus);

        if (hasFocus && mToolbarDataProvider.hasTab() && !mToolbarDataProvider.isIncognito()) {
            if (mNativeInitialized
                    && TemplateUrlService.getInstance().isDefaultSearchEngineGoogle()) {
                GeolocationHeader.primeLocationForGeoHeader();
            } else {
                mDeferredNativeRunnables.add(new Runnable() {
                    @Override
                    public void run() {
                        if (TemplateUrlService.getInstance().isDefaultSearchEngineGoogle()) {
                            GeolocationHeader.primeLocationForGeoHeader();
                        }
                    }
                });
            }
        }

        if (mNativeInitialized) {
            startZeroSuggest();
        } else {
            mDeferredNativeRunnables.add(new Runnable() {
                @Override
                public void run() {
                    if (TextUtils.isEmpty(mUrlBar.getTextWithAutocomplete())) {
                        startZeroSuggest();
                    }
                }
            });
        }

        if (!hasFocus) {
            mHasStartedNewOmniboxEditSession = false;
            mNewOmniboxEditSessionTimestamp = -1;
        }
    }

    /**
     * Handle and run any necessary animations that are triggered off focusing the UrlBar.
     * @param hasFocus Whether focus was gained.
     */
    protected void handleUrlFocusAnimation(boolean hasFocus) {
        if (hasFocus) mUrlFocusedWithoutAnimations = false;
        for (UrlFocusChangeListener listener : mUrlFocusChangeListeners) {
            listener.onUrlFocusChange(hasFocus);
        }

        updateOmniboxResultsContainer();
        if (hasFocus) updateFadingBackgroundView(true);
    }

    /**
     * Make a zero suggest request if:
     * - Native is loaded.
     * - The URL bar has focus.
     * - The current tab is not incognito.
     */
    private void startZeroSuggest() {
        // hasWindowFocus() can return true before onWindowFocusChanged has been called, so this
        // is an optimization, but not entirely reliable.  The underlying controller needs to also
        // ensure we do not double trigger zero query.
        if (!hasWindowFocus()) return;

        // Reset "edited" state in the omnibox if zero suggest is triggered -- new edits
        // now count as a new session.
        mHasStartedNewOmniboxEditSession = false;
        mNewOmniboxEditSessionTimestamp = -1;
        if (mNativeInitialized && mUrlHasFocus && mToolbarDataProvider.hasTab()) {
            mAutocomplete.startZeroSuggest(mToolbarDataProvider.getProfile(),
                    mUrlBar.getTextWithAutocomplete(), mToolbarDataProvider.getCurrentUrl(),
                    mToolbarDataProvider.getTitle(), mUrlFocusedFromFakebox);
        }
    }

    @Override
    public void onTextChangedForAutocomplete() {
        // crbug.com/764749
        Log.w(TAG, "onTextChangedForAutocomplete");
        cancelPendingAutocompleteStart();

        updateButtonVisibility();
        updateNavigationButton();

        if (!mHasStartedNewOmniboxEditSession && mNativeInitialized) {
            mAutocomplete.resetSession();
            mHasStartedNewOmniboxEditSession = true;
            mNewOmniboxEditSessionTimestamp = SystemClock.elapsedRealtime();
        }

        if (!isInTouchMode() && mSuggestionList != null) {
            mSuggestionList.setSelection(0);
        }

        stopAutocomplete(false);
        if (TextUtils.isEmpty(mUrlBar.getTextWithoutAutocomplete())) {
            // crbug.com/764749
            Log.w(TAG, "onTextChangedForAutocomplete: url is empty");
            hideSuggestions();
            startZeroSuggest();
        } else {
            assert mRequestSuggestions == null : "Multiple omnibox requests in flight.";
            mRequestSuggestions = new Runnable() {
                @Override
                public void run() {
                    String textWithoutAutocomplete = mUrlBar.getTextWithoutAutocomplete();
                    boolean preventAutocomplete = !mUrlBar.shouldAutocomplete();
                    mRequestSuggestions = null;

                    if (!mToolbarDataProvider.hasTab()) {
                        // crbug.com/764749
                        Log.w(TAG, "onTextChangedForAutocomplete: no tab");
                        return;
                    }

                    Profile profile = mToolbarDataProvider.getProfile();
                    int cursorPosition = -1;
                    if (mUrlBar.getSelectionStart() == mUrlBar.getSelectionEnd()) {
                        // Conveniently, if there is no selection, those two functions return -1,
                        // exactly the same value needed to pass to start() to indicate no cursor
                        // position.  Hence, there's no need to check for -1 here explicitly.
                        cursorPosition = mUrlBar.getSelectionStart();
                    }
                    mAutocomplete.start(profile, mToolbarDataProvider.getCurrentUrl(),
                            textWithoutAutocomplete, cursorPosition, preventAutocomplete,
                            mUrlFocusedFromFakebox);
                }
            };
            if (mNativeInitialized) {
                postDelayed(mRequestSuggestions, OMNIBOX_SUGGESTION_START_DELAY_MS);
            } else {
                mDeferredNativeRunnables.add(mRequestSuggestions);
            }
        }
    }

    @Override
    public void setDefaultTextEditActionModeCallback(ToolbarActionModeCallback callback) {
        mUrlBar.setCustomSelectionActionModeCallback(callback);
    }

    @Override
    public void requestUrlFocusFromFakebox(String pastedText) {
        mUrlFocusedFromFakebox = true;
        if (mUrlHasFocus && mUrlFocusedWithoutAnimations) {
            handleUrlFocusAnimation(mUrlHasFocus);
        } else {
            setUrlBarFocus(true);
        }

        if (pastedText != null) {
            // This must be happen after requestUrlFocus(), which changes the selection.
            mUrlBar.setUrl(UrlBarData.forNonUrlText(pastedText));
            mUrlBar.setSelection(mUrlBar.getText().length());
        }
    }

    @Override
    public boolean isCurrentPage(NativePage nativePage) {
        assert nativePage != null;
        return nativePage == mToolbarDataProvider.getNewTabPageForCurrentTab();
    }

    @Override
    public void showUrlBarCursorWithoutFocusAnimations() {
        if (mUrlHasFocus || mUrlFocusedFromFakebox) return;

        mUrlFocusedWithoutAnimations = true;
        setUrlBarFocus(true);
    }

    /**
     * Sets the toolbar that owns this LocationBar.
     */
    @Override
    public void setToolbarDataProvider(ToolbarDataProvider toolbarDataProvider) {
        mToolbarDataProvider = toolbarDataProvider;

        updateButtonVisibility();

        mUrlBar.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View v, final boolean hasFocus) {
                onUrlFocusChange(hasFocus);
            }
        });
    }

    @Override
    public void setBottomSheet(BottomSheet sheet) {
        mBottomSheet = sheet;
    }

    @Override
    public void addUrlFocusChangeListener(UrlFocusChangeListener listener) {
        mUrlFocusChangeListeners.addObserver(listener);
    }

    @Override
    public void removeUrlFocusChangeListener(UrlFocusChangeListener listener) {
        mUrlFocusChangeListeners.removeObserver(listener);
    }

    @Override
    public final ToolbarDataProvider getToolbarDataProvider() {
        return mToolbarDataProvider;
    }

    private static NavigationButtonType suggestionTypeToNavigationButtonType(
            OmniboxSuggestion suggestion) {
        if (suggestion.isUrlSuggestion()) {
            return NavigationButtonType.PAGE;
        } else {
            return NavigationButtonType.MAGNIFIER;
        }
    }

    // Updates the navigation button based on the URL string
    private void updateNavigationButton() {
        NavigationButtonType type = NavigationButtonType.EMPTY;
        if (mIsTablet && !mSuggestionItems.isEmpty()) {
            // If there are suggestions showing, show the icon for the default suggestion.
            type = suggestionTypeToNavigationButtonType(
                    mSuggestionItems.get(0).getSuggestion());
        } else if (mIsTablet) {
            type = NavigationButtonType.PAGE;
        }

        if (type != mNavigationButtonType) setNavigationButtonType(type);
    }

    /**
     * Updates the security icon displayed in the LocationBar.
     */
    @Override
    public void updateSecurityIcon() {
        @DrawableRes
        int id = mToolbarDataProvider.getSecurityIconResource(mIsTablet);
        if (id == 0) {
            mSecurityButton.setImageDrawable(null);
        } else {
            // ImageView#setImageResource is no-op if given resource is the current one.
            mSecurityButton.setImageResource(id);
            mSecurityButton.setTint(mToolbarDataProvider.getSecurityIconColorStateList());
        }

        updateVerboseStatusVisibility();

        boolean shouldEmphasizeHttpsScheme = shouldEmphasizeHttpsScheme();
        if (mSecurityIconResource == id && mIsEmphasizingHttpsScheme == shouldEmphasizeHttpsScheme
                && mLocationBarButtonType == getLocationBarButtonToShow()) {
            return;
        }
        mSecurityIconResource = id;

        changeLocationBarIcon();
        updateLocationBarIconContainerVisibility();

        emphasizeUrl();
        mIsEmphasizingHttpsScheme = shouldEmphasizeHttpsScheme;
    }

    private void emphasizeUrl() {
        if (mToolbarDataProvider.shouldDisplaySearchTerms()) return;
        mUrlBar.emphasizeUrl();
    }

    @Override
    public boolean shouldEmphasizeUrl() {
        return true;
    }

    @Override
    public boolean shouldEmphasizeHttpsScheme() {
        if (mToolbarDataProvider.isUsingBrandColor() || mToolbarDataProvider.isIncognito()) {
            return false;
        }
        return true;
    }

    /**
     * @return Whether the security button is currently being displayed.
     */
    @VisibleForTesting
    public boolean isSecurityButtonShown() {
        return mLocationBarButtonType == BUTTON_TYPE_SECURITY_ICON;
    }

    /**
     * @return The ID of the drawable currently shown in the security icon.
     */
    @VisibleForTesting
    @DrawableRes
    int getSecurityIconResourceId() {
        return mSecurityIconResource;
    }

    /**
     * Sets the type of the current navigation type and updates the UI to match it.
     * @param buttonType The type of navigation button to be shown.
     */
    private void setNavigationButtonType(NavigationButtonType buttonType) {
        if (!mIsTablet) return;
        switch (buttonType) {
            case PAGE:
                Drawable page = TintedDrawable.constructTintedDrawable(getResources(),
                        R.drawable.ic_omnibox_page,
                        mUseDarkColors ? R.color.dark_mode_tint : R.color.light_mode_tint);
                mNavigationButton.setImageDrawable(page);
                break;
            case MAGNIFIER:
                mNavigationButton.setImageResource(R.drawable.ic_omnibox_magnifier);
                break;
            case EMPTY:
                mNavigationButton.setImageDrawable(null);
                break;
            default:
                assert false;
        }

        if (mNavigationButton.getVisibility() != VISIBLE) {
            mNavigationButton.setVisibility(VISIBLE);
        }
        mNavigationButtonType = buttonType;

        updateLocationBarIconContainerVisibility();
    }

    /**
     * Update visibility of the verbose status based on the button type and focus state of the
     * omnibox.
     */
    private void updateVerboseStatusVisibility() {
        boolean verboseStatusVisible =
                !mUrlHasFocus && mToolbarDataProvider.shouldShowVerboseStatus();

        int verboseStatusVisibility = verboseStatusVisible ? VISIBLE : GONE;

        mVerboseStatusTextView.setTextColor(ApiCompatibilityUtils.getColor(getResources(),
                mUseDarkColors ? R.color.locationbar_status_color
                        : R.color.locationbar_status_color_light));
        mVerboseStatusTextView.setVisibility(verboseStatusVisibility);

        View separator = findViewById(R.id.location_bar_verbose_status_separator);
        separator.setBackgroundColor(ApiCompatibilityUtils.getColor(getResources(), mUseDarkColors
                ? R.color.locationbar_status_separator_color
                : R.color.locationbar_status_separator_color_light));
        separator.setVisibility(verboseStatusVisibility);

        findViewById(R.id.location_bar_verbose_status_extra_space)
                .setVisibility(verboseStatusVisibility);
    }

    /**
     * Update the visibility of the location bar icon container based on the state of the
     * security and navigation icons.
     */
    protected void updateLocationBarIconContainerVisibility() {
        @LocationBarButtonType
        int buttonToShow = getLocationBarButtonToShow();
        findViewById(R.id.location_bar_icon)
                .setVisibility(buttonToShow != BUTTON_TYPE_NONE ? VISIBLE : GONE);
    }

    /**
     * @return The margin to be applied to the URL bar based on the buttons currently visible next
     *         to it, used to avoid text overlapping the buttons and vice versa.
     */
    @Override
    public int getUrlContainerMarginEnd() {
        // When Chrome Home is enabled, the URL actions container slides out of view during the
        // URL defocus animation. Adding margin during this animation creates a hole.
        boolean addMarginForActionsContainer =
                mBottomSheet == null || !mUrlFocusChangeInProgress || isUrlBarFocused();

        int urlContainerMarginEnd = 0;
        if (addMarginForActionsContainer) {
            for (View childView : getUrlContainerViewsForMargin()) {
                ViewGroup.MarginLayoutParams childLayoutParams =
                        (ViewGroup.MarginLayoutParams) childView.getLayoutParams();
                urlContainerMarginEnd += childLayoutParams.width
                        + ApiCompatibilityUtils.getMarginStart(childLayoutParams)
                        + ApiCompatibilityUtils.getMarginEnd(childLayoutParams);
            }
        }
        return urlContainerMarginEnd;
    }

    /**
     * Updates the layout params for the location bar start aligned views.
     */
    protected void updateLayoutParams() {
        int startMargin = 0;
        for (int i = 0; i < getChildCount(); i++) {
            View childView = getChildAt(i);
            if (childView.getVisibility() != GONE) {
                LayoutParams childLayoutParams = (LayoutParams) childView.getLayoutParams();
                if (ApiCompatibilityUtils.getMarginStart(childLayoutParams) != startMargin) {
                    ApiCompatibilityUtils.setMarginStart(childLayoutParams, startMargin);
                    childView.setLayoutParams(childLayoutParams);
                }
                if (childView == mUrlBar) break;

                int widthMeasureSpec;
                int heightMeasureSpec;
                if (childLayoutParams.width == LayoutParams.WRAP_CONTENT) {
                    widthMeasureSpec = MeasureSpec.makeMeasureSpec(
                            getMeasuredWidth(), MeasureSpec.AT_MOST);
                } else if (childLayoutParams.width == LayoutParams.MATCH_PARENT) {
                    widthMeasureSpec = MeasureSpec.makeMeasureSpec(
                            getMeasuredWidth(), MeasureSpec.EXACTLY);
                } else {
                    widthMeasureSpec = MeasureSpec.makeMeasureSpec(
                            childLayoutParams.width, MeasureSpec.EXACTLY);
                }
                if (childLayoutParams.height == LayoutParams.WRAP_CONTENT) {
                    heightMeasureSpec = MeasureSpec.makeMeasureSpec(
                            getMeasuredHeight(), MeasureSpec.AT_MOST);
                } else if (childLayoutParams.height == LayoutParams.MATCH_PARENT) {
                    heightMeasureSpec = MeasureSpec.makeMeasureSpec(
                            getMeasuredHeight(), MeasureSpec.EXACTLY);
                } else {
                    heightMeasureSpec = MeasureSpec.makeMeasureSpec(
                            childLayoutParams.height, MeasureSpec.EXACTLY);
                }
                childView.measure(widthMeasureSpec, heightMeasureSpec);
                startMargin += childView.getMeasuredWidth();
            }
        }

        int urlContainerMarginEnd = getUrlContainerMarginEnd();
        LayoutParams urlLayoutParams = (LayoutParams) mUrlBar.getLayoutParams();
        if (ApiCompatibilityUtils.getMarginEnd(urlLayoutParams) != urlContainerMarginEnd) {
            ApiCompatibilityUtils.setMarginEnd(urlLayoutParams, urlContainerMarginEnd);
            mUrlBar.setLayoutParams(urlLayoutParams);
        }
    }

    /**
     * Gets the list of views that need to be taken into account for adding margin to the end of the
     * URL bar.
     *
     * @return A {@link List} of the views to be taken into account for URL bar margin to avoid
     *         overlapping text and buttons.
     */
    protected List<View> getUrlContainerViewsForMargin() {
        List<View> outList = new ArrayList<View>();
        if (mUrlActionContainer == null) return outList;

        for (int i = 0; i < mUrlActionContainer.getChildCount(); i++) {
            View childView = mUrlActionContainer.getChildAt(i);
            if (childView.getVisibility() != GONE) outList.add(childView);
        }
        return outList;
    }

    /**
     * @return Whether the delete button should be shown.
     */
    protected boolean shouldShowDeleteButton() {
        // Show the delete button at the end when the bar has focus and has some text.
        boolean hasText = !TextUtils.isEmpty(mUrlBar.getText());
        return hasText && (mUrlBar.hasFocus() || mUrlFocusChangeInProgress);
    }

    /**
     * Updates the display of the delete URL content button.
     */
    protected void updateDeleteButtonVisibility() {
        mDeleteButton.setVisibility(shouldShowDeleteButton() ? VISIBLE : GONE);
    }

    /**
     * @return The suggestion list popup containing the omnibox results (or
     *         null if it has not yet been created).
     */
    @VisibleForTesting
    public OmniboxSuggestionsList getSuggestionList() {
        return mSuggestionList;
    }

    @Override
    public boolean useModernDesign() {
        return FeatureUtilities.isChromeModernDesignEnabled();
    }

    /**
     * Initiates the mSuggestionListPopup.  Done on demand to not slow down
     * the initial inflation of the location bar.
     */
    private void initSuggestionList() {
        // Only called from onSuggestionsReceived(), which is a callback from a listener set up by
        // onNativeLibraryReady(), so this assert is safe.
        assert mNativeInitialized || mShowCachedZeroSuggestResults
                : "Trying to initialize native suggestions list before native init";
        if (mSuggestionList != null) return;
        mSuggestionListAdapter.setUseModernDesign(useModernDesign());

        OnLayoutChangeListener suggestionListResizer = new OnLayoutChangeListener() {
            @Override
            public void onLayoutChange(View v, int left, int top, int right, int bottom,
                    int oldLeft, int oldTop, int oldRight, int oldBottom) {
                // On ICS, this update does not take affect unless it is posted to the end of the
                // current message queue.
                post(new Runnable() {
                    @Override
                    public void run() {
                        if (mSuggestionList.isShown()) mSuggestionList.updateLayoutParams();
                    }
                });
            }
        };
        getRootView().findViewById(R.id.control_container)
                .addOnLayoutChangeListener(suggestionListResizer);

        OmniboxSuggestionsList.OmniboxSuggestionListEmbedder embedder =
                new OmniboxSuggestionsList.OmniboxSuggestionListEmbedder() {
                    @Override
                    public boolean useModernDesign() {
                        return LocationBarLayout.this.useModernDesign();
                    }

                    @Override
                    public boolean isTablet() {
                        return mIsTablet;
                    }

                    @Override
                    public boolean isIncognito() {
                        return mToolbarDataProvider.isIncognito();
                    }

                    @Override
                    public WindowDelegate getWindowDelegate() {
                        return mWindowDelegate;
                    }

                    @Override
                    public BottomSheet getBottomSheet() {
                        return mBottomSheet;
                    }

                    @Override
                    public View getAnchorView() {
                        return getRootView().findViewById(R.id.toolbar);
                    }
                };
        mSuggestionList = new OmniboxSuggestionsList(getContext(), embedder);

        // Ensure the results container is initialized and add the suggestion list to it.
        initOmniboxResultsContainer();
        mOmniboxResultsContainer.addView(mSuggestionList);

        // Start with visibility GONE to ensure that show() is called. http://crbug.com/517438
        mSuggestionList.setVisibility(GONE);
        mSuggestionList.setAdapter(mSuggestionListAdapter);
        mSuggestionList.setClipToPadding(false);
        mSuggestionListAdapter.setSuggestionDelegate(new OmniboxSuggestionDelegate() {
            @Override
            public void onSelection(OmniboxSuggestion suggestion, int position) {
                if (mShowCachedZeroSuggestResults && !mNativeInitialized) {
                    mDeferredOnSelection = new DeferredOnSelectionRunnable(suggestion, position) {
                        @Override
                        public void run() {
                            onSelection(this.mSuggestion, this.mPosition);
                        }
                    };
                    return;
                }
                String suggestionMatchUrl = updateSuggestionUrlIfNeeded(
                        suggestion, position, false);
                loadUrlFromOmniboxMatch(suggestionMatchUrl, position, suggestion);
                hideSuggestions();
                UiUtils.hideKeyboard(mUrlBar);
            }

            @Override
            public void onRefineSuggestion(OmniboxSuggestion suggestion) {
                stopAutocomplete(false);
                mUrlBar.setUrl(UrlBarData.forNonUrlText(suggestion.getFillIntoEdit()));
                mUrlBar.setSelection(mUrlBar.getText().length());
                if (suggestion.isUrlSuggestion()) {
                    RecordUserAction.record("MobileOmniboxRefineSuggestion.Url");
                } else {
                    RecordUserAction.record("MobileOmniboxRefineSuggestion.Search");
                }
            }

            @Override
            public void onSetUrlToSuggestion(OmniboxSuggestion suggestion) {
                if (mIgnoreOmniboxItemSelection) return;
                setUrlBarText(UrlBarData.forNonUrlText(suggestion.getFillIntoEdit()));
                mUrlBar.setSelection(mUrlBar.getText().length());
                mIgnoreOmniboxItemSelection = true;
            }

            @Override
            public void onDeleteSuggestion(OmniboxSuggestion suggestion, int position) {
                if (mAutocomplete != null) {
                    mAutocomplete.deleteSuggestion(position, suggestion.hashCode());
                }
            }

            @Override
            public void onGestureDown() {
                stopAutocomplete(false);
            }

            @Override
            public void onShowModal() {
                mSuggestionModalShown = true;
            }

            @Override
            public void onHideModal() {
                mSuggestionModalShown = false;
            }

            @Override
            public void onTextWidthsUpdated(float requiredWidth, float matchContentsWidth) {
                mSuggestionList.updateMaxTextWidths(requiredWidth, matchContentsWidth);
            }

            @Override
            public float getMaxRequiredWidth() {
                return mSuggestionList.getMaxRequiredWidth();
            }

            @Override
            public float getMaxMatchContentsWidth() {
                return mSuggestionList.getMaxMatchContentsWidth();
            }
        });
    }

    /**
     * Handles showing/hiding the suggestions list.
     * @param visible Whether the suggestions list should be visible.
     */
    protected void setSuggestionsListVisibility(final boolean visible) {
        mSuggestionsShown = visible;
        if (mSuggestionList != null) {
            final boolean isShowing = mSuggestionList.isShown();
            if (visible && !isShowing) {
                mIgnoreOmniboxItemSelection = true; // Reset to default value.
                mSuggestionList.show();
            } else if (!visible && isShowing) {
                mSuggestionList.setVisibility(GONE);
            }
        }
        updateOmniboxResultsContainer();
    }

    /**
     * Updates the URL we will navigate to from suggestion, if needed. This will update the search
     * URL to be of the corpus type if query in the omnibox is displayed and update aqs= parameter
     * on regular web search URLs.
     *
     * @param suggestion The chosen omnibox suggestion.
     * @param selectedIndex The index of the chosen omnibox suggestion.
     * @param skipCheck Whether to skip an out of bounds check.
     * @return The url to navigate to.
     */
    @SuppressWarnings("ReferenceEquality")
    private String updateSuggestionUrlIfNeeded(
            OmniboxSuggestion suggestion, int selectedIndex, boolean skipCheck) {
        // Only called once we have suggestions, and don't have a listener though which we can
        // receive suggestions until the native side is ready, so this is safe
        assert mNativeInitialized
                : "updateSuggestionUrlIfNeeded called before native initialization";

        String updatedUrl = null;
        if (suggestion.getType() != OmniboxSuggestionType.VOICE_SUGGEST) {
            int verifiedIndex = -1;
            if (!skipCheck) {
                if (mSuggestionItems.size() > selectedIndex
                        && mSuggestionItems.get(selectedIndex).getSuggestion() == suggestion) {
                    verifiedIndex = selectedIndex;
                } else {
                    // Underlying omnibox results may have changed since the selection was made,
                    // find the suggestion item, if possible.
                    for (int i = 0; i < mSuggestionItems.size(); i++) {
                        if (suggestion.equals(mSuggestionItems.get(i).getSuggestion())) {
                            verifiedIndex = i;
                            break;
                        }
                    }
                }
            }

            // If we do not have the suggestion as part of our results, skip the URL update.
            if (verifiedIndex == -1) return suggestion.getUrl();

            // TODO(mariakhomenko): Ideally we want to update match destination URL with new aqs
            // for query in the omnibox and voice suggestions, but it's currently difficult to do.
            long elapsedTimeSinceInputChange = mNewOmniboxEditSessionTimestamp > 0
                    ? (SystemClock.elapsedRealtime() - mNewOmniboxEditSessionTimestamp) : -1;
            updatedUrl = mAutocomplete.updateMatchDestinationUrlWithQueryFormulationTime(
                    verifiedIndex, suggestion.hashCode(), elapsedTimeSinceInputChange);
        }

        return updatedUrl == null ? suggestion.getUrl() : updatedUrl;
    }

    private void clearSuggestions(boolean notifyChange) {
        mSuggestionItems.clear();
        // Make sure to notify the adapter. If the ListView becomes out of sync
        // with its adapter and it has not been notified, it will throw an
        // exception when some UI events are propagated.
        if (notifyChange) mSuggestionListAdapter.notifyDataSetChanged();
    }

    /**
     * Hides the omnibox suggestion popup.
     *
     * <p>
     * Signals the autocomplete controller to stop generating omnibox suggestions.
     *
     * @see AutocompleteController#stop(boolean)
     */
    @Override
    public void hideSuggestions() {
        if (mAutocomplete == null || !mNativeInitialized) return;

        if (mShowSuggestions != null) removeCallbacks(mShowSuggestions);

        stopAutocomplete(true);

        setSuggestionsListVisibility(false);
        clearSuggestions(true);
        updateNavigationButton();
    }

    /**
     * Signals the autocomplete controller to stop generating omnibox suggestions and cancels the
     * queued task to start the autocomplete controller, if any.
     *
     * @param clear Whether to clear the most recent autocomplete results.
     */
    private void stopAutocomplete(boolean clear) {
        if (mAutocomplete != null) mAutocomplete.stop(clear);
        cancelPendingAutocompleteStart();
    }

    /**
     * Cancels the queued task to start the autocomplete controller, if any.
     */
    @VisibleForTesting
    void cancelPendingAutocompleteStart() {
        if (mRequestSuggestions != null) {
            // There is a request for suggestions either waiting for the native side
            // to start, or on the message queue. Remove it from wherever it is.
            if (!mDeferredNativeRunnables.remove(mRequestSuggestions)) {
                removeCallbacks(mRequestSuggestions);
            }
            mRequestSuggestions = null;
        }
    }

    @Override
    protected void dispatchRestoreInstanceState(SparseArray<Parcelable> container) {
        // Don't restore the state of the location bar, it can lead to all kind of bad states with
        // the popup.
        // When we restore tabs, we focus the selected tab so the URL of the page shows.
    }

    /**
     * Performs a search query on the current {@link Tab}.  This calls
     * {@link TemplateUrlService#getUrlForSearchQuery(String)} to get a url based on {@code query}
     * and loads that url in the current {@link Tab}.
     * @param query The {@link String} that represents the text query that should be searched for.
     */
    @VisibleForTesting
    public void performSearchQueryForTest(String query) {
        if (TextUtils.isEmpty(query)) return;

        String queryUrl = TemplateUrlService.getInstance().getUrlForSearchQuery(query);

        if (!TextUtils.isEmpty(queryUrl)) {
            loadUrl(queryUrl, PageTransition.GENERATED);
        } else {
            setSearchQuery(query);
        }
    }

    /**
     * Sets the query string in the omnibox (ensuring the URL bar has focus and triggering
     * autocomplete for the specified query) as if the user typed it.
     *
     * @param query The query to be set in the omnibox.
     */
    @Override
    public void setSearchQuery(final String query) {
        if (TextUtils.isEmpty(query)) return;

        if (!mNativeInitialized) {
            mDeferredNativeRunnables.add(new Runnable() {
                @Override
                public void run() {
                    setSearchQuery(query);
                }
            });
            return;
        }

        setUrlBarText(UrlBarData.forNonUrlText(query));
        setUrlBarFocus(true);
        selectAll();
        stopAutocomplete(false);
        if (mToolbarDataProvider.hasTab()) {
            mAutocomplete.start(mToolbarDataProvider.getProfile(),
                    mToolbarDataProvider.getCurrentUrl(), query, -1, false, false);
        }
        post(new Runnable() {
            @Override
            public void run() {
                UiUtils.showKeyboard(mUrlBar);
            }
        });
    }

    /**
     * Whether {@code v} is a view (location icon, verbose status, ...) which can be clicked to
     * show the Page Info popup.
     */
    private boolean shouldShowPageInfoForView(View v) {
        return v == mSecurityButton || v == mNavigationButton || v == mVerboseStatusTextView;
    }

    @Override
    public void onClick(View v) {
        if (v == mDeleteButton) {
            if (!TextUtils.isEmpty(mUrlBar.getTextWithAutocomplete())) {
                setUrlBarText(UrlBarData.EMPTY);
                hideSuggestions();
                updateButtonVisibility();
            }

            startZeroSuggest();
            RecordUserAction.record("MobileOmniboxDeleteUrl");
            return;
        } else if (!mUrlHasFocus && shouldShowPageInfoForView(v)) {
            if (mToolbarDataProvider.hasTab() && getCurrentTab().getWebContents() != null
                    && mWindowAndroid != null) {
                Activity activity = mWindowAndroid.getActivity().get();
                if (activity != null) {
                    PageInfoController.show(activity, getCurrentTab(), null,
                            PageInfoController.OPENED_FROM_TOOLBAR);
                }
            }
        } else if (v == mMicButton && mVoiceRecognitionHandler != null) {
            RecordUserAction.record("MobileOmniboxVoiceSearch");
            mVoiceRecognitionHandler.startVoiceRecognition(
                    LocationBarVoiceRecognitionHandler.VoiceInteractionSource.OMNIBOX);
        }
    }

    @Override
    public void onSuggestionsReceived(List<OmniboxSuggestion> newSuggestions,
            String inlineAutocompleteText) {
        // This is a callback from a listener that is set up by onNativeLibraryReady,
        // so can only be called once the native side is set up unless we are showing
        // cached java-only suggestions.
        assert mNativeInitialized || mShowCachedZeroSuggestResults
                : "Native suggestions received before native side intialialized";

        if (mDeferredOnSelection != null) {
            mDeferredOnSelection.setShouldLog(newSuggestions.size() > mDeferredOnSelection.mPosition
                    && mDeferredOnSelection.mSuggestion.equals(
                            newSuggestions.get(mDeferredOnSelection.mPosition)));
            mDeferredOnSelection.run();
            mDeferredOnSelection = null;
        }
        String userText = mUrlBar.getTextWithoutAutocomplete();
        mUrlTextAfterSuggestionsReceived = userText + inlineAutocompleteText;

        boolean itemsChanged = false;
        boolean itemCountChanged = false;
        // If the length of the incoming suggestions matches that of those currently being shown,
        // replace them inline to allow transient entries to retain their proper highlighting.
        if (mSuggestionItems.size() == newSuggestions.size()) {
            for (int index = 0; index < newSuggestions.size(); index++) {
                OmniboxResultItem suggestionItem = mSuggestionItems.get(index);
                OmniboxSuggestion suggestion = suggestionItem.getSuggestion();
                OmniboxSuggestion newSuggestion = newSuggestions.get(index);
                // Determine whether the suggestions have changed. If not, save some time by not
                // redrawing the suggestions UI.
                if (suggestion.equals(newSuggestion)
                        && suggestion.getType() != OmniboxSuggestionType.SEARCH_SUGGEST_TAIL) {
                    if (suggestionItem.getMatchedQuery().equals(userText)) {
                        continue;
                    } else if (!suggestion.getDisplayText().startsWith(userText)
                            && !suggestion.getUrl().contains(userText)) {
                        continue;
                    }
                }
                mSuggestionItems.set(index, new OmniboxResultItem(newSuggestion, userText));
                itemsChanged = true;
            }
        } else {
            itemsChanged = true;
            itemCountChanged = true;
            clearSuggestions(false);
            for (int i = 0; i < newSuggestions.size(); i++) {
                mSuggestionItems.add(new OmniboxResultItem(newSuggestions.get(i), userText));
            }
        }

        if (mSuggestionItems.isEmpty()) {
            if (mSuggestionsShown) hideSuggestions();
            return;
        }

        if (mUrlBar.shouldAutocomplete()) {
            mUrlBar.setAutocompleteText(userText, inlineAutocompleteText);
        }

        // Show the suggestion list.
        initSuggestionList();  // It may not have been initialized yet.
        mSuggestionList.resetMaxTextWidths();

        // Handle the case where suggestions (in particular zero suggest) are received without the
        // URL focusing happening.
        if (mUrlFocusedWithoutAnimations && mUrlHasFocus) {
            handleUrlFocusAnimation(mUrlHasFocus);
        }

        if (itemsChanged) mSuggestionListAdapter.notifySuggestionsChanged();

        if (mUrlBar.hasFocus()) {
            final boolean updateLayoutParams = itemCountChanged;
            mShowSuggestions = new Runnable() {
                @Override
                public void run() {
                    setSuggestionsListVisibility(true);
                    if (updateLayoutParams) {
                        mSuggestionList.updateLayoutParams();
                    }
                    mShowSuggestions = null;
                }
            };
            if (!isUrlFocusChangeInProgress()) {
                mShowSuggestions.run();
            } else {
                postDelayed(mShowSuggestions, ToolbarPhone.URL_FOCUS_CHANGE_ANIMATION_DURATION_MS);
            }
        }

        // Update the navigation button to show the default suggestion's icon.
        updateNavigationButton();

        if (mNativeInitialized
                && !CommandLine.getInstance().hasSwitch(ChromeSwitches.DISABLE_INSTANT)
                && PrivacyPreferencesManager.getInstance().shouldPrerender()
                && mToolbarDataProvider.hasTab()) {
            mOmniboxPrerender.prerenderMaybe(userText, getOriginalUrl(),
                    mAutocomplete.getCurrentNativeAutocompleteResult(),
                    mToolbarDataProvider.getProfile(), getCurrentTab());
        }
    }

    @Override
    public void backKeyPressed() {
        setUrlBarFocus(false);
        hideSuggestions();
        UiUtils.hideKeyboard(mUrlBar);
        // Revert the URL to match the current page.
        setUrlToPageUrl();
        focusCurrentTab();
    }

    @Override
    public boolean shouldForceLTR() {
        return !mToolbarDataProvider.shouldDisplaySearchTerms();
    }

    @Override
    @UrlBar.ScrollType
    public int getScrollType() {
        return mToolbarDataProvider.shouldDisplaySearchTerms() ? UrlBar.SCROLL_TO_BEGINNING
                                                               : UrlBar.SCROLL_TO_TLD;
    }

    @Override
    public boolean shouldCutCopyVerbatim() {
        // When cutting/copying text in the URL bar, it will try to copy some version of the actual
        // URL to the clipboard, not the currently displayed URL bar contents. We want to avoid this
        // when displaying search terms.
        return mToolbarDataProvider.shouldDisplaySearchTerms();
    }

    /**
     * @return Returns the original url of the page.
     */
    public String getOriginalUrl() {
        return mOriginalUrl;
    }

    /**
     * Sets the displayed URL to be the URL of the page currently showing.
     *
     * <p>The URL is converted to the most user friendly format (removing HTTP:// for example).
     *
     * <p>If the current tab is null, the URL text will be cleared.
     */
    @Override
    public void setUrlToPageUrl() {
        String currentUrl = mToolbarDataProvider.getCurrentUrl();

        // If the URL is currently focused, do not replace the text they have entered with the URL.
        // Once they stop editing the URL, the current tab's URL will automatically be filled in.
        if (mUrlBar.hasFocus()) {
            if (mUrlFocusedWithoutAnimations && !NewTabPage.isNTPUrl(currentUrl)) {
                // If we did not run the focus animations, then the user has not typed any text.
                // So, clear the focus and accept whatever URL the page is currently attempting to
                // display. If the NTP is showing, the current page's URL should not be displayed.
                setUrlBarFocus(false);
            } else {
                return;
            }
        }

        mOriginalUrl = currentUrl;
        if (setUrlBarText(mToolbarDataProvider.getUrlBarData())) {
            emphasizeUrl();
        }
        if (!mToolbarDataProvider.hasTab()) return;

        // Profile may be null if switching to a tab that has not yet been initialized.
        Profile profile = mToolbarDataProvider.getProfile();
        if (profile != null) mOmniboxPrerender.clear(profile);
    }

    /**
     * Changes the text on the url bar.
     * @param urlBarData The contents of the URL bar, both for editing and displaying.
     * @return Whether the URL was changed as a result of this call.
     */
    private boolean setUrlBarText(UrlBarData urlBarData) {
        mUrlBar.setIgnoreTextChangesForAutocomplete(true);
        boolean urlChanged = mUrlBar.setUrl(urlBarData);
        mUrlBar.setIgnoreTextChangesForAutocomplete(false);
        return urlChanged;
    }

    private void loadUrlFromOmniboxMatch(
            String url, int matchPosition, OmniboxSuggestion suggestion) {
        // loadUrl modifies AutocompleteController's state clearing the native
        // AutocompleteResults needed by onSuggestionsSelected. Therefore,
        // loadUrl should should be invoked last.
        int transition = suggestion.getTransition();
        int type = suggestion.getType();
        String currentPageUrl = mToolbarDataProvider.getCurrentUrl();
        WebContents webContents =
                mToolbarDataProvider.hasTab() ? getCurrentTab().getWebContents() : null;
        long elapsedTimeSinceModified = mNewOmniboxEditSessionTimestamp > 0
                ? (SystemClock.elapsedRealtime() - mNewOmniboxEditSessionTimestamp) : -1;
        boolean shouldSkipNativeLog = mShowCachedZeroSuggestResults
                && (mDeferredOnSelection != null)
                && !mDeferredOnSelection.shouldLog();
        if (!shouldSkipNativeLog) {
            mAutocomplete.onSuggestionSelected(matchPosition, suggestion.hashCode(), type,
                    currentPageUrl, mUrlFocusedFromFakebox, elapsedTimeSinceModified,
                    mUrlBar.getAutocompleteLength(), webContents);
        }
        if (((transition & PageTransition.CORE_MASK) == PageTransition.TYPED)
                && TextUtils.equals(url, mToolbarDataProvider.getCurrentUrl())) {
            // When the user hit enter on the existing permanent URL, treat it like a
            // reload for scoring purposes.  We could detect this by just checking
            // user_input_in_progress_, but it seems better to treat "edits" that end
            // up leaving the URL unchanged (e.g. deleting the last character and then
            // retyping it) as reloads too.  We exclude non-TYPED transitions because if
            // the transition is GENERATED, the user input something that looked
            // different from the current URL, even if it wound up at the same place
            // (e.g. manually retyping the same search query), and it seems wrong to
            // treat this as a reload.
            transition = PageTransition.RELOAD;
        } else if (type == OmniboxSuggestionType.URL_WHAT_YOU_TYPED && mUrlBar.wasLastEditPaste()) {
            // It's important to use the page transition from the suggestion or we might end
            // up saving generated URLs as typed URLs, which would then pollute the subsequent
            // omnibox results. There is one special case where the suggestion text was pasted,
            // where we want the transition type to be LINK.

            transition = PageTransition.LINK;
        }
        loadUrl(url, transition);
    }

    @Override
    public void loadUrlFromVoice(String url) {
        loadUrl(url, PageTransition.TYPED);
    }

    /**
     * Load the url given with the given transition. Exposed for child classes to overwrite as
     * necessary.
     */
    protected void loadUrl(String url, int transition) {
        Tab currentTab = getCurrentTab();

        // The code of the rest of this class ensures that this can't be called until the native
        // side is initialized
        assert mNativeInitialized : "Loading URL before native side initialized";

        if (currentTab != null
                && (currentTab.isNativePage() || NewTabPage.isNTPUrl(currentTab.getUrl()))) {
            NewTabPageUma.recordOmniboxNavigation(url, transition);
            // Passing in an empty string should not do anything unless the user is at the NTP.
            // Since the NTP has no url, pressing enter while clicking on the URL bar should refresh
            // the page as it does when you click and press enter on any other site.
            if (url.isEmpty()) url = currentTab.getUrl();
        }

        // Loads the |url| in a new tab or the current ContentView and gives focus to the
        // ContentView.
        if (currentTab != null && !url.isEmpty()) {
            LoadUrlParams loadUrlParams = new LoadUrlParams(url);
            loadUrlParams.setVerbatimHeaders(GeolocationHeader.getGeoHeader(url, currentTab));
            loadUrlParams.setTransitionType(transition | PageTransition.FROM_ADDRESS_BAR);

            // If the bottom sheet exists, route the navigation through it instead of the tab.
            if (mBottomSheet != null) {
                mBottomSheet.loadUrl(loadUrlParams, currentTab.isIncognito());
            } else {
                currentTab.loadUrl(loadUrlParams);
            }
            RecordUserAction.record("MobileOmniboxUse");
        }
        LocaleManager.getInstance().recordLocaleBasedSearchMetrics(false, url, transition);

        focusCurrentTab();
        // Prevent any upcoming omnibox suggestions from showing. We have to do this after we load
        // the URL as this will hide the suggestions and trigger a cancel of the prerendered page.
        stopAutocomplete(true);
    }

    /**
     * Update the location bar visuals based on a loading state change.
     * @param updateUrl Whether to update the URL as a result of this call.
     */
    @Override
    public void updateLoadingState(boolean updateUrl) {
        if (updateUrl) setUrlToPageUrl();
        updateNavigationButton();
        updateSecurityIcon();
    }

    @Override
    public Tab getCurrentTab() {
        if (mToolbarDataProvider == null) return null;
        return mToolbarDataProvider.getTab();
    }

    @Override
    public boolean allowKeyboardLearning() {
        if (mToolbarDataProvider == null) return false;
        return !mToolbarDataProvider.isIncognito();
    }

    private void initOmniboxResultsContainer() {
        if (mOmniboxResultsContainer != null) return;

        ViewStub overlayStub =
                (ViewStub) getRootView().findViewById(R.id.omnibox_results_container_stub);
        mOmniboxResultsContainer = (ViewGroup) overlayStub.inflate();
    }

    private void updateOmniboxResultsContainer() {
        if (mSuggestionsShown || mUrlHasFocus) {
            initOmniboxResultsContainer();
            updateOmniboxResultsContainerVisibility(true);
        } else if (mOmniboxResultsContainer != null) {
            updateFadingBackgroundView(false);
        }
    }

    private void updateOmniboxResultsContainerVisibility(boolean visible) {
        if (mOmniboxResultsContainer == null) return;

        boolean currentlyVisible = mOmniboxResultsContainer.getVisibility() == VISIBLE;
        if (currentlyVisible == visible) return;

        if (visible) {
            mOmniboxResultsContainer.setVisibility(VISIBLE);
        } else {
            mOmniboxResultsContainer.setVisibility(INVISIBLE);
        }
    }

    /**
     * Initialize the fading background for when the omnibox is focused.
     */
    protected void initFadingOverlayView() {
        mFadingView =
                (FadingBackgroundView) getRootView().findViewById(R.id.fading_focus_target);
        mFadingView.addObserver(this);
    }

    @Override
    public void onFadingViewClick() {
        setUrlBarFocus(false);

        // If the bottom sheet is used, it will control the fading view.
        if (mBottomSheet == null) updateFadingBackgroundView(false);
    }

    @Override
    public void onFadingViewVisibilityChanged(boolean visible) {
        Activity activity = mWindowAndroid.getActivity().get();
        if (!(activity instanceof ChromeActivity)) return;
        ChromeActivity chromeActivity = (ChromeActivity) activity;

        if (visible) {
            chromeActivity.addViewObscuringAllTabs(mFadingView);
        } else {
            chromeActivity.removeViewObscuringAllTabs(mFadingView);
            updateOmniboxResultsContainerVisibility(false);
        }
    }

    /**
     * Update the fading background view that shows when the omnibox is focused. If Chrome Home is
     * enabled, this method is a no-op.
     * @param visible Whether the background should be made visible.
     */
    private void updateFadingBackgroundView(boolean visible) {
        if (mFadingView == null) initFadingOverlayView();

        // If Chrome Home is enabled (the bottom sheet is not null), it will be controlling the
        // fading view, so block any updating here.
        if (mToolbarDataProvider == null || mBottomSheet != null) return;

        NewTabPage ntp = mToolbarDataProvider.getNewTabPageForCurrentTab();
        boolean locationBarShownInNTP = ntp != null && ntp.isLocationBarShownInNTP();

        if (visible && !locationBarShownInNTP) {
            // If the location bar is shown in the NTP, the toolbar will eventually trigger a
            // fade in.
            mFadingView.showFadingOverlay();
        } else {
            mFadingView.hideFadingOverlay(!locationBarShownInNTP);
        }
    }

    @Override
    public void onWindowFocusChanged(boolean hasWindowFocus) {
        super.onWindowFocusChanged(hasWindowFocus);
        if (!hasWindowFocus && !mSuggestionModalShown) {
            hideSuggestions();
        } else if (hasWindowFocus && mUrlHasFocus && mNativeInitialized) {
            Editable currentUrlBarText = mUrlBar.getText();
            if (TextUtils.isEmpty(currentUrlBarText)
                    || TextUtils.equals(currentUrlBarText,
                               mToolbarDataProvider.getUrlBarData().getEditingOrDisplayText())) {
                startZeroSuggest();
            } else {
                onTextChangedForAutocomplete();
            }
        }
    }

    @Override
    protected void onWindowVisibilityChanged(int visibility) {
        super.onWindowVisibilityChanged(visibility);
        if (visibility == View.VISIBLE) updateMicButtonState();
    }

    /**
     * Call to update the visibility of the buttons inside the location bar.
     */
    protected void updateButtonVisibility() {
        updateDeleteButtonVisibility();
    }

    /**
     * Call to notify the location bar that the state of the voice search microphone button may
     * need to be updated.
     */
    @Override
    public void updateMicButtonState() {
        mVoiceSearchEnabled = mVoiceRecognitionHandler.isVoiceSearchEnabled();
        updateButtonVisibility();
    }

    /**
     * Updates the display of the mic button.
     *
     * @param urlFocusChangePercent The completion percentage of the URL focus change animation.
     */
    protected void updateMicButtonVisibility(float urlFocusChangePercent) {
        boolean visible = mOmniboxVoiceSearchAlwaysVisible || !shouldShowDeleteButton();
        boolean showMicButton = mVoiceSearchEnabled && visible
                && (mUrlBar.hasFocus() || mUrlFocusChangeInProgress || urlFocusChangePercent > 0f);
        mMicButton.setVisibility(showMicButton ? VISIBLE : GONE);
    }

    /**
     * Call to force the UI to update the state of various buttons based on whether or not the
     * current tab is incognito.
     */
    @Override
    public void updateVisualsForState() {
        if (updateUseDarkColors() || mIsEmphasizingHttpsScheme != shouldEmphasizeHttpsScheme()) {
            updateSecurityIcon();
        }
        ColorStateList colorStateList = ApiCompatibilityUtils.getColorStateList(getResources(),
                mUseDarkColors ? R.color.dark_mode_tint : R.color.light_mode_tint);
        mMicButton.setTint(colorStateList);
        mDeleteButton.setTint(colorStateList);

        setNavigationButtonType(mNavigationButtonType);
        mUrlBar.setUseDarkTextColors(mUseDarkColors);

        if (mSuggestionList != null) {
            mSuggestionList.refreshPopupBackground();
        }
        mSuggestionListAdapter.setUseDarkColors(mUseDarkColors);
    }

    /**
     * Checks the current specs and updates {@link LocationBarLayout#mUseDarkColors} if necessary.
     * @return Whether {@link LocationBarLayout#mUseDarkColors} has been updated.
     */
    private boolean updateUseDarkColors() {
        boolean brandColorNeedsLightText = false;
        if (mToolbarDataProvider.isUsingBrandColor() && !mUrlHasFocus) {
            int currentPrimaryColor = mToolbarDataProvider.getPrimaryColor();
            brandColorNeedsLightText =
                    ColorUtils.shouldUseLightForegroundOnBackground(currentPrimaryColor);
        }

        boolean useDarkColors = !mToolbarDataProvider.isIncognito()
                && (!mToolbarDataProvider.hasTab() || !brandColorNeedsLightText);
        boolean hasChanged = useDarkColors != mUseDarkColors;
        mUseDarkColors = useDarkColors;

        return hasChanged;
    }

    @Override
    public void onTabLoadingNTP(NewTabPage ntp) {
        ntp.setFakeboxDelegate(this);
        ntp.setVoiceRecognitionHandler(mVoiceRecognitionHandler);
    }

    @Override
    public View getContainerView() {
        return this;
    }

    @Override
    public void setTitleToPageTitle() { }

    @Override
    public void setShowTitle(boolean showTitle) { }

    @Override
    public boolean mustQueryUrlBarLocationForSuggestions() {
        return mIsTablet;
    }

    @Override
    public AutocompleteController getAutocompleteController() {
        return mAutocomplete;
    }

    @Override
    public WindowAndroid getWindowAndroid() {
        return mWindowAndroid;
    }
}
