// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.webcontents;

import android.annotation.SuppressLint;
import android.graphics.Bitmap;
import android.graphics.Rect;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.Parcel;
import android.os.ParcelUuid;
import android.os.Parcelable;
import android.support.annotation.Nullable;
import android.view.Surface;

import org.chromium.base.Callback;
import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.content.browser.AppWebMessagePort;
import org.chromium.content.browser.MediaSessionImpl;
import org.chromium.content.browser.RenderCoordinatesImpl;
import org.chromium.content.browser.WindowEventObserver;
import org.chromium.content.browser.WindowEventObserverManager;
import org.chromium.content.browser.accessibility.WebContentsAccessibilityImpl;
import org.chromium.content.browser.framehost.RenderFrameHostDelegate;
import org.chromium.content.browser.framehost.RenderFrameHostImpl;
import org.chromium.content.browser.input.SelectPopup;
import org.chromium.content.browser.selection.SelectionPopupControllerImpl;
import org.chromium.content_public.browser.AccessibilitySnapshotCallback;
import org.chromium.content_public.browser.AccessibilitySnapshotNode;
import org.chromium.content_public.browser.ChildProcessImportance;
import org.chromium.content_public.browser.ImageDownloadCallback;
import org.chromium.content_public.browser.JavaScriptCallback;
import org.chromium.content_public.browser.MessagePort;
import org.chromium.content_public.browser.NavigationController;
import org.chromium.content_public.browser.RenderFrameHost;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.WebContents.UserDataFactory;
import org.chromium.content_public.browser.WebContentsInternals;
import org.chromium.content_public.browser.WebContentsObserver;
import org.chromium.ui.OverscrollRefreshHandler;
import org.chromium.ui.base.EventForwarder;
import org.chromium.ui.base.ViewAndroidDelegate;
import org.chromium.ui.base.WindowAndroid;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;

/**
 * The WebContentsImpl Java wrapper to allow communicating with the native WebContentsImpl
 * object.
 */
@JNINamespace("content")
public class WebContentsImpl implements WebContents, RenderFrameHostDelegate, WindowEventObserver {
    private static final String TAG = "cr_WebContentsImpl";

    private static final String PARCEL_VERSION_KEY = "version";
    private static final String PARCEL_WEBCONTENTS_KEY = "webcontents";
    private static final String PARCEL_PROCESS_GUARD_KEY = "processguard";

    private static final long PARCELABLE_VERSION_ID = 0;
    // Non-final for testing purposes, so resetting of the UUID can happen.
    private static UUID sParcelableUUID = UUID.randomUUID();

    /**
     * Used to reset the internal tracking for whether or not a serialized {@link WebContents}
     * was created in this process or not.
     */
    @VisibleForTesting
    public static void invalidateSerializedWebContentsForTesting() {
        sParcelableUUID = UUID.randomUUID();
    }

    /**
     * A {@link android.os.Parcelable.Creator} instance that is used to build
     * {@link WebContentsImpl} objects from a {@link Parcel}.
     */
    // TODO(crbug.com/635567): Fix this properly.
    @SuppressLint("ParcelClassLoader")
    public static final Parcelable.Creator<WebContents> CREATOR =
            new Parcelable.Creator<WebContents>() {
                @Override
                public WebContents createFromParcel(Parcel source) {
                    Bundle bundle = source.readBundle();

                    // Check the version.
                    if (bundle.getLong(PARCEL_VERSION_KEY, -1) != 0) return null;

                    // Check that we're in the same process.
                    ParcelUuid parcelUuid = bundle.getParcelable(PARCEL_PROCESS_GUARD_KEY);
                    if (sParcelableUUID.compareTo(parcelUuid.getUuid()) != 0) return null;

                    // Attempt to retrieve the WebContents object from the native pointer.
                    return nativeFromNativePtr(bundle.getLong(PARCEL_WEBCONTENTS_KEY));
                }

                @Override
                public WebContents[] newArray(int size) {
                    return new WebContents[size];
                }
            };

    // Note this list may be incomplete. Frames that never had to initialize java side would
    // not have an entry here. This is here mainly to keep the java RenderFrameHosts alive, since
    // native side generally cannot safely hold strong references to them.
    private final List<RenderFrameHostImpl> mFrames = new ArrayList<>();

    private long mNativeWebContentsAndroid;
    private NavigationController mNavigationController;

    // Lazily created proxy observer for handling all Java-based WebContentsObservers.
    private WebContentsObserverProxy mObserverProxy;

    // The media session for this WebContents. It is constructed by the native MediaSession and has
    // the same life time as native MediaSession.
    private MediaSessionImpl mMediaSession;

    // True while WebContents is functional and alive. Set to false when the native WebContents is
    // gone, or destroy() is called explicitly.
    private boolean mIsAlive;

    private class SmartClipCallback {
        public SmartClipCallback(final Handler smartClipHandler) {
            mHandler = smartClipHandler;
        }

        public void onSmartClipDataExtracted(String text, String html, Rect clipRect) {
            // The clipRect is in dip scale here. Add the contentOffset in same scale.
            RenderCoordinatesImpl coordinateSpace = getRenderCoordinates();
            clipRect.offset(0,
                    (int) (coordinateSpace.getContentOffsetYPix()
                            / coordinateSpace.getDeviceScaleFactor()));
            Bundle bundle = new Bundle();
            bundle.putString("url", getVisibleUrl());
            bundle.putString("title", getTitle());
            bundle.putString("text", text);
            bundle.putString("html", html);
            bundle.putParcelable("rect", clipRect);

            Message msg = Message.obtain(mHandler, 0);
            msg.setData(bundle);
            msg.sendToTarget();
        }

        final Handler mHandler;
    }
    private SmartClipCallback mSmartClipCallback;

    private EventForwarder mEventForwarder;

    private static class DefaultInternalsHolder implements InternalsHolder {
        private WebContentsInternals mInternals;

        @Override
        public void set(WebContentsInternals internals) {
            mInternals = internals;
        }

        @Override
        public WebContentsInternals get() {
            return mInternals;
        }
    }

    // Cached copy of all positions and scales as reported by the renderer.
    private RenderCoordinatesImpl mRenderCoordinates;

    private InternalsHolder mInternalsHolder;

    private static class WebContentsInternalsImpl implements WebContentsInternals {
        public HashMap<Class, WebContentsUserData> userDataMap;
        public ViewAndroidDelegate viewAndroidDelegate;
    }

    private WebContentsImpl(
            long nativeWebContentsAndroid, NavigationController navigationController) {
        mNativeWebContentsAndroid = nativeWebContentsAndroid;
        mNavigationController = navigationController;

        // Initialize |mInternalsHolder| with a default one that keeps all the internals
        // inside WebContentsImpl. It holds a strong reference until an embedder invokes
        // |setInternalsHolder| to get the internals handed over to it.
        WebContentsInternalsImpl internals = new WebContentsInternalsImpl();
        internals.userDataMap = new HashMap<>();

        mRenderCoordinates = new RenderCoordinatesImpl();
        mRenderCoordinates.reset();

        mInternalsHolder = new DefaultInternalsHolder();
        mInternalsHolder.set(internals);
        WindowEventObserverManager.from(this).addObserver(this);
        mIsAlive = true;
    }

    @CalledByNative
    private static WebContentsImpl create(
            long nativeWebContentsAndroid, NavigationController navigationController) {
        return new WebContentsImpl(nativeWebContentsAndroid, navigationController);
    }

    @CalledByNative
    private void clearNativePtr() {
        mNativeWebContentsAndroid = 0;
        mNavigationController = null;
        if (mObserverProxy != null) {
            mObserverProxy.destroy();
            mObserverProxy = null;
        }
        mIsAlive = false;
    }

    @Override
    public void setInternalsHolder(InternalsHolder internalsHolder) {
        // Ensure |setInternalsHolder()| is be called at most once.
        assert mInternalsHolder instanceof DefaultInternalsHolder;
        internalsHolder.set(mInternalsHolder.get());
        mInternalsHolder = internalsHolder;
    }

    // =================== RenderFrameHostDelegate overrides ===================
    @Override
    public void renderFrameCreated(RenderFrameHostImpl host) {
        assert !mFrames.contains(host);
        mFrames.add(host);
    }

    @Override
    public void renderFrameDeleted(RenderFrameHostImpl host) {
        assert mFrames.contains(host);
        mFrames.remove(host);
    }
    // ================= end RenderFrameHostDelegate overrides =================

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        // This is wrapped in a Bundle so that failed deserialization attempts don't corrupt the
        // overall Parcel.  If we failed a UUID or Version check and didn't read the rest of the
        // fields it would corrupt the serialized stream.
        Bundle data = new Bundle();
        data.putLong(PARCEL_VERSION_KEY, PARCELABLE_VERSION_ID);
        data.putParcelable(PARCEL_PROCESS_GUARD_KEY, new ParcelUuid(sParcelableUUID));
        data.putLong(PARCEL_WEBCONTENTS_KEY, mNativeWebContentsAndroid);
        dest.writeBundle(data);
    }

    @CalledByNative
    private long getNativePointer() {
        return mNativeWebContentsAndroid;
    }

    @Override
    public WindowAndroid getTopLevelNativeWindow() {
        return nativeGetTopLevelNativeWindow(mNativeWebContentsAndroid);
    }

    @Override
    public void setTopLevelNativeWindow(WindowAndroid windowAndroid) {
        nativeSetTopLevelNativeWindow(mNativeWebContentsAndroid, windowAndroid);
        WindowEventObserverManager.from(this).onWindowAndroidChanged(windowAndroid);
    }

    @Override
    public ViewAndroidDelegate getViewAndroidDelegate() {
        WebContentsInternals internals = mInternalsHolder.get();
        if (internals == null) return null;
        return ((WebContentsInternalsImpl) internals).viewAndroidDelegate;
    }

    public void setViewAndroidDelegate(ViewAndroidDelegate viewDelegate) {
        WebContentsInternals internals = mInternalsHolder.get();
        assert internals != null;
        WebContentsInternalsImpl impl = (WebContentsInternalsImpl) internals;
        assert impl.viewAndroidDelegate == null;
        impl.viewAndroidDelegate = viewDelegate;
        nativeSetViewAndroidDelegate(mNativeWebContentsAndroid, viewDelegate);
    }

    @Override
    public void destroy() {
        if (!ThreadUtils.runningOnUiThread()) {
            throw new IllegalStateException("Attempting to destroy WebContents on non-UI thread");
        }
        if (mNativeWebContentsAndroid != 0) nativeDestroyWebContents(mNativeWebContentsAndroid);
    }

    public void destroyContentsInternal() {
        mIsAlive = false;
    }

    @Override
    public boolean isDestroyed() {
        return !mIsAlive || mNativeWebContentsAndroid == 0;
    }

    @Override
    public NavigationController getNavigationController() {
        return mNavigationController;
    }

    @Override
    public RenderFrameHost getMainFrame() {
        return nativeGetMainFrame(mNativeWebContentsAndroid);
    }

    @Override
    public String getTitle() {
        return nativeGetTitle(mNativeWebContentsAndroid);
    }

    @Override
    public String getVisibleUrl() {
        return nativeGetVisibleURL(mNativeWebContentsAndroid);
    }

    @Override
    public String getEncoding() {
        return nativeGetEncoding(mNativeWebContentsAndroid);
    }

    @Override
    public boolean isLoading() {
        return nativeIsLoading(mNativeWebContentsAndroid);
    }

    @Override
    public boolean isLoadingToDifferentDocument() {
        return nativeIsLoadingToDifferentDocument(mNativeWebContentsAndroid);
    }

    @Override
    public void stop() {
        nativeStop(mNativeWebContentsAndroid);
    }

    /**
     * Cut the selected content.
     */
    public void cut() {
        nativeCut(mNativeWebContentsAndroid);
    }

    /**
     * Copy the selected content.
     */
    public void copy() {
        nativeCopy(mNativeWebContentsAndroid);
    }

    /**
     * Paste content from the clipboard.
     */
    public void paste() {
        nativePaste(mNativeWebContentsAndroid);
    }

    /**
     * Paste content from the clipboard without format.
     */
    public void pasteAsPlainText() {
        nativePasteAsPlainText(mNativeWebContentsAndroid);
    }

    /**
     * Replace the selected text with the {@code word}.
     */
    public void replace(String word) {
        nativeReplace(mNativeWebContentsAndroid, word);
    }

    /**
     * Select all content.
     */
    public void selectAll() {
        nativeSelectAll(mNativeWebContentsAndroid);
    }

    /**
     * Collapse the selection to the end of selection range.
     */
    public void collapseSelection() {
        // collapseSelection may get triggered when certain selection-related widgets
        // are destroyed. As the timing for such destruction is unpredictable,
        // safely guard against this case.
        if (isDestroyed()) return;
        nativeCollapseSelection(mNativeWebContentsAndroid);
    }

    @Override
    public void onHide() {
        SelectionPopupControllerImpl controller = getSelectionPopupController();
        if (controller != null) controller.hidePopupsAndPreserveSelection();
        nativeOnHide(mNativeWebContentsAndroid);
    }

    @Override
    public void onShow() {
        WebContentsAccessibilityImpl wcax = WebContentsAccessibilityImpl.fromWebContents(this);
        if (wcax != null) wcax.refreshState();
        SelectionPopupControllerImpl controller = getSelectionPopupController();
        if (controller != null) controller.restoreSelectionPopupsIfNecessary();
        nativeOnShow(mNativeWebContentsAndroid);
    }

    private SelectionPopupControllerImpl getSelectionPopupController() {
        return SelectionPopupControllerImpl.fromWebContents(this);
    }

    @Override
    public void setImportance(@ChildProcessImportance int mainFrameImportance) {
        nativeSetImportance(mNativeWebContentsAndroid, mainFrameImportance);
    }

    @Override
    public void suspendAllMediaPlayers() {
        nativeSuspendAllMediaPlayers(mNativeWebContentsAndroid);
    }

    @Override
    public void setAudioMuted(boolean mute) {
        nativeSetAudioMuted(mNativeWebContentsAndroid, mute);
    }

    @Override
    public int getBackgroundColor() {
        return nativeGetBackgroundColor(mNativeWebContentsAndroid);
    }

    @Override
    public boolean isShowingInterstitialPage() {
        return nativeIsShowingInterstitialPage(mNativeWebContentsAndroid);
    }

    @Override
    public boolean focusLocationBarByDefault() {
        return nativeFocusLocationBarByDefault(mNativeWebContentsAndroid);
    }

    @Override
    public boolean isReady() {
        return nativeIsRenderWidgetHostViewReady(mNativeWebContentsAndroid);
    }

    @Override
    public void exitFullscreen() {
        nativeExitFullscreen(mNativeWebContentsAndroid);
    }

    @Override
    public void scrollFocusedEditableNodeIntoView() {
        // The native side keeps track of whether the zoom and scroll actually occurred. It is
        // more efficient to do it this way and sometimes fire an unnecessary message rather
        // than synchronize with the renderer and always have an additional message.
        nativeScrollFocusedEditableNodeIntoView(mNativeWebContentsAndroid);
    }

    @Override
    public void selectWordAroundCaret() {
        nativeSelectWordAroundCaret(mNativeWebContentsAndroid);
    }

    @Override
    public void adjustSelectionByCharacterOffset(
            int startAdjust, int endAdjust, boolean showSelectionMenu) {
        nativeAdjustSelectionByCharacterOffset(
                mNativeWebContentsAndroid, startAdjust, endAdjust, showSelectionMenu);
    }

    @Override
    public String getLastCommittedUrl() {
        return nativeGetLastCommittedURL(mNativeWebContentsAndroid);
    }

    @Override
    public boolean isIncognito() {
        return nativeIsIncognito(mNativeWebContentsAndroid);
    }

    @Override
    public void resumeLoadingCreatedWebContents() {
        nativeResumeLoadingCreatedWebContents(mNativeWebContentsAndroid);
    }

    @Override
    public void evaluateJavaScript(String script, JavaScriptCallback callback) {
        if (isDestroyed() || script == null) return;
        nativeEvaluateJavaScript(mNativeWebContentsAndroid, script, callback);
    }

    @Override
    @VisibleForTesting
    public void evaluateJavaScriptForTests(String script, JavaScriptCallback callback) {
        if (script == null) return;
        nativeEvaluateJavaScriptForTests(mNativeWebContentsAndroid, script, callback);
    }

    @Override
    public void addMessageToDevToolsConsole(int level, String message) {
        nativeAddMessageToDevToolsConsole(mNativeWebContentsAndroid, level, message);
    }

    @Override
    public void postMessageToFrame(String frameName, String message,
            String sourceOrigin, String targetOrigin, MessagePort[] ports) {
        if (ports != null) {
            for (MessagePort port : ports) {
                if (port.isClosed() || port.isTransferred()) {
                    throw new IllegalStateException("Port is already closed or transferred");
                }
                if (port.isStarted()) {
                    throw new IllegalStateException("Port is already started");
                }
            }
        }
        // Treat "*" as a wildcard. Internally, a wildcard is a empty string.
        if (targetOrigin.equals("*")) {
            targetOrigin = "";
        }
        nativePostMessageToFrame(
                mNativeWebContentsAndroid, frameName, message, sourceOrigin, targetOrigin, ports);
    }

    @Override
    public AppWebMessagePort[] createMessageChannel()
            throws IllegalStateException {
        return AppWebMessagePort.createPair();
    }

    @Override
    public boolean hasAccessedInitialDocument() {
        return nativeHasAccessedInitialDocument(mNativeWebContentsAndroid);
    }

    @CalledByNative
    private static void onEvaluateJavaScriptResult(
            String jsonResult, JavaScriptCallback callback) {
        callback.handleJavaScriptResult(jsonResult);
    }

    @Override
    public int getThemeColor() {
        return nativeGetThemeColor(mNativeWebContentsAndroid);
    }

    @Override
    public void requestSmartClipExtract(int x, int y, int width, int height) {
        if (mSmartClipCallback == null) return;
        RenderCoordinatesImpl coordinateSpace = getRenderCoordinates();
        float dpi = coordinateSpace.getDeviceScaleFactor();
        y = y - (int) coordinateSpace.getContentOffsetYPix();
        nativeRequestSmartClipExtract(mNativeWebContentsAndroid, mSmartClipCallback,
                (int) (x / dpi), (int) (y / dpi), (int) (width / dpi), (int) (height / dpi));
    }

    @Override
    public void setSmartClipResultHandler(final Handler smartClipHandler) {
        if (smartClipHandler == null) {
            mSmartClipCallback = null;
            return;
        }
        mSmartClipCallback = new SmartClipCallback(smartClipHandler);
    }

    @CalledByNative
    private static void onSmartClipDataExtracted(String text, String html, int left, int top,
            int right, int bottom, SmartClipCallback callback) {
        callback.onSmartClipDataExtracted(text, html, new Rect(left, top, right, bottom));
    }

    @Override
    public void requestAccessibilitySnapshot(AccessibilitySnapshotCallback callback) {
        nativeRequestAccessibilitySnapshot(mNativeWebContentsAndroid, callback);
    }

    @Override
    @VisibleForTesting
    public void simulateRendererKilledForTesting(boolean wasOomProtected) {
        if (mObserverProxy != null) {
            mObserverProxy.renderProcessGone(wasOomProtected);
        }
    }

    /**
     * @return The amount of the top controls height if controls are in the state
     *    of shrinking Blink's view size, otherwise 0.
     */
    @VisibleForTesting
    public int getTopControlsShrinkBlinkHeightForTesting() {
        // TODO(jinsukkim): Let callsites provide with its own top controls height to remove
        //                  the test-only method in content layer.
        if (mNativeWebContentsAndroid == 0) return 0;
        return nativeGetTopControlsShrinkBlinkHeightPixForTesting(mNativeWebContentsAndroid);
    }

    @VisibleForTesting
    @Override
    public boolean isSelectPopupVisibleForTesting() {
        return SelectPopup.fromWebContents(this).isVisibleForTesting();
    }

    // root node can be null if parsing fails.
    @CalledByNative
    private static void onAccessibilitySnapshot(AccessibilitySnapshotNode root,
            AccessibilitySnapshotCallback callback) {
        callback.onAccessibilitySnapshot(root);
    }

    @CalledByNative
    private static void addAccessibilityNodeAsChild(AccessibilitySnapshotNode parent,
            AccessibilitySnapshotNode child) {
        parent.addChild(child);
    }

    @CalledByNative
    private static AccessibilitySnapshotNode createAccessibilitySnapshotNode(int parentRelativeLeft,
            int parentRelativeTop, int width, int height, boolean isRootNode, String text,
            int color, int bgcolor, float size, boolean bold, boolean italic, boolean underline,
            boolean lineThrough, String className) {
        AccessibilitySnapshotNode node = new AccessibilitySnapshotNode(text, className);

        // if size is smaller than 0, then style information does not exist.
        if (size >= 0.0) {
            node.setStyle(color, bgcolor, size, bold, italic, underline, lineThrough);
        }
        node.setLocationInfo(parentRelativeLeft, parentRelativeTop, width, height, isRootNode);
        return node;
    }

    @CalledByNative
    private static void setAccessibilitySnapshotSelection(
            AccessibilitySnapshotNode node, int start, int end) {
        node.setSelection(start, end);
    }

    @Override
    public EventForwarder getEventForwarder() {
        assert mNativeWebContentsAndroid != 0;
        if (mEventForwarder == null) {
            mEventForwarder = nativeGetOrCreateEventForwarder(mNativeWebContentsAndroid);
        }
        return mEventForwarder;
    }

    @Override
    public void addObserver(WebContentsObserver observer) {
        assert mNativeWebContentsAndroid != 0;
        if (mObserverProxy == null) mObserverProxy = new WebContentsObserverProxy(this);
        mObserverProxy.addObserver(observer);
    }

    @Override
    public void removeObserver(WebContentsObserver observer) {
        if (mObserverProxy == null) return;
        mObserverProxy.removeObserver(observer);
    }

    @Override
    public void setOverscrollRefreshHandler(OverscrollRefreshHandler handler) {
        nativeSetOverscrollRefreshHandler(mNativeWebContentsAndroid, handler);
    }

    @Override
    public void getContentBitmapAsync(
            int width, int height, String path, Callback<String> callback) {
        nativeGetContentBitmap(mNativeWebContentsAndroid, width, height, path, callback);
    }

    @Override
    public void reloadLoFiImages() {
        nativeReloadLoFiImages(mNativeWebContentsAndroid);
    }

    @Override
    public int downloadImage(String url, boolean isFavicon, int maxBitmapSize,
            boolean bypassCache, ImageDownloadCallback callback) {
        return nativeDownloadImage(mNativeWebContentsAndroid,
                url, isFavicon, maxBitmapSize, bypassCache, callback);
    }

    @CalledByNative
    private void onDownloadImageFinished(ImageDownloadCallback callback, int id, int httpStatusCode,
            String imageUrl, List<Bitmap> bitmaps, List<Rect> sizes) {
        callback.onFinishDownloadImage(id, httpStatusCode, imageUrl, bitmaps, sizes);
    }

    /**
     * Removes handles used in text selection.
     */
    public void dismissTextHandles() {
        if (isDestroyed()) return;
        nativeDismissTextHandles(mNativeWebContentsAndroid);
    }

    /**
     * Shows paste popup menu at the touch handle at specified location.
     */
    public void showContextMenuAtTouchHandle(int x, int y) {
        nativeShowContextMenuAtTouchHandle(mNativeWebContentsAndroid, x, y);
    }

    @Override
    public void setHasPersistentVideo(boolean value) {
        nativeSetHasPersistentVideo(mNativeWebContentsAndroid, value);
    }

    @Override
    public boolean hasActiveEffectivelyFullscreenVideo() {
        return nativeHasActiveEffectivelyFullscreenVideo(mNativeWebContentsAndroid);
    }

    @Override
    public boolean isPictureInPictureAllowedForFullscreenVideo() {
        return nativeIsPictureInPictureAllowedForFullscreenVideo(mNativeWebContentsAndroid);
    }

    @Override
    public @Nullable Rect getFullscreenVideoSize() {
        return nativeGetFullscreenVideoSize(mNativeWebContentsAndroid);
    }

    @Override
    public void setSize(int width, int height) {
        nativeSetSize(mNativeWebContentsAndroid, width, height);
    }

    @Override
    public int getWidth() {
        return nativeGetWidth(mNativeWebContentsAndroid);
    }

    @Override
    public int getHeight() {
        return nativeGetHeight(mNativeWebContentsAndroid);
    }

    @CalledByNative
    private final void setMediaSession(MediaSessionImpl mediaSession) {
        mMediaSession = mediaSession;
    }

    @CalledByNative
    private static List<Bitmap> createBitmapList() {
        return new ArrayList<Bitmap>();
    }

    @CalledByNative
    private static void addToBitmapList(List<Bitmap> bitmaps, Bitmap bitmap) {
        bitmaps.add(bitmap);
    }

    @CalledByNative
    private static List<Rect> createSizeList() {
        return new ArrayList<Rect>();
    }

    @CalledByNative
    private static void createSizeAndAddToList(List<Rect> sizes, int width, int height) {
        sizes.add(new Rect(0, 0, width, height));
    }

    @CalledByNative
    private static Rect createSize(int width, int height) {
        return new Rect(0, 0, width, height);
    }

    /**
     * Returns {@link RenderCoordinatesImpl}.
     */
    public RenderCoordinatesImpl getRenderCoordinates() {
        return mRenderCoordinates;
    }

    @Override
    @SuppressWarnings("unchecked")
    public <T> T getOrSetUserData(Class key, UserDataFactory<T> userDataFactory) {
        Map<Class, WebContentsUserData> userDataMap = getUserDataMap();

        // Map can be null after WebView gets gc'ed on its way to destruction.
        if (userDataMap == null) {
            Log.e(TAG, "UserDataMap can't be found");
            return null;
        }

        WebContentsUserData data = userDataMap.get(key);
        if (data == null && userDataFactory != null) {
            assert !userDataMap.containsKey(key); // Do not allow duplicated Data

            T object = userDataFactory.create(this);
            assert key.isInstance(object);
            userDataMap.put(key, new WebContentsUserData(object));
            // Retrieves from the map again to return null in case |setUserData| fails
            // to store the object.
            data = userDataMap.get(key);
        }
        // Casting Object to T is safe since we make sure the object was of type T upon creation.
        return data != null ? (T) data.getObject() : null;
    }

    /**
     * @return {@code UserDataMap} that contains internal user data. {@code null} if
     *         the map is already gc'ed.
     */
    private Map<Class, WebContentsUserData> getUserDataMap() {
        WebContentsInternals internals = mInternalsHolder.get();
        if (internals == null) return null;
        return ((WebContentsInternalsImpl) internals).userDataMap;
    }

    // WindowEventObserver

    @Override
    public void onRotationChanged(int rotation) {
        if (mNativeWebContentsAndroid == 0) return;
        int rotationDegrees = 0;
        switch (rotation) {
            case Surface.ROTATION_0:
                rotationDegrees = 0;
                break;
            case Surface.ROTATION_90:
                rotationDegrees = 90;
                break;
            case Surface.ROTATION_180:
                rotationDegrees = 180;
                break;
            case Surface.ROTATION_270:
                rotationDegrees = -90;
                break;
            default:
                throw new IllegalStateException(
                        "Display.getRotation() shouldn't return that value");
        }
        nativeSendOrientationChangeEvent(mNativeWebContentsAndroid, rotationDegrees);
    }

    @Override
    public void onDIPScaleChanged(float dipScale) {
        if (mNativeWebContentsAndroid == 0) return;
        mRenderCoordinates.setDeviceScaleFactor(dipScale);
        nativeOnScaleFactorChanged(mNativeWebContentsAndroid);
    }

    // This is static to avoid exposing a public destroy method on the native side of this class.
    private static native void nativeDestroyWebContents(long webContentsAndroidPtr);

    private static native WebContents nativeFromNativePtr(long webContentsAndroidPtr);

    private native WindowAndroid nativeGetTopLevelNativeWindow(long nativeWebContentsAndroid);
    private native void nativeSetTopLevelNativeWindow(
            long nativeWebContentsAndroid, WindowAndroid windowAndroid);
    private native RenderFrameHost nativeGetMainFrame(long nativeWebContentsAndroid);
    private native String nativeGetTitle(long nativeWebContentsAndroid);
    private native String nativeGetVisibleURL(long nativeWebContentsAndroid);
    private native String nativeGetEncoding(long nativeWebContentsAndroid);
    private native boolean nativeIsLoading(long nativeWebContentsAndroid);
    private native boolean nativeIsLoadingToDifferentDocument(long nativeWebContentsAndroid);
    private native void nativeStop(long nativeWebContentsAndroid);
    private native void nativeCut(long nativeWebContentsAndroid);
    private native void nativeCopy(long nativeWebContentsAndroid);
    private native void nativePaste(long nativeWebContentsAndroid);
    private native void nativePasteAsPlainText(long nativeWebContentsAndroid);
    private native void nativeReplace(long nativeWebContentsAndroid, String word);
    private native void nativeSelectAll(long nativeWebContentsAndroid);
    private native void nativeCollapseSelection(long nativeWebContentsAndroid);
    private native void nativeOnHide(long nativeWebContentsAndroid);
    private native void nativeOnShow(long nativeWebContentsAndroid);
    private native void nativeSetImportance(long nativeWebContentsAndroid, int importance);
    private native void nativeSuspendAllMediaPlayers(long nativeWebContentsAndroid);
    private native void nativeSetAudioMuted(long nativeWebContentsAndroid, boolean mute);
    private native int nativeGetBackgroundColor(long nativeWebContentsAndroid);
    private native boolean nativeIsShowingInterstitialPage(long nativeWebContentsAndroid);
    private native boolean nativeFocusLocationBarByDefault(long nativeWebContentsAndroid);
    private native boolean nativeIsRenderWidgetHostViewReady(long nativeWebContentsAndroid);
    private native void nativeExitFullscreen(long nativeWebContentsAndroid);
    private native void nativeScrollFocusedEditableNodeIntoView(long nativeWebContentsAndroid);
    private native void nativeSelectWordAroundCaret(long nativeWebContentsAndroid);
    private native void nativeAdjustSelectionByCharacterOffset(long nativeWebContentsAndroid,
            int startAdjust, int endAdjust, boolean showSelectionMenu);
    private native String nativeGetLastCommittedURL(long nativeWebContentsAndroid);
    private native boolean nativeIsIncognito(long nativeWebContentsAndroid);
    private native void nativeResumeLoadingCreatedWebContents(long nativeWebContentsAndroid);
    private native void nativeEvaluateJavaScript(long nativeWebContentsAndroid,
            String script, JavaScriptCallback callback);
    private native void nativeEvaluateJavaScriptForTests(long nativeWebContentsAndroid,
            String script, JavaScriptCallback callback);
    private native void nativeAddMessageToDevToolsConsole(
            long nativeWebContentsAndroid, int level, String message);
    private native void nativePostMessageToFrame(long nativeWebContentsAndroid, String frameName,
            String message, String sourceOrigin, String targetOrigin, MessagePort[] ports);
    private native boolean nativeHasAccessedInitialDocument(
            long nativeWebContentsAndroid);
    private native int nativeGetThemeColor(long nativeWebContentsAndroid);
    private native void nativeRequestSmartClipExtract(long nativeWebContentsAndroid,
            SmartClipCallback callback, int x, int y, int width, int height);
    private native void nativeRequestAccessibilitySnapshot(
            long nativeWebContentsAndroid, AccessibilitySnapshotCallback callback);
    private native void nativeSetOverscrollRefreshHandler(
            long nativeWebContentsAndroid, OverscrollRefreshHandler nativeOverscrollRefreshHandler);
    private native void nativeGetContentBitmap(long nativeWebContentsAndroid, int width, int height,
            String path, Callback<String> callback);
    private native void nativeReloadLoFiImages(long nativeWebContentsAndroid);
    private native int nativeDownloadImage(long nativeWebContentsAndroid,
            String url, boolean isFavicon, int maxBitmapSize,
            boolean bypassCache, ImageDownloadCallback callback);
    private native void nativeDismissTextHandles(long nativeWebContentsAndroid);
    private native void nativeShowContextMenuAtTouchHandle(
            long nativeWebContentsAndroid, int x, int y);
    private native void nativeSetHasPersistentVideo(long nativeWebContentsAndroid, boolean value);
    private native boolean nativeHasActiveEffectivelyFullscreenVideo(long nativeWebContentsAndroid);
    private native boolean nativeIsPictureInPictureAllowedForFullscreenVideo(
            long nativeWebContentsAndroid);
    private native Rect nativeGetFullscreenVideoSize(long nativeWebContentsAndroid);
    private native void nativeSetSize(long nativeWebContentsAndroid, int width, int height);
    private native int nativeGetWidth(long nativeWebContentsAndroid);
    private native int nativeGetHeight(long nativeWebContentsAndroid);
    private native EventForwarder nativeGetOrCreateEventForwarder(long nativeWebContentsAndroid);
    private native int nativeGetTopControlsShrinkBlinkHeightPixForTesting(
            long nativeWebContentsAndroid);
    private native void nativeSetViewAndroidDelegate(
            long nativeWebContentsAndroid, ViewAndroidDelegate viewDelegate);
    private native void nativeSendOrientationChangeEvent(
            long nativeWebContentsAndroid, int orientation);
    private native void nativeOnScaleFactorChanged(long nativeWebContentsAndroid);
}
