/*
 * Copyright (C) 2016 The Android Open Source Project
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

/**
 * @addtogroup Audio
 * @{
 */

/**
 * @file AAudio.h
 */

/**
 * This is the 'C' API for AAudio.
 */
#ifndef AAUDIO_AAUDIO_H
#define AAUDIO_AAUDIO_H

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This is used to represent a value that has not been specified.
 * For example, an application could use AAUDIO_UNSPECIFIED to indicate
 * that is did not not care what the specific value of a parameter was
 * and would accept whatever it was given.
 */
#define AAUDIO_UNSPECIFIED           0

enum {
    AAUDIO_DIRECTION_OUTPUT,
    AAUDIO_DIRECTION_INPUT
};
typedef int32_t aaudio_direction_t;

enum {
    AAUDIO_FORMAT_INVALID = -1,
    AAUDIO_FORMAT_UNSPECIFIED = 0,
    AAUDIO_FORMAT_PCM_I16,
    AAUDIO_FORMAT_PCM_FLOAT
};
typedef int32_t aaudio_format_t;

enum {
    AAUDIO_OK,
    AAUDIO_ERROR_BASE = -900, // TODO review
    AAUDIO_ERROR_DISCONNECTED,
    AAUDIO_ERROR_ILLEGAL_ARGUMENT,
    // reserved
    AAUDIO_ERROR_INTERNAL = AAUDIO_ERROR_ILLEGAL_ARGUMENT + 2,
    AAUDIO_ERROR_INVALID_STATE,
    // reserved
    // reserved
    AAUDIO_ERROR_INVALID_HANDLE = AAUDIO_ERROR_INVALID_STATE + 3,
    // reserved
    AAUDIO_ERROR_UNIMPLEMENTED = AAUDIO_ERROR_INVALID_HANDLE + 2,
    AAUDIO_ERROR_UNAVAILABLE,
    AAUDIO_ERROR_NO_FREE_HANDLES,
    AAUDIO_ERROR_NO_MEMORY,
    AAUDIO_ERROR_NULL,
    AAUDIO_ERROR_TIMEOUT,
    AAUDIO_ERROR_WOULD_BLOCK,
    AAUDIO_ERROR_INVALID_FORMAT,
    AAUDIO_ERROR_OUT_OF_RANGE,
    AAUDIO_ERROR_NO_SERVICE,
    AAUDIO_ERROR_INVALID_RATE
};
typedef int32_t  aaudio_result_t;

enum
{
    AAUDIO_STREAM_STATE_UNINITIALIZED = 0,
    AAUDIO_STREAM_STATE_UNKNOWN,
    AAUDIO_STREAM_STATE_OPEN,
    AAUDIO_STREAM_STATE_STARTING,
    AAUDIO_STREAM_STATE_STARTED,
    AAUDIO_STREAM_STATE_PAUSING,
    AAUDIO_STREAM_STATE_PAUSED,
    AAUDIO_STREAM_STATE_FLUSHING,
    AAUDIO_STREAM_STATE_FLUSHED,
    AAUDIO_STREAM_STATE_STOPPING,
    AAUDIO_STREAM_STATE_STOPPED,
    AAUDIO_STREAM_STATE_CLOSING,
    AAUDIO_STREAM_STATE_CLOSED,
    AAUDIO_STREAM_STATE_DISCONNECTED
};
typedef int32_t aaudio_stream_state_t;


enum {
    /**
     * This will be the only stream using a particular source or sink.
     * This mode will provide the lowest possible latency.
     * You should close EXCLUSIVE streams immediately when you are not using them.
     */
            AAUDIO_SHARING_MODE_EXCLUSIVE,
    /**
     * Multiple applications will be mixed by the AAudio Server.
     * This will have higher latency than the EXCLUSIVE mode.
     */
            AAUDIO_SHARING_MODE_SHARED
};
typedef int32_t aaudio_sharing_mode_t;


enum {
    /**
     * No particular performance needs. Default.
     */
            AAUDIO_PERFORMANCE_MODE_NONE = 10,

    /**
     * Extending battery life is most important.
     */
            AAUDIO_PERFORMANCE_MODE_POWER_SAVING,

    /**
     * Reducing latency is most important.
     */
            AAUDIO_PERFORMANCE_MODE_LOW_LATENCY
};
typedef int32_t aaudio_performance_mode_t;

typedef struct AAudioStreamStruct         AAudioStream;
typedef struct AAudioStreamBuilderStruct  AAudioStreamBuilder;

#ifndef AAUDIO_API
#define AAUDIO_API /* export this symbol */
#endif

// ============================================================
// Audio System
// ============================================================

/**
 * The text is the ASCII symbol corresponding to the returnCode,
 * or an English message saying the returnCode is unrecognized.
 * This is intended for developers to use when debugging.
 * It is not for display to users.
 *
 * @return pointer to a text representation of an AAudio result code.
 */
AAUDIO_API const char * AAudio_convertResultToText(aaudio_result_t returnCode);

/**
 * The text is the ASCII symbol corresponding to the stream state,
 * or an English message saying the state is unrecognized.
 * This is intended for developers to use when debugging.
 * It is not for display to users.
 *
 * @return pointer to a text representation of an AAudio state.
 */
AAUDIO_API const char * AAudio_convertStreamStateToText(aaudio_stream_state_t state);

// ============================================================
// StreamBuilder
// ============================================================

/**
 * Create a StreamBuilder that can be used to open a Stream.
 *
 * The deviceId is initially unspecified, meaning that the current default device will be used.
 *
 * The default direction is AAUDIO_DIRECTION_OUTPUT.
 * The default sharing mode is AAUDIO_SHARING_MODE_SHARED.
 * The data format, samplesPerFrames and sampleRate are unspecified and will be
 * chosen by the device when it is opened.
 *
 * AAudioStreamBuilder_delete() must be called when you are done using the builder.
 */
AAUDIO_API aaudio_result_t AAudio_createStreamBuilder(AAudioStreamBuilder** builder);

/**
 * Request an audio device identified device using an ID.
 * On Android, for example, the ID could be obtained from the Java AudioManager.
 *
 * The default, if you do not call this function, is AAUDIO_UNSPECIFIED,
 * in which case the primary device will be used.
 *
 * @param builder reference provided by AAudio_createStreamBuilder()
 * @param deviceId device identifier or AAUDIO_UNSPECIFIED
 */
AAUDIO_API void AAudioStreamBuilder_setDeviceId(AAudioStreamBuilder* builder,
                                                     int32_t deviceId);

/**
 * Request a sample rate in Hertz.
 *
 * The default, if you do not call this function, is AAUDIO_UNSPECIFIED.
 * An optimal value will then be chosen when the stream is opened.
 * After opening a stream with an unspecified value, the application must
 * query for the actual value, which may vary by device.
 *
 * If an exact value is specified then an opened stream will use that value.
 * If a stream cannot be opened with the specified value then the open will fail.
 *
 * @param builder reference provided by AAudio_createStreamBuilder()
 * @param sampleRate frames per second. Common rates include 44100 and 48000 Hz.
 */
AAUDIO_API void AAudioStreamBuilder_setSampleRate(AAudioStreamBuilder* builder,
                                                       int32_t sampleRate);

/**
 * Request a number of channels for the stream.
 *
 * The default, if you do not call this function, is AAUDIO_UNSPECIFIED.
 * An optimal value will then be chosen when the stream is opened.
 * After opening a stream with an unspecified value, the application must
 * query for the actual value, which may vary by device.
 *
 * If an exact value is specified then an opened stream will use that value.
 * If a stream cannot be opened with the specified value then the open will fail.
 *
 * @param builder reference provided by AAudio_createStreamBuilder()
 * @param channelCount Number of channels desired.
 */
AAUDIO_API void AAudioStreamBuilder_setChannelCount(AAudioStreamBuilder* builder,
                                                   int32_t channelCount);

/**
 * Identical to AAudioStreamBuilder_setChannelCount().
 *
 * @param builder reference provided by AAudio_createStreamBuilder()
 * @param samplesPerFrame Number of samples in a frame.
 */
AAUDIO_API void AAudioStreamBuilder_setSamplesPerFrame(AAudioStreamBuilder* builder,
                                                       int32_t samplesPerFrame);

/**
 * Request a sample data format, for example AAUDIO_FORMAT_PCM_I16.
 *
 * The default, if you do not call this function, is AAUDIO_UNSPECIFIED.
 * An optimal value will then be chosen when the stream is opened.
 * After opening a stream with an unspecified value, the application must
 * query for the actual value, which may vary by device.
 *
 * If an exact value is specified then an opened stream will use that value.
 * If a stream cannot be opened with the specified value then the open will fail.
 *
 * @param builder reference provided by AAudio_createStreamBuilder()
 * @param format common formats are AAUDIO_FORMAT_PCM_FLOAT and AAUDIO_FORMAT_PCM_I16.
 */
AAUDIO_API void AAudioStreamBuilder_setFormat(AAudioStreamBuilder* builder,
                                                   aaudio_format_t format);

/**
 * Request a mode for sharing the device.
 *
 * The default, if you do not call this function, is AAUDIO_SHARING_MODE_SHARED.
 *
 * The requested sharing mode may not be available.
 * The application can query for the actual mode after the stream is opened.
 *
 * @param builder reference provided by AAudio_createStreamBuilder()
 * @param sharingMode AAUDIO_SHARING_MODE_SHARED or AAUDIO_SHARING_MODE_EXCLUSIVE
 */
AAUDIO_API void AAudioStreamBuilder_setSharingMode(AAudioStreamBuilder* builder,
                                                        aaudio_sharing_mode_t sharingMode);

/**
 * Request the direction for a stream.
 *
 * The default, if you do not call this function, is AAUDIO_DIRECTION_OUTPUT.
 *
 * @param builder reference provided by AAudio_createStreamBuilder()
 * @param direction AAUDIO_DIRECTION_OUTPUT or AAUDIO_DIRECTION_INPUT
 */
AAUDIO_API void AAudioStreamBuilder_setDirection(AAudioStreamBuilder* builder,
                                                            aaudio_direction_t direction);

/**
 * Set the requested buffer capacity in frames.
 * The final AAudioStream capacity may differ, but will probably be at least this big.
 *
 * The default, if you do not call this function, is AAUDIO_UNSPECIFIED.
 *
 * @param builder reference provided by AAudio_createStreamBuilder()
 * @param numFrames the desired buffer capacity in frames or AAUDIO_UNSPECIFIED
 */
AAUDIO_API void AAudioStreamBuilder_setBufferCapacityInFrames(AAudioStreamBuilder* builder,
                                                                 int32_t numFrames);

/**
 * Set the requested performance mode.
 *
 * The default, if you do not call this function, is AAUDIO_PERFORMANCE_MODE_NONE.
 *
 * @param builder reference provided by AAudio_createStreamBuilder()
 * @param mode the desired performance mode, eg. AAUDIO_PERFORMANCE_MODE_LOW_LATENCY
 */
AAUDIO_API void AAudioStreamBuilder_setPerformanceMode(AAudioStreamBuilder* builder,
                                                aaudio_performance_mode_t mode);

/**
 * Return one of these values from the data callback function.
 */
enum {

    /**
     * Continue calling the callback.
     */
    AAUDIO_CALLBACK_RESULT_CONTINUE = 0,

    /**
     * Stop calling the callback.
     *
     * The application will still need to call AAudioStream_requestPause()
     * or AAudioStream_requestStop().
     */
    AAUDIO_CALLBACK_RESULT_STOP,

};
typedef int32_t aaudio_data_callback_result_t;

/**
 * Prototype for the data function that is passed to AAudioStreamBuilder_setDataCallback().
 *
 * For an output stream, this function should render and write numFrames of data
 * in the streams current data format to the audioData buffer.
 *
 * For an input stream, this function should read and process numFrames of data
 * from the audioData buffer.
 *
 * Note that this callback function should be considered a "real-time" function.
 * It must not do anything that could cause an unbounded delay because that can cause the
 * audio to glitch or pop.
 *
 * These are things the function should NOT do:
 * <ul>
 * <li>allocate memory using, for example, malloc() or new</li>
 * <li>any file operations such as opening, closing, reading or writing</li>
 * <li>any network operations such as streaming</li>
 * <li>use any mutexes or other synchronization primitives</li>
 * <li>sleep</li>
 * </ul>
 *
 * If you need to move data, eg. MIDI commands, in or out of the callback function then
 * we recommend the use of non-blocking techniques such as an atomic FIFO.
 *
 * @param stream reference provided by AAudioStreamBuilder_openStream()
 * @param userData the same address that was passed to AAudioStreamBuilder_setCallback()
 * @param audioData a pointer to the audio data
 * @param numFrames the number of frames to be processed
 * @return AAUDIO_CALLBACK_RESULT_*
 */
typedef aaudio_data_callback_result_t (*AAudioStream_dataCallback)(
        AAudioStream *stream,
        void *userData,
        void *audioData,
        int32_t numFrames);

/**
 * Request that AAudio call this functions when the stream is running.
 *
 * Note that when using this callback, the audio data will be passed in or out
 * of the function as an argument.
 * So you cannot call AAudioStream_write() or AAudioStream_read() on the same stream
 * that has an active data callback.
 *
 * The callback function will start being called after AAudioStream_requestStart() is called.
 * It will stop being called after AAudioStream_requestPause() or
 * AAudioStream_requestStop() is called.
 *
 * This callback function will be called on a real-time thread owned by AAudio. See
 * {@link AAudioStream_dataCallback} for more information.
 *
 * Note that the AAudio callbacks will never be called simultaneously from multiple threads.
 *
 * @param builder reference provided by AAudio_createStreamBuilder()
 * @param callback pointer to a function that will process audio data.
 * @param userData pointer to an application data structure that will be passed
 *          to the callback functions.
 */
AAUDIO_API void AAudioStreamBuilder_setDataCallback(AAudioStreamBuilder* builder,
                                                 AAudioStream_dataCallback callback,
                                                 void *userData);

/**
 * Set the requested data callback buffer size in frames.
 * See {@link AAudioStream_dataCallback}.
 *
 * The default, if you do not call this function, is AAUDIO_UNSPECIFIED.
 *
 * For the lowest possible latency, do not call this function. AAudio will then
 * call the dataProc callback function with whatever size is optimal.
 * That size may vary from one callback to another.
 *
 * Only use this function if the application requires a specific number of frames for processing.
 * The application might, for example, be using an FFT that requires
 * a specific power-of-two sized buffer.
 *
 * AAudio may need to add additional buffering in order to adapt between the internal
 * buffer size and the requested buffer size.
 *
 * If you do call this function then the requested size should be less than
 * half the buffer capacity, to allow double buffering.
 *
 * @param builder reference provided by AAudio_createStreamBuilder()
 * @param numFrames the desired buffer size in frames or AAUDIO_UNSPECIFIED
 */
AAUDIO_API void AAudioStreamBuilder_setFramesPerDataCallback(AAudioStreamBuilder* builder,
                                                             int32_t numFrames);

/**
 * Prototype for the callback function that is passed to
 * AAudioStreamBuilder_setErrorCallback().
 *
 * @param stream reference provided by AAudioStreamBuilder_openStream()
 * @param userData the same address that was passed to AAudioStreamBuilder_setErrorCallback()
 * @param error an AAUDIO_ERROR_* value.
 */
typedef void (*AAudioStream_errorCallback)(
        AAudioStream *stream,
        void *userData,
        aaudio_result_t error);

/**
 * Request that AAudio call this functions if any error occurs on a callback thread.
 *
 * It will be called, for example, if a headset or a USB device is unplugged causing the stream's
 * device to be unavailable.
 * In response, this function could signal or launch another thread to reopen a
 * stream on another device. Do not reopen the stream in this callback.
 *
 * This will not be called because of actions by the application, such as stopping
 * or closing a stream.
 *
 * Another possible cause of error would be a timeout or an unanticipated internal error.
 *
 * Note that the AAudio callbacks will never be called simultaneously from multiple threads.
 *
 * @param builder reference provided by AAudio_createStreamBuilder()
 * @param callback pointer to a function that will be called if an error occurs.
 * @param userData pointer to an application data structure that will be passed
 *          to the callback functions.
 */
AAUDIO_API void AAudioStreamBuilder_setErrorCallback(AAudioStreamBuilder* builder,
                                                AAudioStream_errorCallback callback,
                                                void *userData);

/**
 * Open a stream based on the options in the StreamBuilder.
 *
 * AAudioStream_close must be called when finished with the stream to recover
 * the memory and to free the associated resources.
 *
 * @param builder reference provided by AAudio_createStreamBuilder()
 * @param stream pointer to a variable to receive the new stream reference
 * @return AAUDIO_OK or a negative error.
 */
AAUDIO_API aaudio_result_t  AAudioStreamBuilder_openStream(AAudioStreamBuilder* builder,
                                                     AAudioStream** stream);

/**
 * Delete the resources associated with the StreamBuilder.
 *
 * @param builder reference provided by AAudio_createStreamBuilder()
 * @return AAUDIO_OK or a negative error.
 */
AAUDIO_API aaudio_result_t  AAudioStreamBuilder_delete(AAudioStreamBuilder* builder);

// ============================================================
// Stream Control
// ============================================================

/**
 * Free the resources associated with a stream created by AAudioStreamBuilder_openStream()
 *
 * @param stream reference provided by AAudioStreamBuilder_openStream()
 * @return AAUDIO_OK or a negative error.
 */
AAUDIO_API aaudio_result_t  AAudioStream_close(AAudioStream* stream);

/**
 * Asynchronously request to start playing the stream. For output streams, one should
 * write to the stream to fill the buffer before starting.
 * Otherwise it will underflow.
 * After this call the state will be in AAUDIO_STREAM_STATE_STARTING or AAUDIO_STREAM_STATE_STARTED.
 *
 * @param stream reference provided by AAudioStreamBuilder_openStream()
 * @return AAUDIO_OK or a negative error.
 */
AAUDIO_API aaudio_result_t  AAudioStream_requestStart(AAudioStream* stream);

/**
 * Asynchronous request for the stream to pause.
 * Pausing a stream will freeze the data flow but not flush any buffers.
 * Use AAudioStream_Start() to resume playback after a pause.
 * After this call the state will be in AAUDIO_STREAM_STATE_PAUSING or AAUDIO_STREAM_STATE_PAUSED.
 *
 * This will return AAUDIO_ERROR_UNIMPLEMENTED for input streams.
 * For input streams use AAudioStream_requestStop().
 *
 * @param stream reference provided by AAudioStreamBuilder_openStream()
 * @return AAUDIO_OK or a negative error.
 */
AAUDIO_API aaudio_result_t  AAudioStream_requestPause(AAudioStream* stream);

/**
 * Asynchronous request for the stream to flush.
 * Flushing will discard any pending data.
 * This call only works if the stream is pausing or paused. TODO review
 * Frame counters are not reset by a flush. They may be advanced.
 * After this call the state will be in AAUDIO_STREAM_STATE_FLUSHING or AAUDIO_STREAM_STATE_FLUSHED.
 *
 * This will return AAUDIO_ERROR_UNIMPLEMENTED for input streams.
 *
 * @param stream reference provided by AAudioStreamBuilder_openStream()
 * @return AAUDIO_OK or a negative error.
 */
AAUDIO_API aaudio_result_t  AAudioStream_requestFlush(AAudioStream* stream);

/**
 * Asynchronous request for the stream to stop.
 * The stream will stop after all of the data currently buffered has been played.
 * After this call the state will be in AAUDIO_STREAM_STATE_STOPPING or AAUDIO_STREAM_STATE_STOPPED.
 *
 * @param stream reference provided by AAudioStreamBuilder_openStream()
 * @return AAUDIO_OK or a negative error.
 */
AAUDIO_API aaudio_result_t  AAudioStream_requestStop(AAudioStream* stream);

/**
 * Query the current state of the client, eg. AAUDIO_STREAM_STATE_PAUSING
 *
 * This function will immediately return the state without updating the state.
 * If you want to update the client state based on the server state then
 * call AAudioStream_waitForStateChange() with currentState
 * set to AAUDIO_STREAM_STATE_UNKNOWN and a zero timeout.
 *
 * @param stream reference provided by AAudioStreamBuilder_openStream()
 */
AAUDIO_API aaudio_stream_state_t AAudioStream_getState(AAudioStream* stream);

/**
 * Wait until the current state no longer matches the input state.
 *
 * This will update the current client state.
 *
 * <pre><code>
 * aaudio_stream_state_t currentState;
 * aaudio_result_t result = AAudioStream_getState(stream, &currentState);
 * while (result == AAUDIO_OK && currentState != AAUDIO_STREAM_STATE_PAUSING) {
 *     result = AAudioStream_waitForStateChange(
 *                                   stream, currentState, &currentState, MY_TIMEOUT_NANOS);
 * }
 * </code></pre>
 *
 * @param stream A reference provided by AAudioStreamBuilder_openStream()
 * @param inputState The state we want to avoid.
 * @param nextState Pointer to a variable that will be set to the new state.
 * @param timeoutNanoseconds Maximum number of nanoseconds to wait for completion.
 * @return AAUDIO_OK or a negative error.
 */
AAUDIO_API aaudio_result_t AAudioStream_waitForStateChange(AAudioStream* stream,
                                            aaudio_stream_state_t inputState,
                                            aaudio_stream_state_t *nextState,
                                            int64_t timeoutNanoseconds);

// ============================================================
// Stream I/O
// ============================================================

/**
 * Read data from the stream.
 *
 * The call will wait until the read is complete or until it runs out of time.
 * If timeoutNanos is zero then this call will not wait.
 *
 * Note that timeoutNanoseconds is a relative duration in wall clock time.
 * Time will not stop if the thread is asleep.
 * So it will be implemented using CLOCK_BOOTTIME.
 *
 * This call is "strong non-blocking" unless it has to wait for data.
 *
 * @param stream A stream created using AAudioStreamBuilder_openStream().
 * @param buffer The address of the first sample.
 * @param numFrames Number of frames to read. Only complete frames will be written.
 * @param timeoutNanoseconds Maximum number of nanoseconds to wait for completion.
 * @return The number of frames actually read or a negative error.
 */
AAUDIO_API aaudio_result_t AAudioStream_read(AAudioStream* stream,
                               void *buffer,
                               int32_t numFrames,
                               int64_t timeoutNanoseconds);

/**
 * Write data to the stream.
 *
 * The call will wait until the write is complete or until it runs out of time.
 * If timeoutNanos is zero then this call will not wait.
 *
 * Note that timeoutNanoseconds is a relative duration in wall clock time.
 * Time will not stop if the thread is asleep.
 * So it will be implemented using CLOCK_BOOTTIME.
 *
 * This call is "strong non-blocking" unless it has to wait for room in the buffer.
 *
 * @param stream A stream created using AAudioStreamBuilder_openStream().
 * @param buffer The address of the first sample.
 * @param numFrames Number of frames to write. Only complete frames will be written.
 * @param timeoutNanoseconds Maximum number of nanoseconds to wait for completion.
 * @return The number of frames actually written or a negative error.
 */
AAUDIO_API aaudio_result_t AAudioStream_write(AAudioStream* stream,
                               const void *buffer,
                               int32_t numFrames,
                               int64_t timeoutNanoseconds);

// ============================================================
// Stream - queries
// ============================================================

/**
 * This can be used to adjust the latency of the buffer by changing
 * the threshold where blocking will occur.
 * By combining this with AAudioStream_getXRunCount(), the latency can be tuned
 * at run-time for each device.
 *
 * This cannot be set higher than AAudioStream_getBufferCapacityInFrames().
 *
 * Note that you will probably not get the exact size you request.
 * Call AAudioStream_getBufferSizeInFrames() to see what the actual final size is.
 *
 * @param stream reference provided by AAudioStreamBuilder_openStream()
 * @param numFrames requested number of frames that can be filled without blocking
 * @return actual buffer size in frames or a negative error
 */
AAUDIO_API aaudio_result_t AAudioStream_setBufferSizeInFrames(AAudioStream* stream,
                                                      int32_t numFrames);

/**
 * Query the maximum number of frames that can be filled without blocking.
 *
 * @param stream reference provided by AAudioStreamBuilder_openStream()
 * @return buffer size in frames.
 */
AAUDIO_API int32_t AAudioStream_getBufferSizeInFrames(AAudioStream* stream);

/**
 * Query the number of frames that the application should read or write at
 * one time for optimal performance. It is OK if an application writes
 * a different number of frames. But the buffer size may need to be larger
 * in order to avoid underruns or overruns.
 *
 * Note that this may or may not match the actual device burst size.
 * For some endpoints, the burst size can vary dynamically.
 * But these tend to be devices with high latency.
 *
 * @param stream reference provided by AAudioStreamBuilder_openStream()
 * @return burst size
 */
AAUDIO_API int32_t AAudioStream_getFramesPerBurst(AAudioStream* stream);

/**
 * Query maximum buffer capacity in frames.
 *
 * @param stream reference provided by AAudioStreamBuilder_openStream()
 * @return  buffer capacity in frames
 */
AAUDIO_API int32_t AAudioStream_getBufferCapacityInFrames(AAudioStream* stream);

/**
 * Query the size of the buffer that will be passed to the dataProc callback
 * in the numFrames parameter.
 *
 * This call can be used if the application needs to know the value of numFrames before
 * the stream is started. This is not normally necessary.
 *
 * If a specific size was requested by calling AAudioStreamBuilder_setCallbackSizeInFrames()
 * then this will be the same size.
 *
 * If AAudioStreamBuilder_setCallbackSizeInFrames() was not called then this will
 * return the size chosen by AAudio, or AAUDIO_UNSPECIFIED.
 *
 * AAUDIO_UNSPECIFIED indicates that the callback buffer size for this stream
 * may vary from one dataProc callback to the next.
 *
 * @param stream reference provided by AAudioStreamBuilder_openStream()
 * @return callback buffer size in frames or AAUDIO_UNSPECIFIED
 */
AAUDIO_API int32_t AAudioStream_getFramesPerDataCallback(AAudioStream* stream);

/**
 * An XRun is an Underrun or an Overrun.
 * During playing, an underrun will occur if the stream is not written in time
 * and the system runs out of valid data.
 * During recording, an overrun will occur if the stream is not read in time
 * and there is no place to put the incoming data so it is discarded.
 *
 * An underrun or overrun can cause an audible "pop" or "glitch".
 *
 * Note that some INPUT devices may not support this function.
 * In that case a 0 will always be returned.
 *
 * @param stream reference provided by AAudioStreamBuilder_openStream()
 * @return the underrun or overrun count
 */
AAUDIO_API int32_t AAudioStream_getXRunCount(AAudioStream* stream);

/**
 * @param stream reference provided by AAudioStreamBuilder_openStream()
 * @return actual sample rate
 */
AAUDIO_API int32_t AAudioStream_getSampleRate(AAudioStream* stream);

/**
 * A stream has one or more channels of data.
 * A frame will contain one sample for each channel.
 *
 * @param stream reference provided by AAudioStreamBuilder_openStream()
 * @return actual number of channels
 */
AAUDIO_API int32_t AAudioStream_getChannelCount(AAudioStream* stream);

/**
 * Identical to AAudioStream_getChannelCount().
 *
 * @param stream reference provided by AAudioStreamBuilder_openStream()
 * @return actual number of samples frame
 */
AAUDIO_API int32_t AAudioStream_getSamplesPerFrame(AAudioStream* stream);

/**
 * @param stream reference provided by AAudioStreamBuilder_openStream()
 * @return actual device ID
 */
AAUDIO_API int32_t AAudioStream_getDeviceId(AAudioStream* stream);

/**
 * @param stream reference provided by AAudioStreamBuilder_openStream()
 * @return actual data format
 */
AAUDIO_API aaudio_format_t AAudioStream_getFormat(AAudioStream* stream);

/**
 * Provide actual sharing mode.
 * @param stream reference provided by AAudioStreamBuilder_openStream()
 * @return  actual sharing mode
 */
AAUDIO_API aaudio_sharing_mode_t AAudioStream_getSharingMode(AAudioStream* stream);

/**
 * Get the performance mode used by the stream.
 *
 * @param stream reference provided by AAudioStreamBuilder_openStream()
 */
AAUDIO_API aaudio_performance_mode_t AAudioStream_getPerformanceMode(AAudioStream* stream);

/**
 * @param stream reference provided by AAudioStreamBuilder_openStream()
 * @return direction
 */
AAUDIO_API aaudio_direction_t AAudioStream_getDirection(AAudioStream* stream);

/**
 * Passes back the number of frames that have been written since the stream was created.
 * For an output stream, this will be advanced by the application calling write().
 * For an input stream, this will be advanced by the endpoint.
 *
 * The frame position is monotonically increasing.
 *
 * @param stream reference provided by AAudioStreamBuilder_openStream()
 * @return frames written
 */
AAUDIO_API int64_t AAudioStream_getFramesWritten(AAudioStream* stream);

/**
 * Passes back the number of frames that have been read since the stream was created.
 * For an output stream, this will be advanced by the endpoint.
 * For an input stream, this will be advanced by the application calling read().
 *
 * The frame position is monotonically increasing.
 *
 * @param stream reference provided by AAudioStreamBuilder_openStream()
 * @return frames read
 */
AAUDIO_API int64_t AAudioStream_getFramesRead(AAudioStream* stream);

/**
 * Passes back the time at which a particular frame was presented.
 * This can be used to synchronize audio with video or MIDI.
 * It can also be used to align a recorded stream with a playback stream.
 *
 * Timestamps are only valid when the stream is in AAUDIO_STREAM_STATE_STARTED.
 * AAUDIO_ERROR_INVALID_STATE will be returned if the stream is not started.
 * Note that because requestStart() is asynchronous, timestamps will not be valid until
 * a short time after calling requestStart().
 * So AAUDIO_ERROR_INVALID_STATE should not be considered a fatal error.
 * Just try calling again later.
 *
 * If an error occurs, then the position and time will not be modified.
 *
 * The position and time passed back are monotonically increasing.
 *
 * @param stream reference provided by AAudioStreamBuilder_openStream()
 * @param clockid CLOCK_MONOTONIC or CLOCK_BOOTTIME
 * @param framePosition pointer to a variable to receive the position
 * @param timeNanoseconds pointer to a variable to receive the time
 * @return AAUDIO_OK or a negative error
 */
AAUDIO_API aaudio_result_t AAudioStream_getTimestamp(AAudioStream* stream,
                                      clockid_t clockid,
                                      int64_t *framePosition,
                                      int64_t *timeNanoseconds);

#ifdef __cplusplus
}
#endif

#endif //AAUDIO_AAUDIO_H

/** @} */
