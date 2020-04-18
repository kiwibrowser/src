package com.android.ex.photo;

import android.app.Activity;
import android.app.ActivityManager;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.database.Cursor;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Process;
import android.support.annotation.IdRes;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.LoaderManager;
import android.support.v4.content.Loader;
import android.support.v4.view.ViewPager.OnPageChangeListener;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewPropertyAnimator;
import android.view.ViewTreeObserver.OnGlobalLayoutListener;
import android.view.WindowManager;
import android.view.accessibility.AccessibilityManager;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.view.animation.Animation.AnimationListener;
import android.view.animation.AnimationSet;
import android.view.animation.ScaleAnimation;
import android.view.animation.TranslateAnimation;
import android.widget.ImageView;

import com.android.ex.photo.ActionBarInterface.OnMenuVisibilityListener;
import com.android.ex.photo.PhotoViewPager.InterceptType;
import com.android.ex.photo.PhotoViewPager.OnInterceptTouchListener;
import com.android.ex.photo.adapters.PhotoPagerAdapter;
import com.android.ex.photo.fragments.PhotoViewFragment;
import com.android.ex.photo.loaders.PhotoBitmapLoader;
import com.android.ex.photo.loaders.PhotoBitmapLoaderInterface.BitmapResult;
import com.android.ex.photo.loaders.PhotoPagerLoader;
import com.android.ex.photo.provider.PhotoContract;
import com.android.ex.photo.util.ImageUtils;
import com.android.ex.photo.util.Util;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

/**
 * This class implements all the logic of the photo view activity. An activity should use this class
 * calling through from relevant activity methods to the methods of the same name here.
 *
 * To customize the photo viewer activity, you should subclass this and implement your
 * customizations here. Then subclass {@link PhotoViewActivity} and override just
 * {@link PhotoViewActivity#createController createController} to instantiate your controller
 * subclass.
 */
public class PhotoViewController implements
        LoaderManager.LoaderCallbacks<Cursor>, OnPageChangeListener, OnInterceptTouchListener,
        OnMenuVisibilityListener, PhotoViewCallbacks  {

    /**
     * Defines the interface between the Activity and this class.
     *
     * The activity itself must delegate all appropriate method calls into this class, to the
     * methods of the same name.
     */
    public interface ActivityInterface {
        public Context getContext();
        public Context getApplicationContext();
        public Intent getIntent();
        public void setContentView(int resId);
        public View findViewById(int id);
        public Resources getResources();
        public FragmentManager getSupportFragmentManager();
        public LoaderManager getSupportLoaderManager();
        public ActionBarInterface getActionBarInterface();
        public boolean onOptionsItemSelected(MenuItem item);
        public void finish();
        public void overridePendingTransition(int enterAnim, int exitAnim);
        public PhotoViewController getController();
    }

    private final static String TAG = "PhotoViewController";

    private final static String STATE_INITIAL_URI_KEY =
            "com.android.ex.PhotoViewFragment.INITIAL_URI";
    private final static String STATE_CURRENT_URI_KEY =
            "com.android.ex.PhotoViewFragment.CURRENT_URI";
    private final static String STATE_CURRENT_INDEX_KEY =
            "com.android.ex.PhotoViewFragment.CURRENT_INDEX";
    private final static String STATE_FULLSCREEN_KEY =
            "com.android.ex.PhotoViewFragment.FULLSCREEN";
    private final static String STATE_ACTIONBARTITLE_KEY =
            "com.android.ex.PhotoViewFragment.ACTIONBARTITLE";
    private final static String STATE_ACTIONBARSUBTITLE_KEY =
            "com.android.ex.PhotoViewFragment.ACTIONBARSUBTITLE";
    private final static String STATE_ENTERANIMATIONFINISHED_KEY =
            "com.android.ex.PhotoViewFragment.SCALEANIMATIONFINISHED";

    protected final static String ARG_IMAGE_URI = "image_uri";

    public static final int LOADER_PHOTO_LIST = 100;

    /** Count used when the real photo count is unknown [but, may be determined] */
    public static final int ALBUM_COUNT_UNKNOWN = -1;

    public static final int ENTER_ANIMATION_DURATION_MS = 250;
    public static final int EXIT_ANIMATION_DURATION_MS = 250;

    /** Argument key for the dialog message */
    public static final String KEY_MESSAGE = "dialog_message";

    public static int sMemoryClass;
    public static int sMaxPhotoSize; // The maximum size (either width or height)

    private final ActivityInterface mActivity;

    private int mLastFlags;

    private final View.OnSystemUiVisibilityChangeListener mSystemUiVisibilityChangeListener;

    /** The URI of the photos we're viewing; may be {@code null} */
    private String mPhotosUri;
    /** The uri of the initial photo */
    private String mInitialPhotoUri;
    /** The index of the currently viewed photo */
    private int mCurrentPhotoIndex;
    /** The uri of the currently viewed photo */
    private String mCurrentPhotoUri;
    /** The query projection to use; may be {@code null} */
    private String[] mProjection;
    /** The total number of photos; only valid if {@link #mIsEmpty} is {@code false}. */
    protected int mAlbumCount = ALBUM_COUNT_UNKNOWN;
    /** {@code true} if the view is empty. Otherwise, {@code false}. */
    protected boolean mIsEmpty;
    /** the main root view */
    protected View mRootView;
    /** Background image that contains nothing, so it can be alpha faded from
     * transparent to black without affecting any other views. */
    @Nullable
    protected View mBackground;
    /** The main pager; provides left/right swipe between photos */
    protected PhotoViewPager mViewPager;
    /** The temporary image so that we can quickly scale up the fullscreen thumbnail */
    @Nullable
    protected ImageView mTemporaryImage;
    /** Adapter to create pager views */
    protected PhotoPagerAdapter mAdapter;
    /** Whether or not we're in "full screen" mode */
    protected boolean mFullScreen;
    /** The listeners wanting full screen state for each screen position */
    private final Map<Integer, OnScreenListener>
            mScreenListeners = new HashMap<Integer, OnScreenListener>();
    /** The set of listeners wanting full screen state */
    private final Set<CursorChangedListener> mCursorListeners = new HashSet<CursorChangedListener>();
    /** When {@code true}, restart the loader when the activity becomes active */
    private boolean mKickLoader;
    /** Don't attempt operations that may trigger a fragment transaction when the activity is
     * destroyed */
    private boolean mIsDestroyedCompat;
    /** Whether or not this activity is paused */
    protected boolean mIsPaused = true;
    /** The maximum scale factor applied to images when they are initially displayed */
    protected float mMaxInitialScale;
    /** The title in the actionbar */
    protected String mActionBarTitle;
    /** The subtitle in the actionbar */
    protected String mActionBarSubtitle;

    private boolean mEnterAnimationFinished;
    protected boolean mScaleAnimationEnabled;
    protected int mAnimationStartX;
    protected int mAnimationStartY;
    protected int mAnimationStartWidth;
    protected int mAnimationStartHeight;

    /** Whether lights out should invoked based on timer */
    protected boolean mIsTimerLightsOutEnabled;
    protected boolean mActionBarHiddenInitially;
    protected boolean mDisplayThumbsFullScreen;

    private final AccessibilityManager mAccessibilityManager;

    protected BitmapCallback mBitmapCallback;
    protected final Handler mHandler = new Handler();

    // TODO Find a better way to do this. We basically want the activity to display the
    // "loading..." progress until the fragment takes over and shows it's own "loading..."
    // progress [located in photo_header_view.xml]. We could potentially have all status displayed
    // by the activity, but, that gets tricky when it comes to screen rotation. For now, we
    // track the loading by this variable which is fragile and may cause phantom "loading..."
    // text.
    private long mEnterFullScreenDelayTime;

    private int lastAnnouncedTitle = -1;

    public PhotoViewController(ActivityInterface activity) {
        mActivity = activity;

        // View.OnSystemUiVisibilityChangeListener is an API that was introduced in API level 11.
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.HONEYCOMB) {
            mSystemUiVisibilityChangeListener = null;
        } else {
            mSystemUiVisibilityChangeListener = new View.OnSystemUiVisibilityChangeListener() {
                @Override
                public void onSystemUiVisibilityChange(int visibility) {
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT &&
                            visibility == 0 && mLastFlags == 3846) {
                        setFullScreen(false /* fullscreen */, true /* setDelayedRunnable */);
                    }
                }
            };
        }

        mAccessibilityManager = (AccessibilityManager)
                activity.getContext().getSystemService(Context.ACCESSIBILITY_SERVICE);
    }

    public PhotoPagerAdapter createPhotoPagerAdapter(Context context,
            android.support.v4.app.FragmentManager fm, Cursor c, float maxScale) {
        return new PhotoPagerAdapter(context, fm, c, maxScale, mDisplayThumbsFullScreen);
    }

    public PhotoViewController.ActivityInterface getActivity() {
        return mActivity;
    }

    public void onCreate(Bundle savedInstanceState) {
        initMaxPhotoSize();
        final ActivityManager mgr = (ActivityManager) mActivity.getApplicationContext().
                getSystemService(Activity.ACTIVITY_SERVICE);
        sMemoryClass = mgr.getMemoryClass();

        final Intent intent = mActivity.getIntent();
        // uri of the photos to view; optional
        if (intent.hasExtra(Intents.EXTRA_PHOTOS_URI)) {
            mPhotosUri = intent.getStringExtra(Intents.EXTRA_PHOTOS_URI);
        }

        mIsTimerLightsOutEnabled = intent.getBooleanExtra(
                Intents.EXTRA_ENABLE_TIMER_LIGHTS_OUT, true);

        if (intent.getBooleanExtra(Intents.EXTRA_SCALE_UP_ANIMATION, false)) {
            mScaleAnimationEnabled = true;
            mAnimationStartX = intent.getIntExtra(Intents.EXTRA_ANIMATION_START_X, 0);
            mAnimationStartY = intent.getIntExtra(Intents.EXTRA_ANIMATION_START_Y, 0);
            mAnimationStartWidth = intent.getIntExtra(Intents.EXTRA_ANIMATION_START_WIDTH, 0);
            mAnimationStartHeight = intent.getIntExtra(Intents.EXTRA_ANIMATION_START_HEIGHT, 0);
        }
        mActionBarHiddenInitially = intent.getBooleanExtra(
                Intents.EXTRA_ACTION_BAR_HIDDEN_INITIALLY, false)
                && !Util.isTouchExplorationEnabled(mAccessibilityManager);
        mDisplayThumbsFullScreen = intent.getBooleanExtra(
                Intents.EXTRA_DISPLAY_THUMBS_FULLSCREEN, false);

        // projection for the query; optional
        // If not set, the default projection is used.
        // This projection must include the columns from the default projection.
        if (intent.hasExtra(Intents.EXTRA_PROJECTION)) {
            mProjection = intent.getStringArrayExtra(Intents.EXTRA_PROJECTION);
        } else {
            mProjection = null;
        }

        // Set the max initial scale, defaulting to 1x
        mMaxInitialScale = intent.getFloatExtra(Intents.EXTRA_MAX_INITIAL_SCALE, 1.0f);
        mCurrentPhotoUri = null;
        mCurrentPhotoIndex = -1;

        // We allow specifying the current photo by either index or uri.
        // This is because some users may have live datasets that can change,
        // adding new items to either the beginning or end of the set. For clients
        // that do not need that capability, ability to specify the current photo
        // by index is offered as a convenience.
        if (intent.hasExtra(Intents.EXTRA_PHOTO_INDEX)) {
            mCurrentPhotoIndex = intent.getIntExtra(Intents.EXTRA_PHOTO_INDEX, -1);
        }
        if (intent.hasExtra(Intents.EXTRA_INITIAL_PHOTO_URI)) {
            mInitialPhotoUri = intent.getStringExtra(Intents.EXTRA_INITIAL_PHOTO_URI);
            mCurrentPhotoUri = mInitialPhotoUri;
        }
        mIsEmpty = true;

        if (savedInstanceState != null) {
            mInitialPhotoUri = savedInstanceState.getString(STATE_INITIAL_URI_KEY);
            mCurrentPhotoUri = savedInstanceState.getString(STATE_CURRENT_URI_KEY);
            mCurrentPhotoIndex = savedInstanceState.getInt(STATE_CURRENT_INDEX_KEY);
            mFullScreen = savedInstanceState.getBoolean(STATE_FULLSCREEN_KEY, false)
                    && !Util.isTouchExplorationEnabled(mAccessibilityManager);
            mActionBarTitle = savedInstanceState.getString(STATE_ACTIONBARTITLE_KEY);
            mActionBarSubtitle = savedInstanceState.getString(STATE_ACTIONBARSUBTITLE_KEY);
            mEnterAnimationFinished = savedInstanceState.getBoolean(
                    STATE_ENTERANIMATIONFINISHED_KEY, false);
        } else {
            mFullScreen = mActionBarHiddenInitially;
        }

        mActivity.setContentView(getContentViewId());

        // Create the adapter and add the view pager
        mAdapter = createPhotoPagerAdapter(mActivity.getContext(),
                        mActivity.getSupportFragmentManager(), null, mMaxInitialScale);
        final Resources resources = mActivity.getResources();
        mRootView = findViewById(getRootViewId());
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
            mRootView.setOnSystemUiVisibilityChangeListener(getSystemUiVisibilityChangeListener());
        }
        mBackground = getBackground();
        mTemporaryImage = getTemporaryImage();
        mViewPager = (PhotoViewPager) findViewById(R.id.photo_view_pager);
        mViewPager.setAdapter(mAdapter);
        mViewPager.setOnPageChangeListener(this);
        mViewPager.setOnInterceptTouchListener(this);
        mViewPager.setPageMargin(resources.getDimensionPixelSize(R.dimen.photo_page_margin));

        mBitmapCallback = new BitmapCallback();
        if (!mScaleAnimationEnabled || mEnterAnimationFinished) {
            // We are not running the scale up animation. Just let the fragments
            // display and handle the animation.
            mActivity.getSupportLoaderManager().initLoader(LOADER_PHOTO_LIST, null, this);
            // Make the background opaque immediately so that we don't see the activity
            // behind this one.
            if (hasBackground()) {
                mBackground.setVisibility(View.VISIBLE);
            }
        } else {
            // Attempt to load the initial image thumbnail. Once we have the
            // image, animate it up. Once the animation is complete, we can kick off
            // loading the ViewPager. After the primary fullres image is loaded, we will
            // make our temporary image invisible and display the ViewPager.
            mViewPager.setVisibility(View.GONE);
            Bundle args = new Bundle();
            args.putString(ARG_IMAGE_URI, mInitialPhotoUri);
            mActivity.getSupportLoaderManager().initLoader(
                    BITMAP_LOADER_THUMBNAIL, args, mBitmapCallback);
        }

        mEnterFullScreenDelayTime =
                resources.getInteger(R.integer.reenter_fullscreen_delay_time_in_millis);

        final ActionBarInterface actionBar = mActivity.getActionBarInterface();
        if (actionBar != null) {
            actionBar.setDisplayHomeAsUpEnabled(true);
            actionBar.addOnMenuVisibilityListener(this);
            actionBar.setDisplayOptionsShowTitle();
            // Set the title and subtitle immediately here, rather than waiting
            // for the fragment to be initialized.
            setActionBarTitles(actionBar);
        }

        if (!mScaleAnimationEnabled) {
            setLightsOutMode(mFullScreen);
        } else {
            // Keep lights out mode as false. This is to prevent jank cause by concurrent
            // animations during the enter animation.
            setLightsOutMode(false);
        }
    }

    private void initMaxPhotoSize() {
        if (sMaxPhotoSize == 0) {
            final DisplayMetrics metrics = new DisplayMetrics();
            final WindowManager wm = (WindowManager)
                    mActivity.getContext().getSystemService(Context.WINDOW_SERVICE);
            final ImageUtils.ImageSize imageSize = ImageUtils.sUseImageSize;
            wm.getDefaultDisplay().getMetrics(metrics);
            switch (imageSize) {
                case EXTRA_SMALL:
                    // Use a photo that's 80% of the "small" size
                    sMaxPhotoSize = (Math.min(metrics.heightPixels, metrics.widthPixels) * 800) / 1000;
                    break;
                case SMALL:
                    // Fall through.
                case NORMAL:
                    // Fall through.
                default:
                    sMaxPhotoSize = Math.min(metrics.heightPixels, metrics.widthPixels);
                    break;
            }
        }
    }

    public boolean onCreateOptionsMenu(Menu menu) {
        return true;
    }

    public boolean onPrepareOptionsMenu(Menu menu) {
        return true;
    }

    public void onActivityResult(int requestCode, int resultCode, Intent data) {}

    protected View findViewById(int id) {
        return mActivity.findViewById(id);
    }

    /**
     * Returns the android id of the viewer's root view. Subclasses should override this method if
     * they provide their own layout.
     */
    @IdRes
    protected int getRootViewId() {
      return R.id.photo_activity_root_view;
    }

    /**
     * Returns the android layout id of the root layout that should be inflated for the viewer.
     * Subclasses should override this method if they provide their own layout.
     */
    @IdRes
    protected int getContentViewId() {
        return R.layout.photo_activity_view;
    }

    /**
     * Returns the android view for the viewer's background view, if it has one. Subclasses should
     * override this if they have a different (or no) background view.
     */
    @Nullable
    protected View getBackground() {
        return findViewById(R.id.photo_activity_background);
    }

    /**
     * Returns whether or not the view has a background object. Subclasses should override this if
     * they do not contain a background object.
     */
    protected boolean hasBackground() {
        return mBackground != null;
    }

    /**
     * Returns the android view for the viewer's temporary image, if it has one. Subclasses should
     * override this if they have a different (or no) temporary image view.
     */
    @Nullable
    protected ImageView getTemporaryImage() {
        return (ImageView) findViewById(R.id.photo_activity_temporary_image);
    }

    /**
     * Returns whether or not the view has a temporary image view. Subclasses should override this
     * if they do not use a temporary image.
     */
    protected boolean hasTemporaryImage() {
        return mTemporaryImage != null;
    }

    public void onStart() {}

    public void onResume() {
        setFullScreen(mFullScreen, false);

        mIsPaused = false;
        if (mKickLoader) {
            mKickLoader = false;
            mActivity.getSupportLoaderManager().initLoader(LOADER_PHOTO_LIST, null, this);
        }
    }

    public void onPause() {
        mIsPaused = true;
    }

    public void onStop() {}

    public void onDestroy() {
        mIsDestroyedCompat = true;
    }

    private boolean isDestroyedCompat() {
        return mIsDestroyedCompat;
    }

    public boolean onBackPressed() {
        // If we are in fullscreen mode, and the default is not full screen, then
        // switch back to actionBar display mode.
        if (mFullScreen && !mActionBarHiddenInitially) {
            toggleFullScreen();
        } else {
            if (mScaleAnimationEnabled) {
                runExitAnimation();
            } else {
                return false;
            }
        }
        return true;
    }

    public void onSaveInstanceState(Bundle outState) {
        outState.putString(STATE_INITIAL_URI_KEY, mInitialPhotoUri);
        outState.putString(STATE_CURRENT_URI_KEY, mCurrentPhotoUri);
        outState.putInt(STATE_CURRENT_INDEX_KEY, mCurrentPhotoIndex);
        outState.putBoolean(STATE_FULLSCREEN_KEY, mFullScreen);
        outState.putString(STATE_ACTIONBARTITLE_KEY, mActionBarTitle);
        outState.putString(STATE_ACTIONBARSUBTITLE_KEY, mActionBarSubtitle);
        outState.putBoolean(STATE_ENTERANIMATIONFINISHED_KEY, mEnterAnimationFinished);
    }

    public boolean onOptionsItemSelected(MenuItem item) {
       switch (item.getItemId()) {
          case android.R.id.home:
             mActivity.finish();
             return true;
          default:
             return false;
       }
    }

    @Override
    public void addScreenListener(int position, OnScreenListener listener) {
        mScreenListeners.put(position, listener);
    }

    @Override
    public void removeScreenListener(int position) {
        mScreenListeners.remove(position);
    }

    @Override
    public synchronized void addCursorListener(CursorChangedListener listener) {
        mCursorListeners.add(listener);
    }

    @Override
    public synchronized void removeCursorListener(CursorChangedListener listener) {
        mCursorListeners.remove(listener);
    }

    @Override
    public boolean isFragmentFullScreen(Fragment fragment) {
        if (mViewPager == null || mAdapter == null || mAdapter.getCount() == 0) {
            return mFullScreen;
        }
        return mFullScreen || (mViewPager.getCurrentItem() != mAdapter.getItemPosition(fragment));
    }

    @Override
    public void toggleFullScreen() {
        setFullScreen(!mFullScreen, true);
    }

    public void onPhotoRemoved(long photoId) {
        final Cursor data = mAdapter.getCursor();
        if (data == null) {
            // Huh?! How would this happen?
            return;
        }

        final int dataCount = data.getCount();
        if (dataCount <= 1) {
            mActivity.finish();
            return;
        }

        mActivity.getSupportLoaderManager().restartLoader(LOADER_PHOTO_LIST, null, this);
    }

    @Override
    public Loader<Cursor> onCreateLoader(int id, Bundle args) {
        if (id == LOADER_PHOTO_LIST) {
            return new PhotoPagerLoader(mActivity.getContext(), Uri.parse(mPhotosUri), mProjection);
        }
        return null;
    }

    @Override
    public Loader<BitmapResult> onCreateBitmapLoader(int id, Bundle args, String uri) {
        switch (id) {
            case BITMAP_LOADER_AVATAR:
            case BITMAP_LOADER_THUMBNAIL:
            case BITMAP_LOADER_PHOTO:
                return new PhotoBitmapLoader(mActivity.getContext(), uri);
            default:
                return null;
        }
    }

    @Override
    public void onLoadFinished(Loader<Cursor> loader, Cursor data) {
        final int id = loader.getId();
        if (id == LOADER_PHOTO_LIST) {
            if (data == null || data.getCount() == 0) {
                mIsEmpty = true;
                mAdapter.swapCursor(null);
            } else {
                mAlbumCount = data.getCount();
                if (mCurrentPhotoUri != null) {
                    int index = 0;
                    // Clear query params. Compare only the path.
                    final int uriIndex = data.getColumnIndex(PhotoContract.PhotoViewColumns.URI);
                    final Uri currentPhotoUri;
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
                        currentPhotoUri = Uri.parse(mCurrentPhotoUri).buildUpon()
                                .clearQuery().build();
                    } else {
                        currentPhotoUri = Uri.parse(mCurrentPhotoUri).buildUpon()
                                .query(null).build();
                    }
                    // Rewind data cursor to the start if it has already advanced.
                    data.moveToPosition(-1);
                    while (data.moveToNext()) {
                        final String uriString = data.getString(uriIndex);
                        final Uri uri;
                        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
                            uri = Uri.parse(uriString).buildUpon().clearQuery().build();
                        } else {
                            uri = Uri.parse(uriString).buildUpon().query(null).build();
                        }
                        if (currentPhotoUri != null && currentPhotoUri.equals(uri)) {
                            mCurrentPhotoIndex = index;
                            break;
                        }
                        index++;
                    }
                }

                // We're paused; don't do anything now, we'll get re-invoked
                // when the activity becomes active again
                if (mIsPaused) {
                    mKickLoader = true;
                    mAdapter.swapCursor(null);
                    return;
                }
                boolean wasEmpty = mIsEmpty;
                mIsEmpty = false;

                mAdapter.swapCursor(data);
                if (mViewPager.getAdapter() == null) {
                    mViewPager.setAdapter(mAdapter);
                }
                notifyCursorListeners(data);

                // Use an index of 0 if the index wasn't specified or couldn't be found
                if (mCurrentPhotoIndex < 0) {
                    mCurrentPhotoIndex = 0;
                }

                mViewPager.setCurrentItem(mCurrentPhotoIndex, false);
                if (wasEmpty) {
                    setViewActivated(mCurrentPhotoIndex);
                }
            }
            // Update the any action items
            updateActionItems();
        }
    }

    @Override
    public void onLoaderReset(android.support.v4.content.Loader<Cursor> loader) {
        // If the loader is reset, remove the reference in the adapter to this cursor
        if (!isDestroyedCompat()) {
            // This will cause a fragment transaction which can't happen if we're destroyed,
            // but we don't care in that case because we're destroyed anyways.
            mAdapter.swapCursor(null);
        }
    }

    public void updateActionItems() {
        // Do nothing, but allow extending classes to do work
    }

    private synchronized void notifyCursorListeners(Cursor data) {
        // tell all of the objects listening for cursor changes
        // that the cursor has changed
        for (CursorChangedListener listener : mCursorListeners) {
            listener.onCursorChanged(data);
        }
    }

    @Override
    public void onPageScrolled(int position, float positionOffset, int positionOffsetPixels) {
        if (positionOffset < 0.0001) {
            OnScreenListener before = mScreenListeners.get(position - 1);
            if (before != null) {
                before.onViewUpNext();
            }
            OnScreenListener after = mScreenListeners.get(position + 1);
            if (after != null) {
                after.onViewUpNext();
            }
        }
    }

    @Override
    public void onPageSelected(int position) {
        mCurrentPhotoIndex = position;
        setViewActivated(position);
    }

    @Override
    public void onPageScrollStateChanged(int state) {
    }

    @Override
    public boolean isFragmentActive(Fragment fragment) {
        if (mViewPager == null || mAdapter == null) {
            return false;
        }
        return mViewPager.getCurrentItem() == mAdapter.getItemPosition(fragment);
    }

    @Override
    public void onFragmentVisible(PhotoViewFragment fragment) {
        // Do nothing, we handle this in setViewActivated
    }

    @Override
    public InterceptType onTouchIntercept(float origX, float origY) {
        boolean interceptLeft = false;
        boolean interceptRight = false;

        for (OnScreenListener listener : mScreenListeners.values()) {
            if (!interceptLeft) {
                interceptLeft = listener.onInterceptMoveLeft(origX, origY);
            }
            if (!interceptRight) {
                interceptRight = listener.onInterceptMoveRight(origX, origY);
            }
        }

        if (interceptLeft) {
            if (interceptRight) {
                return InterceptType.BOTH;
            }
            return InterceptType.LEFT;
        } else if (interceptRight) {
            return InterceptType.RIGHT;
        }
        return InterceptType.NONE;
    }

    /**
     * Updates the title bar according to the value of {@link #mFullScreen}.
     */
    protected void setFullScreen(boolean fullScreen, boolean setDelayedRunnable) {
        if (Util.isTouchExplorationEnabled(mAccessibilityManager)) {
            // Disallow full screen mode when accessibility is enabled so that the action bar
            // stays accessible.
            fullScreen = false;
            setDelayedRunnable = false;
        }

        final boolean fullScreenChanged = (fullScreen != mFullScreen);
        mFullScreen = fullScreen;

        if (mFullScreen) {
            setLightsOutMode(true);
            cancelEnterFullScreenRunnable();
        } else {
            setLightsOutMode(false);
            if (setDelayedRunnable) {
                postEnterFullScreenRunnableWithDelay();
            }
        }

        if (fullScreenChanged) {
            for (OnScreenListener listener : mScreenListeners.values()) {
                listener.onFullScreenChanged(mFullScreen);
            }
        }
    }

    /**
     * Posts a runnable to enter full screen after mEnterFullScreenDelayTime. This method is a
     * no-op if mIsTimerLightsOutEnabled is set to false.
     */
    private void postEnterFullScreenRunnableWithDelay() {
        if (mIsTimerLightsOutEnabled) {
            mHandler.postDelayed(mEnterFullScreenRunnable, mEnterFullScreenDelayTime);
        }
    }

    private void cancelEnterFullScreenRunnable() {
        mHandler.removeCallbacks(mEnterFullScreenRunnable);
    }

    protected void setLightsOutMode(boolean enabled) {
        setImmersiveMode(enabled);
    }

    private final Runnable mEnterFullScreenRunnable = new Runnable() {
        @Override
        public void run() {
            setFullScreen(true, true);
        }
    };

    @Override
    public void setViewActivated(int position) {
        OnScreenListener listener = mScreenListeners.get(position);
        if (listener != null) {
            listener.onViewActivated();
        }
        final Cursor cursor = getCursorAtProperPosition();
        mCurrentPhotoIndex = position;
        // FLAG: get the column indexes once in onLoadFinished().
        // That would make this more efficient, instead of looking these up
        // repeatedly whenever we want them.
        int uriIndex = cursor.getColumnIndex(PhotoContract.PhotoViewColumns.URI);
        mCurrentPhotoUri = cursor.getString(uriIndex);
        updateActionBar();
        if (mAccessibilityManager.isEnabled() && lastAnnouncedTitle != position) {
            String announcement = getPhotoAccessibilityAnnouncement(position);
            if (announcement != null) {
                Util.announceForAccessibility(mRootView, mAccessibilityManager, announcement);
                lastAnnouncedTitle = position;
            }
        }

        // Restart the timer to return to fullscreen.
        cancelEnterFullScreenRunnable();
        postEnterFullScreenRunnableWithDelay();
    }

    /**
     * Adjusts the activity title and subtitle to reflect the photo name and count.
     */
    public void updateActionBar() {
        final int position = mViewPager.getCurrentItem() + 1;
        final boolean hasAlbumCount = mAlbumCount >= 0;

        final Cursor cursor = getCursorAtProperPosition();
        if (cursor != null) {
            // FLAG: We should grab the indexes when we first get the cursor
            // and store them so we don't need to do it each time.
            final int photoNameIndex = cursor.getColumnIndex(PhotoContract.PhotoViewColumns.NAME);
            mActionBarTitle = cursor.getString(photoNameIndex);
        } else {
            mActionBarTitle = null;
        }

        if (mIsEmpty || !hasAlbumCount || position <= 0) {
            mActionBarSubtitle = null;
        } else {
            mActionBarSubtitle = mActivity.getResources().getString(
                    R.string.photo_view_count, position, mAlbumCount);
        }

        setActionBarTitles(mActivity.getActionBarInterface());
    }

    /**
     * Returns a string used as an announcement for accessibility after the user moves to a new
     * photo. It will be called after {@link #updateActionBar} has been called.
     * @param position the index in the album of the currently active photo
     * @return announcement for accessibility
     */
    protected String getPhotoAccessibilityAnnouncement(int position) {
        String announcement = mActionBarTitle;
        if (mActionBarSubtitle != null) {
            announcement = mActivity.getContext().getResources().getString(
                    R.string.titles, mActionBarTitle, mActionBarSubtitle);
        }
        return announcement;
    }

    /**
     * Sets the Action Bar title to {@link #mActionBarTitle} and the subtitle to
     * {@link #mActionBarSubtitle}
     */
    protected final void setActionBarTitles(ActionBarInterface actionBar) {
        if (actionBar == null) {
            return;
        }
        actionBar.setTitle(getInputOrEmpty(mActionBarTitle));
        actionBar.setSubtitle(getInputOrEmpty(mActionBarSubtitle));
    }

    /**
     * If the input string is non-null, it is returned, otherwise an empty string is returned;
     * @param in
     * @return
     */
    private static final String getInputOrEmpty(String in) {
        if (in == null) {
            return "";
        }
        return in;
    }

    /**
     * Utility method that will return the cursor that contains the data
     * at the current position so that it refers to the current image on screen.
     * @return the cursor at the current position or
     * null if no cursor exists or if the {@link PhotoViewPager} is null.
     */
    public Cursor getCursorAtProperPosition() {
        if (mViewPager == null) {
            return null;
        }

        final int position = mViewPager.getCurrentItem();
        final Cursor cursor = mAdapter.getCursor();

        if (cursor == null) {
            return null;
        }

        cursor.moveToPosition(position);

        return cursor;
    }

    public Cursor getCursor() {
        return (mAdapter == null) ? null : mAdapter.getCursor();
    }

    @Override
    public void onMenuVisibilityChanged(boolean isVisible) {
        if (isVisible) {
            cancelEnterFullScreenRunnable();
        } else {
            postEnterFullScreenRunnableWithDelay();
        }
    }

    @Override
    public void onNewPhotoLoaded(int position) {
        // do nothing
    }

    protected void setPhotoIndex(int index) {
        mCurrentPhotoIndex = index;
    }

    @Override
    public void onFragmentPhotoLoadComplete(PhotoViewFragment fragment, boolean success) {
        if (hasTemporaryImage() && mTemporaryImage.getVisibility() != View.GONE &&
                TextUtils.equals(fragment.getPhotoUri(), mCurrentPhotoUri)) {
            if (success) {
                // The fragment for the current image is now ready for display.
                if (hasTemporaryImage()) {
                    mTemporaryImage.setVisibility(View.GONE);
                }
                mViewPager.setVisibility(View.VISIBLE);
            } else {
                // This means that we are unable to load the fragment's photo.
                // I'm not sure what the best thing to do here is, but at least if
                // we display the viewPager, the fragment itself can decide how to
                // display the failure of its own image.
                Log.w(TAG, "Failed to load fragment image");
                if (hasTemporaryImage()) {
                    mTemporaryImage.setVisibility(View.GONE);
                }
                mViewPager.setVisibility(View.VISIBLE);
            }
            mActivity.getSupportLoaderManager().destroyLoader(
                    PhotoViewCallbacks.BITMAP_LOADER_THUMBNAIL);
        }
    }

    protected boolean isFullScreen() {
        return mFullScreen;
    }

    @Override
    public void onCursorChanged(PhotoViewFragment fragment, Cursor cursor) {
        // do nothing
    }

    @Override
    public PhotoPagerAdapter getAdapter() {
        return mAdapter;
    }

    public void onEnterAnimationComplete() {
        mEnterAnimationFinished = true;
        mViewPager.setVisibility(View.VISIBLE);
        setLightsOutMode(mFullScreen);
    }

    private void onExitAnimationComplete() {
        mActivity.finish();
        mActivity.overridePendingTransition(0, 0);
    }

    private void runEnterAnimation() {
        final int totalWidth = mRootView.getMeasuredWidth();
        final int totalHeight = mRootView.getMeasuredHeight();

        // FLAG: Need to handle the aspect ratio of the bitmap.  If it's a portrait
        // bitmap, then we need to position the view higher so that the middle
        // pixels line up.
        if (hasTemporaryImage()) {
            mTemporaryImage.setVisibility(View.VISIBLE);
        }
        // We need to take a full screen image, and scale/translate it so that
        // it appears at exactly the same location onscreen as it is in the
        // prior activity.
        // The final image will take either the full screen width or height (or both).

        final float scaleW = (float) mAnimationStartWidth / totalWidth;
        final float scaleY = (float) mAnimationStartHeight / totalHeight;
        final float scale = Math.max(scaleW, scaleY);

        final int translateX = calculateTranslate(mAnimationStartX, mAnimationStartWidth,
                totalWidth, scale);
        final int translateY = calculateTranslate(mAnimationStartY, mAnimationStartHeight,
                totalHeight, scale);

        final int version = android.os.Build.VERSION.SDK_INT;
        if (version >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
            if (hasBackground()) {
                mBackground.setAlpha(0f);
                mBackground.animate().alpha(1f).setDuration(ENTER_ANIMATION_DURATION_MS).start();
                mBackground.setVisibility(View.VISIBLE);
            }

            if (hasTemporaryImage()) {
                mTemporaryImage.setScaleX(scale);
                mTemporaryImage.setScaleY(scale);
                mTemporaryImage.setTranslationX(translateX);
                mTemporaryImage.setTranslationY(translateY);

                Runnable endRunnable = new Runnable() {
                    @Override
                    public void run() {
                        PhotoViewController.this.onEnterAnimationComplete();
                    }
                };
                ViewPropertyAnimator animator = mTemporaryImage.animate().scaleX(1f).scaleY(1f)
                    .translationX(0).translationY(0).setDuration(ENTER_ANIMATION_DURATION_MS);
                if (version >= Build.VERSION_CODES.JELLY_BEAN) {
                    animator.withEndAction(endRunnable);
                } else {
                    mHandler.postDelayed(endRunnable, ENTER_ANIMATION_DURATION_MS);
                }
                animator.start();
            }
        } else {
            if (hasBackground()) {
                final Animation alphaAnimation = new AlphaAnimation(0f, 1f);
                alphaAnimation.setDuration(ENTER_ANIMATION_DURATION_MS);
                mBackground.startAnimation(alphaAnimation);
                mBackground.setVisibility(View.VISIBLE);
            }

            if (hasTemporaryImage()) {
                final Animation translateAnimation = new TranslateAnimation(translateX,
                        translateY, 0, 0);
                translateAnimation.setDuration(ENTER_ANIMATION_DURATION_MS);
                Animation scaleAnimation = new ScaleAnimation(scale, scale, 0, 0);
                scaleAnimation.setDuration(ENTER_ANIMATION_DURATION_MS);

                AnimationSet animationSet = new AnimationSet(true);
                animationSet.addAnimation(translateAnimation);
                animationSet.addAnimation(scaleAnimation);
                AnimationListener listener = new AnimationListener() {
                    @Override
                    public void onAnimationEnd(Animation arg0) {
                        PhotoViewController.this.onEnterAnimationComplete();
                    }

                    @Override
                    public void onAnimationRepeat(Animation arg0) {
                    }

                    @Override
                    public void onAnimationStart(Animation arg0) {
                    }
                };
                animationSet.setAnimationListener(listener);
                mTemporaryImage.startAnimation(animationSet);
            }
        }
    }

    private void runExitAnimation() {
        Intent intent = mActivity.getIntent();
        // FLAG: should just fall back to a standard animation if either:
        // 1. images have been added or removed since we've been here, or
        // 2. we are currently looking at some image other than the one we
        // started on.

        final int totalWidth = mRootView.getMeasuredWidth();
        final int totalHeight = mRootView.getMeasuredHeight();

        // We need to take a full screen image, and scale/translate it so that
        // it appears at exactly the same location onscreen as it is in the
        // prior activity.
        // The final image will take either the full screen width or height (or both).
        final float scaleW = (float) mAnimationStartWidth / totalWidth;
        final float scaleY = (float) mAnimationStartHeight / totalHeight;
        final float scale = Math.max(scaleW, scaleY);

        final int translateX = calculateTranslate(mAnimationStartX, mAnimationStartWidth,
                totalWidth, scale);
        final int translateY = calculateTranslate(mAnimationStartY, mAnimationStartHeight,
                totalHeight, scale);
        final int version = android.os.Build.VERSION.SDK_INT;
        if (version >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
            if (hasBackground()) {
                mBackground.animate().alpha(0f).setDuration(EXIT_ANIMATION_DURATION_MS).start();
                mBackground.setVisibility(View.VISIBLE);
            }

            Runnable endRunnable = new Runnable() {
                @Override
                public void run() {
                    PhotoViewController.this.onExitAnimationComplete();
                }
            };
            // If the temporary image is still visible it means that we have
            // not yet loaded the fullres image, so we need to animate
            // the temporary image out.
            ViewPropertyAnimator animator = null;
            if (hasTemporaryImage() && mTemporaryImage.getVisibility() == View.VISIBLE) {
                animator = mTemporaryImage.animate().scaleX(scale).scaleY(scale)
                    .translationX(translateX).translationY(translateY)
                    .setDuration(EXIT_ANIMATION_DURATION_MS);
            } else {
                animator = mViewPager.animate().scaleX(scale).scaleY(scale)
                    .translationX(translateX).translationY(translateY)
                    .setDuration(EXIT_ANIMATION_DURATION_MS);
            }
            // If the user has swiped to a different photo, fade out the current photo
            // along with the scale animation.
            if (!mInitialPhotoUri.equals(mCurrentPhotoUri)) {
                animator.alpha(0f);
            }
            if (version >= android.os.Build.VERSION_CODES.JELLY_BEAN) {
                animator.withEndAction(endRunnable);
            } else {
                mHandler.postDelayed(endRunnable, EXIT_ANIMATION_DURATION_MS);
            }
            animator.start();
        } else {
            if (hasBackground()) {
                final Animation alphaAnimation = new AlphaAnimation(1f, 0f);
                alphaAnimation.setDuration(EXIT_ANIMATION_DURATION_MS);
                mBackground.startAnimation(alphaAnimation);
                mBackground.setVisibility(View.VISIBLE);
            }

            final Animation scaleAnimation = new ScaleAnimation(1f, 1f, scale, scale);
            scaleAnimation.setDuration(EXIT_ANIMATION_DURATION_MS);
            AnimationListener listener = new AnimationListener() {
                @Override
                public void onAnimationEnd(Animation arg0) {
                    PhotoViewController.this.onExitAnimationComplete();
                }

                @Override
                public void onAnimationRepeat(Animation arg0) {
                }

                @Override
                public void onAnimationStart(Animation arg0) {
                }
            };
            scaleAnimation.setAnimationListener(listener);
            // If the temporary image is still visible it means that we have
            // not yet loaded the fullres image, so we need to animate
            // the temporary image out.
            if (hasTemporaryImage() && mTemporaryImage.getVisibility() == View.VISIBLE) {
                mTemporaryImage.startAnimation(scaleAnimation);
            } else {
                mViewPager.startAnimation(scaleAnimation);
            }
        }
    }

    private int calculateTranslate(int start, int startSize, int totalSize, float scale) {
        // Translation takes precedence over scale.  What this means is that if
        // we want an view's upper left corner to be a particular spot on screen,
        // but that view is scaled to something other than 1, we need to take into
        // account the pixels lost to scaling.
        // So if we have a view that is 200x300, and we want it's upper left corner
        // to be at 50x50, but it's scaled by 50%, we can't just translate it to 50x50.
        // If we were to do that, the view's *visible* upper left corner would be at
        // 100x200.  We need to take into account the difference between the outside
        // size of the view (i.e. the size prior to scaling) and the scaled size.
        // scaleFromEdge is the difference between the visible left edge and the
        // actual left edge, due to scaling.
        // scaleFromTop is the difference between the visible top edge, and the
        // actual top edge, due to scaling.
        int scaleFromEdge = Math.round((totalSize - totalSize * scale) / 2);

        // The imageView is fullscreen, regardless of the aspect ratio of the actual image.
        // This means that some portion of the imageView will be blank.  We need to
        // take into account the size of the blank area so that the actual image
        // lines up with the starting image.
        int blankSize = Math.round((totalSize * scale - startSize) / 2);

        return start - scaleFromEdge - blankSize;
    }

    private void initTemporaryImage(Drawable drawable) {
        if (mEnterAnimationFinished) {
            // Forget this, we've already run the animation.
            return;
        }
        if (hasTemporaryImage()) {
            mTemporaryImage.setImageDrawable(drawable);
        }
        if (drawable != null) {
            // We have not yet run the enter animation. Start it now.
            int totalWidth = mRootView.getMeasuredWidth();
            if (totalWidth == 0) {
                // the measure pass has not yet finished.  We can't properly
                // run out animation until that is done. Listen for the layout
                // to occur, then fire the animation.
                final View base = mRootView;
                base.getViewTreeObserver().addOnGlobalLayoutListener(
                        new OnGlobalLayoutListener() {
                    @Override
                    public void onGlobalLayout() {
                        int version = android.os.Build.VERSION.SDK_INT;
                        if (version >= android.os.Build.VERSION_CODES.JELLY_BEAN) {
                            base.getViewTreeObserver().removeOnGlobalLayoutListener(this);
                        } else {
                            base.getViewTreeObserver().removeGlobalOnLayoutListener(this);
                        }
                        runEnterAnimation();
                    }
                });
            } else {
                // initiate the animation
                runEnterAnimation();
            }
        }
        // Kick off the photo list loader
        mActivity.getSupportLoaderManager().initLoader(LOADER_PHOTO_LIST, null, this);
    }

    public void showActionBar() {
        mActivity.getActionBarInterface().show();
    }

    public void hideActionBar() {
        mActivity.getActionBarInterface().hide();
    }

    public boolean isScaleAnimationEnabled() {
        return mScaleAnimationEnabled;
    }

    public boolean isEnterAnimationFinished() {
        return mEnterAnimationFinished;
    }

    public View getRootView() {
        return mRootView;
    }

    private class BitmapCallback implements LoaderManager.LoaderCallbacks<BitmapResult> {

        @Override
        public Loader<BitmapResult> onCreateLoader(int id, Bundle args) {
            String uri = args.getString(ARG_IMAGE_URI);
            switch (id) {
                case PhotoViewCallbacks.BITMAP_LOADER_THUMBNAIL:
                    return onCreateBitmapLoader(PhotoViewCallbacks.BITMAP_LOADER_THUMBNAIL,
                            args, uri);
                case PhotoViewCallbacks.BITMAP_LOADER_AVATAR:
                    return onCreateBitmapLoader(PhotoViewCallbacks.BITMAP_LOADER_AVATAR,
                            args, uri);
            }
            return null;
        }

        @Override
        public void onLoadFinished(Loader<BitmapResult> loader, BitmapResult result) {
            Drawable drawable = result.getDrawable(mActivity.getResources());
            final ActionBarInterface actionBar = mActivity.getActionBarInterface();
            switch (loader.getId()) {
                case PhotoViewCallbacks.BITMAP_LOADER_THUMBNAIL:
                    // We just loaded the initial thumbnail that we can display
                    // while waiting for the full viewPager to get initialized.
                    initTemporaryImage(drawable);
                    break;
                case PhotoViewCallbacks.BITMAP_LOADER_AVATAR:
                    if (drawable == null) {
                        actionBar.setLogo(null);
                    } else {
                        actionBar.setLogo(drawable);
                    }
                    break;
            }
        }

        @Override
        public void onLoaderReset(Loader<BitmapResult> loader) {
            // Do nothing
        }
    }

    public void setImmersiveMode(boolean enabled) {
        int flags = 0;
        final int version = Build.VERSION.SDK_INT;
        final boolean manuallyUpdateActionBar = version < Build.VERSION_CODES.JELLY_BEAN;
        if (enabled &&
                (!isScaleAnimationEnabled() || isEnterAnimationFinished())) {
            // Turning on immersive mode causes an animation. If the scale animation is enabled and
            // the enter animation isn't yet complete, then an immersive mode animation should not
            // occur, since two concurrent animations are very janky.

            // Disable immersive mode for seconary users to prevent b/12015090 (freezing crash)
            // This is fixed in KK_MR2 but there is no way to differentiate between  KK and KK_MR2.
            if (version > Build.VERSION_CODES.KITKAT ||
                    version == Build.VERSION_CODES.KITKAT && !kitkatIsSecondaryUser()) {
                flags = View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                        | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_IMMERSIVE;
            } else if (version >= Build.VERSION_CODES.JELLY_BEAN) {
                // Clients that use the scale animation should set the following system UI flags to
                // prevent janky animations on exit when the status bar is hidden:
                //     View.SYSTEM_UI_FLAG_VISIBLE | View.SYSTEM_UI_FLAG_STABLE
                // As well, client should ensure `android:fitsSystemWindows` is set on the root
                // content view.
                flags = View.SYSTEM_UI_FLAG_LOW_PROFILE
                        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                        | View.SYSTEM_UI_FLAG_FULLSCREEN;
            } else if (version >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
                flags = View.SYSTEM_UI_FLAG_LOW_PROFILE;
            } else if (version >= Build.VERSION_CODES.HONEYCOMB) {
                flags = View.STATUS_BAR_HIDDEN;
            }

            if (manuallyUpdateActionBar) {
                hideActionBar();
            }
        } else {
            if (version >= Build.VERSION_CODES.KITKAT) {
                flags = View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_LAYOUT_STABLE;
            } else if (version >= Build.VERSION_CODES.JELLY_BEAN) {
                flags = View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_LAYOUT_STABLE;
            } else if (version >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
                flags = View.SYSTEM_UI_FLAG_VISIBLE;
            } else if (version >= Build.VERSION_CODES.HONEYCOMB) {
                flags = View.STATUS_BAR_VISIBLE;
            }

            if (manuallyUpdateActionBar) {
                showActionBar();
            }
        }

        if (version >= Build.VERSION_CODES.HONEYCOMB) {
            mLastFlags = flags;
            getRootView().setSystemUiVisibility(flags);
        }
    }

    /**
     * Return true iff the app is being run as a secondary user on kitkat.
     *
     * This is a hack which we only know to work on kitkat.
     */
    private boolean kitkatIsSecondaryUser() {
        if (Build.VERSION.SDK_INT != Build.VERSION_CODES.KITKAT) {
            throw new IllegalStateException("kitkatIsSecondary user is only callable on KitKat");
        }
        return Process.myUid() > 100000;
    }

    /**
     * Note: This should only be called when API level is 11 or above.
     */
    public View.OnSystemUiVisibilityChangeListener getSystemUiVisibilityChangeListener() {
        return mSystemUiVisibilityChangeListener;
    }
}
