// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.media;

import android.annotation.SuppressLint;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder.AudioSource;
import android.media.audiofx.AcousticEchoCanceler;
import android.media.audiofx.AudioEffect;
import android.media.audiofx.AudioEffect.Descriptor;
import android.os.Process;

import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

import java.nio.ByteBuffer;

// Owned by its native counterpart declared in audio_record_input.h. Refer to
// that class for general comments.
@JNINamespace("media")
class AudioRecordInput {
    private static final String TAG = "cr.media";
    // Set to true to enable debug logs. Always check in as false.
    private static final boolean DEBUG = false;
    // We are unable to obtain a precise measurement of the hardware delay on
    // Android. This is a conservative lower-bound based on measurments. It
    // could surely be tightened with further testing.
    // TODO(dalecurtis): This should use AudioRecord.getTimestamp() in API 24+.
    private static final int HARDWARE_DELAY_MS = 100;

    private final long mNativeAudioRecordInputStream;
    private final int mSampleRate;
    private final int mChannels;
    private final int mBitsPerSample;
    private final boolean mUsePlatformAEC;
    private ByteBuffer mBuffer;
    private AudioRecord mAudioRecord;
    private AudioRecordThread mAudioRecordThread;
    private AcousticEchoCanceler mAEC;

    private class AudioRecordThread extends Thread {
        // The "volatile" synchronization technique is discussed here:
        // http://stackoverflow.com/a/106787/299268
        // and more generally in this article:
        // https://www.ibm.com/developerworks/java/library/j-jtp06197/
        private volatile boolean mKeepAlive = true;

        @Override
        public void run() {
            Process.setThreadPriority(Process.THREAD_PRIORITY_URGENT_AUDIO);
            try {
                mAudioRecord.startRecording();
            } catch (IllegalStateException e) {
                Log.e(TAG, "startRecording failed", e);
                return;
            }

            while (mKeepAlive) {
                int bytesRead = mAudioRecord.read(mBuffer, mBuffer.capacity());
                if (bytesRead > 0) {
                    nativeOnData(mNativeAudioRecordInputStream, bytesRead, HARDWARE_DELAY_MS);
                } else {
                    Log.e(TAG, "read failed: %d", bytesRead);
                    if (bytesRead == AudioRecord.ERROR_INVALID_OPERATION) {
                        // This can happen if there is already an active
                        // AudioRecord (e.g. in another tab).
                        mKeepAlive = false;
                    }
                }
            }

            try {
                mAudioRecord.stop();
            } catch (IllegalStateException e) {
                Log.e(TAG, "stop failed", e);
            }
        }

        public void joinRecordThread() {
            mKeepAlive = false;
            while (isAlive()) {
                try {
                    join();
                } catch (InterruptedException e) {
                    // Ignore.
                }
            }
        }
    }

    @CalledByNative
    private static AudioRecordInput createAudioRecordInput(long nativeAudioRecordInputStream,
            int sampleRate, int channels, int bitsPerSample, int bytesPerBuffer,
            boolean usePlatformAEC) {
        return new AudioRecordInput(nativeAudioRecordInputStream, sampleRate, channels,
                                    bitsPerSample, bytesPerBuffer, usePlatformAEC);
    }

    private AudioRecordInput(long nativeAudioRecordInputStream, int sampleRate, int channels,
                             int bitsPerSample, int bytesPerBuffer, boolean usePlatformAEC) {
        mNativeAudioRecordInputStream = nativeAudioRecordInputStream;
        mSampleRate = sampleRate;
        mChannels = channels;
        mBitsPerSample = bitsPerSample;
        mUsePlatformAEC = usePlatformAEC;

        // We use a direct buffer so that the native class can have access to
        // the underlying memory address. This avoids the need to copy from a
        // jbyteArray to native memory. More discussion of this here:
        // http://developer.android.com/training/articles/perf-jni.html
        mBuffer = ByteBuffer.allocateDirect(bytesPerBuffer);
        // Rather than passing the ByteBuffer with every OnData call (requiring
        // the potentially expensive GetDirectBufferAddress) we simply have the
        // the native class cache the address to the memory once.
        //
        // Unfortunately, profiling with traceview was unable to either confirm
        // or deny the advantage of this approach, as the values for
        // nativeOnData() were not stable across runs.
        nativeCacheDirectBufferAddress(mNativeAudioRecordInputStream, mBuffer);
    }

    @SuppressLint("NewApi")
    @CalledByNative
    private boolean open() {
        if (mAudioRecord != null) {
            Log.e(TAG, "open() called twice without a close()");
            return false;
        }
        int channelConfig;
        if (mChannels == 1) {
            channelConfig = AudioFormat.CHANNEL_IN_MONO;
        } else if (mChannels == 2) {
            channelConfig = AudioFormat.CHANNEL_IN_STEREO;
        } else {
            Log.e(TAG, "Unsupported number of channels: %d", mChannels);
            return false;
        }

        int audioFormat;
        if (mBitsPerSample == 8) {
            audioFormat = AudioFormat.ENCODING_PCM_8BIT;
        } else if (mBitsPerSample == 16) {
            audioFormat = AudioFormat.ENCODING_PCM_16BIT;
        } else {
            Log.e(TAG, "Unsupported bits per sample: %d", mBitsPerSample);
            return false;
        }

        // TODO(ajm): Do we need to make this larger to avoid underruns? The
        // Android documentation notes "this size doesn't guarantee a smooth
        // recording under load".
        int minBufferSize = AudioRecord.getMinBufferSize(mSampleRate, channelConfig, audioFormat);
        if (minBufferSize < 0) {
            Log.e(TAG, "getMinBufferSize error: %d", minBufferSize);
            return false;
        }

        // We will request mBuffer.capacity() with every read call. The
        // underlying AudioRecord buffer should be at least this large.
        int audioRecordBufferSizeInBytes = Math.max(mBuffer.capacity(), minBufferSize);
        try {
            // TODO(ajm): Allow other AudioSource types to be requested?
            mAudioRecord = new AudioRecord(AudioSource.VOICE_COMMUNICATION,
                                           mSampleRate,
                                           channelConfig,
                                           audioFormat,
                                           audioRecordBufferSizeInBytes);
        } catch (IllegalArgumentException e) {
            Log.e(TAG, "AudioRecord failed", e);
            return false;
        }

        if (AcousticEchoCanceler.isAvailable()) {
            mAEC = AcousticEchoCanceler.create(mAudioRecord.getAudioSessionId());
            if (mAEC == null) {
                Log.e(TAG, "AcousticEchoCanceler.create failed");
                return false;
            }
            int ret = mAEC.setEnabled(mUsePlatformAEC);
            if (ret != AudioEffect.SUCCESS) {
                Log.e(TAG, "setEnabled error: %d", ret);
                return false;
            }

            if (DEBUG) {
                Descriptor descriptor = mAEC.getDescriptor();
                Log.d(TAG, "AcousticEchoCanceler name: %s, implementor: %s, uuid: %s",
                            descriptor.name, descriptor.implementor, descriptor.uuid);
            }
        }
        return true;
    }

    @CalledByNative
    private void start() {
        if (mAudioRecord == null) {
            Log.e(TAG, "start() called before open().");
            return;
        }
        if (mAudioRecordThread != null) {
            // start() was already called.
            return;
        }
        mAudioRecordThread = new AudioRecordThread();
        mAudioRecordThread.start();
    }

    @CalledByNative
    private void stop() {
        if (mAudioRecordThread == null) {
            // start() was never called, or stop() was already called.
            return;
        }
        mAudioRecordThread.joinRecordThread();
        mAudioRecordThread = null;
    }

    @SuppressLint("NewApi")
    @CalledByNative
    private void close() {
        if (mAudioRecordThread != null) {
            Log.e(TAG, "close() called before stop().");
            return;
        }
        if (mAudioRecord == null) {
            // open() was not called.
            return;
        }

        if (mAEC != null) {
            mAEC.release();
            mAEC = null;
        }
        mAudioRecord.release();
        mAudioRecord = null;
    }

    private native void nativeCacheDirectBufferAddress(long nativeAudioRecordInputStream,
                                                       ByteBuffer buffer);
    private native void nativeOnData(
            long nativeAudioRecordInputStream, int size, int hardwareDelayMs);
}
