/*
 * Copyright (C) 2014 The Android Open Source Project
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

package android.databinding;

import android.annotation.TargetApi;
import android.content.res.ColorStateList;
import android.databinding.CallbackRegistry.NotifierCallback;
import android.graphics.drawable.Drawable;
import android.os.Build.VERSION;
import android.os.Build.VERSION_CODES;
import android.os.Handler;
import android.os.Looper;
import android.text.TextUtils;
import android.util.LongSparseArray;
import android.util.SparseArray;
import android.util.SparseBooleanArray;
import android.util.SparseIntArray;
import android.util.SparseLongArray;
import android.view.Choreographer;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnAttachStateChangeListener;
import android.view.ViewGroup;

import com.android.databinding.library.R;

import java.lang.ref.WeakReference;
import java.util.List;
import java.util.Map;

/**
 * Base class for generated data binding classes. If possible, the generated binding should
 * be instantiated using one of its generated static bind or inflate methods. If the specific
 * binding is unknown, {@link DataBindingUtil#bind(View)} or
 * {@link DataBindingUtil#inflate(LayoutInflater, int, ViewGroup, boolean)} should be used.
 */
public abstract class ViewDataBinding extends BaseObservable {

    /**
     * Instead of directly accessing Build.VERSION.SDK_INT, generated code uses this value so that
     * we can test API dependent behavior.
     */
    static int SDK_INT = VERSION.SDK_INT;

    private static final int REBIND = 1;
    private static final int HALTED = 2;
    private static final int REBOUND = 3;

    /**
     * Prefix for android:tag on Views with binding. The root View and include tags will not have
     * android:tag attributes and will use ids instead.
     *
     * @hide
     */
    public static final String BINDING_TAG_PREFIX = "binding_";

    // The length of BINDING_TAG_PREFIX prevents calling length repeatedly.
    private static final int BINDING_NUMBER_START = BINDING_TAG_PREFIX.length();

    // ICS (v 14) fixes a leak when using setTag(int, Object)
    private static final boolean USE_TAG_ID = DataBinderMapper.TARGET_MIN_SDK >= 14;

    private static final boolean USE_CHOREOGRAPHER = SDK_INT >= 16;

    /**
     * Method object extracted out to attach a listener to a bound Observable object.
     */
    private static final CreateWeakListener CREATE_PROPERTY_LISTENER = new CreateWeakListener() {
        @Override
        public WeakListener create(ViewDataBinding viewDataBinding, int localFieldId) {
            return new WeakPropertyListener(viewDataBinding, localFieldId).getListener();
        }
    };

    /**
     * Method object extracted out to attach a listener to a bound ObservableList object.
     */
    private static final CreateWeakListener CREATE_LIST_LISTENER = new CreateWeakListener() {
        @Override
        public WeakListener create(ViewDataBinding viewDataBinding, int localFieldId) {
            return new WeakListListener(viewDataBinding, localFieldId).getListener();
        }
    };

    /**
     * Method object extracted out to attach a listener to a bound ObservableMap object.
     */
    private static final CreateWeakListener CREATE_MAP_LISTENER = new CreateWeakListener() {
        @Override
        public WeakListener create(ViewDataBinding viewDataBinding, int localFieldId) {
            return new WeakMapListener(viewDataBinding, localFieldId).getListener();
        }
    };

    private static final CallbackRegistry.NotifierCallback<OnRebindCallback, ViewDataBinding, Void>
        REBIND_NOTIFIER = new NotifierCallback<OnRebindCallback, ViewDataBinding, Void>() {
        @Override
        public void onNotifyCallback(OnRebindCallback callback, ViewDataBinding sender, int mode,
                Void arg2) {
            switch (mode) {
                case REBIND:
                    if (!callback.onPreBind(sender)) {
                        sender.mRebindHalted = true;
                    }
                    break;
                case HALTED:
                    callback.onCanceled(sender);
                    break;
                case REBOUND:
                    callback.onBound(sender);
                    break;
            }
        }
    };

    private static final OnAttachStateChangeListener ROOT_REATTACHED_LISTENER;

    static {
        if (VERSION.SDK_INT < VERSION_CODES.KITKAT) {
            ROOT_REATTACHED_LISTENER = null;
        } else {
            ROOT_REATTACHED_LISTENER = new OnAttachStateChangeListener() {
                @TargetApi(VERSION_CODES.KITKAT)
                @Override
                public void onViewAttachedToWindow(View v) {
                    // execute the pending bindings.
                    final ViewDataBinding binding = getBinding(v);
                    binding.mRebindRunnable.run();
                    v.removeOnAttachStateChangeListener(this);
                }

                @Override
                public void onViewDetachedFromWindow(View v) {
                }
            };
        }
    }

    /**
     * Runnable executed on animation heartbeat to rebind the dirty Views.
     */
    private final Runnable mRebindRunnable = new Runnable() {
        @Override
        public void run() {
            synchronized (this) {
                mPendingRebind = false;
            }
            if (VERSION.SDK_INT >= VERSION_CODES.KITKAT) {
                // Nested so that we don't get a lint warning in IntelliJ
                if (!mRoot.isAttachedToWindow()) {
                    // Don't execute the pending bindings until the View
                    // is attached again.
                    mRoot.removeOnAttachStateChangeListener(ROOT_REATTACHED_LISTENER);
                    mRoot.addOnAttachStateChangeListener(ROOT_REATTACHED_LISTENER);
                    return;
                }
            }
            executePendingBindings();
        }
    };

    /**
     * Flag indicates that there are pending bindings that need to be reevaluated.
     */
    private boolean mPendingRebind = false;

    /**
     * Indicates that a onPreBind has stopped the executePendingBindings call.
     */
    private boolean mRebindHalted = false;

    /**
     * The observed expressions.
     */
    private WeakListener[] mLocalFieldObservers;

    /**
     * The root View that this Binding is associated with.
     */
    private final View mRoot;

    /**
     * The collection of OnRebindCallbacks.
     */
    private CallbackRegistry<OnRebindCallback, ViewDataBinding, Void> mRebindCallbacks;

    /**
     * Flag to prevent reentrant executePendingBinding calls.
     */
    private boolean mIsExecutingPendingBindings;

    // null api < 16
    private Choreographer mChoreographer;

    private final Choreographer.FrameCallback mFrameCallback;

    // null api >= 16
    private Handler mUIThreadHandler;

    /**
     * The DataBindingComponent used by this data binding. This is used for BindingAdapters
     * that are instance methods to retrieve the class instance that implements the
     * adapter.
     *
     * @hide
     */
    protected final DataBindingComponent mBindingComponent;

    /**
     * @hide
     */
    protected ViewDataBinding(DataBindingComponent bindingComponent, View root, int localFieldCount) {
        mBindingComponent = bindingComponent;
        mLocalFieldObservers = new WeakListener[localFieldCount];
        this.mRoot = root;
        if (Looper.myLooper() == null) {
            throw new IllegalStateException("DataBinding must be created in view's UI Thread");
        }
        if (USE_CHOREOGRAPHER) {
            mChoreographer = Choreographer.getInstance();
            mFrameCallback = new Choreographer.FrameCallback() {
                @Override
                public void doFrame(long frameTimeNanos) {
                    mRebindRunnable.run();
                }
            };
        } else {
            mFrameCallback = null;
            mUIThreadHandler = new Handler(Looper.myLooper());
        }
    }

    /**
     * @hide
     */
    protected void setRootTag(View view) {
        if (USE_TAG_ID) {
            view.setTag(R.id.dataBinding, this);
        } else {
            view.setTag(this);
        }
    }

    /**
     * @hide
     */
    protected void setRootTag(View[] views) {
        if (USE_TAG_ID) {
            for (View view : views) {
                view.setTag(R.id.dataBinding, this);
            }
        } else {
            for (View view : views) {
                view.setTag(this);
            }
        }
    }

    /**
     * @hide
     */
    public static int getBuildSdkInt() {
        return SDK_INT;
    }

    /**
     * Called when an observed object changes. Sets the appropriate dirty flag if applicable.
     * @param localFieldId The index into mLocalFieldObservers that this Object resides in.
     * @param object The object that has changed.
     * @param fieldId The BR ID of the field being changed or _all if
     *                no specific field is being notified.
     * @return true if this change should cause a change to the UI.
     * @hide
     */
    protected abstract boolean onFieldChange(int localFieldId, Object object, int fieldId);

    /**
     * Set a value value in the Binding class.
     * <p>
     * Typically, the developer will be able to call the subclass's set method directly. For
     * example, if there is a variable <code>x</code> in the Binding, a <code>setX</code> method
     * will be generated. However, there are times when the specific subclass of ViewDataBinding
     * is unknown, so the generated method cannot be discovered without reflection. The
     * setVariable call allows the values of variables to be set without reflection.
     *
     * @param variableId the BR id of the variable to be set. For example, if the variable is
     *                   <code>x</code>, then variableId will be <code>BR.x</code>.
     * @param value The new value of the variable to be set.
     * @return <code>true</code> if the variable is declared or used in the binding or
     * <code>false</code> otherwise.
     */
    public abstract boolean setVariable(int variableId, Object value);

    /**
     * Add a listener to be called when reevaluating dirty fields. This also allows automatic
     * updates to be halted, but does not stop explicit calls to {@link #executePendingBindings()}.
     *
     * @param listener The listener to add.
     */
    public void addOnRebindCallback(OnRebindCallback listener) {
        if (mRebindCallbacks == null) {
            mRebindCallbacks = new CallbackRegistry<OnRebindCallback, ViewDataBinding, Void>(REBIND_NOTIFIER);
        }
        mRebindCallbacks.add(listener);
    }

    /**
     * Removes a listener that was added in {@link #addOnRebindCallback(OnRebindCallback)}.
     *
     * @param listener The listener to remove.
     */
    public void removeOnRebindCallback(OnRebindCallback listener) {
        if (mRebindCallbacks != null) {
            mRebindCallbacks.remove(listener);
        }
    }

    /**
     * Evaluates the pending bindings, updating any Views that have expressions bound to
     * modified variables. This <b>must</b> be run on the UI thread.
     */
    public void executePendingBindings() {
        if (mIsExecutingPendingBindings) {
            requestRebind();
            return;
        }
        if (!hasPendingBindings()) {
            return;
        }
        mIsExecutingPendingBindings = true;
        mRebindHalted = false;
        if (mRebindCallbacks != null) {
            mRebindCallbacks.notifyCallbacks(this, REBIND, null);

            // The onRebindListeners will change mPendingHalted
            if (mRebindHalted) {
                mRebindCallbacks.notifyCallbacks(this, HALTED, null);
            }
        }
        if (!mRebindHalted) {
            executeBindings();
            if (mRebindCallbacks != null) {
                mRebindCallbacks.notifyCallbacks(this, REBOUND, null);
            }
        }
        mIsExecutingPendingBindings = false;
    }

    void forceExecuteBindings() {
        executeBindings();
    }

    /**
     * @hide
     */
    protected abstract void executeBindings();

    /**
     * Invalidates all binding expressions and requests a new rebind to refresh UI.
     */
    public abstract void invalidateAll();

    /**
     * Returns whether the UI needs to be refresh to represent the current data.
     *
     * @return true if any field has changed and the binding should be evaluated.
     */
    public abstract boolean hasPendingBindings();

    /**
     * Removes binding listeners to expression variables.
     */
    public void unbind() {
        for (WeakListener weakListener : mLocalFieldObservers) {
            if (weakListener != null) {
                weakListener.unregister();
            }
        }
    }

    @Override
    protected void finalize() throws Throwable {
        unbind();
    }

    static ViewDataBinding getBinding(View v) {
        if (v != null) {
            if (USE_TAG_ID) {
                return (ViewDataBinding) v.getTag(R.id.dataBinding);
            } else {
                final Object tag = v.getTag();
                if (tag instanceof ViewDataBinding) {
                    return (ViewDataBinding) tag;
                }
            }
        }
        return null;
    }

    /**
     * Returns the outermost View in the layout file associated with the Binding. If this
     * binding is for a merge layout file, this will return the first root in the merge tag.
     *
     * @return the outermost View in the layout file associated with the Binding.
     */
    public View getRoot() {
        return mRoot;
    }

    private void handleFieldChange(int mLocalFieldId, Object object, int fieldId) {
        boolean result = onFieldChange(mLocalFieldId, object, fieldId);
        if (result) {
            requestRebind();
        }
    }

    /**
     * @hide
     */
    protected boolean unregisterFrom(int localFieldId) {
        WeakListener listener = mLocalFieldObservers[localFieldId];
        if (listener != null) {
            return listener.unregister();
        }
        return false;
    }

    /**
     * @hide
     */
    protected void requestRebind() {
        synchronized (this) {
            if (mPendingRebind) {
                return;
            }
            mPendingRebind = true;
        }
        if (USE_CHOREOGRAPHER) {
            mChoreographer.postFrameCallback(mFrameCallback);
        } else {
            mUIThreadHandler.post(mRebindRunnable);
        }

    }

    /**
     * @hide
     */
    protected Object getObservedField(int localFieldId) {
        WeakListener listener = mLocalFieldObservers[localFieldId];
        if (listener == null) {
            return null;
        }
        return listener.getTarget();
    }

    private boolean updateRegistration(int localFieldId, Object observable,
            CreateWeakListener listenerCreator) {
        if (observable == null) {
            return unregisterFrom(localFieldId);
        }
        WeakListener listener = mLocalFieldObservers[localFieldId];
        if (listener == null) {
            registerTo(localFieldId, observable, listenerCreator);
            return true;
        }
        if (listener.getTarget() == observable) {
            return false;//nothing to do, same object
        }
        unregisterFrom(localFieldId);
        registerTo(localFieldId, observable, listenerCreator);
        return true;
    }

    /**
     * @hide
     */
    protected boolean updateRegistration(int localFieldId, Observable observable) {
        return updateRegistration(localFieldId, observable, CREATE_PROPERTY_LISTENER);
    }

    /**
     * @hide
     */
    protected boolean updateRegistration(int localFieldId, ObservableList observable) {
        return updateRegistration(localFieldId, observable, CREATE_LIST_LISTENER);
    }

    /**
     * @hide
     */
    protected boolean updateRegistration(int localFieldId, ObservableMap observable) {
        return updateRegistration(localFieldId, observable, CREATE_MAP_LISTENER);
    }

    /**
     * @hide
     */
    protected void ensureBindingComponentIsNotNull(Class<?> oneExample) {
        if (mBindingComponent == null) {
            String errorMessage = "Required DataBindingComponent is null in class " +
                    getClass().getSimpleName() + ". A BindingAdapter in " +
                    oneExample.getCanonicalName() +
                    " is not static and requires an object to use, retrieved from the " +
                    "DataBindingComponent. If you don't use an inflation method taking a " +
                    "DataBindingComponent, use DataBindingUtil.setDefaultComponent or " +
                    "make all BindingAdapter methods static.";
            throw new IllegalStateException(errorMessage);
        }
    }

    /**
     * @hide
     */
    protected void registerTo(int localFieldId, Object observable,
            CreateWeakListener listenerCreator) {
        if (observable == null) {
            return;
        }
        WeakListener listener = mLocalFieldObservers[localFieldId];
        if (listener == null) {
            listener = listenerCreator.create(this, localFieldId);
            mLocalFieldObservers[localFieldId] = listener;
        }
        listener.setTarget(observable);
    }

    /**
     * @hide
     */
    protected static ViewDataBinding bind(DataBindingComponent bindingComponent, View view,
            int layoutId) {
        return DataBindingUtil.bind(bindingComponent, view, layoutId);
    }

    /**
     * Walks the view hierarchy under root and pulls out tagged Views, includes, and views with
     * IDs into an Object[] that is returned. This is used to walk the view hierarchy once to find
     * all bound and ID'd views.
     *
     * @param bindingComponent The binding component to use with this binding.
     * @param root The root of the view hierarchy to walk.
     * @param numBindings The total number of ID'd views, views with expressions, and includes
     * @param includes The include layout information, indexed by their container's index.
     * @param viewsWithIds Indexes of views that don't have tags, but have IDs.
     * @return An array of size numBindings containing all Views in the hierarchy that have IDs
     * (with elements in viewsWithIds), are tagged containing expressions, or the bindings for
     * included layouts.
     * @hide
     */
    protected static Object[] mapBindings(DataBindingComponent bindingComponent, View root,
            int numBindings, IncludedLayouts includes, SparseIntArray viewsWithIds) {
        Object[] bindings = new Object[numBindings];
        mapBindings(bindingComponent, root, bindings, includes, viewsWithIds, true);
        return bindings;
    }

    /** @hide */
    protected int getColorFromResource(int resourceId) {
        if (VERSION.SDK_INT >= VERSION_CODES.M) {
            return getRoot().getContext().getColor(resourceId);
        } else {
            return getRoot().getResources().getColor(resourceId);
        }
    }

    /** @hide */
    protected ColorStateList getColorStateListFromResource(int resourceId) {
        if (VERSION.SDK_INT >= VERSION_CODES.M) {
            return getRoot().getContext().getColorStateList(resourceId);
        } else {
            return getRoot().getResources().getColorStateList(resourceId);
        }
    }

    /** @hide */
    protected Drawable getDrawableFromResource(int resourceId) {
        if (VERSION.SDK_INT >= VERSION_CODES.LOLLIPOP) {
            return getRoot().getContext().getDrawable(resourceId);
        } else {
            return getRoot().getResources().getDrawable(resourceId);
        }
    }

    /** @hide */
    protected static <T> T getFromArray(T[] arr, int index) {
        if (arr == null || index < 0 || index >= arr.length) {
            return null;
        }
        return arr[index];
    }

    /** @hide */
    protected static <T> void setTo(T[] arr, int index, T value) {
        if (arr == null || index < 0 || index >= arr.length) {
            return;
        }
        arr[index] = value;
    }

    /** @hide */
    protected static boolean getFromArray(boolean[] arr, int index) {
        if (arr == null || index < 0 || index >= arr.length) {
            return false;
        }
        return arr[index];
    }

    /** @hide */
    protected static void setTo(boolean[] arr, int index, boolean value) {
        if (arr == null || index < 0 || index >= arr.length) {
            return;
        }
        arr[index] = value;
    }

    /** @hide */
    protected static byte getFromArray(byte[] arr, int index) {
        if (arr == null || index < 0 || index >= arr.length) {
            return 0;
        }
        return arr[index];
    }

    /** @hide */
    protected static void setTo(byte[] arr, int index, byte value) {
        if (arr == null || index < 0 || index >= arr.length) {
            return;
        }
        arr[index] = value;
    }

    /** @hide */
    protected static short getFromArray(short[] arr, int index) {
        if (arr == null || index < 0 || index >= arr.length) {
            return 0;
        }
        return arr[index];
    }

    /** @hide */
    protected static void setTo(short[] arr, int index, short value) {
        if (arr == null || index < 0 || index >= arr.length) {
            return;
        }
        arr[index] = value;
    }

    /** @hide */
    protected static char getFromArray(char[] arr, int index) {
        if (arr == null || index < 0 || index >= arr.length) {
            return 0;
        }
        return arr[index];
    }

    /** @hide */
    protected static void setTo(char[] arr, int index, char value) {
        if (arr == null || index < 0 || index >= arr.length) {
            return;
        }
        arr[index] = value;
    }

    /** @hide */
    protected static int getFromArray(int[] arr, int index) {
        if (arr == null || index < 0 || index >= arr.length) {
            return 0;
        }
        return arr[index];
    }

    /** @hide */
    protected static void setTo(int[] arr, int index, int value) {
        if (arr == null || index < 0 || index >= arr.length) {
            return;
        }
        arr[index] = value;
    }

    /** @hide */
    protected static long getFromArray(long[] arr, int index) {
        if (arr == null || index < 0 || index >= arr.length) {
            return 0;
        }
        return arr[index];
    }

    /** @hide */
    protected static void setTo(long[] arr, int index, long value) {
        if (arr == null || index < 0 || index >= arr.length) {
            return;
        }
        arr[index] = value;
    }

    /** @hide */
    protected static float getFromArray(float[] arr, int index) {
        if (arr == null || index < 0 || index >= arr.length) {
            return 0;
        }
        return arr[index];
    }

    /** @hide */
    protected static void setTo(float[] arr, int index, float value) {
        if (arr == null || index < 0 || index >= arr.length) {
            return;
        }
        arr[index] = value;
    }

    /** @hide */
    protected static double getFromArray(double[] arr, int index) {
        if (arr == null || index < 0 || index >= arr.length) {
            return 0;
        }
        return arr[index];
    }

    /** @hide */
    protected static void setTo(double[] arr, int index, double value) {
        if (arr == null || index < 0 || index >= arr.length) {
            return;
        }
        arr[index] = value;
    }

    /** @hide */
    protected static <T> T getFromList(List<T> list, int index) {
        if (list == null || index < 0 || index >= list.size()) {
            return null;
        }
        return list.get(index);
    }

    /** @hide */
    protected static <T> void setTo(List<T> list, int index, T value) {
        if (list == null || index < 0 || index >= list.size()) {
            return;
        }
        list.set(index, value);
    }

    /** @hide */
    protected static <T> T getFromList(SparseArray<T> list, int index) {
        if (list == null || index < 0) {
            return null;
        }
        return list.get(index);
    }

    /** @hide */
    protected static <T> void setTo(SparseArray<T> list, int index, T value) {
        if (list == null || index < 0 || index >= list.size()) {
            return;
        }
        list.put(index, value);
    }

    /** @hide */
    @TargetApi(VERSION_CODES.JELLY_BEAN)
    protected static <T> T getFromList(LongSparseArray<T> list, int index) {
        if (list == null || index < 0) {
            return null;
        }
        return list.get(index);
    }

    /** @hide */
    @TargetApi(VERSION_CODES.JELLY_BEAN)
    protected static <T> void setTo(LongSparseArray<T> list, int index, T value) {
        if (list == null || index < 0 || index >= list.size()) {
            return;
        }
        list.put(index, value);
    }

    /** @hide */
    protected static <T> T getFromList(android.support.v4.util.LongSparseArray<T> list, int index) {
        if (list == null || index < 0) {
            return null;
        }
        return list.get(index);
    }

    /** @hide */
    protected static <T> void setTo(android.support.v4.util.LongSparseArray<T> list, int index,
            T value) {
        if (list == null || index < 0 || index >= list.size()) {
            return;
        }
        list.put(index, value);
    }

    /** @hide */
    protected static boolean getFromList(SparseBooleanArray list, int index) {
        if (list == null || index < 0) {
            return false;
        }
        return list.get(index);
    }

    /** @hide */
    protected static void setTo(SparseBooleanArray list, int index, boolean value) {
        if (list == null || index < 0 || index >= list.size()) {
            return;
        }
        list.put(index, value);
    }

    /** @hide */
    protected static int getFromList(SparseIntArray list, int index) {
        if (list == null || index < 0) {
            return 0;
        }
        return list.get(index);
    }

    /** @hide */
    protected static void setTo(SparseIntArray list, int index, int value) {
        if (list == null || index < 0 || index >= list.size()) {
            return;
        }
        list.put(index, value);
    }

    /** @hide */
    @TargetApi(VERSION_CODES.JELLY_BEAN_MR2)
    protected static long getFromList(SparseLongArray list, int index) {
        if (list == null || index < 0) {
            return 0;
        }
        return list.get(index);
    }

    /** @hide */
    @TargetApi(VERSION_CODES.JELLY_BEAN_MR2)
    protected static void setTo(SparseLongArray list, int index, long value) {
        if (list == null || index < 0 || index >= list.size()) {
            return;
        }
        list.put(index, value);
    }

    /** @hide */
    protected static <K, T> T getFrom(Map<K, T> map, K key) {
        if (map == null) {
            return null;
        }
        return map.get(key);
    }

    /** @hide */
    protected static <K, T> void setTo(Map<K, T> map, K key, T value) {
        if (map == null) {
            return;
        }
        map.put(key, value);
    }

    /** @hide */
    protected static void setBindingInverseListener(ViewDataBinding binder,
            InverseBindingListener oldListener, PropertyChangedInverseListener listener) {
        if (oldListener != listener) {
            if (oldListener != null) {
                binder.removeOnPropertyChangedCallback(
                        (PropertyChangedInverseListener) oldListener);
            }
            if (listener != null) {
                binder.addOnPropertyChangedCallback(listener);
            }
        }
    }

    /**
     * Walks the view hierarchy under roots and pulls out tagged Views, includes, and views with
     * IDs into an Object[] that is returned. This is used to walk the view hierarchy once to find
     * all bound and ID'd views.
     *
     * @param bindingComponent The binding component to use with this binding.
     * @param roots The root Views of the view hierarchy to walk. This is used with merge tags.
     * @param numBindings The total number of ID'd views, views with expressions, and includes
     * @param includes The include layout information, indexed by their container's index.
     * @param viewsWithIds Indexes of views that don't have tags, but have IDs.
     * @return An array of size numBindings containing all Views in the hierarchy that have IDs
     * (with elements in viewsWithIds), are tagged containing expressions, or the bindings for
     * included layouts.
     * @hide
     */
    protected static Object[] mapBindings(DataBindingComponent bindingComponent, View[] roots,
            int numBindings, IncludedLayouts includes, SparseIntArray viewsWithIds) {
        Object[] bindings = new Object[numBindings];
        for (int i = 0; i < roots.length; i++) {
            mapBindings(bindingComponent, roots[i], bindings, includes, viewsWithIds, true);
        }
        return bindings;
    }

    private static void mapBindings(DataBindingComponent bindingComponent, View view,
            Object[] bindings, IncludedLayouts includes, SparseIntArray viewsWithIds,
            boolean isRoot) {
        final int indexInIncludes;
        final ViewDataBinding existingBinding = getBinding(view);
        if (existingBinding != null) {
            return;
        }
        final String tag = (String) view.getTag();
        boolean isBound = false;
        if (isRoot && tag != null && tag.startsWith("layout")) {
            final int underscoreIndex = tag.lastIndexOf('_');
            if (underscoreIndex > 0 && isNumeric(tag, underscoreIndex + 1)) {
                final int index = parseTagInt(tag, underscoreIndex + 1);
                if (bindings[index] == null) {
                    bindings[index] = view;
                }
                indexInIncludes = includes == null ? -1 : index;
                isBound = true;
            } else {
                indexInIncludes = -1;
            }
        } else if (tag != null && tag.startsWith(BINDING_TAG_PREFIX)) {
            int tagIndex = parseTagInt(tag, BINDING_NUMBER_START);
            if (bindings[tagIndex] == null) {
                bindings[tagIndex] = view;
            }
            isBound = true;
            indexInIncludes = includes == null ? -1 : tagIndex;
        } else {
            // Not a bound view
            indexInIncludes = -1;
        }
        if (!isBound) {
            final int id = view.getId();
            if (id > 0) {
                int index;
                if (viewsWithIds != null && (index = viewsWithIds.get(id, -1)) >= 0 &&
                        bindings[index] == null) {
                    bindings[index] = view;
                }
            }
        }

        if (view instanceof  ViewGroup) {
            final ViewGroup viewGroup = (ViewGroup) view;
            final int count = viewGroup.getChildCount();
            int minInclude = 0;
            for (int i = 0; i < count; i++) {
                final View child = viewGroup.getChildAt(i);
                boolean isInclude = false;
                if (indexInIncludes >= 0) {
                    String childTag = (String) child.getTag();
                    if (childTag != null && childTag.endsWith("_0") &&
                            childTag.startsWith("layout") && childTag.indexOf('/') > 0) {
                        // This *could* be an include. Test against the expected includes.
                        int includeIndex = findIncludeIndex(childTag, minInclude,
                                includes, indexInIncludes);
                        if (includeIndex >= 0) {
                            isInclude = true;
                            minInclude = includeIndex + 1;
                            final int index = includes.indexes[indexInIncludes][includeIndex];
                            final int layoutId = includes.layoutIds[indexInIncludes][includeIndex];
                            int lastMatchingIndex = findLastMatching(viewGroup, i);
                            if (lastMatchingIndex == i) {
                                bindings[index] = DataBindingUtil.bind(bindingComponent, child,
                                        layoutId);
                            } else {
                                final int includeCount =  lastMatchingIndex - i + 1;
                                final View[] included = new View[includeCount];
                                for (int j = 0; j < includeCount; j++) {
                                    included[j] = viewGroup.getChildAt(i + j);
                                }
                                bindings[index] = DataBindingUtil.bind(bindingComponent, included,
                                        layoutId);
                                i += includeCount - 1;
                            }
                        }
                    }
                }
                if (!isInclude) {
                    mapBindings(bindingComponent, child, bindings, includes, viewsWithIds, false);
                }
            }
        }
    }

    private static int findIncludeIndex(String tag, int minInclude,
            IncludedLayouts included, int includedIndex) {
        final int slashIndex = tag.indexOf('/');
        final CharSequence layoutName = tag.subSequence(slashIndex + 1, tag.length() - 2);

        final String[] layouts = included.layouts[includedIndex];
        final int length = layouts.length;
        for (int i = minInclude; i < length; i++) {
            final String layout = layouts[i];
            if (TextUtils.equals(layoutName, layout)) {
                return i;
            }
        }
        return -1;
    }

    private static int findLastMatching(ViewGroup viewGroup, int firstIncludedIndex) {
        final View firstView = viewGroup.getChildAt(firstIncludedIndex);
        final String firstViewTag = (String) firstView.getTag();
        final String tagBase = firstViewTag.substring(0, firstViewTag.length() - 1); // don't include the "0"
        final int tagSequenceIndex = tagBase.length();

        final int count = viewGroup.getChildCount();
        int max = firstIncludedIndex;
        for (int i = firstIncludedIndex + 1; i < count; i++) {
            final View view = viewGroup.getChildAt(i);
            final String tag = (String) view.getTag();
            if (tag != null && tag.startsWith(tagBase)) {
                if (tag.length() == firstViewTag.length() && tag.charAt(tag.length() - 1) == '0') {
                    return max; // Found another instance of the include
                }
                if (isNumeric(tag, tagSequenceIndex)) {
                    max = i;
                }
            }
        }
        return max;
    }

    private static boolean isNumeric(String tag, int startIndex) {
        int length = tag.length();
        if (length == startIndex) {
            return false; // no numerals
        }
        for (int i = startIndex; i < length; i++) {
            if (!Character.isDigit(tag.charAt(i))) {
                return false;
            }
        }
        return true;
    }

    /**
     * Parse the tag without creating a new String object. This is fast and assumes the
     * tag is in the correct format.
     * @param str The tag string.
     * @return The binding tag number parsed from the tag string.
     */
    private static int parseTagInt(String str, int startIndex) {
        final int end = str.length();
        int val = 0;
        for (int i = startIndex; i < end; i++) {
            val *= 10;
            char c = str.charAt(i);
            val += (c - '0');
        }
        return val;
    }

    private interface ObservableReference<T> {
        WeakListener<T> getListener();
        void addListener(T target);
        void removeListener(T target);
    }

    private static class WeakListener<T> extends WeakReference<ViewDataBinding> {
        private final ObservableReference<T> mObservable;
        protected final int mLocalFieldId;
        private T mTarget;

        public WeakListener(ViewDataBinding binder, int localFieldId,
                ObservableReference<T> observable) {
            super(binder);
            mLocalFieldId = localFieldId;
            mObservable = observable;
        }

        public void setTarget(T object) {
            unregister();
            mTarget = object;
            if (mTarget != null) {
                mObservable.addListener(mTarget);
            }
        }

        public boolean unregister() {
            boolean unregistered = false;
            if (mTarget != null) {
                mObservable.removeListener(mTarget);
                unregistered = true;
            }
            mTarget = null;
            return unregistered;
        }

        public T getTarget() {
            return mTarget;
        }

        protected ViewDataBinding getBinder() {
            ViewDataBinding binder = get();
            if (binder == null) {
                unregister(); // The binder is dead
            }
            return binder;
        }
    }

    private static class WeakPropertyListener extends Observable.OnPropertyChangedCallback
            implements ObservableReference<Observable> {
        final WeakListener<Observable> mListener;

        public WeakPropertyListener(ViewDataBinding binder, int localFieldId) {
            mListener = new WeakListener<Observable>(binder, localFieldId, this);
        }

        @Override
        public WeakListener<Observable> getListener() {
            return mListener;
        }

        @Override
        public void addListener(Observable target) {
            target.addOnPropertyChangedCallback(this);
        }

        @Override
        public void removeListener(Observable target) {
            target.removeOnPropertyChangedCallback(this);
        }

        @Override
        public void onPropertyChanged(Observable sender, int propertyId) {
            ViewDataBinding binder = mListener.getBinder();
            if (binder == null) {
                return;
            }
            Observable obj = mListener.getTarget();
            if (obj != sender) {
                return; // notification from the wrong object?
            }
            binder.handleFieldChange(mListener.mLocalFieldId, sender, propertyId);
        }
    }

    private static class WeakListListener extends ObservableList.OnListChangedCallback
            implements ObservableReference<ObservableList> {
        final WeakListener<ObservableList> mListener;

        public WeakListListener(ViewDataBinding binder, int localFieldId) {
            mListener = new WeakListener<ObservableList>(binder, localFieldId, this);
        }

        @Override
        public WeakListener<ObservableList> getListener() {
            return mListener;
        }

        @Override
        public void addListener(ObservableList target) {
            target.addOnListChangedCallback(this);
        }

        @Override
        public void removeListener(ObservableList target) {
            target.removeOnListChangedCallback(this);
        }

        @Override
        public void onChanged(ObservableList sender) {
            ViewDataBinding binder = mListener.getBinder();
            if (binder == null) {
                return;
            }
            ObservableList target = mListener.getTarget();
            if (target != sender) {
                return; // We expect notifications only from sender
            }
            binder.handleFieldChange(mListener.mLocalFieldId, target, 0);
        }

        @Override
        public void onItemRangeChanged(ObservableList sender, int positionStart, int itemCount) {
            onChanged(sender);
        }

        @Override
        public void onItemRangeInserted(ObservableList sender, int positionStart, int itemCount) {
            onChanged(sender);
        }

        @Override
        public void onItemRangeMoved(ObservableList sender, int fromPosition, int toPosition,
                int itemCount) {
            onChanged(sender);
        }

        @Override
        public void onItemRangeRemoved(ObservableList sender, int positionStart, int itemCount) {
            onChanged(sender);
        }
    }

    private static class WeakMapListener extends ObservableMap.OnMapChangedCallback
            implements ObservableReference<ObservableMap> {
        final WeakListener<ObservableMap> mListener;

        public WeakMapListener(ViewDataBinding binder, int localFieldId) {
            mListener = new WeakListener<ObservableMap>(binder, localFieldId, this);
        }

        @Override
        public WeakListener<ObservableMap> getListener() {
            return mListener;
        }

        @Override
        public void addListener(ObservableMap target) {
            target.addOnMapChangedCallback(this);
        }

        @Override
        public void removeListener(ObservableMap target) {
            target.removeOnMapChangedCallback(this);
        }

        @Override
        public void onMapChanged(ObservableMap sender, Object key) {
            ViewDataBinding binder = mListener.getBinder();
            if (binder == null || sender != mListener.getTarget()) {
                return;
            }
            binder.handleFieldChange(mListener.mLocalFieldId, sender, 0);
        }
    }

    private interface CreateWeakListener {
        WeakListener create(ViewDataBinding viewDataBinding, int localFieldId);
    }

    /**
     * This class is used by generated subclasses of {@link ViewDataBinding} to track the
     * included layouts contained in the bound layout. This class is an implementation
     * detail of how binding expressions are mapped to Views after inflation.
     * @hide
     */
    protected static class IncludedLayouts {
        public final String[][] layouts;
        public final int[][] indexes;
        public final int[][] layoutIds;

        public IncludedLayouts(int bindingCount) {
            layouts = new String[bindingCount][];
            indexes = new int[bindingCount][];
            layoutIds = new int[bindingCount][];
        }

        public void setIncludes(int index, String[] layouts, int[] indexes, int[] layoutIds) {
            this.layouts[index] = layouts;
            this.indexes[index] = indexes;
            this.layoutIds[index] = layoutIds;
        }
    }

    /**
     * This class is used by generated subclasses of {@link ViewDataBinding} to listen for
     * changes on variables of Bindings. This is important for two-way data binding on variables
     * in included Bindings.
     * @hide
     */
    protected static abstract class PropertyChangedInverseListener
            extends Observable.OnPropertyChangedCallback implements InverseBindingListener {
        final int mPropertyId;

        public PropertyChangedInverseListener(int propertyId) {
            mPropertyId = propertyId;
        }

        @Override
        public void onPropertyChanged(Observable sender, int propertyId) {
            if (propertyId == mPropertyId || propertyId == 0) {
                onChange();
            }
        }
    }
}
