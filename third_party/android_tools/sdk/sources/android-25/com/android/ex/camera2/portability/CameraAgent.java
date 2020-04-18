/*
 * Copyright (C) 2012 The Android Open Source Project
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

package com.android.ex.camera2.portability;

import android.annotation.TargetApi;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.hardware.Camera.OnZoomChangeListener;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.view.SurfaceHolder;

import com.android.ex.camera2.portability.debug.Log;

/**
 * An interface which provides possible camera device operations.
 *
 * The client should call {@code CameraAgent.openCamera} to get an instance
 * of {@link CameraAgent.CameraProxy} to control the camera. Classes
 * implementing this interface should have its own one unique {@code Thread}
 * other than the main thread for camera operations. Camera device callbacks
 * are wrapped since the client should not deal with
 * {@code android.hardware.Camera} directly.
 *
 * TODO: provide callback interfaces for:
 * {@code android.hardware.Camera.ErrorCallback},
 * {@code android.hardware.Camera.OnZoomChangeListener}, and
 */
public abstract class CameraAgent {
    public static final long CAMERA_OPERATION_TIMEOUT_MS = 3500;

    private static final Log.Tag TAG = new Log.Tag("CamAgnt");

    public static class CameraStartPreviewCallbackForward
            implements CameraStartPreviewCallback {
        private final Handler mHandler;
        private final CameraStartPreviewCallback mCallback;

        public static CameraStartPreviewCallbackForward getNewInstance(
                Handler handler, CameraStartPreviewCallback cb) {
            if (handler == null || cb == null) {
                return null;
            }
            return new CameraStartPreviewCallbackForward(handler, cb);
        }

        private CameraStartPreviewCallbackForward(Handler h,
                CameraStartPreviewCallback cb) {
            mHandler = h;
            mCallback = cb;
        }

        @Override
        public void onPreviewStarted() {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    mCallback.onPreviewStarted();
                }});
        }
    }

    /**
     * A callback helps to invoke the original callback on another
     * {@link android.os.Handler}.
     */
    public static class CameraOpenCallbackForward implements CameraOpenCallback {
        private final Handler mHandler;
        private final CameraOpenCallback mCallback;

        /**
         * Returns a new instance of {@link FaceDetectionCallbackForward}.
         *
         * @param handler The handler in which the callback will be invoked in.
         * @param cb The callback to be invoked.
         * @return The instance of the {@link FaceDetectionCallbackForward}, or
         *         null if any parameter is null.
         */
        public static CameraOpenCallbackForward getNewInstance(
                Handler handler, CameraOpenCallback cb) {
            if (handler == null || cb == null) {
                return null;
            }
            return new CameraOpenCallbackForward(handler, cb);
        }

        private CameraOpenCallbackForward(Handler h, CameraOpenCallback cb) {
            // Given that we are using the main thread handler, we can create it
            // here instead of holding onto the PhotoModule objects. In this
            // way, we can avoid memory leak.
            mHandler = new Handler(Looper.getMainLooper());
            mCallback = cb;
        }

        @Override
        public void onCameraOpened(final CameraProxy camera) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    mCallback.onCameraOpened(camera);
                }});
        }

        @Override
        public void onCameraDisabled(final int cameraId) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    mCallback.onCameraDisabled(cameraId);
                }});
        }

        @Override
        public void onDeviceOpenFailure(final int cameraId, final String info) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    mCallback.onDeviceOpenFailure(cameraId, info);
                }});
        }

        @Override
        public void onDeviceOpenedAlready(final int cameraId, final String info) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    mCallback.onDeviceOpenedAlready(cameraId, info);
                }});
        }

        @Override
        public void onReconnectionFailure(final CameraAgent mgr, final String info) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    mCallback.onReconnectionFailure(mgr, info);
                }});
        }
    }

    /**
     * An interface which wraps
     * {@link android.hardware.Camera.ErrorCallback}
     */
    public static interface CameraErrorCallback {
        public void onError(int error, CameraProxy camera);
    }

    /**
     * An interface which wraps
     * {@link android.hardware.Camera.AutoFocusCallback}.
     */
    public static interface CameraAFCallback {
        public void onAutoFocus(boolean focused, CameraProxy camera);
    }

    /**
     * An interface which wraps
     * {@link android.hardware.Camera.AutoFocusMoveCallback}.
     */
    public static interface CameraAFMoveCallback {
        public void onAutoFocusMoving(boolean moving, CameraProxy camera);
    }

    /**
     * An interface which wraps
     * {@link android.hardware.Camera.ShutterCallback}.
     */
    public static interface CameraShutterCallback {
        public void onShutter(CameraProxy camera);
    }

    /**
     * An interface which wraps
     * {@link android.hardware.Camera.PictureCallback}.
     */
    public static interface CameraPictureCallback {
        public void onPictureTaken(byte[] data, CameraProxy camera);
    }

    /**
     * An interface which wraps
     * {@link android.hardware.Camera.PreviewCallback}.
     */
    public static interface CameraPreviewDataCallback {
        public void onPreviewFrame(byte[] data, CameraProxy camera);
    }

    /**
     * An interface which wraps
     * {@link android.hardware.Camera.FaceDetectionListener}.
     */
    public static interface CameraFaceDetectionCallback {
        /**
         * Callback for face detection.
         *
         * @param faces   Recognized face in the preview.
         * @param camera  The camera which the preview image comes from.
         */
        public void onFaceDetection(Camera.Face[] faces, CameraProxy camera);
    }

    /**
     * An interface to be called when the camera preview has started.
     */
    public static interface CameraStartPreviewCallback {
        /**
         * Callback when the preview starts.
         */
        public void onPreviewStarted();
    }

    /**
     * An interface to be called for any events when opening or closing the
     * camera device. This error callback is different from the one defined
     * in the framework, {@link android.hardware.Camera.ErrorCallback}, which
     * is used after the camera is opened.
     */
    public static interface CameraOpenCallback {
        /**
         * Callback when camera open succeeds.
         */
        public void onCameraOpened(CameraProxy camera);

        /**
         * Callback when {@link com.android.camera.CameraDisabledException} is
         * caught.
         *
         * @param cameraId The disabled camera.
         */
        public void onCameraDisabled(int cameraId);

        /**
         * Callback when {@link com.android.camera.CameraHardwareException} is
         * caught.
         *
         * @param cameraId The camera with the hardware failure.
         * @param info The extra info regarding this failure.
         */
        public void onDeviceOpenFailure(int cameraId, String info);

        /**
         * Callback when trying to open the camera which is already opened.
         *
         * @param cameraId The camera which is causing the open error.
         */
        public void onDeviceOpenedAlready(int cameraId, String info);

        /**
         * Callback when {@link java.io.IOException} is caught during
         * {@link android.hardware.Camera#reconnect()}.
         *
         * @param mgr The {@link CameraAgent}
         *            with the reconnect failure.
         */
        public void onReconnectionFailure(CameraAgent mgr, String info);
    }

    /**
     * Opens the camera of the specified ID asynchronously. The camera device
     * will be opened in the camera handler thread and will be returned through
     * the {@link CameraAgent.CameraOpenCallback#
     * onCameraOpened(com.android.camera.cameradevice.CameraAgent.CameraProxy)}.
     *
     * @param handler The {@link android.os.Handler} in which the callback
     *                was handled.
     * @param callback The callback for the result.
     * @param cameraId The camera ID to open.
     */
    public void openCamera(final Handler handler, final int cameraId,
                           final CameraOpenCallback callback) {
        try {
            getDispatchThread().runJob(new Runnable() {
                @Override
                public void run() {
                    getCameraHandler().obtainMessage(CameraActions.OPEN_CAMERA, cameraId, 0,
                            CameraOpenCallbackForward.getNewInstance(handler, callback)).sendToTarget();
                }
            });
        } catch (final RuntimeException ex) {
            getCameraExceptionHandler().onDispatchThreadException(ex);
        }
    }

    /**
     * Closes the camera device.
     *
     * @param camera The camera to close. {@code null} means all.
     * @param synced Whether this call should be synchronous.
     */
    public void closeCamera(CameraProxy camera, boolean synced) {
        try {
            if (synced) {
                // Don't bother to wait since camera is in bad state.
                if (getCameraState().isInvalid()) {
                    return;
                }
                final WaitDoneBundle bundle = new WaitDoneBundle();

                getDispatchThread().runJobSync(new Runnable() {
                    @Override
                    public void run() {
                        getCameraHandler().obtainMessage(CameraActions.RELEASE).sendToTarget();
                        getCameraHandler().post(bundle.mUnlockRunnable);
                    }}, bundle.mWaitLock, CAMERA_OPERATION_TIMEOUT_MS, "camera release");
            } else {
                getDispatchThread().runJob(new Runnable() {
                    @Override
                    public void run() {
                        getCameraHandler().removeCallbacksAndMessages(null);
                        getCameraHandler().obtainMessage(CameraActions.RELEASE).sendToTarget();
                    }});
            }
        } catch (final RuntimeException ex) {
            getCameraExceptionHandler().onDispatchThreadException(ex);
        }
    }

    /**
     * Sets a callback for handling camera api runtime exceptions on
     * a handler.
     */
    public abstract void setCameraExceptionHandler(CameraExceptionHandler exceptionHandler);

    /**
     * Recycles the resources used by this instance. CameraAgent will be in
     * an unusable state after calling this.
     */
    public abstract void recycle();

    /**
     * @return The camera devices info.
     */
    public abstract CameraDeviceInfo getCameraDeviceInfo();

    /**
     * @return The handler to which camera tasks should be posted.
     */
    protected abstract Handler getCameraHandler();

    /**
     * @return The thread used on which client callbacks are served.
     */
    protected abstract DispatchThread getDispatchThread();

    /**
     * @return The state machine tracking the camera API's current status.
     */
    protected abstract CameraStateHolder getCameraState();

    /**
     * @return The exception handler.
     */
    protected abstract CameraExceptionHandler getCameraExceptionHandler();

    /**
     * An interface that takes camera operation requests and post messages to the
     * camera handler thread. All camera operations made through this interface is
     * asynchronous by default except those mentioned specifically.
     */
    public abstract static class CameraProxy {

        /**
         * Returns the underlying {@link android.hardware.Camera} object used
         * by this proxy. This method should only be used when handing the
         * camera device over to {@link android.media.MediaRecorder} for
         * recording.
         */
        @Deprecated
        public abstract android.hardware.Camera getCamera();

        /**
         * @return The camera ID associated to by this
         * {@link CameraAgent.CameraProxy}.
         */
        public abstract int getCameraId();

        /**
         * @return The camera characteristics.
         */
        public abstract CameraDeviceInfo.Characteristics getCharacteristics();

        /**
         * @return The camera capabilities.
         */
        public abstract CameraCapabilities getCapabilities();

        /**
         * @return The camera agent which creates this proxy.
         */
        public abstract CameraAgent getAgent();

        /**
         * Reconnects to the camera device. On success, the camera device will
         * be returned through {@link CameraAgent
         * .CameraOpenCallback#onCameraOpened(com.android.camera.cameradevice.CameraAgent
         * .CameraProxy)}.
         * @see android.hardware.Camera#reconnect()
         *
         * @param handler The {@link android.os.Handler} in which the callback
         *                was handled.
         * @param cb The callback when any error happens.
         */
        public void reconnect(final Handler handler, final CameraOpenCallback cb) {
            try {
                getDispatchThread().runJob(new Runnable() {
                    @Override
                    public void run() {
                        getCameraHandler().obtainMessage(CameraActions.RECONNECT, getCameraId(), 0,
                                CameraOpenCallbackForward.getNewInstance(handler, cb)).sendToTarget();
                    }});
            } catch (final RuntimeException ex) {
                getAgent().getCameraExceptionHandler().onDispatchThreadException(ex);
            }
        }

        /**
         * Unlocks the camera device.
         *
         * @see android.hardware.Camera#unlock()
         */
        public void unlock() {
            // Don't bother to wait since camera is in bad state.
            if (getCameraState().isInvalid()) {
                return;
            }
            final WaitDoneBundle bundle = new WaitDoneBundle();
            try {
                getDispatchThread().runJobSync(new Runnable() {
                    @Override
                    public void run() {
                        getCameraHandler().sendEmptyMessage(CameraActions.UNLOCK);
                        getCameraHandler().post(bundle.mUnlockRunnable);
                    }
                }, bundle.mWaitLock, CAMERA_OPERATION_TIMEOUT_MS, "camera unlock");
            } catch (final RuntimeException ex) {
                getAgent().getCameraExceptionHandler().onDispatchThreadException(ex);
            }
        }

        /**
         * Locks the camera device.
         * @see android.hardware.Camera#lock()
         */
        public void lock() {
            try {
                getDispatchThread().runJob(new Runnable() {
                    @Override
                    public void run() {
                        getCameraHandler().sendEmptyMessage(CameraActions.LOCK);
                    }});
            } catch (final RuntimeException ex) {
                getAgent().getCameraExceptionHandler().onDispatchThreadException(ex);
            }
        }

        /**
         * Sets the {@link android.graphics.SurfaceTexture} for preview.
         *
         * <p>Note that, once this operation has been performed, it is no longer
         * possible to change the preview or photo sizes in the
         * {@link CameraSettings} instance for this camera, and the mutators for
         * these fields are allowed to ignore all further invocations until the
         * preview is stopped with {@link #stopPreview}.</p>
         *
         * @param surfaceTexture The {@link SurfaceTexture} for preview.
         *
         * @see CameraSettings#setPhotoSize
         * @see CameraSettings#setPreviewSize
         */
        // XXX: Despite the above documentation about locking the sizes, the API
        // 1 implementation doesn't currently enforce this at all, although the
        // Camera class warns that preview sizes shouldn't be changed while a
        // preview is running. Furthermore, the API 2 implementation doesn't yet
        // unlock the sizes when stopPreview() is invoked (see related FIXME on
        // the STOP_PREVIEW case in its handler; in the meantime, changing API 2
        // sizes would require closing and reopening the camera.
        public void setPreviewTexture(final SurfaceTexture surfaceTexture) {
            try {
                getDispatchThread().runJob(new Runnable() {
                    @Override
                    public void run() {
                        getCameraHandler()
                                .obtainMessage(CameraActions.SET_PREVIEW_TEXTURE_ASYNC, surfaceTexture)
                                .sendToTarget();
                    }});
            } catch (final RuntimeException ex) {
                getAgent().getCameraExceptionHandler().onDispatchThreadException(ex);
            }
        }

        /**
         * Blocks until a {@link android.graphics.SurfaceTexture} has been set
         * for preview.
         *
         * <p>Note that, once this operation has been performed, it is no longer
         * possible to change the preview or photo sizes in the
         * {@link CameraSettings} instance for this camera, and the mutators for
         * these fields are allowed to ignore all further invocations.</p>
         *
         * @param surfaceTexture The {@link SurfaceTexture} for preview.
         *
         * @see CameraSettings#setPhotoSize
         * @see CameraSettings#setPreviewSize
         */
        public void setPreviewTextureSync(final SurfaceTexture surfaceTexture) {
            // Don't bother to wait since camera is in bad state.
            if (getCameraState().isInvalid()) {
                return;
            }
            final WaitDoneBundle bundle = new WaitDoneBundle();
            try {
                getDispatchThread().runJobSync(new Runnable() {
                    @Override
                    public void run() {
                        getCameraHandler()
                                .obtainMessage(CameraActions.SET_PREVIEW_TEXTURE_ASYNC, surfaceTexture)
                                .sendToTarget();
                        getCameraHandler().post(bundle.mUnlockRunnable);
                    }}, bundle.mWaitLock, CAMERA_OPERATION_TIMEOUT_MS, "set preview texture");
            } catch (final RuntimeException ex) {
                getAgent().getCameraExceptionHandler().onDispatchThreadException(ex);
            }
        }

        /**
         * Sets the {@link android.view.SurfaceHolder} for preview.
         *
         * @param surfaceHolder The {@link SurfaceHolder} for preview.
         */
        public void setPreviewDisplay(final SurfaceHolder surfaceHolder) {
            try {
                getDispatchThread().runJob(new Runnable() {
                    @Override
                    public void run() {
                        getCameraHandler()
                                .obtainMessage(CameraActions.SET_PREVIEW_DISPLAY_ASYNC, surfaceHolder)
                                .sendToTarget();
                    }});
            } catch (final RuntimeException ex) {
                getAgent().getCameraExceptionHandler().onDispatchThreadException(ex);
            }
        }

        /**
         * Starts the camera preview.
         */
        public void startPreview() {
            try {
            getDispatchThread().runJob(new Runnable() {
                @Override
                public void run() {
                    getCameraHandler()
                            .obtainMessage(CameraActions.START_PREVIEW_ASYNC, null).sendToTarget();
                }});
            } catch (final RuntimeException ex) {
                getAgent().getCameraExceptionHandler().onDispatchThreadException(ex);
            }
        }

        /**
         * Starts the camera preview and executes a callback on a handler once
         * the preview starts.
         */
        public void startPreviewWithCallback(final Handler h, final CameraStartPreviewCallback cb) {
            try {
            getDispatchThread().runJob(new Runnable() {
                @Override
                public void run() {
                    getCameraHandler().obtainMessage(CameraActions.START_PREVIEW_ASYNC,
                            CameraStartPreviewCallbackForward.getNewInstance(h, cb))
                                    .sendToTarget();
                }});
            } catch (final RuntimeException ex) {
                getAgent().getCameraExceptionHandler().onDispatchThreadException(ex);
            }
        }

        /**
         * Stops the camera preview synchronously.
         * {@code stopPreview()} must be synchronous to ensure that the caller can
         * continues to release resources related to camera preview.
         */
        public void stopPreview() {
            // Don't bother to wait since camera is in bad state.
            if (getCameraState().isInvalid()) {
                return;
            }
            final WaitDoneBundle bundle = new WaitDoneBundle();
            try {
                getDispatchThread().runJobSync(new Runnable() {
                    @Override
                    public void run() {
                        getCameraHandler().obtainMessage(CameraActions.STOP_PREVIEW, bundle)
                                .sendToTarget();
                    }}, bundle.mWaitLock, CAMERA_OPERATION_TIMEOUT_MS, "stop preview");
            } catch (final RuntimeException ex) {
                getAgent().getCameraExceptionHandler().onDispatchThreadException(ex);
            }
        }

        /**
         * Sets the callback for preview data.
         *
         * @param handler    The {@link android.os.Handler} in which the callback was handled.
         * @param cb         The callback to be invoked when the preview data is available.
         * @see  android.hardware.Camera#setPreviewCallback(android.hardware.Camera.PreviewCallback)
         */
        public abstract void setPreviewDataCallback(Handler handler, CameraPreviewDataCallback cb);

        /**
         * Sets the one-time callback for preview data.
         *
         * @param handler    The {@link android.os.Handler} in which the callback was handled.
         * @param cb         The callback to be invoked when the preview data for
         *                   next frame is available.
         * @see  android.hardware.Camera#setPreviewCallback(android.hardware.Camera.PreviewCallback)
         */
        public abstract void setOneShotPreviewCallback(Handler handler,
                                                       CameraPreviewDataCallback cb);

        /**
         * Sets the callback for preview data.
         *
         * @param handler The handler in which the callback will be invoked.
         * @param cb      The callback to be invoked when the preview data is available.
         * @see android.hardware.Camera#setPreviewCallbackWithBuffer(android.hardware.Camera.PreviewCallback)
         */
        public abstract void setPreviewDataCallbackWithBuffer(Handler handler,
                                                              CameraPreviewDataCallback cb);

        /**
         * Adds buffer for the preview callback.
         *
         * @param callbackBuffer The buffer allocated for the preview data.
         */
        public void addCallbackBuffer(final byte[] callbackBuffer) {
            try {
                getDispatchThread().runJob(new Runnable() {
                    @Override
                    public void run() {
                        getCameraHandler()
                                .obtainMessage(CameraActions.ADD_CALLBACK_BUFFER, callbackBuffer)
                                .sendToTarget();
                        }
                    });
            } catch (final RuntimeException ex) {
                getAgent().getCameraExceptionHandler().onDispatchThreadException(ex);
            }
        }

        /**
         * Starts the auto-focus process. The result will be returned through the callback.
         *
         * @param handler The handler in which the callback will be invoked.
         * @param cb      The auto-focus callback.
         */
        public abstract void autoFocus(Handler handler, CameraAFCallback cb);

        /**
         * Cancels the auto-focus process.
         *
         * <p>This action has the highest priority and will get processed before anything
         * else that is pending. Moreover, any pending auto-focuses that haven't yet
         * began will also be ignored.</p>
         */
        public void cancelAutoFocus() {
             // Do not use the dispatch thread since we want to avoid a wait-cycle
             // between applySettingsHelper which waits until the state is not FOCUSING.
             // cancelAutoFocus should get executed asap, set the state back to idle.
            getCameraHandler().sendMessageAtFrontOfQueue(
                    getCameraHandler().obtainMessage(CameraActions.CANCEL_AUTO_FOCUS));
            getCameraHandler().sendEmptyMessage(CameraActions.CANCEL_AUTO_FOCUS_FINISH);
        }

        /**
         * Sets the auto-focus callback
         *
         * @param handler The handler in which the callback will be invoked.
         * @param cb      The callback to be invoked when the preview data is available.
         */
        @TargetApi(Build.VERSION_CODES.JELLY_BEAN)
        public abstract void setAutoFocusMoveCallback(Handler handler, CameraAFMoveCallback cb);

        /**
         * Instrument the camera to take a picture.
         *
         * @param handler   The handler in which the callback will be invoked.
         * @param shutter   The callback for shutter action, may be null.
         * @param raw       The callback for uncompressed data, may be null.
         * @param postview  The callback for postview image data, may be null.
         * @param jpeg      The callback for jpeg image data, may be null.
         * @see android.hardware.Camera#takePicture(
         *         android.hardware.Camera.ShutterCallback,
         *         android.hardware.Camera.PictureCallback,
         *         android.hardware.Camera.PictureCallback)
         */
        public abstract void takePicture(
                Handler handler,
                CameraShutterCallback shutter,
                CameraPictureCallback raw,
                CameraPictureCallback postview,
                CameraPictureCallback jpeg);

        /**
         * Sets the display orientation for camera to adjust the preview and JPEG orientation.
         *
         * @param degrees The counterclockwise rotation in degrees, relative to the device's natural
         *                orientation. Should be 0, 90, 180 or 270.
         */
        public void setDisplayOrientation(final int degrees) {
            setDisplayOrientation(degrees, true);
        }

        /**
         * Sets the display orientation for camera to adjust the preview&mdash;and, optionally,
         * JPEG&mdash;orientations.
         * <p>If capture rotation is not requested, future captures will be returned in the sensor's
         * physical rotation, which does not necessarily match the device's natural orientation.</p>
         *
         * @param degrees The counterclockwise rotation in degrees, relative to the device's natural
         *                orientation. Should be 0, 90, 180 or 270.
         * @param capture Whether to adjust the JPEG capture orientation as well as the preview one.
         */
        public void setDisplayOrientation(final int degrees, final boolean capture) {
            try {
                getDispatchThread().runJob(new Runnable() {
                    @Override
                    public void run() {
                        getCameraHandler()
                                .obtainMessage(CameraActions.SET_DISPLAY_ORIENTATION, degrees,
                                        capture ? 1 : 0)
                                .sendToTarget();
                    }});
            } catch (final RuntimeException ex) {
                getAgent().getCameraExceptionHandler().onDispatchThreadException(ex);
            }
        }

        public void setJpegOrientation(final int degrees) {
            try {
                getDispatchThread().runJob(new Runnable() {
                    @Override
                    public void run() {
                        getCameraHandler()
                                .obtainMessage(CameraActions.SET_JPEG_ORIENTATION, degrees, 0)
                                .sendToTarget();
                    }});
            } catch (final RuntimeException ex) {
                getAgent().getCameraExceptionHandler().onDispatchThreadException(ex);
            }
        }

        /**
         * Sets the listener for zoom change.
         *
         * @param listener The listener.
         */
        public abstract void setZoomChangeListener(OnZoomChangeListener listener);

        /**
         * Sets the face detection listener.
         *
         * @param handler  The handler in which the callback will be invoked.
         * @param callback The callback for face detection results.
         */
        public abstract void setFaceDetectionCallback(Handler handler,
                                                      CameraFaceDetectionCallback callback);

        /**
         * Starts the face detection.
         */
        public void startFaceDetection() {
            try {
                getDispatchThread().runJob(new Runnable() {
                    @Override
                    public void run() {
                        getCameraHandler().sendEmptyMessage(CameraActions.START_FACE_DETECTION);
                    }});
            } catch (final RuntimeException ex) {
                getAgent().getCameraExceptionHandler().onDispatchThreadException(ex);
            }
        }

        /**
         * Stops the face detection.
         */
        public void stopFaceDetection() {
            try {
                getDispatchThread().runJob(new Runnable() {
                    @Override
                    public void run() {
                        getCameraHandler().sendEmptyMessage(CameraActions.STOP_FACE_DETECTION);
                    }});
            } catch (final RuntimeException ex) {
                getAgent().getCameraExceptionHandler().onDispatchThreadException(ex);
            }
        }

        /**
         * Sets the camera parameters.
         *
         * @param params The camera parameters to use.
         */
        @Deprecated
        public abstract void setParameters(Camera.Parameters params);

        /**
         * Gets the current camera parameters synchronously. This method is
         * synchronous since the caller has to wait for the camera to return
         * the parameters. If the parameters are already cached, it returns
         * immediately.
         */
        @Deprecated
        public abstract Camera.Parameters getParameters();

        /**
         * Gets the current camera settings synchronously.
         * <p>This method is synchronous since the caller has to wait for the
         * camera to return the parameters. If the parameters are already
         * cached, it returns immediately.</p>
         */
        public abstract CameraSettings getSettings();

        /**
         * Default implementation of {@link #applySettings(CameraSettings)}
         * that is only missing the set of states it needs to wait for
         * before applying the settings.
         *
         * @param settings The settings to use on the device.
         * @param statesToAwait Bitwise OR of the required camera states.
         * @return Whether the settings can be applied.
         */
        protected boolean applySettingsHelper(CameraSettings settings,
                                              final int statesToAwait) {
            if (settings == null) {
                Log.v(TAG, "null argument in applySettings()");
                return false;
            }
            if (!getCapabilities().supports(settings)) {
                Log.w(TAG, "Unsupported settings in applySettings()");
                return false;
            }

            final CameraSettings copyOfSettings = settings.copy();
            try {
                getDispatchThread().runJob(new Runnable() {
                    @Override
                    public void run() {
                        CameraStateHolder cameraState = getCameraState();
                        // Don't bother to wait since camera is in bad state.
                        if (cameraState.isInvalid()) {
                            return;
                        }
                        cameraState.waitForStates(statesToAwait);
                        getCameraHandler().obtainMessage(CameraActions.APPLY_SETTINGS, copyOfSettings)
                                .sendToTarget();
                    }});
            } catch (final RuntimeException ex) {
                getAgent().getCameraExceptionHandler().onDispatchThreadException(ex);
            }
            return true;
        }

        /**
         * Applies the settings to the camera device.
         *
         * <p>If the camera is either focusing or capturing; settings applications
         * will be (asynchronously) deferred until those operations complete.</p>
         *
         * @param settings The settings to use on the device.
         * @return Whether the settings can be applied.
         */
        public abstract boolean applySettings(CameraSettings settings);

        /**
         * Forces {@code CameraProxy} to update the cached version of the camera
         * settings regardless of the dirty bit.
         */
        public void refreshSettings() {
            try {
                getDispatchThread().runJob(new Runnable() {
                    @Override
                    public void run() {
                        getCameraHandler().sendEmptyMessage(CameraActions.REFRESH_PARAMETERS);
                    }});
            } catch (final RuntimeException ex) {
                getAgent().getCameraExceptionHandler().onDispatchThreadException(ex);
            }
        }

        /**
         * Enables/Disables the camera shutter sound.
         *
         * @param enable   {@code true} to enable the shutter sound,
         *                 {@code false} to disable it.
         */
        public void enableShutterSound(final boolean enable) {
            try {
                getDispatchThread().runJob(new Runnable() {
                    @Override
                    public void run() {
                        getCameraHandler()
                                .obtainMessage(CameraActions.ENABLE_SHUTTER_SOUND, (enable ? 1 : 0), 0)
                                .sendToTarget();
                    }});
            } catch (final RuntimeException ex) {
                getAgent().getCameraExceptionHandler().onDispatchThreadException(ex);
            }
        }

        /**
         * Dumps the current settings of the camera device.
         *
         * <p>The content varies based on the underlying camera API settings
         * implementation.</p>
         *
         * @return The content of the device settings represented by a string.
         */
        public abstract String dumpDeviceSettings();

        /**
         * @return The handler to which camera tasks should be posted.
         */
        public abstract Handler getCameraHandler();

        /**
         * @return The thread used on which client callbacks are served.
         */
        public abstract DispatchThread getDispatchThread();

        /**
         * @return The state machine tracking the camera API's current mode.
         */
        public abstract CameraStateHolder getCameraState();
    }

    public static class WaitDoneBundle {
        public final Runnable mUnlockRunnable;
        public final Object mWaitLock;

        WaitDoneBundle() {
            mWaitLock = new Object();
            mUnlockRunnable = new Runnable() {
                @Override
                public void run() {
                    synchronized (mWaitLock) {
                        mWaitLock.notifyAll();
                    }
                }};
        }

        /**
         * Notify all synchronous waiters waiting on message completion with {@link #mWaitLock}.
         *
         * <p>This assumes that the message was sent with {@code this} as the {@code Message#obj}.
         * Otherwise the message is ignored.</p>
         */
        /*package*/ static void unblockSyncWaiters(Message msg) {
            if (msg == null) return;

            if (msg.obj instanceof WaitDoneBundle) {
                WaitDoneBundle bundle = (WaitDoneBundle)msg.obj;
                bundle.mUnlockRunnable.run();
            }
        }
    }
}
