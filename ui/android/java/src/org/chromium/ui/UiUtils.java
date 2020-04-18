// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.ui;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.os.Build;
import android.os.Environment;
import android.os.Handler;
import android.os.StrictMode;
import android.text.TextUtils;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.MeasureSpec;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodInfo;
import android.view.inputmethod.InputMethodManager;
import android.view.inputmethod.InputMethodSubtype;
import android.widget.AbsListView;
import android.widget.ListAdapter;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.Log;

import java.io.File;
import java.io.IOException;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * Utility functions for common Android UI tasks.
 * This class is not supposed to be instantiated.
 */
public class UiUtils {
    private static final String TAG = "UiUtils";

    private static final int KEYBOARD_RETRY_ATTEMPTS = 10;
    private static final long KEYBOARD_RETRY_DELAY_MS = 100;

    public static final String EXTERNAL_IMAGE_FILE_PATH = "browser-images";
    // Keep this variable in sync with the value defined in file_paths.xml.
    public static final String IMAGE_FILE_PATH = "images";

    /**
     * Guards this class from being instantiated.
     */
    private UiUtils() {
    }

    /** The minimum size of the bottom margin below the app to detect a keyboard. */
    private static final float KEYBOARD_DETECT_BOTTOM_THRESHOLD_DP = 100;

    /** A delegate that allows disabling keyboard visibility detection. */
    private static KeyboardShowingDelegate sKeyboardShowingDelegate;

    /** A delegate for the photo picker. */
    private static PhotoPickerDelegate sPhotoPickerDelegate;

    /**
     * A delegate that can be implemented to override whether or not keyboard detection will be
     * used.
     */
    public interface KeyboardShowingDelegate {
        /**
         * Will be called to determine whether or not to detect if the keyboard is visible.
         * @param context A {@link Context} instance.
         * @param view    A {@link View}.
         * @return        Whether or not the keyboard check should be disabled.
         */
        boolean disableKeyboardCheck(Context context, View view);
    }

    /**
     * A delegate interface for the photo picker.
     */
    public interface PhotoPickerDelegate {
        /**
         * Called to display the photo picker.
         * @param context  The context to use.
         * @param listener The listener that will be notified of the action the user took in the
         *                 picker.
         * @param allowMultiple Whether the dialog should allow multiple images to be selected.
         * @param mimeTypes A list of mime types to show in the dialog.
         */
        void showPhotoPicker(Context context, PhotoPickerListener listener, boolean allowMultiple,
                List<String> mimeTypes);

        /**
         * Called when the photo picker dialog has been dismissed.
         */
        void onPhotoPickerDismissed();
    }

    // PhotoPickerDelegate:

    /**
     * Allows setting a delegate to override the default Android stock photo picker.
     * @param delegate A {@link PhotoPickerDelegate} instance.
     */
    public static void setPhotoPickerDelegate(PhotoPickerDelegate delegate) {
        sPhotoPickerDelegate = delegate;
    }

    /**
     * Returns whether a photo picker should be called.
     */
    public static boolean shouldShowPhotoPicker() {
        return sPhotoPickerDelegate != null;
    }

    /**
     * Called to display the photo picker.
     * @param context  The context to use.
     * @param listener The listener that will be notified of the action the user took in the
     *                 picker.
     * @param allowMultiple Whether the dialog should allow multiple images to be selected.
     * @param mimeTypes A list of mime types to show in the dialog.
     */
    public static boolean showPhotoPicker(Context context, PhotoPickerListener listener,
            boolean allowMultiple, List<String> mimeTypes) {
        if (sPhotoPickerDelegate == null) return false;
        sPhotoPickerDelegate.showPhotoPicker(context, listener, allowMultiple, mimeTypes);
        return true;
    }

    /**
     * Called when the photo picker dialog has been dismissed.
     */
    public static void onPhotoPickerDismissed() {
        if (sPhotoPickerDelegate == null) return;
        sPhotoPickerDelegate.onPhotoPickerDismissed();
    }

    // KeyboardShowingDelegate:

    /**
     * Allows setting a delegate to override the default software keyboard visibility detection.
     * @param delegate A {@link KeyboardShowingDelegate} instance.
     */
    public static void setKeyboardShowingDelegate(KeyboardShowingDelegate delegate) {
        sKeyboardShowingDelegate = delegate;
    }

    /**
     * Shows the software keyboard if necessary.
     * @param view The currently focused {@link View}, which would receive soft keyboard input.
     */
    public static void showKeyboard(final View view) {
        final Handler handler = new Handler();
        final AtomicInteger attempt = new AtomicInteger();
        Runnable openRunnable = new Runnable() {
            @Override
            public void run() {
                // Not passing InputMethodManager.SHOW_IMPLICIT as it does not trigger the
                // keyboard in landscape mode.
                InputMethodManager imm =
                        (InputMethodManager) view.getContext().getSystemService(
                                Context.INPUT_METHOD_SERVICE);
                // Third-party touches disk on showSoftInput call. http://crbug.com/619824,
                // http://crbug.com/635118
                StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskWrites();
                try {
                    imm.showSoftInput(view, 0);
                } catch (IllegalArgumentException e) {
                    if (attempt.incrementAndGet() <= KEYBOARD_RETRY_ATTEMPTS) {
                        handler.postDelayed(this, KEYBOARD_RETRY_DELAY_MS);
                    } else {
                        Log.e(TAG, "Unable to open keyboard.  Giving up.", e);
                    }
                } finally {
                    StrictMode.setThreadPolicy(oldPolicy);
                }
            }
        };
        openRunnable.run();
    }

    /**
     * Hides the keyboard.
     * @param view The {@link View} that is currently accepting input.
     * @return Whether the keyboard was visible before.
     */
    public static boolean hideKeyboard(View view) {
        InputMethodManager imm =
                (InputMethodManager) view.getContext().getSystemService(
                        Context.INPUT_METHOD_SERVICE);
        return imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
    }

    /**
     * Detects whether or not the keyboard is showing.  This is a best guess as there is no
     * standardized/foolproof way to do this.
     * @param context A {@link Context} instance.
     * @param view    A {@link View}.
     * @return        Whether or not the software keyboard is visible and taking up screen space.
     */
    @SuppressLint("NewApi")
    public static boolean isKeyboardShowing(Context context, View view) {
        if (sKeyboardShowingDelegate != null
                && sKeyboardShowingDelegate.disableKeyboardCheck(context, view)) {
            return false;
        }

        View rootView = view.getRootView();
        if (rootView == null) return false;
        Rect appRect = new Rect();
        rootView.getWindowVisibleDisplayFrame(appRect);

        // Assume status bar is always at the top of the screen.
        final int statusBarHeight = appRect.top;

        int bottomMargin = rootView.getHeight() - (appRect.height() + statusBarHeight);

        // If there is no bottom margin, the keyboard is not showing.
        if (bottomMargin <= 0) return false;

        // If the display frame width is < root view width, controls are on the side of the screen.
        // The inverse is not necessarily true; i.e. if navControlsOnSide is false, it doesn't mean
        // the controls are not on the side or that they _are_ at the bottom. It might just mean the
        // app is not responsible for drawing their background.
        boolean navControlsOnSide = appRect.width() != rootView.getWidth();

        // If the Android nav controls are on the sides instead of at the bottom, its height is not
        // needed.
        if (!navControlsOnSide) {
            // When available, get the root view insets.
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                bottomMargin -= rootView.getRootWindowInsets().getStableInsetBottom();
            } else {
                // In the event we couldn't get the bottom nav height, use a best guess of the
                // keyboard height. In certain cases this also means including the height of the
                // Android navigation.
                final float density = context.getResources().getDisplayMetrics().density;
                bottomMargin = (int) (bottomMargin - KEYBOARD_DETECT_BOTTOM_THRESHOLD_DP * density);
            }
        }

        // After subtracting the bottom navigation, the remaining margin represents the keyboard.
        return bottomMargin > 0;
    }

    /**
     * Gets the set of locales supported by the current enabled Input Methods.
     * @param context A {@link Context} instance.
     * @return A possibly-empty {@link Set} of locale strings.
     */
    public static Set<String> getIMELocales(Context context) {
        LinkedHashSet<String> locales = new LinkedHashSet<String>();
        InputMethodManager imManager =
                (InputMethodManager) context.getSystemService(Context.INPUT_METHOD_SERVICE);
        List<InputMethodInfo> enabledMethods = imManager.getEnabledInputMethodList();
        for (int i = 0; i < enabledMethods.size(); i++) {
            List<InputMethodSubtype> subtypes =
                    imManager.getEnabledInputMethodSubtypeList(enabledMethods.get(i), true);
            if (subtypes == null) continue;
            for (int j = 0; j < subtypes.size(); j++) {
                String locale = ApiCompatibilityUtils.getLocale(subtypes.get(j));
                if (!TextUtils.isEmpty(locale)) locales.add(locale);
            }
        }
        return locales;
    }

    /**
     * Inserts a {@link View} into a {@link ViewGroup} after directly before a given {@View}.
     * @param container The {@link View} to add newView to.
     * @param newView The new {@link View} to add.
     * @param existingView The {@link View} to insert the newView before.
     * @return The index where newView was inserted, or -1 if it was not inserted.
     */
    public static int insertBefore(ViewGroup container, View newView, View existingView) {
        return insertView(container, newView, existingView, false);
    }

    /**
     * Inserts a {@link View} into a {@link ViewGroup} after directly after a given {@View}.
     * @param container The {@link View} to add newView to.
     * @param newView The new {@link View} to add.
     * @param existingView The {@link View} to insert the newView after.
     * @return The index where newView was inserted, or -1 if it was not inserted.
     */
    public static int insertAfter(ViewGroup container, View newView, View existingView) {
        return insertView(container, newView, existingView, true);
    }

    private static int insertView(
            ViewGroup container, View newView, View existingView, boolean after) {
        // See if the view has already been added.
        int index = container.indexOfChild(newView);
        if (index >= 0) return index;

        // Find the location of the existing view.
        index = container.indexOfChild(existingView);
        if (index < 0) return -1;

        // Add the view.
        if (after) index++;
        container.addView(newView, index);
        return index;
    }

    /**
     * Generates a scaled screenshot of the given view.  The maximum size of the screenshot is
     * determined by maximumDimension.
     *
     * @param currentView      The view to generate a screenshot of.
     * @param maximumDimension The maximum width or height of the generated screenshot.  The bitmap
     *                         will be scaled to ensure the maximum width or height is equal to or
     *                         less than this.  Any value <= 0, will result in no scaling.
     * @param bitmapConfig     Bitmap config for the generated screenshot (ARGB_8888 or RGB_565).
     * @return The screen bitmap of the view or null if a problem was encountered.
     */
    public static Bitmap generateScaledScreenshot(
            View currentView, int maximumDimension, Bitmap.Config bitmapConfig) {
        Bitmap screenshot = null;
        boolean drawingCacheEnabled = currentView.isDrawingCacheEnabled();
        try {
            prepareViewHierarchyForScreenshot(currentView, true);
            if (!drawingCacheEnabled) currentView.setDrawingCacheEnabled(true);
            // Android has a maximum drawing cache size and if the drawing cache is bigger
            // than that, getDrawingCache() returns null.
            Bitmap originalBitmap = currentView.getDrawingCache();
            if (originalBitmap != null) {
                double originalHeight = originalBitmap.getHeight();
                double originalWidth = originalBitmap.getWidth();
                int newWidth = (int) originalWidth;
                int newHeight = (int) originalHeight;
                if (maximumDimension > 0) {
                    double scale = maximumDimension / Math.max(originalWidth, originalHeight);
                    newWidth = (int) Math.round(originalWidth * scale);
                    newHeight = (int) Math.round(originalHeight * scale);
                }
                Bitmap scaledScreenshot =
                        Bitmap.createScaledBitmap(originalBitmap, newWidth, newHeight, true);
                if (scaledScreenshot.getConfig() != bitmapConfig) {
                    screenshot = scaledScreenshot.copy(bitmapConfig, false);
                    scaledScreenshot.recycle();
                    scaledScreenshot = null;
                } else {
                    screenshot = scaledScreenshot;
                }
            } else if (currentView.getMeasuredHeight() > 0 && currentView.getMeasuredWidth() > 0) {
                double originalHeight = currentView.getMeasuredHeight();
                double originalWidth = currentView.getMeasuredWidth();
                int newWidth = (int) originalWidth;
                int newHeight = (int) originalHeight;
                if (maximumDimension > 0) {
                    double scale = maximumDimension / Math.max(originalWidth, originalHeight);
                    newWidth = (int) Math.round(originalWidth * scale);
                    newHeight = (int) Math.round(originalHeight * scale);
                }
                Bitmap bitmap = Bitmap.createBitmap(newWidth, newHeight, bitmapConfig);
                Canvas canvas = new Canvas(bitmap);
                canvas.scale((float) (newWidth / originalWidth),
                        (float) (newHeight / originalHeight));
                currentView.draw(canvas);
                screenshot = bitmap;
            }
        } catch (OutOfMemoryError e) {
            Log.d(TAG, "Unable to capture screenshot and scale it down." + e.getMessage());
        } finally {
            if (!drawingCacheEnabled) currentView.setDrawingCacheEnabled(false);
            prepareViewHierarchyForScreenshot(currentView, false);
        }
        return screenshot;
    }

    private static void prepareViewHierarchyForScreenshot(View view, boolean takingScreenshot) {
        if (view instanceof ViewGroup) {
            ViewGroup viewGroup = (ViewGroup) view;
            for (int i = 0; i < viewGroup.getChildCount(); i++) {
                prepareViewHierarchyForScreenshot(viewGroup.getChildAt(i), takingScreenshot);
            }
        } else if (view instanceof SurfaceView) {
            view.setWillNotDraw(!takingScreenshot);
        }
    }

    /**
     * Get a directory for the image capture operation. For devices with JB MR2
     * or latter android versions, the directory is IMAGE_FILE_PATH directory.
     * For ICS devices, the directory is CAPTURE_IMAGE_DIRECTORY.
     *
     * @param context The application context.
     * @return directory for the captured image to be stored.
     */
    public static File getDirectoryForImageCapture(Context context) throws IOException {
        // Temporarily allowing disk access while fixing. TODO: http://crbug.com/562173
        StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskReads();
        try {
            File path;
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
                path = new File(context.getFilesDir(), IMAGE_FILE_PATH);
                if (!path.exists() && !path.mkdir()) {
                    throw new IOException("Folder cannot be created.");
                }
            } else {
                File externalDataDir =
                        Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM);
                path = new File(externalDataDir.getAbsolutePath()
                                + File.separator
                                + EXTERNAL_IMAGE_FILE_PATH);
                if (!path.exists() && !path.mkdirs()) {
                    path = externalDataDir;
                }
            }
            return path;
        } finally {
            StrictMode.setThreadPolicy(oldPolicy);
        }
    }

    /**
     * Removes the view from its parent {@link ViewGroup}. No-op if the {@link View} is not yet
     * attached to the view hierarchy.
     *
     * @param view The view to be removed from the parent.
     */
    public static void removeViewFromParent(View view) {
        ViewGroup parent = (ViewGroup) view.getParent();
        if (parent == null) return;
        parent.removeView(view);
    }

    /**
     * Creates a {@link Typeface} that represents medium-weighted text.  This function returns
     * Roboto Medium when it is available (Lollipop and up) and Roboto Bold where it isn't.
     *
     * @return Typeface that can be applied to a View.
     */
    public static Typeface createRobotoMediumTypeface() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            // Roboto Medium, regular.
            return Typeface.create("sans-serif-medium", Typeface.NORMAL);
        } else {
            return Typeface.create("sans-serif", Typeface.BOLD);
        }
    }

    /**
     * Iterates through all items in the specified ListAdapter (including header and footer views)
     * and returns the width of the widest item (when laid out with height and width set to
     * WRAP_CONTENT).
     *
     * WARNING: do not call this on a ListAdapter with more than a handful of items, the performance
     * will be terrible since it measures every single item.
     *
     * @param adapter The ListAdapter whose widest item's width will be returned.
     * @return The measured width (in pixels) of the widest item in the passed-in ListAdapter.
     */
    public static int computeMaxWidthOfListAdapterItems(ListAdapter adapter) {
        final int widthMeasureSpec = MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED);
        final int heightMeasureSpec = MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED);
        AbsListView.LayoutParams params = new AbsListView.LayoutParams(
                AbsListView.LayoutParams.WRAP_CONTENT, AbsListView.LayoutParams.WRAP_CONTENT);

        int maxWidth = 0;
        View[] itemViews = new View[adapter.getViewTypeCount()];
        for (int i = 0; i < adapter.getCount(); ++i) {
            View itemView;
            int type = adapter.getItemViewType(i);
            if (type < 0) {
                // Type is negative for header/footer views, or views the adapter does not want
                // recycled.
                itemView = adapter.getView(i, null, null);
            } else {
                itemViews[type] = adapter.getView(i, itemViews[type], null);
                itemView = itemViews[type];
            }

            itemView.setLayoutParams(params);
            itemView.measure(widthMeasureSpec, heightMeasureSpec);
            maxWidth = Math.max(maxWidth, itemView.getMeasuredWidth());
        }

        return maxWidth;
    }

    /**
     * Get the index of a child {@link View} in a {@link ViewGroup}.
     * @param child The child to find the index of.
     * @return The index of the child in its parent. -1 if the child has no parent.
     */
    public static int getChildIndexInParent(View child) {
        if (child.getParent() == null) return -1;
        ViewGroup parent = (ViewGroup) child.getParent();
        int indexInParent = -1;
        for (int i = 0; i < parent.getChildCount(); i++) {
            if (parent.getChildAt(i) == child) {
                indexInParent = i;
                break;
            }
        }
        return indexInParent;
    }
}
