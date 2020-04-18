// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.support.annotation.Nullable;
import android.support.v7.widget.DefaultItemAnimator;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.RecyclerView.AdapterDataObserver;
import android.support.v7.widget.RecyclerView.ViewHolder;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewStub;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.TextView;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.TraceEvent;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.locale.LocaleManager;
import org.chromium.chrome.browser.ntp.LogoBridge.Logo;
import org.chromium.chrome.browser.ntp.LogoBridge.LogoObserver;
import org.chromium.chrome.browser.ntp.NewTabPage.FakeboxDelegate;
import org.chromium.chrome.browser.ntp.NewTabPage.OnSearchBoxScrollListener;
import org.chromium.chrome.browser.ntp.cards.NewTabPageAdapter;
import org.chromium.chrome.browser.ntp.cards.NewTabPageRecyclerView;
import org.chromium.chrome.browser.offlinepages.OfflinePageBridge;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.suggestions.DestructionObserver;
import org.chromium.chrome.browser.suggestions.SiteSection;
import org.chromium.chrome.browser.suggestions.SiteSectionViewHolder;
import org.chromium.chrome.browser.suggestions.SuggestionsConfig;
import org.chromium.chrome.browser.suggestions.SuggestionsDependencyFactory;
import org.chromium.chrome.browser.suggestions.SuggestionsUiDelegate;
import org.chromium.chrome.browser.suggestions.Tile;
import org.chromium.chrome.browser.suggestions.TileGroup;
import org.chromium.chrome.browser.suggestions.TileRenderer;
import org.chromium.chrome.browser.suggestions.TileView;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.util.FeatureUtilities;
import org.chromium.chrome.browser.util.MathUtils;
import org.chromium.chrome.browser.util.ViewUtils;
import org.chromium.chrome.browser.vr_shell.VrShellDelegate;
import org.chromium.chrome.browser.widget.displaystyle.UiConfig;
import org.chromium.ui.base.DeviceFormFactor;

/**
 * The native new tab page, represented by some basic data such as title and url, and an Android
 * View that displays the page.
 */
public class NewTabPageView
        extends FrameLayout implements TileGroup.Observer, VrShellDelegate.VrModeObserver {
    private static final String TAG = "NewTabPageView";

    private static final long SNAP_SCROLL_DELAY_MS = 30;

    /**
     * Parameter for the simplified NTP ablation experiment arm which removes the additional
     * suggestions sections without replacing them with shortcut buttons.
     */
    private static final String PARAM_SIMPLIFIED_NTP_ABLATION = "simplified_ntp_ablation";

    private NewTabPageRecyclerView mRecyclerView;

    private NewTabPageLayout mNewTabPageLayout;
    private LogoView mSearchProviderLogoView;
    private View mSearchBoxView;
    private ImageView mVoiceSearchButton;
    private SiteSectionViewHolder mSiteSectionViewHolder;
    private View mTileGridPlaceholder;
    private View mNoSearchLogoSpacer;
    private ViewGroup mShortcutsView;

    private OnSearchBoxScrollListener mSearchBoxScrollListener;

    private NewTabPageManager mManager;
    private Tab mTab;
    private LogoDelegateImpl mLogoDelegate;
    private TileGroup mTileGroup;
    private UiConfig mUiConfig;
    private Runnable mSnapScrollRunnable;
    private Runnable mUpdateSearchBoxOnScrollRunnable;

    /**
     * Whether the tiles shown in the layout have finished loading.
     * With {@link #mHasShownView}, it's one of the 2 flags used to track initialisation progress.
     */
    private boolean mTilesLoaded;

    /**
     * Whether the view has been shown at least once.
     * With {@link #mTilesLoaded}, it's one of the 2 flags used to track initialization progress.
     */
    private boolean mHasShownView;

    private boolean mSearchProviderHasLogo = true;
    private boolean mSearchProviderIsGoogle;

    private boolean mPendingSnapScroll;
    private boolean mInitialized;
    private int mLastScrollY = -1;

    private float mUrlFocusChangePercent;
    private boolean mDisableUrlFocusChangeAnimations;
    private boolean mIsMovingNewTabPageView;

    /** Flag used to request some layout changes after the next layout pass is completed. */
    private boolean mTileCountChanged;
    private boolean mSnapshotTileGridChanged;
    private boolean mNewTabPageRecyclerViewChanged;
    private int mSnapshotWidth;
    private int mSnapshotHeight;
    private int mSnapshotScrollY;
    private ContextMenuManager mContextMenuManager;

    /**
     * Vertical inset to add to the top and bottom of the search box bounds. May be 0 if no inset
     * should be applied. See {@link Rect#inset(int, int)}.
     */
    private int mSearchBoxBoundsVerticalInset;

    /**
     * @return Whether the simplified NTP ablation experiment arm which removes the additional
     *         suggestions sections without replacing them with shortcut buttons is enabled.
     */
    public static boolean isSimplifiedNtpAblationEnabled() {
        return ChromeFeatureList.getFieldTrialParamByFeatureAsBoolean(
                ChromeFeatureList.SIMPLIFIED_NTP, PARAM_SIMPLIFIED_NTP_ABLATION, false);
    }

    /**
     * Manages the view interaction with the rest of the system.
     */
    public interface NewTabPageManager extends SuggestionsUiDelegate {
        /** @return Whether the location bar is shown in the NTP. */
        boolean isLocationBarShownInNTP();

        /** @return Whether voice search is enabled and the microphone should be shown. */
        boolean isVoiceSearchEnabled();

        /**
         * Animates the search box up into the omnibox and bring up the keyboard.
         * @param beginVoiceSearch Whether to begin a voice search.
         * @param pastedText Text to paste in the omnibox after it's been focused. May be null.
         */
        void focusSearchBox(boolean beginVoiceSearch, String pastedText);

        /**
         * @return whether the {@link NewTabPage} associated with this manager is the current page
         * displayed to the user.
         */
        boolean isCurrentPage();

        /**
         * Called when the NTP has completely finished loading (all views will be inflated
         * and any dependent resources will have been loaded).
         */
        void onLoadingComplete();
    }

    /**
     * Default constructor required for XML inflation.
     */
    public NewTabPageView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    /**
     * Initializes the NTP. This must be called immediately after inflation, before this object is
     * used in any other way.
     *
     * @param manager NewTabPageManager used to perform various actions when the user interacts
     *                with the page.
     * @param tab The Tab that is showing this new tab page.
     * @param searchProviderHasLogo Whether the search provider has a logo.
     * @param searchProviderIsGoogle Whether the search provider is Google.
     * @param scrollPosition The adapter scroll position to initialize to.
     */
    public void initialize(NewTabPageManager manager, Tab tab, TileGroup.Delegate tileGroupDelegate,
            boolean searchProviderHasLogo, boolean searchProviderIsGoogle, int scrollPosition) {
        TraceEvent.begin(TAG + ".initialize()");
        mTab = tab;
        mManager = manager;
        mUiConfig = new UiConfig(this);

        assert manager.getSuggestionsSource() != null;

        mRecyclerView = new NewTabPageRecyclerView(getContext());
        mRecyclerView.setContainsLocationBar(manager.isLocationBarShownInNTP());
        addView(mRecyclerView);

        // Don't attach now, the recyclerView itself will determine when to do it.
        mNewTabPageLayout = mRecyclerView.getAboveTheFoldView();

        mRecyclerView.setItemAnimator(new DefaultItemAnimator() {
            @Override
            public boolean animateMove(ViewHolder holder, int fromX, int fromY, int toX, int toY) {
                // If |mNewTabPageLayout| is animated by the RecyclerView because an item below it
                // was dismissed, avoid also manipulating its vertical offset in our scroll handling
                // at the same time. The onScrolled() method is called when an item is dismissed and
                // the item at the top of the viewport is repositioned.
                if (holder.itemView == mNewTabPageLayout) mIsMovingNewTabPageView = true;

                // Cancel any pending scroll update handling, a new one will be scheduled in
                // onAnimationFinished().
                mRecyclerView.removeCallbacks(mUpdateSearchBoxOnScrollRunnable);

                return super.animateMove(holder, fromX, fromY, toX, toY);
            }

            @Override
            public void onAnimationFinished(ViewHolder viewHolder) {
                super.onAnimationFinished(viewHolder);

                // When an item is dismissed, the items at the top of the viewport might not move,
                // and onScrolled() might not be called. We can get in the situation where the
                // toolbar buttons disappear, so schedule an update for it. This can be cancelled
                // from animateMove() in case |mNewTabPageLayout| will be moved. We don't know that
                // from here, as the RecyclerView will animate multiple items when one is dismissed,
                // and some will "finish" synchronously if they are already in the correct place,
                // before other moves have even been scheduled.
                if (viewHolder.itemView == mNewTabPageLayout) mIsMovingNewTabPageView = false;
                mRecyclerView.removeCallbacks(mUpdateSearchBoxOnScrollRunnable);
                mRecyclerView.post(mUpdateSearchBoxOnScrollRunnable);
            }
        });

        // Don't store a direct reference to the activity, because it might change later if the tab
        // is reparented.
        Runnable closeContextMenuCallback = () -> {
            mTab.getActivity().closeContextMenu();
        };
        mContextMenuManager = new ContextMenuManager(mManager.getNavigationDelegate(),
                mRecyclerView::setTouchEnabled, closeContextMenuCallback);
        mTab.getWindowAndroid().addContextMenuCloseListener(mContextMenuManager);

        Profile profile = Profile.getLastUsedProfile();
        OfflinePageBridge offlinePageBridge =
                SuggestionsDependencyFactory.getInstance().getOfflinePageBridge(profile);
        TileRenderer tileRenderer =
                new TileRenderer(mTab.getActivity(), SuggestionsConfig.getTileStyle(mUiConfig),
                        getTileTitleLines(), mManager.getImageFetcher());
        mTileGroup = new TileGroup(tileRenderer, mManager, mContextMenuManager, tileGroupDelegate,
                /* observer = */ this, offlinePageBridge);

        mSiteSectionViewHolder =
                SiteSection.createViewHolder(mNewTabPageLayout.getSiteSectionView(), mUiConfig);
        mSiteSectionViewHolder.bindDataSource(mTileGroup, tileRenderer);

        mSearchProviderLogoView = mNewTabPageLayout.findViewById(R.id.search_provider_logo);
        mLogoDelegate = new LogoDelegateImpl(
                mManager.getNavigationDelegate(), mSearchProviderLogoView, profile);

        mSearchBoxView = mNewTabPageLayout.findViewById(R.id.search_box);
        if (SuggestionsConfig.useModernLayout()) {
            mSearchBoxView.setBackgroundResource(R.drawable.ntp_search_box);
            mSearchBoxView.getLayoutParams().height =
                    getResources().getDimensionPixelSize(R.dimen.ntp_search_box_height_modern);

            if (!DeviceFormFactor.isTablet()) {
                mSearchBoxBoundsVerticalInset = getResources().getDimensionPixelSize(
                        R.dimen.ntp_search_box_bounds_vertical_inset_modern);
            }
        }
        mNoSearchLogoSpacer = mNewTabPageLayout.findViewById(R.id.no_search_logo_spacer);

        mSnapScrollRunnable = new SnapScrollRunnable();
        mUpdateSearchBoxOnScrollRunnable = new UpdateSearchBoxOnScrollRunnable();

        initializeShortcuts();
        initializeSearchBoxTextView();
        initializeVoiceSearchButton();
        initializeLayoutChangeListeners();
        setSearchProviderInfo(searchProviderHasLogo, searchProviderIsGoogle);
        mSearchProviderLogoView.showSearchProviderInitialView();

        mTileGroup.startObserving(getMaxTileRows(searchProviderHasLogo) * getMaxTileColumns());

        mRecyclerView.init(mUiConfig, mContextMenuManager);

        // Set up snippets
        NewTabPageAdapter newTabPageAdapter =
                new NewTabPageAdapter(mManager, mNewTabPageLayout, /* logoView = */ null, mUiConfig,
                        offlinePageBridge, mContextMenuManager, /* tileGroupDelegate = */ null);
        newTabPageAdapter.refreshSuggestions();
        mRecyclerView.setAdapter(newTabPageAdapter);
        mRecyclerView.getLinearLayoutManager().scrollToPosition(scrollPosition);

        setupScrollHandling();

        // When the NewTabPageAdapter's data changes we need to invalidate any previous
        // screen captures of the NewTabPageView.
        newTabPageAdapter.registerAdapterDataObserver(new AdapterDataObserver() {
            @Override
            public void onChanged() {
                mNewTabPageRecyclerViewChanged = true;
            }

            @Override
            public void onItemRangeChanged(int positionStart, int itemCount) {
                onChanged();
            }

            @Override
            public void onItemRangeInserted(int positionStart, int itemCount) {
                onChanged();
            }

            @Override
            public void onItemRangeRemoved(int positionStart, int itemCount) {
                onChanged();
            }

            @Override
            public void onItemRangeMoved(int fromPosition, int toPosition, int itemCount) {
                onChanged();
            }
        });

        VrShellDelegate.registerVrModeObserver(this);
        if (VrShellDelegate.isInVr()) onEnterVr();

        manager.addDestructionObserver(new DestructionObserver() {
            @Override
            public void onDestroy() {
                NewTabPageView.this.onDestroy();
            }
        });

        mInitialized = true;

        TraceEvent.end(TAG + ".initialize()");
    }

    /**
     * Sets the {@link FakeboxDelegate} associated with the new tab page.
     * @param fakeboxDelegate The {@link FakeboxDelegate} used to determine whether the URL bar
     *                        has focus.
     */
    public void setFakeboxDelegate(FakeboxDelegate fakeboxDelegate) {
        mRecyclerView.setFakeboxDelegate(fakeboxDelegate);
    }

    /**
     * Sets up the hint text and event handlers for the search box text view.
     */
    private void initializeSearchBoxTextView() {
        TraceEvent.begin(TAG + ".initializeSearchBoxTextView()");

        final TextView searchBoxTextView =
                (TextView) mSearchBoxView.findViewById(R.id.search_box_text);
        String hintText = getResources().getString(R.string.search_or_type_web_address);
        if (!DeviceFormFactor.isNonMultiDisplayContextOnTablet(getContext())
                || SuggestionsConfig.useModernLayout()) {
            searchBoxTextView.setHint(hintText);
        } else {
            searchBoxTextView.setContentDescription(hintText);
        }
        searchBoxTextView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mManager.focusSearchBox(false, null);
            }
        });
        searchBoxTextView.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
            }

            @Override
            public void afterTextChanged(Editable s) {
                if (s.length() == 0) return;
                mManager.focusSearchBox(false, s.toString());
                searchBoxTextView.setText("");
            }
        });
        TraceEvent.end(TAG + ".initializeSearchBoxTextView()");
    }

    /**
     * Updates the small search engine logo shown in the search box.
     */
    private void updateSearchBoxLogo() {
        TextView searchBoxTextView = (TextView) mSearchBoxView.findViewById(R.id.search_box_text);
        LocaleManager localeManager = LocaleManager.getInstance();
        if (mSearchProviderIsGoogle && !localeManager.hasCompletedSearchEnginePromo()
                && !localeManager.hasShownSearchEnginePromoThisSession()
                && ChromeFeatureList.isEnabled(ChromeFeatureList.NTP_SHOW_GOOGLE_G_IN_OMNIBOX)) {
            searchBoxTextView.setCompoundDrawablePadding(
                    getResources().getDimensionPixelOffset(R.dimen.ntp_search_box_logo_padding));
            ApiCompatibilityUtils.setCompoundDrawablesRelativeWithIntrinsicBounds(
                    searchBoxTextView, R.drawable.ic_logo_googleg_24dp, 0, 0, 0);
        } else {
            searchBoxTextView.setCompoundDrawablePadding(0);

            // Not using the relative version of this call because we only want to clear
            // the drawables.
            searchBoxTextView.setCompoundDrawables(null, null, null, null);
        }
    }

    private void initializeVoiceSearchButton() {
        TraceEvent.begin(TAG + ".initializeVoiceSearchButton()");
        mVoiceSearchButton = (ImageView) mNewTabPageLayout.findViewById(R.id.voice_search_button);
        mVoiceSearchButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mManager.focusSearchBox(true, null);
            }
        });

        if (SuggestionsConfig.useModernLayout() && !DeviceFormFactor.isTablet()) {
            ApiCompatibilityUtils.setMarginEnd(
                    (MarginLayoutParams) mVoiceSearchButton.getLayoutParams(),
                    getResources().getDimensionPixelSize(
                            R.dimen.ntp_search_box_voice_search_margin_end_modern));
        }

        TraceEvent.end(TAG + ".initializeVoiceSearchButton()");
    }

    private void initializeLayoutChangeListeners() {
        TraceEvent.begin(TAG + ".initializeLayoutChangeListeners()");
        mNewTabPageLayout.addOnLayoutChangeListener(new OnLayoutChangeListener() {
            @Override
            public void onLayoutChange(View v, int left, int top, int right, int bottom,
                    int oldLeft, int oldTop, int oldRight, int oldBottom) {
                int oldHeight = oldBottom - oldTop;
                int newHeight = bottom - top;

                if (oldHeight == newHeight && !mTileCountChanged) return;
                mTileCountChanged = false;

                // Re-apply the url focus change amount after a rotation to ensure the views are
                // correctly placed with their new layout configurations.
                onUrlFocusAnimationChanged();
                updateSearchBoxOnScroll();

                // The positioning of elements may have been changed (since the elements expand to
                // fill the available vertical space), so adjust the scroll.
                mRecyclerView.snapScroll(mSearchBoxView, getHeight());
            }
        });

        // Listen for layout changes on the NewTabPageView itself to catch changes in scroll
        // position that are due to layout changes after e.g. device rotation. This contrasts with
        // regular scrolling, which is observed through an OnScrollListener.
        addOnLayoutChangeListener(new OnLayoutChangeListener() {
            @Override
            public void onLayoutChange(View v, int left, int top, int right, int bottom,
                    int oldLeft, int oldTop, int oldRight, int oldBottom) {
                int scrollY = mRecyclerView.computeVerticalScrollOffset();
                if (mLastScrollY != scrollY) {
                    mLastScrollY = scrollY;
                    handleScroll();
                }
            }
        });
        TraceEvent.end(TAG + ".initializeLayoutChangeListeners()");
    }

    private void updateSearchBoxOnScroll() {
        if (mDisableUrlFocusChangeAnimations || mIsMovingNewTabPageView) return;

        // When the page changes (tab switching or new page loading), it is possible that events
        // (e.g. delayed RecyclerView change notifications) trigger calls to these methods after
        // the current page changes. We check it again to make sure we don't attempt to update the
        // wrong page.
        if (!mManager.isCurrentPage()) return;

        if (mSearchBoxScrollListener != null) {
            mSearchBoxScrollListener.onNtpScrollChanged(getToolbarTransitionPercentage());
        }
    }

    /**
     * Calculates the percentage (between 0 and 1) of the transition from the search box to the
     * omnibox at the top of the New Tab Page, which is determined by the amount of scrolling and
     * the position of the search box.
     *
     * @return the transition percentage
     */
    private float getToolbarTransitionPercentage() {
        // During startup the view may not be fully initialized, so we only calculate the current
        // percentage if some basic view properties (height of the containing view, position of the
        // search box) are sane.
        if (getRecyclerView().getHeight() == 0) return 0f;

        if (!mRecyclerView.isFirstItemVisible()) {
            // getVerticalScroll is valid only for the RecyclerView if the first item is visible.
            // If the first item is not visible, we must have scrolled quite far and we know the
            // toolbar transition should be 100%. This might be the initial scroll position due to
            // the scroll restore feature, so the search box will not have been laid out yet.
            return 1f;
        }

        int searchBoxTop = mSearchBoxView.getTop();
        if (searchBoxTop == 0) return 0f;

        // For all other calculations, add the search box padding, because it defines where the
        // visible "border" of the search box is.
        searchBoxTop += mSearchBoxView.getPaddingTop();

        final int scrollY = mRecyclerView.computeVerticalScrollOffset();
        final float transitionLength =
                getResources().getDimension(R.dimen.ntp_search_box_transition_length);
        // Tab strip height is zero on phones, nonzero on tablets.
        int tabStripHeight = getResources().getDimensionPixelSize(R.dimen.tab_strip_height);

        // |scrollY - searchBoxTop + tabStripHeight| gives the distance the search bar is from the
        // top of the tab.
        return MathUtils.clamp((scrollY - searchBoxTop + transitionLength + tabStripHeight)
                / transitionLength, 0f, 1f);
    }

    @VisibleForTesting
    public NewTabPageRecyclerView getRecyclerView() {
        return mRecyclerView;
    }

    /**
     * @return The placeholder that is shown above the fold when there is no other content to show,
     *         or null if it has not been inflated yet.
     */
    @VisibleForTesting
    @Nullable
    public View getPlaceholder() {
        return mTileGridPlaceholder;
    }

    @VisibleForTesting
    public TileGroup getTileGroup() {
        return mTileGroup;
    }

    /**
     * Adds listeners to scrolling to take care of snap scrolling and updating the search box on
     * scroll.
     */
    private void setupScrollHandling() {
        TraceEvent.begin(TAG + ".setupScrollHandling()");
        mRecyclerView.addOnScrollListener(new RecyclerView.OnScrollListener() {
            @Override
            public void onScrolled(RecyclerView recyclerView, int dx, int dy) {
                mLastScrollY = mRecyclerView.computeVerticalScrollOffset();
                handleScroll();
            }
        });

        mRecyclerView.setOnTouchListener(new OnTouchListener() {
            @Override
            @SuppressLint("ClickableViewAccessibility")
            public boolean onTouch(View v, MotionEvent event) {
                mRecyclerView.removeCallbacks(mSnapScrollRunnable);

                if (event.getActionMasked() == MotionEvent.ACTION_CANCEL
                        || event.getActionMasked() == MotionEvent.ACTION_UP) {
                    mPendingSnapScroll = true;
                    mRecyclerView.postDelayed(mSnapScrollRunnable, SNAP_SCROLL_DELAY_MS);
                } else {
                    mPendingSnapScroll = false;
                }
                return false;
            }
        });
        TraceEvent.end(TAG + ".setupScrollHandling()");
    }

    private void handleScroll() {
        if (mPendingSnapScroll) {
            mRecyclerView.removeCallbacks(mSnapScrollRunnable);
            mRecyclerView.postDelayed(mSnapScrollRunnable, SNAP_SCROLL_DELAY_MS);
        }
        updateSearchBoxOnScroll();
    }

    /**
     * Should be called every time of the flags used to track initialisation progress changes.
     * Finalises initialisation once all the preliminary steps are complete.
     *
     * @see #mHasShownView
     * @see #mTilesLoaded
     */
    private void onInitialisationProgressChanged() {
        if (!hasLoadCompleted()) return;

        mManager.onLoadingComplete();

        // Load the logo after everything else is finished, since it's lower priority.
        loadSearchProviderLogo();
    }

    /**
     * To be called to notify that the tiles have finished loading. Will do nothing if a load was
     * previously completed.
     */
    public void onTilesLoaded() {
        if (mTilesLoaded) return;
        mTilesLoaded = true;

        onInitialisationProgressChanged();
    }

    /**
     * Loads the search provider logo (e.g. Google doodle), if any.
     */
    public void loadSearchProviderLogo() {
        if (!mSearchProviderHasLogo) return;

        mSearchProviderLogoView.showSearchProviderInitialView();

        mLogoDelegate.getSearchProviderLogo(new LogoObserver() {
            @Override
            public void onLogoAvailable(Logo logo, boolean fromCache) {
                if (logo == null && fromCache) return;

                mSearchProviderLogoView.setDelegate(mLogoDelegate);
                mSearchProviderLogoView.updateLogo(logo);
                mSnapshotTileGridChanged = true;
            }
        });
    }

    /**
     * Changes the layout depending on whether the selected search provider (e.g. Google, Bing)
     * has a logo.
     * @param hasLogo Whether the search provider has a logo.
     * @param isGoogle Whether the search provider is Google.
     */
    public void setSearchProviderInfo(boolean hasLogo, boolean isGoogle) {
        if (hasLogo == mSearchProviderHasLogo && isGoogle == mSearchProviderIsGoogle
                && mInitialized) {
            return;
        }
        mSearchProviderHasLogo = hasLogo;
        mSearchProviderIsGoogle = isGoogle;

        updateTileGridPadding();

        // Hide or show the views above the tile grid as needed, including logo, search box, and
        // spacers.
        int visibility = mSearchProviderHasLogo ? View.VISIBLE : View.GONE;
        int logoVisibility = shouldShowLogo() ? View.VISIBLE : View.GONE;
        int childCount = mNewTabPageLayout.getChildCount();
        for (int i = 0; i < childCount; i++) {
            View child = mNewTabPageLayout.getChildAt(i);
            if (mShortcutsView != null && child == mShortcutsView) break;
            if (child == mSiteSectionViewHolder.itemView) break;

            // Don't change the visibility of a ViewStub as that will automagically inflate it.
            if (child instanceof ViewStub) continue;

            if (child == mSearchProviderLogoView) {
                child.setVisibility(logoVisibility);
            } else {
                child.setVisibility(visibility);
            }
        }

        // Update snap scrolling for the fakebox.
        mRecyclerView.setContainsLocationBar(mManager.isLocationBarShownInNTP());

        updateTileGridPlaceholderVisibility();

        onUrlFocusAnimationChanged();

        updateSearchBoxLogo();

        mSnapshotTileGridChanged = true;
    }

    /**
     * Updates the padding for the tile grid based on what is shown above it.
     */
    private void updateTileGridPadding() {
        int paddingTop;
        if (mShortcutsView != null) {
            // If the shortcuts view is visible, padding will be built into that view.
            paddingTop = 0;
        } else {
            int paddingWithLogoId = SuggestionsConfig.useModernLayout()
                    ? R.dimen.tile_grid_layout_modern_padding_top
                    : R.dimen.tile_grid_layout_padding_top;
            // Set a bit more top padding on the tile grid if there is no logo.
            paddingTop = getResources().getDimensionPixelSize(shouldShowLogo()
                            ? paddingWithLogoId
                            : R.dimen.tile_grid_layout_no_logo_padding_top);
        }

        mSiteSectionViewHolder.itemView.setPadding(
                0, paddingTop, 0, mSiteSectionViewHolder.itemView.getPaddingBottom());
    }

    /**
     * Updates whether the NewTabPage should animate on URL focus changes.
     * @param disable Whether to disable the animations.
     */
    void setUrlFocusAnimationsDisabled(boolean disable) {
        if (disable == mDisableUrlFocusChangeAnimations) return;
        mDisableUrlFocusChangeAnimations = disable;
        if (!disable) onUrlFocusAnimationChanged();
    }

    /**
     * @return Whether URL focus animations are currently disabled.
     */
    boolean urlFocusAnimationsDisabled() {
        return mDisableUrlFocusChangeAnimations;
    }

    /**
     * Specifies the percentage the URL is focused during an animation.  1.0 specifies that the URL
     * bar has focus and has completed the focus animation.  0 is when the URL bar is does not have
     * any focus.
     *
     * @param percent The percentage of the URL bar focus animation.
     */
    void setUrlFocusChangeAnimationPercent(float percent) {
        mUrlFocusChangePercent = percent;
        onUrlFocusAnimationChanged();
    }

    /**
     * @return The percentage that the URL bar is focused during an animation.
     */
    @VisibleForTesting
    float getUrlFocusChangeAnimationPercent() {
        return mUrlFocusChangePercent;
    }

    private void onUrlFocusAnimationChanged() {
        if (mDisableUrlFocusChangeAnimations || FeatureUtilities.isChromeHomeEnabled()
                || mIsMovingNewTabPageView) {
            return;
        }

        // Translate so that the search box is at the top, but only upwards.
        float percent = mSearchProviderHasLogo ? mUrlFocusChangePercent : 0;
        int basePosition = mRecyclerView.computeVerticalScrollOffset()
                + mNewTabPageLayout.getPaddingTop();
        int target = Math.max(basePosition,
                mSearchBoxView.getBottom() - mSearchBoxView.getPaddingBottom()
                        - mSearchBoxBoundsVerticalInset);

        mNewTabPageLayout.setTranslationY(percent * (basePosition - target));
    }

    /**
     * Updates the opacity of the search box when scrolling.
     *
     * @param alpha opacity (alpha) value to use.
     */
    public void setSearchBoxAlpha(float alpha) {
        mSearchBoxView.setAlpha(alpha);

        // Disable the search box contents if it is the process of being animated away.
        ViewUtils.setEnabledRecursive(mSearchBoxView, mSearchBoxView.getAlpha() == 1.0f);
    }

    /**
     * Updates the opacity of the search provider logo when scrolling.
     *
     * @param alpha opacity (alpha) value to use.
     */
    public void setSearchProviderLogoAlpha(float alpha) {
        mSearchProviderLogoView.setAlpha(alpha);
    }

    /**
     * Set the search box background drawable.
     *
     * @param drawable The search box background.
     */
    public void setSearchBoxBackground(Drawable drawable) {
        mSearchBoxView.setBackground(drawable);
    }

    /**
     * Get the bounds of the search box in relation to the top level NewTabPage view.
     *
     * @param bounds The current drawing location of the search box.
     * @param translation The translation applied to the search box by the parent view hierarchy up
     *                    to the NewTabPage view.
     */
    void getSearchBoxBounds(Rect bounds, Point translation) {
        int searchBoxX = (int) mSearchBoxView.getX();
        int searchBoxY = (int) mSearchBoxView.getY();
        bounds.set(searchBoxX + mSearchBoxView.getPaddingLeft(),
                searchBoxY + mSearchBoxView.getPaddingTop(),
                searchBoxX + mSearchBoxView.getWidth() - mSearchBoxView.getPaddingRight(),
                searchBoxY + mSearchBoxView.getHeight() - mSearchBoxView.getPaddingBottom());

        translation.set(0, 0);

        View view = mSearchBoxView;
        while (true) {
            view = (View) view.getParent();
            if (view == null) {
                // The |mSearchBoxView| is not a child of this view. This can happen if the
                // RecyclerView detaches the NewTabPageLayout after it has been scrolled out of
                // view. Set the translation to the minimum Y value as an approximation.
                translation.y = Integer.MIN_VALUE;
                break;
            }
            translation.offset(-view.getScrollX(), -view.getScrollY());
            if (view == this) break;
            translation.offset((int) view.getX(), (int) view.getY());
        }
        bounds.offset(translation.x, translation.y);

        if (translation.y != Integer.MIN_VALUE) {
            bounds.inset(0, mSearchBoxBoundsVerticalInset);
        }
    }

    /**
     * Sets the listener for search box scroll changes.
     * @param listener The listener to be notified on changes.
     */
    void setSearchBoxScrollListener(OnSearchBoxScrollListener listener) {
        mSearchBoxScrollListener = listener;
        if (mSearchBoxScrollListener != null) updateSearchBoxOnScroll();
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        assert mManager != null;

        if (!mHasShownView) {
            mHasShownView = true;
            onInitialisationProgressChanged();
            NewTabPageUma.recordSearchAvailableLoadTime(mTab.getActivity());
            TraceEvent.instant("NewTabPageSearchAvailable)");
        } else {
            // Trigger a scroll update when reattaching the window to signal the toolbar that
            // it needs to reset the NTP state.
            if (mManager.isLocationBarShownInNTP()) updateSearchBoxOnScroll();
        }
    }

    /**
     * Update the visibility of the voice search button based on whether the feature is currently
     * enabled.
     */
    void updateVoiceSearchButtonVisibility() {
        mVoiceSearchButton.setVisibility(mManager.isVoiceSearchEnabled() ? VISIBLE : GONE);
    }

    @Override
    protected void onWindowVisibilityChanged(int visibility) {
        super.onWindowVisibilityChanged(visibility);

        // On first run, the NewTabPageView is initialized behind the First Run Experience, meaning
        // the UiConfig will pickup the screen layout then. However onConfigurationChanged is not
        // called on orientation changes until the FRE is completed. This means that if a user
        // starts the FRE in one orientation, changes an orientation and then leaves the FRE the
        // UiConfig will have the wrong orientation. https://crbug.com/683886.
        mUiConfig.updateDisplayStyle();

        if (visibility == VISIBLE) {
            updateVoiceSearchButtonVisibility();
        }
    }

    /**
     * @see org.chromium.chrome.browser.compositor.layouts.content.
     *         InvalidationAwareThumbnailProvider#shouldCaptureThumbnail()
     */
    boolean shouldCaptureThumbnail() {
        if (getWidth() == 0 || getHeight() == 0) return false;

        return mNewTabPageRecyclerViewChanged || mSnapshotTileGridChanged
                || getWidth() != mSnapshotWidth || getHeight() != mSnapshotHeight
                || mRecyclerView.computeVerticalScrollOffset() != mSnapshotScrollY;
    }

    /**
     * @see org.chromium.chrome.browser.compositor.layouts.content.
     *         InvalidationAwareThumbnailProvider#captureThumbnail(Canvas)
     */
    void captureThumbnail(Canvas canvas) {
        mSearchProviderLogoView.endFadeAnimation();
        ViewUtils.captureBitmap(this, canvas);
        mSnapshotWidth = getWidth();
        mSnapshotHeight = getHeight();
        mSnapshotScrollY = mRecyclerView.computeVerticalScrollOffset();
        mSnapshotTileGridChanged = false;
        mNewTabPageRecyclerViewChanged = false;
    }

    /**
     * Shows the most visited placeholder ("Nothing to see here") if there are no most visited
     * items and there is no search provider logo.
     */
    private void updateTileGridPlaceholderVisibility() {
        boolean showPlaceholder =
                mTileGroup.hasReceivedData() && mTileGroup.isEmpty() && !mSearchProviderHasLogo;

        mNoSearchLogoSpacer.setVisibility(
                (mSearchProviderHasLogo || showPlaceholder) ? View.GONE : View.INVISIBLE);

        mSiteSectionViewHolder.itemView.setVisibility(showPlaceholder ? GONE : VISIBLE);

        if (showPlaceholder) {
            if (mTileGridPlaceholder == null) {
                ViewStub placeholderStub =
                        mNewTabPageLayout.findViewById(R.id.tile_grid_placeholder_stub);
                mTileGridPlaceholder = placeholderStub.inflate();
            }
            mTileGridPlaceholder.setVisibility(VISIBLE);
        } else if (mTileGridPlaceholder != null) {
            mTileGridPlaceholder.setVisibility(GONE);
        }
    }

    private static int getMaxTileRows(boolean searchProviderHasLogo) {
        return 2;
    }

    /**
     * Determines The maximum number of tiles to try and fit in a row. On smaller screens, there
     * may not be enough space to fit all of them.
     */
    private int getMaxTileColumns() {
        if (!mUiConfig.getCurrentDisplayStyle().isSmall()
                && SuggestionsConfig.getTileStyle(mUiConfig) == TileView.Style.CLASSIC_CONDENSED) {
            return 5;
        }
        return 4;
    }

    private static int getTileTitleLines() {
        return 1;
    }

    private boolean shouldShowLogo() {
        return mSearchProviderHasLogo;
    }

    /**
     * Scrolls to the top of content suggestions header if one exists. If not, scrolls to the top
     * of the first article suggestion. Uses scrollToPositionWithOffset to position the suggestions
     * below the toolbar and not below the status bar.
     */
    void scrollToSuggestions() {
        int scrollPosition = getSuggestionsScrollPosition();
        // Nothing to scroll to; return early.
        if (scrollPosition == RecyclerView.NO_POSITION) return;

        mRecyclerView.getLinearLayoutManager().scrollToPositionWithOffset(
                scrollPosition, getScrollToSuggestionsOffset());
    }

    /**
     * Retrieves the position of articles or of their header in the NTP adapter to scroll to.
     * @return The header's position if a header is present. Otherwise, the first
     *         suggestion card's position.
     */
    private int getSuggestionsScrollPosition() {
        // Header always exists.
        if (ChromeFeatureList.isEnabled(
                    ChromeFeatureList.NTP_ARTICLE_SUGGESTIONS_EXPANDABLE_HEADER)) {
            return mRecyclerView.getNewTabPageAdapter().getArticleHeaderPosition();
        }

        // Only articles are visible. Headers are not present.
        if (ChromeFeatureList.isEnabled(ChromeFeatureList.SIMPLIFIED_NTP)) {
            return mRecyclerView.getNewTabPageAdapter().getFirstSnippetPosition();
        }

        // With Simplified NTP not enabled, bookmarks/downloads and their headers are added to the
        // NTP if they're not empty.
        int scrollPosition = mRecyclerView.getNewTabPageAdapter().getArticleHeaderPosition();
        return scrollPosition == RecyclerView.NO_POSITION
                ? mRecyclerView.getNewTabPageAdapter().getFirstSnippetPosition()
                : scrollPosition;
    }

    private int getScrollToSuggestionsOffset() {
        int offset = getResources().getDimensionPixelSize(R.dimen.toolbar_height_no_shadow);

        if (needsExtraOffset()) {
            offset += getResources().getDimensionPixelSize(
                              R.dimen.content_suggestions_card_modern_margin)
                    / 2;
        }
        return offset;
    }

    /**
     * Checks if extra offset needs to be added for aesthetic reasons.
     * @return True if modern is enabled (and space exists between each suggestion card) and no
     *         header is showing.
     */
    private boolean needsExtraOffset() {
        return SuggestionsConfig.useModernLayout()
                && !ChromeFeatureList.isEnabled(
                           ChromeFeatureList.NTP_ARTICLE_SUGGESTIONS_EXPANDABLE_HEADER)
                && mRecyclerView.getNewTabPageAdapter().getArticleHeaderPosition()
                == RecyclerView.NO_POSITION;
    }

    /**
     * @return The adapter position the user has scrolled to.
     */
    public int getScrollPosition() {
        return mRecyclerView.getScrollPosition();
    }

    private boolean hasLoadCompleted() {
        return mHasShownView && mTilesLoaded;
    }

    // TileGroup.Observer interface.

    @Override
    public void onTileDataChanged() {
        mSiteSectionViewHolder.refreshData();
        mSnapshotTileGridChanged = true;

        // The page contents are initially hidden; otherwise they'll be drawn centered on the page
        // before the tiles are available and then jump upwards to make space once the tiles are
        // available.
        if (mNewTabPageLayout.getVisibility() != View.VISIBLE) {
            mNewTabPageLayout.setVisibility(View.VISIBLE);
        }
    }

    @Override
    public void onTileCountChanged() {
        // If the number of tile rows change while the URL bar is focused, the icons'
        // position will be wrong. Schedule the translation to be updated.
        if (mUrlFocusChangePercent == 1f) mTileCountChanged = true;
        updateTileGridPlaceholderVisibility();
    }

    @Override
    public void onTileIconChanged(Tile tile) {
        mSiteSectionViewHolder.updateIconView(tile);
        mSnapshotTileGridChanged = true;
    }

    @Override
    public void onTileOfflineBadgeVisibilityChanged(Tile tile) {
        mSiteSectionViewHolder.updateOfflineBadge(tile);
        mSnapshotTileGridChanged = true;
    }

    private class SnapScrollRunnable implements Runnable {
        @Override
        public void run() {
            assert mPendingSnapScroll;
            mPendingSnapScroll = false;

            mRecyclerView.snapScroll(mSearchBoxView, getHeight());
        }
    }

    private class UpdateSearchBoxOnScrollRunnable implements Runnable {
        @Override
        public void run() {
            updateSearchBoxOnScroll();
        }
    }

    @Override
    public void onEnterVr() {
        mSearchBoxView.setVisibility(GONE);
    }

    @Override
    public void onExitVr() {
        mSearchBoxView.setVisibility(VISIBLE);
    }

    private void onDestroy() {
        mTab.getWindowAndroid().removeContextMenuCloseListener(mContextMenuManager);
        VrShellDelegate.unregisterVrModeObserver(this);
    }

    private void initializeShortcuts() {
        if (!ChromeFeatureList.isEnabled(ChromeFeatureList.SIMPLIFIED_NTP)
                || isSimplifiedNtpAblationEnabled()) {
            return;
        }

        ViewStub shortcutsStub =
                mRecyclerView.getAboveTheFoldView().findViewById(R.id.shortcuts_stub);
        mShortcutsView = (ViewGroup) shortcutsStub.inflate();

        mShortcutsView.findViewById(R.id.bookmarks_button)
                .setOnClickListener(view -> mManager.getNavigationDelegate().navigateToBookmarks());

        mShortcutsView.findViewById(R.id.downloads_button)
                .setOnClickListener(
                        view -> mManager.getNavigationDelegate().navigateToDownloadManager());
    }
}
